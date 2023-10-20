
#include "StarAligner.hpp"


//�ȵ������ʼ�����ٵ���_get_gaps�� ����int�������ݣ� ���������������Ϊn��vector��ÿ��vec_set_centretor�洢���ɸ�Insertion����index+number
auto star_alignment::StarAligner::get_gaps(std::vector<utils::seq_NCBI2NA> &sequences, std::vector<std::vector<utils::Insertion>> &N_gap, std::vector<std::string>& name, std::vector<bool>& sign, std::vector<size_t>& Length,std::vector<bool>& TU, std::vector<utils::more_block>& more_gap, bool mg_tag, utils::MAF_info& MAFinfo, int threshold1, int threshold2, int center) -> std::vector<std::vector<utils::Insertion>>
{
    return StarAligner(sequences,N_gap, name, sign,Length,TU, more_gap, mg_tag, MAFinfo,center)._get_gaps(threshold1, threshold2);//��������󣬵���_get_gaps�ڶ���ķ��ؽ��
}
//���ʼ��
star_alignment::StarAligner::StarAligner(std::vector<utils::seq_NCBI2NA>& sequences, std::vector<std::vector<utils::Insertion>>& N_gap, std::vector<std::string>& name, std::vector<bool>& sign, std::vector<size_t>& Length, std::vector<bool>& TU, std::vector<utils::more_block>& more_gap, bool mg_tag, utils::MAF_info& MAFinfo, int center)
    : _sequences(sequences) //���д���
    , _name(name)
    , _sign(sign)
    , _lengths(Length) //��¼ÿ�����еĳ���
    , _TU(TU)
    , _N_gap(N_gap)
    , _more_gap(more_gap)
    , _mg_tag(mg_tag)
    , _MAFinfo(MAFinfo)
    , _row(_sequences.size())  // ��������
    , _centre(_set_centre(center))   //�������Ѱ����������
    , _centre_len(_lengths[_centre]) //�������еĳ���
{}
//��¼ÿ�����еĳ���

star_alignment::StarAligner::~StarAligner()
{
    delete[] _sequences[_centre].seq;
    delete filter1;
    delete filter0;
}

std::vector<size_t> star_alignment::StarAligner::_set_lengths() 
{
    std::vector<size_t> lengths(_row);

    for (size_t i = 0; i != _row; ++i) lengths[i] = _sequences[i].length;
    return lengths;
}
//�������Ѱ����������
//std::vector<unsigned char> star_alignment::StarAligner::_set_centre() const 
size_t star_alignment::StarAligner::_set_centre(int center)
{

    size_t centre_index = 0;
    _sequences[0].length = _lengths[0];
    for (size_t i = 1; i != _row; ++i)
    {
        _sequences[i].length = _lengths[i];
        if (_lengths[i] > _lengths[centre_index])
            centre_index = i;
    }
    _max_len = _lengths[centre_index];
    if (center != -1)
        centre_index = center;
    std::cout << "                    | Info : Center sequence: " << _name[centre_index] << "\t" << _lengths[centre_index] << "\n";
    std::cout << "                    | Info : Sequence nummer: " << _name.size() <<"\n";
    std::string center_line;
    utils::get_BIN_seq_from_noN(centre_index, center_line);//������������
    _sequences[centre_index].length = center_line.size();
    _sequences[centre_index].seq = utils::BLAST_PackDNA(center_line);
    cout_cur_time();
    std::cout << "Start: Building prediction dictionary \n";
    _K = utils::get_Jaccard_Bloom(center_line, &filter1, &filter0);
    
    cout_cur_time();
    std::cout <<"End  : Building prediction dictionary\n";
    return centre_index;
}

//���Ѿ���ʼ��������Ҫ���������//���������������Ϊn��vector��ÿ��vector�洢���ɸ�Insertion����index+number
auto star_alignment::StarAligner::_get_gaps(int threshold1, int threshold2) const
        -> std::vector<std::vector<utils::Insertion>>
{
    if (_mg_tag)
        return std::move(_merge_results(mul_sv_maf_pairwise_align(threshold1, threshold2)));// ���أ� ˫�����ģ����Ϻ�Ľ��
    else
        return std::move(_merge_results(mul_pairwise_align(threshold1, threshold2)));// ���أ� ˫�����ģ����Ϻ�Ľ��
}

void star_alignment::StarAligner::get_common_substrings_vector(Substrings& C_Strings, std::vector<triple>& substrings, bool sign, size_t read_len)
{
    Substrings& tmp = C_Strings;
    tmp.sign = sign;

    size_t thresh1 = arguments::sv_thresh_len < 200 ? 200 : arguments::sv_thresh_len;;// read_len * 0.05; //���
    size_t thresh2 = arguments::sv_thresh_len;//read_len * 0.02; //����һ��,����С����
    float thresh3 = 0.5;
    //thresh2 = thresh2 > 50 ? thresh2 : 50;
    std::vector<std::vector<triple>> Subs;
    if (substrings.size() < 1)    //[A.index��B.index��length] , A is center
        return;
    bool tag = false; //�Ƿ��Ѿ���ͷ
    for (int i = 1; i < substrings.size(); i++)
    {
        if (tag)
        {
            if (std::abs((int)(substrings[i][0] - substrings[i - 1][0] - substrings[i][1] + substrings[i - 1][1])) < thresh1)
            {
                Subs.back().push_back(substrings[i]);
            }
            else
            {
                tag = false;
            }
        }
        else
        {
            if (std::abs((int)(substrings[i][0] - substrings[i - 1][0] - substrings[i][1] + substrings[i - 1][1])) < thresh1)
            {
                Subs.push_back(std::vector<triple>());
                Subs.back().push_back(substrings[i - 1]);
                Subs.back().push_back(substrings[i]);
                tag = true;
            }
            else
            {
                if (substrings[i - 1][2] > thresh2)
                {
                    Subs.push_back(std::vector<triple>());
                    Subs.back().push_back(substrings[i - 1]);
                }
            }
        }
    }
    int x, y, z;
    for (int i = 0; i < Subs.size(); i++)
    {
        x = 0; y = 0; z = 0;
        for (int j = 0; j < Subs[i].size(); j++)
            x += Subs[i][j][2];
        y = Subs[i].back()[1] - Subs[i][0][1];
        z = Subs[i].back()[0] - Subs[i][0][0];
        if (Subs[i].size() > 3)
            tmp.Subs.push_back(Subs[i]);
        else if (y * thresh3 <= x && z * thresh3 <= x)
            tmp.Subs.push_back(Subs[i]);
    }
    if (tmp.Subs.size() == 0)
        return;
    tmp.add_len = 0;
    for (int i = 0; i < tmp.Subs.size(); i++)
        tmp.add_len += (tmp.Subs[i].back()[1] + tmp.Subs[i].back()[2] - tmp.Subs[i][0][1]);
    if (tmp.add_len >= thresh2)
        ; //C_Strings=tmp;//?
    else
    {
        C_Strings.add_len = 0;
        std::vector<std::vector<triple>>().swap(C_Strings.Subs);
    }

}

//�ϲ����棬�����������ü��洮,�°�
void star_alignment::StarAligner::hebing_substrings(Substrings& C_Strings, std::vector<quadras>& new_intervals, std::vector<quadra>& intervals, std::vector<triple>& common_substrings, std::vector<triple>& common_substrings0, size_t _centre_len, size_t length, bool sign)
{
    
    quadra C_Si;
    C_Strings.add_len = 0;
    quadra new_element;
    int i, j, it, B_start0, B_start1;
    int thresh = arguments::sv_thresh_len;
    if (!sign)//-
    {
        get_common_substrings_vector(C_Strings, common_substrings, true, length);//�����٣����Ķ�
        intervals.resize(common_substrings0.size() + C_Strings.Subs.size());
        //��������

        it = 0;
        i = C_Strings.Subs.size() - 1;
        j = 0;
        while (i >= 0 && j < common_substrings0.size())
        {
            B_start0 = length - C_Strings.Subs[i].back()[1] - C_Strings.Subs[i].back()[2];
            B_start1 = common_substrings0[j][1];
            if (B_start0 <= B_start1)
            {
                intervals[it][4] = 0;//Ӳ�ȱ�ʶ ����
                //intervals[it][0] = _centre_len - C_Strings.Subs[i].back()[0] - C_Strings.Subs[i].back()[2];//A_start   ����ҿ�����
                //intervals[it][1] = _centre_len - C_Strings.Subs[i][0][0];//A_end
                intervals[it][0] = C_Strings.Subs[i][0][0];//A_start   ����ҿ�����
                intervals[it][1] = C_Strings.Subs[i].back()[0] + C_Strings.Subs[i].back()[2];//A_end
                intervals[it][2] = length - C_Strings.Subs[i].back()[1] - C_Strings.Subs[i].back()[2];//B_start
                intervals[it][3] = length - C_Strings.Subs[i][0][1];//B_end
                it++;
                i--;
            }
            else
            {
                intervals[it][4] = 1;// ����
                intervals[it][0] = common_substrings0[j][0];//A_start   ����ҿ�����
                intervals[it][1] = common_substrings0[j][0] + common_substrings0[j][2];//A_end
                intervals[it][2] = common_substrings0[j][1];//B_start
                intervals[it][3] = common_substrings0[j][1] + common_substrings0[j][2];//B_end
                it++;
                j++;
            }
            /*
            while (j < intervals.size())
            {
                if (intervals[j][3] <= C_Si[2])//interval  ĩ�� �����
                    ;
                else if (intervals[j][3] > C_Si[2] && intervals[j][3] <= C_Si[3]) //interval  ĩ�� ���м�
                {
                    if (intervals[j][2] >= C_Si[2]) //interval  ʼ�� ���м�
                            intervals[j][4] = -1;//ɾ��
                    else//interval  ʼ�� �����,  ɾ���Ҳ��ص�
                    {
                        intervals[j][3] = C_Si[2];
                        intervals[j][1] = C_Si[0];
                        intervals[j][1] = intervals[j][1] < intervals[j][0]? intervals[j][0]: intervals[j][1];
                    }
                }
                else//interval  ĩ�� ���ұ�
                {
                    if (intervals[j][2] >= C_Si[3]) //interval  ʼ�� ���ұ�
                        break;
                    else if (intervals[j][2] >= C_Si[2]) //interval  ʼ�� ���м� //ɾ�����ص�
                    {
                        intervals[j][2] = C_Si[3];
                        intervals[j][0] = C_Si[1];
                        intervals[j][0] = intervals[j][0] > intervals[j][1] ? intervals[j][1] : intervals[j][0];


                    }
                    else//interval  ʼ�� �����,  ɾ�������ص�
                    {

                    }
                }
                j++;
            }
            */
        }
        while (i >= 0)
        {
            intervals[it][4] = 0;//Ӳ�ȱ�ʶ ����
            intervals[it][0] = C_Strings.Subs[i][0][0];//A_start   ����ҿ�����
            intervals[it][1] = C_Strings.Subs[i].back()[0] + C_Strings.Subs[i].back()[2];//A_end
            intervals[it][2] = length - C_Strings.Subs[i].back()[1] - C_Strings.Subs[i].back()[2];//B_start
            intervals[it][3] = length - C_Strings.Subs[i][0][1];//B_end
            it++;
            i--;
        }
        while (j < common_substrings0.size())
        {
            intervals[it][4] = 1;// ����
            intervals[it][0] = common_substrings0[j][0];//A_start   ����ҿ�����
            intervals[it][1] = common_substrings0[j][0] + common_substrings0[j][2];//A_end
            intervals[it][2] = common_substrings0[j][1];//B_start
            intervals[it][3] = common_substrings0[j][1] + common_substrings0[j][2];//B_end
            it++;
            j++;
        }

        /*std::cout << "��B˳��ϲ��� " << intervals.size() << "\n";
        for (int wr = 0; wr < intervals.size(); wr++)
        {
            std::cout << intervals[wr][0] << "\t" << intervals[wr][1] << "\t" << intervals[wr][2] << "\t" << intervals[wr][3] << "\t" << intervals[wr][4] << "\n";
        }
        */
        int pre;
        //��B�ü�����
        for (i = 0; i < intervals.size(); )
        {
            if (intervals[i][4] == 0)
            {
                j = i + 1;
                while (j < intervals.size())
                {
                    if ((intervals[j][4] == 0) || (intervals[j][2] >= intervals[i][3])) //��ȫ�ں���
                        break;
                    else if (intervals[j][3] <= intervals[i][3])//0����1,0������
                    {
                        new_element = intervals[i];
                        intervals[i][1] = intervals[i][1] - intervals[i][3] + intervals[j][2]; //��B�ü�A
                        intervals[i][3] = intervals[j][2]; //�ü�B

                        new_element[0] = new_element[0] + intervals[j][3] - new_element[2]; //��B�ü�A
                        new_element[2] = intervals[j][3]; //�ü�B

                        if ((new_element[3] - new_element[2]) > (intervals[i][3] - intervals[i][2]))//��
                        {
                            intervals.erase(intervals.begin() + i);
                            intervals.insert(intervals.begin() + j, new_element);
                            break;
                        }
                    }
                    else
                    {
                        intervals[i][1] = intervals[i][1] - intervals[i][3] + intervals[j][2]; //��B�ü�A
                        intervals[i][3] = intervals[j][2]; //�ü�B
                        break;
                    }
                }
                //ǰһ��
                pre = i - 1;
                while (pre >= 0)
                {
                    if (intervals[pre][4] == 0)
                        pre--;
                    else
                        break;
                }
                if ((pre >= 0) && (intervals[i][4] == 0) && (intervals[pre][4] == 1) && (intervals[i][2] < intervals[pre][3]))
                {
                    if (intervals[i][3] <= intervals[pre][3]) //1����0��ɾ��0
                    {
                        //intervals.erase(intervals.begin() + i);
                        //j--;
                        //std::cout<< intervals[i][0] << "\t" << intervals[i][1] << "\t" << intervals[i][2] << "\t" << intervals[i][3] << "\t" << intervals[i][4] << "  1b0\n";
                        intervals[i][2] = intervals[i][3];
                    }
                    else
                    {
                        intervals[i][0] = intervals[i][0] + intervals[pre][3] - intervals[i][2]; //��B�ü�A
                        intervals[i][2] = intervals[pre][3]; //�ü�B
                    }
                }
                i = j;
                continue;
            }
            i++;
        }
        /*
        std::ofstream ofs("C:/Users/��ͨ/Desktop/1.txt", std::ios::binary | std::ios::out);

        ofs << "��B�ü������0 " << intervals.size() << "\n";
        for (int wr = 0; wr < intervals.size(); wr++)
        {
            ofs << intervals[wr][0] << "\t" << intervals[wr][1] << "\t" << intervals[wr][2] << "\t" << intervals[wr][3] << "\t" << intervals[wr][4] << "\n";
        }
        ofs << " \n\n\n";
        */
        //�޸Ķ�Ӧ��C_Strings
        for (i = 0, j = C_Strings.Subs.size() - 1; i < intervals.size(); )
        {
            if (intervals[i][4] == 0)
            {
                if ((intervals[i][3] - intervals[i][2]) > thresh)
                {
                    //����
                    while (true)
                    {
                        if (C_Strings.Subs[j].size() == 0)
                            break;
                        if ((length - C_Strings.Subs[j].back()[1] - C_Strings.Subs[j].back()[2]) < intervals[i][2])
                            C_Strings.Subs[j].pop_back();
                        else
                            break;
                    }
                    while (true)
                    {
                        if (C_Strings.Subs[j].size() == 0)
                            break;
                        if ((length - C_Strings.Subs[j][0][1]) > intervals[i][3])
                            C_Strings.Subs[j].erase(C_Strings.Subs[j].begin());
                        else
                            break;
                    }
                    if (C_Strings.Subs[j].size() == 0)
                    {
                        intervals.erase(intervals.begin() + i);
                        C_Strings.Subs.erase(C_Strings.Subs.begin() + j);
                        j--;
                        //ɾ��
                    }
                    else
                    {
                        intervals[i][0] = C_Strings.Subs[j][0][0];//A_start   ����ҿ�����
                        intervals[i][1] = C_Strings.Subs[j].back()[0] + C_Strings.Subs[j].back()[2];//A_end
                        intervals[i][2] = length - C_Strings.Subs[j].back()[1] - C_Strings.Subs[j].back()[2];//B_start
                        intervals[i][3] = length - C_Strings.Subs[j][0][1];//B_end
                        if ((intervals[i][3] - intervals[i][2]) > thresh)
                        {
                            i++;
                            j--;
                        }
                        else
                        {
                            intervals.erase(intervals.begin() + i);
                            C_Strings.Subs.erase(C_Strings.Subs.begin() + j);
                            j--;
                            //ɾ��
                        }
                    }

                }
                else
                {
                    intervals.erase(intervals.begin() + i);
                    C_Strings.Subs.erase(C_Strings.Subs.begin() + j);
                    j--;
                    //ɾ��
                }
            }
            else
                i++;
        }
        //��[4]==1/2�ַ�ͬԴ����
        quadras tmp;
        quadra start;
        bool tag_null = true;
        if (intervals.size() > 0 && intervals[0][4] == 0)
        {
            start = quadra({ 0, 0, 0, 0, 1 });
            it = 0;
        }
        else if (intervals.size() > 0 && intervals[0][4] == 1)
        {
            tmp.push_back(quadra({ 0, intervals[0][0], 0, intervals[0][2],1 }));
            new_intervals.push_back(tmp);
            quadras().swap(tmp);
            start = intervals[0];
            it = 1;
        }
        else//intervals.size() == 0
        {
            tmp.push_back(quadra({ 0, (int)_centre_len, 0, (int)length,1 }));
            new_intervals.push_back(tmp);
            tag_null = false;
            it = 0;
        }
        if (tag_null)
        {
            while (it < intervals.size())
            {
                j = it;
                while (j < intervals.size() && intervals[j][4] == 0)
                    j++;

                if (j == it)
                {
                    tmp.insert(tmp.begin(), quadra({ start[1],intervals[j][0],start[3],intervals[j][2],1 }));
                    new_intervals.push_back(tmp);
                    quadras().swap(tmp);
                    start = intervals[it];
                }
                else
                {
                    i = it;
                    while (i < j)
                    {
                        if ((intervals[i][0] >= start[1]) && (intervals[i][1] <= start[0]))
                            intervals[i][4] = 2;//���Ե���ֹ��
                        i++;
                    }
                    i = it;
                    while (i <= j && (i < intervals.size()))
                    {
                        if (intervals[i][4] == 0)
                            tmp.push_back(intervals[i]);
                        else
                        {
                            tmp.insert(tmp.begin(), quadra({ start[1],intervals[i][0],start[3],intervals[i][2],1 }));
                            new_intervals.push_back(tmp);
                            quadras().swap(tmp);
                            if (intervals[i][4] == 2)
                            {
                                tmp.insert(tmp.begin(), intervals[i]);
                                new_intervals.push_back(tmp);
                                quadras().swap(tmp);
                            }
                            start = intervals[i];
                        }
                        i++;
                    }
                }
                it = j + 1;
            }
            tmp.insert(tmp.begin(), quadra({ start[1],(int)_centre_len,start[3],(int)length,1 }));
            new_intervals.push_back(tmp);
        }
        quadras().swap(tmp);

        std::vector<quadra>().swap(intervals);//�����
    }
    else//+
    {
        get_common_substrings_vector(C_Strings, common_substrings0, false, length);//���Ķ࣬������
        /*
        std::cout << "C_Strings " << C_Strings.Subs.size() << " " << C_Strings.sign << " " << C_Strings.add_len << "\n";
        for (int wr = 0; wr < C_Strings.Subs.size(); wr++)
        {
            std::cout << wr << " " << C_Strings.Subs[wr].size() << "\n";
            for (int wr0 = 0; wr0 < C_Strings.Subs[wr].size(); wr0++)
            {
                std::cout << C_Strings.Subs[wr][wr0][0] << "\t" << C_Strings.Subs[wr][wr0][1] << "\t" << C_Strings.Subs[wr][wr0][2] << "\n";
            }
        }*/

        intervals.resize(common_substrings.size() + C_Strings.Subs.size());
        //std::cout << common_substrings.size() << "\t" << C_Strings.Subs.size() << "\t" << intervals.size() << " CCI\n";


        //��������
        it = 0;
        i = C_Strings.Subs.size() - 1;
        j = 0;
        //std::cout << j << "\t" << i << "\t" << it << "   0\n";
        while (i >= 0 && j < common_substrings.size())
        {
            B_start0 = length - C_Strings.Subs[i].back()[1] - C_Strings.Subs[i].back()[2];
            B_start1 = common_substrings[j][1];
            if (B_start0 <= B_start1)
            {
                //std::cout << j << "\t" << i << "\t" << it << "   1\n";
                intervals[it][4] = 0;//Ӳ�ȱ�ʶ ����
                intervals[it][0] = C_Strings.Subs[i][0][0];//A_start   ����ҿ�����
                intervals[it][1] = C_Strings.Subs[i].back()[0] + C_Strings.Subs[i].back()[2];//A_end
                intervals[it][2] = length - C_Strings.Subs[i].back()[1] - C_Strings.Subs[i].back()[2];//B_start
                intervals[it][3] = length - C_Strings.Subs[i][0][1];//B_end


                it++;
                i--;
            }
            else
            {
                //std::cout << j << "\t" << i << "\t" << it << "   2\n";
                intervals[it][4] = 1;// ����
                intervals[it][0] = common_substrings[j][0];//A_start   ����ҿ�����
                intervals[it][1] = common_substrings[j][0] + common_substrings[j][2];//A_end
                intervals[it][2] = common_substrings[j][1];//B_start
                intervals[it][3] = common_substrings[j][1] + common_substrings[j][2];//B_end
                it++;
                j++;
            }
            /*
            while (j < intervals.size())
            {
                if (intervals[j][3] <= C_Si[2])//interval  ĩ�� �����
                    ;
                else if (intervals[j][3] > C_Si[2] && intervals[j][3] <= C_Si[3]) //interval  ĩ�� ���м�
                {
                    if (intervals[j][2] >= C_Si[2]) //interval  ʼ�� ���м�
                            intervals[j][4] = -1;//ɾ��
                    else//interval  ʼ�� �����,  ɾ���Ҳ��ص�
                    {
                        intervals[j][3] = C_Si[2];
                        intervals[j][1] = C_Si[0];
                        intervals[j][1] = intervals[j][1] < intervals[j][0]? intervals[j][0]: intervals[j][1];
                    }
                }
                else//interval  ĩ�� ���ұ�
                {
                    if (intervals[j][2] >= C_Si[3]) //interval  ʼ�� ���ұ�
                        break;
                    else if (intervals[j][2] >= C_Si[2]) //interval  ʼ�� ���м� //ɾ�����ص�
                    {
                        intervals[j][2] = C_Si[3];
                        intervals[j][0] = C_Si[1];
                        intervals[j][0] = intervals[j][0] > intervals[j][1] ? intervals[j][1] : intervals[j][0];


                    }
                    else//interval  ʼ�� �����,  ɾ�������ص�
                    {

                    }
                }
                j++;
            }
            */
        }
        while (i >= 0)
        {
            //std::cout << j << "\t" << i << "\t" << it << "   3\n";
            intervals[it][4] = 0;//Ӳ�ȱ�ʶ ����
            intervals[it][0] = C_Strings.Subs[i][0][0];//A_start   ����ҿ�����
            intervals[it][1] = C_Strings.Subs[i].back()[0] + C_Strings.Subs[i].back()[2];//A_end
            intervals[it][2] = length - C_Strings.Subs[i].back()[1] - C_Strings.Subs[i].back()[2];//B_start
            intervals[it][3] = length - C_Strings.Subs[i][0][1];//B_end
            it++;
            i--;
        }
        while (j < common_substrings.size())
        {
            //std::cout << j << "\t" << i << "\t" << it << "   4\n";
            intervals[it][4] = 1;// ����
            intervals[it][0] = common_substrings[j][0];//A_start   ����ҿ�����
            intervals[it][1] = common_substrings[j][0] + common_substrings[j][2];//A_end
            intervals[it][2] = common_substrings[j][1];//B_start
            intervals[it][3] = common_substrings[j][1] + common_substrings[j][2];//B_end
            it++;
            j++;
        }
        /*
        std::cout << "��B˳��ϲ��� " << intervals.size() << "\n";
        for (int wr = 0; wr < intervals.size(); wr++)
        {
            std::cout << intervals[wr][0] << "\t" << intervals[wr][1] << "\t" << intervals[wr][2] << "\t" << intervals[wr][3] << "\t" << intervals[wr][4] << "\n";
        }*/

        int pre;
        //��B�ü�����
        for (i = 0; i < intervals.size(); )
        {
            if (intervals[i][4] == 0)
            {
                j = i + 1;
                while (j < intervals.size())
                {
                    if ((intervals[j][4] == 0) || (intervals[j][2] >= intervals[i][3])) //��ȫ�ں���
                        break;
                    else if (intervals[j][3] <= intervals[i][3])//0����1,0������
                    {
                        new_element = intervals[i];
                        intervals[i][1] = intervals[i][1] - intervals[i][3] + intervals[j][2]; //��B�ü�A
                        intervals[i][3] = intervals[j][2]; //�ü�B

                        new_element[0] = new_element[0] + intervals[j][3] - new_element[2]; //��B�ü�A
                        new_element[2] = intervals[j][3]; //�ü�B

                        if ((new_element[3] - new_element[2]) > (intervals[i][3] - intervals[i][2]))//��
                        {
                            intervals.erase(intervals.begin() + i);
                            intervals.insert(intervals.begin() + j, new_element);
                            break;
                        }
                    }
                    else
                    {
                        intervals[i][1] = intervals[i][1] - intervals[i][3] + intervals[j][2]; //��B�ü�A
                        intervals[i][3] = intervals[j][2]; //�ü�B
                        break;
                    }
                }
                //ǰһ��
                pre = i - 1;
                while (pre >= 0)
                {
                    if (intervals[pre][4] == 0)
                        pre--;
                    else
                        break;
                }
                if ((pre >= 0) && (intervals[i][4] == 0) && (intervals[pre][4] == 1) && (intervals[i][2] < intervals[pre][3]))
                {
                    if (intervals[i][3] <= intervals[pre][3]) //1����0��ɾ��0
                    {
                        //intervals.erase(intervals.begin() + i);
                        //j--;
                        //std::cout<< intervals[i][0] << "\t" << intervals[i][1] << "\t" << intervals[i][2] << "\t" << intervals[i][3] << "\t" << intervals[i][4] << "  1b0\n";
                        intervals[i][2] = intervals[i][3];
                    }
                    else
                    {
                        intervals[i][0] = intervals[i][0] + intervals[pre][3] - intervals[i][2]; //��B�ü�A
                        intervals[i][2] = intervals[pre][3]; //�ü�B
                    }
                }
                i = j;
                continue;
            }
            i++;
        }
        /*
        std::ofstream ofs("C:/Users/��ͨ/Desktop/1.txt", std::ios::binary | std::ios::out);

        ofs << "��B�ü������0 " << intervals.size() << "\n";
        for (int wr = 0; wr < intervals.size(); wr++)
        {
            ofs << intervals[wr][0] << "\t" << intervals[wr][1] << "\t" << intervals[wr][2] << "\t" << intervals[wr][3] << "\t" << intervals[wr][4] << "\n";
        }
        ofs << " \n\n\n";
        */
        //�޸Ķ�Ӧ��C_Strings
        for (i = 0, j = C_Strings.Subs.size() - 1; i < intervals.size(); )
        {
            if (intervals[i][4] == 0)
            {
                if ((intervals[i][3] - intervals[i][2]) > thresh)
                {
                    //����
                    while (true)
                    {
                        if (C_Strings.Subs[j].size() == 0)
                            break;
                        if ((length - C_Strings.Subs[j].back()[1] - C_Strings.Subs[j].back()[2]) < intervals[i][2])
                            C_Strings.Subs[j].pop_back();
                        else
                            break;
                    }
                    while (true)
                    {
                        if (C_Strings.Subs[j].size() == 0)
                            break;
                        if ((length - C_Strings.Subs[j][0][1]) > intervals[i][3])
                            C_Strings.Subs[j].erase(C_Strings.Subs[j].begin());
                        else
                            break;
                    }
                    if (C_Strings.Subs[j].size() == 0)
                    {
                        intervals.erase(intervals.begin() + i);
                        C_Strings.Subs.erase(C_Strings.Subs.begin() + j);
                        j--;
                        //ɾ��
                    }
                    else
                    {
                        intervals[i][0] = C_Strings.Subs[j][0][0];//A_start   ����ҿ�����
                        intervals[i][1] = C_Strings.Subs[j].back()[0] + C_Strings.Subs[j].back()[2];//A_end
                        intervals[i][2] = length - C_Strings.Subs[j].back()[1] - C_Strings.Subs[j].back()[2];//B_start
                        intervals[i][3] = length - C_Strings.Subs[j][0][1];//B_end
                        if ((intervals[i][3] - intervals[i][2]) > thresh)
                        {
                            i++;
                            j--;
                        }
                        else
                        {
                            intervals.erase(intervals.begin() + i);
                            C_Strings.Subs.erase(C_Strings.Subs.begin() + j);
                            j--;
                            //ɾ��
                        }
                    }

                }
                else
                {
                    intervals.erase(intervals.begin() + i);
                    C_Strings.Subs.erase(C_Strings.Subs.begin() + j);
                    j--;
                    //ɾ��
                }
            }
            else
                i++;
        }
        /*
        ofs << "��B�ü������1 " << intervals.size() << "\n";
        for (int wr = 0; wr < intervals.size(); wr++)
        {
            ofs << intervals[wr][0] << "\t" << intervals[wr][1] << "\t" << intervals[wr][2] << "\t" << intervals[wr][3] << "\t" << intervals[wr][4] << "\n";
        }
        ofs << " \n\n\n";
        

        ofs << "C_Strings.Subs " << C_Strings.Subs.size() << "\n";
        int num = 0;
        for (i = 0, j = C_Strings.Subs.size() - 1; i < intervals.size() && j>=0;)
        {
            if (intervals[i][4] == 0)
            {
                ofs << i << " " << j << " \n";
                ofs << intervals[i][0] << "\t" << intervals[i][1] << "\t" << intervals[i][2] << "\t" << intervals[i][3] << "\t" << intervals[i][4] << "\n";
                ofs << C_Strings.Subs[j][0][0] << " " << C_Strings.Subs[j].back()[0] + C_Strings.Subs[j].back()[2] << " " << length - C_Strings.Subs[j].back()[1] - C_Strings.Subs[j].back()[2] << " "<<length - C_Strings.Subs[j][0][1] << "\n\n";
                for (auto xx : C_Strings.Subs[j])
                    ofs << "xx " << xx[0] << " \t" << xx[1] << " \t" << xx[2] << " \n";
                i++;
                j--;
                num++;
            }
            else
            {
                i++;
            }
        }
        ofs << i << " " << j << " \n";
        ofs << "intervals " << num << "\n";
        */

        //��[4]==1/2�ַ�ͬԴ����
        quadras tmp;
        quadra start;
        bool tag_null = true;
        if (intervals.size() > 0 && intervals[0][4] == 0)
        {
            start = quadra({ 0, 0, 0, 0, 1 });
            it = 0;
        }
        else if (intervals.size() > 0 && intervals[0][4] == 1)
        {
            tmp.push_back(quadra({ 0, intervals[0][0], 0, intervals[0][2],1 }));
            new_intervals.push_back(tmp);
            quadras().swap(tmp);
            start = intervals[0];
            it = 1;
        }
        else//intervals.size() == 0
        {
            tmp.push_back(quadra({ 0, (int)_centre_len, 0, (int)length,1 }));
            new_intervals.push_back(tmp);
            it = 0;
            tag_null = false;
        }
        if (tag_null) 
        {
            while (it < intervals.size())
            {
                j = it;
                while (j < intervals.size() && intervals[j][4] == 0)
                    j++;
                if (j == it)
                {
                    tmp.insert(tmp.begin(), quadra({ start[1],intervals[j][0],start[3],intervals[j][2],1 }));
                    new_intervals.push_back(tmp);
                    quadras().swap(tmp);
                    start = intervals[it];
                }
                else
                {
                    i = it;
                    while (i < j)
                    {
                        if ((intervals[i][0] >= start[1]) && (intervals[i][1] <= start[0]))
                            intervals[i][4] = 2;//���Ե���ֹ��
                        i++;
                    }
                    i = it;
                    while (i <= j && i < intervals.size())
                    {
                        if (intervals[i][4] == 0)
                            tmp.push_back(intervals[i]);
                        else
                        {
                            tmp.insert(tmp.begin(), quadra({ start[1],intervals[i][0],start[3],intervals[i][2],1 }));
                            new_intervals.push_back(tmp);
                            quadras().swap(tmp);
                            if (intervals[i][4] == 2)
                            {
                                tmp.insert(tmp.begin(), intervals[i]);
                                new_intervals.push_back(tmp);
                                quadras().swap(tmp);
                            }
                            start = intervals[i];
                        }
                        i++;
                    }
                }
                it = j + 1;
            }
            tmp.insert(tmp.begin(), quadra({ start[1],(int)_centre_len,start[3],(int)length,1 }));
            new_intervals.push_back(tmp);
        }
        quadras().swap(tmp);

        std::vector<quadra>().swap(intervals);//�����
        /*
        ofs << "\n��ͬԴ���� " << new_intervals.size() << "\n";
        for (int wr = 0; wr < new_intervals.size(); wr++)
        {
            ofs << wr << " " << new_intervals[wr].size() << "\n";
            for (int wr0 = 0; wr0 < new_intervals[wr].size(); wr0++)
            {
                ofs << new_intervals[wr][wr0][0] << "\t" << new_intervals[wr][wr0][1] << "\t" << new_intervals[wr][wr0][2] << "\t" << new_intervals[wr][wr0][3] << "\t" << new_intervals[wr][wr0][4] << "\n";
            }
        }
        exit(1);
        */
    }
}

//����sufferTree����˫�ȣ�����all_pairwise_gap
//���贫�룬���Ѿ���ʼ��
//���أ�˫���бȶԵõ�������gap������Ϊn��vector��ÿ��Ԫ���г���Ϊ2��array��ÿ��Ԫ���������ɸ�Insertion��vector
void star_alignment::StarAligner::mul_sv_maf_func(int i, std::ifstream &ns, const suffix_array::SuffixArray<nucleic_acid_pseudo::NUMBER>& st,
    std::vector<std::array<std::vector<utils::Insertion>, 2>> &all_pairwise_gaps, std::vector<insert> &SNP_vector,int threshold1, int threshold2) const
{
    std::fstream io(arguments::out_file_name + "/NoN/" + std::to_string(i), std::ios::binary | std::ios::in | std::ios::out); //�ж�����·���Ϸ���
    if (!io)
    {
        std::cout << "cannot access file " << arguments::out_file_name + "/NoN/" + std::to_string(i) << '\n';
        exit(1);
    }

    Kband* kband = new Kband();
    int  common_len1, common_len0;// name_len = 0;
    std::vector<unsigned char> veci;
    insert a_gap;
    insert b_gap;
    quadras tmp;
    unsigned char* array_A = NULL;   //0�����ͬԴ����  1��������ȫ��ͬԴ���� �Լ� �Ѿ���ռ�ķ�ͬԴ  2��������ͬԴ����
    unsigned char* array_B = NULL;
    //std::cout << "----------------------- " << i << " " << _name[i] << " " << _lengths[i] << "------------------------\n";
    std::vector<triple> common_substrings;
    std::vector<triple> common_substrings0;
    std::vector <std::vector<quadra>> chains;
    std::vector <std::vector<quadra>> chains0;
    Substrings C_String;
    std::vector<quadra> interval;
    std::vector<quadras> intervals;
    if (i == _centre)
    {
        common_substrings.push_back(triple({ 0,0,(int)_centre_len }));

    }
    else if (_lengths[i] != 0)
    {
        array_A = new unsigned char[_max_len];   //0�����ͬԴ����  1��������ȫ��ͬԴ���� �Լ� �Ѿ���ռ�ķ�ͬԴ  2��������ͬԴ����
        array_B = new unsigned char[_max_len];
        memset(array_A, 1, _max_len);
        memset(array_B, 1, _max_len);

        utils::read_BYTE_seq(io, veci);//����i����
        _sequences[i].seq = utils::BLAST_PackDNA(veci);//д��_sequences��
        //std::cout << i << " " << _name[i] << " \nJaccardSimilarity\n";
        const auto align_start = std::chrono::high_resolution_clock::now(); //��¼�ȶ���ʼʱ��
        bool similarity01 = utils::JaccardSimilarity(filter1, filter0, veci, _K);
        //std::cout << similarity01 << "\n";
        //std::cout << " consumes " << (std::chrono::high_resolution_clock::now() - align_start) << '\n';
        if (similarity01)//++++++++++++++
        {
            common_substrings = sv_optimal_path(chains, st.get_common_substrings(veci.cbegin(), veci.cend(), threshold1, true), true); //common_substrings [A.index,B.index,length]
            common_len1 = 0;
            for (int j = 0; j < common_substrings.size(); j++)
                common_len1 += common_substrings[j][2];
            //std::cout << common_len1 << " common_len1\n";

            for (int zt = 0; zt < veci.size(); zt++)
                veci[zt] = 3 - veci[zt];
            std::reverse(veci.begin(), veci.end());

            common_substrings0 = sv_optimal_path(chains0, st.get_common_substrings_back(veci.cbegin(), veci.cend(), threshold1, true), false); //common_substrings [A.index,B.index,length]
            common_len0 = 0;
            for (int j = 0; j < common_substrings0.size(); j++)
                common_len0 += common_substrings0[j][2];
            //std::cout << common_len0 << " common_len0_back\n";

            if (common_len0 > common_len1)//Ԥ�����
            {
                std::cout << "                    | Info : Prediction error! \n";
                std::vector<triple>().swap(common_substrings0);
                common_substrings0 = sv_optimal_path(chains0, st.get_common_substrings(veci.cbegin(), veci.cend(), threshold1, true), true); //common_substrings [A.index,B.index,length]
                common_len0 = 0;
                for (int j = 0; j < common_substrings0.size(); j++)
                    common_len0 += common_substrings0[j][2];
                //std::cout << common_len0 << " new_common_len0\n";
            }

        }
        else//-----------------------------------
        {
            common_substrings = sv_optimal_path(chains, st.get_common_substrings_back(veci.cbegin(), veci.cend(), threshold1, true), false); //common_substrings [A.index,B.index,length]
            common_len1 = 0;
            for (int j = 0; j < common_substrings.size(); j++)
                common_len1 += common_substrings[j][2];
            //std::cout << common_len1 << " common_len1_back\n";

            for (int zt = 0; zt < veci.size(); zt++)
                veci[zt] = 3 - veci[zt];
            std::reverse(veci.begin(), veci.end());

            common_substrings0 = sv_optimal_path(chains0, st.get_common_substrings(veci.cbegin(), veci.cend(), threshold1, true), true); //common_substrings [A.index,B.index,length]
            common_len0 = 0;
            for (int j = 0; j < common_substrings0.size(); j++)
                common_len0 += common_substrings0[j][2];
            //std::cout << common_len0 << " common_len0\n";

            if (common_len1 > common_len0)//Ԥ�����
            {
                //std::cout << " Prediction error! \n";
                for (int zt = 0; zt < veci.size(); zt++)
                    veci[zt] = 3 - veci[zt];
                std::reverse(veci.begin(), veci.end());

                std::vector<triple>().swap(common_substrings);

                common_substrings = sv_optimal_path(chains, st.get_common_substrings(veci.cbegin(), veci.cend(), threshold1, true), true); //common_substrings [A.index,B.index,length]
                common_len1 = 0;
                for (int j = 0; j < common_substrings.size(); j++)
                    common_len1 += common_substrings[j][2];
                //std::cout << common_len1 << " new_common_len1\n";
            }
        }

        if (common_len0 > common_len1)//��
        {
            _sign[i] = false;//����-
            //common_substrings.swap(common_substrings0);
            //���Ϻ��� 0  

            std::vector<std::vector<triple>>().swap(C_String.Subs);
            std::vector<quadra>().swap(interval);
            std::vector<quadras>().swap(intervals);
            hebing_substrings(C_String, intervals, interval, common_substrings, common_substrings0, _centre_len, veci.size(), false);

            std::vector<triple>().swap(common_substrings);
            std::vector<triple>().swap(common_substrings0);
            utils::write_BYTE_seq(io, veci, _name[i].size() + 2, _TU[i]);//д��i����
            
            delete []_sequences[i].seq;
            _sequences[i].seq = utils::BLAST_PackDNA(veci);//д��_sequences��
        }
        else//��
        {
            _sign[i] = true;

            std::vector<std::vector<triple>>().swap(C_String.Subs);
            std::vector<quadra>().swap(interval);
            std::vector<quadras>().swap(intervals);
            hebing_substrings(C_String, intervals, interval, common_substrings, common_substrings0, _centre_len, veci.size(), true);

            std::vector<triple>().swap(common_substrings);
            std::vector<triple>().swap(common_substrings0);
            //���Ϻ��� 1
        }

        std::vector<unsigned char>().swap(veci); //�ͷ�veci
        //std::cout <<  _sequences[i].length <<"\n";
    }
    io.close();
    //std::cout << _sequences[0].length<< " "<< _sequences[i].length << "  length\n";
    std::array<std::vector<utils::Insertion>, 2> pairwise_gaps; //���� ˫��gap
    EmptySet();

    std::reverse_iterator<std::vector<std::vector<triple>>::iterator> C_Strer(C_String.Subs.rbegin());
    
    for (size_t j = 0; j != intervals.size(); ++j)//0��2�����䷵��Ӳ�ȵ�gap�ͺã�չ�����油�ȶ�gap��more_gap
    {
        //continue;/////////////////////!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if (intervals[j].size() == 1)
        {
            const int centre_begin = intervals[j][0][0];
            const int centre_end = intervals[j][0][1];
            const int sequence_begin = intervals[j][0][2];
            const int sequence_end = intervals[j][0][3];

            if (intervals[j][0][4] == 1)//�����ȶ�
            {
                memset(array_A + centre_begin, 0, (centre_end - centre_begin) * sizeof(array_A[0]));
                memset(array_B + sequence_begin, 0, (sequence_end - sequence_begin) * sizeof(array_B[0]));
                //std::cout << centre_begin << " " << centre_end << " " << sequence_begin << " " << sequence_end << "  1\n";
                auto [lhs_gaps, rhs_gaps] = new_main_Align(kband, _sequences[_centre], centre_begin, centre_end,  //��ͬԴ���򣬶�̬�滮�ȶ�
                    _sequences[i], sequence_begin, sequence_end, threshold2, SNP_vector[i], _mg_tag, _more_gap[i], false); //���ε���threshС��Ȼ��k-band
                int an = 0, bn = 0;
                for (int ii = 0; ii < lhs_gaps.size(); ii++)
                {
                    an += std::get<1>(lhs_gaps[ii]);
                    if (a_gap.size() && (pairwise_gaps[0].back().index == std::get<0>(lhs_gaps[ii])))
                        pairwise_gaps[0].back().number += std::get<1>(lhs_gaps[ii]);
                    else
                        pairwise_gaps[0].push_back(utils::Insertion({ (size_t)std::get<0>(lhs_gaps[ii]),(size_t)std::get<1>(lhs_gaps[ii]) }));
                }
                for (int ii = 0; ii < rhs_gaps.size(); ii++)
                {
                    bn += std::get<1>(rhs_gaps[ii]);
                    if (a_gap.size() && (pairwise_gaps[1].back().index == std::get<0>(rhs_gaps[ii])))
                        pairwise_gaps[1].back().number += std::get<1>(rhs_gaps[ii]);
                    else
                        pairwise_gaps[1].push_back(utils::Insertion({ (size_t)std::get<0>(rhs_gaps[ii]),(size_t)std::get<1>(rhs_gaps[ii]) }));
                }

                insert().swap(lhs_gaps);
                insert().swap(rhs_gaps);
                //std::cout << centre_begin << " " << centre_end << " " << sequence_begin << " " << sequence_end << "  1\n";
            }
            else if (intervals[j][0][4] == 2)//Ӳ��
            {

                memset(array_A + centre_begin, 2, (centre_end - centre_begin) * sizeof(array_A[0]));
                memset(array_B + sequence_begin, 2, (sequence_end - sequence_begin) * sizeof(array_B[0]));
                //std::cout << centre_begin << " " << centre_end << " " << sequence_begin << " " << sequence_end << " 2\n";
                //����ֱ�Ӵ�������Ӳ��
                insert lhs_gaps;
                insert rhs_gaps;
                if ((sequence_end - sequence_begin) < (centre_end - centre_begin)) //���ĳ�
                {
                    rhs_gaps.push_back(in(sequence_end, centre_end - centre_begin - sequence_end + sequence_begin));
                    for (int ii = 0; ii < rhs_gaps.size(); ii++)
                    {
                        if (a_gap.size() && (pairwise_gaps[1].back().index == std::get<0>(rhs_gaps[ii])))
                            pairwise_gaps[1].back().number += std::get<1>(rhs_gaps[ii]);
                        else
                            pairwise_gaps[1].push_back(utils::Insertion({ (size_t)std::get<0>(rhs_gaps[ii]),(size_t)std::get<1>(rhs_gaps[ii]) }));
                    }
                    insert().swap(rhs_gaps);
                }
                else if ((sequence_end - sequence_begin) > (centre_end - centre_begin))
                {
                    lhs_gaps.push_back(in(centre_end, sequence_end - sequence_begin - centre_end + centre_begin));
                    for (int ii = 0; ii < lhs_gaps.size(); ii++)
                    {
                        if (a_gap.size() && (pairwise_gaps[0].back().index == std::get<0>(lhs_gaps[ii])))
                            pairwise_gaps[0].back().number += std::get<1>(lhs_gaps[ii]);
                        else
                            pairwise_gaps[0].push_back(utils::Insertion({ (size_t)std::get<0>(lhs_gaps[ii]),(size_t)std::get<1>(lhs_gaps[ii]) }));
                    }
                    insert().swap(lhs_gaps);
                }
                /*std::cout << C_Strer - C_String.Subs.rbegin() << " " << C_String.Subs.size() << " 2 C_Strer\n";
                if (C_Strer - C_String.Subs.rbegin() > 22)
                    exit(0);*/
                    //���뺯�������油
                nibu_Align(kband, _sequences[_centre], _sequences[i], threshold2, C_Strer, SNP_vector[i], _more_gap[i], intervals[j][0]);
                //std::cout << centre_begin << " " << centre_end << " " << sequence_begin << " " << sequence_end << "  2\n";
            }
            else
            {
                std::cout << "error: intervals[j][0][4] != 0/2\n";
                exit(-1);
            }
        }
        else
        {
            const int centre_begin = intervals[j][0][0];
            const int centre_end = intervals[j][0][1];
            const int sequence_begin = intervals[j][0][2];
            const int sequence_end = intervals[j][0][3];

            //std::cout << centre_begin << " " << centre_end << " " << sequence_begin << " " << sequence_end << "  3\n";
            if (intervals[j][0][4] != 1)
            {
                std::cout << "error: intervals[j][0][4] != 1\n";
                exit(-1);
            }
            intervals[j].erase(intervals[j].begin());

            for (int me = 0; me < intervals[j].size(); me++)
            {
                memset(array_A + intervals[j][me][0], 2, (intervals[j][me][1] - intervals[j][me][0]) * sizeof(array_A[0]));
                memset(array_B + intervals[j][me][2], 2, (intervals[j][me][3] - intervals[j][me][2]) * sizeof(array_B[0]));
            }
            /*
            std::cout << C_Strer - C_String.Subs.rbegin() <<" "<< intervals[j].size() <<" "<< C_String.Subs.size() << " else C_Strer\n";
            if (C_Strer+ intervals[j].size() - C_String.Subs.rbegin() > 21)
                exit(0);
                */
            auto [lhs_gaps, rhs_gaps] = new_mul_main_Align(kband, _sequences[_centre], centre_begin, centre_end,  //��ͬԴ���򣬶�̬�滮�ȶ�
                _sequences[i], sequence_begin, sequence_end, threshold2, SNP_vector[i], _mg_tag, _more_gap[i], intervals[j], C_Strer); //���ε���threshС��Ȼ��k-band
            int an = 0, bn = 0;
            for (int ii = 0; ii < lhs_gaps.size(); ii++)
            {
                an += std::get<1>(lhs_gaps[ii]);
                if (a_gap.size() && (pairwise_gaps[0].back().index == std::get<0>(lhs_gaps[ii])))
                    pairwise_gaps[0].back().number += std::get<1>(lhs_gaps[ii]);
                else
                    pairwise_gaps[0].push_back(utils::Insertion({ (size_t)std::get<0>(lhs_gaps[ii]),(size_t)std::get<1>(lhs_gaps[ii]) }));
            }
            for (int ii = 0; ii < rhs_gaps.size(); ii++)
            {
                bn += std::get<1>(rhs_gaps[ii]);
                if (a_gap.size() && (pairwise_gaps[1].back().index == std::get<0>(rhs_gaps[ii])))
                    pairwise_gaps[1].back().number += std::get<1>(rhs_gaps[ii]);
                else
                    pairwise_gaps[1].push_back(utils::Insertion({ (size_t)std::get<0>(rhs_gaps[ii]),(size_t)std::get<1>(rhs_gaps[ii]) }));
            }
            //if ((centre_end - centre_begin + an) != (sequence_end - sequence_begin + bn))
            //    std::cout << an << " " << bn << " " << (centre_end - centre_begin + an) << " " << (sequence_end - sequence_begin + bn) << "  ??3\n";
            insert().swap(lhs_gaps);
            insert().swap(rhs_gaps);
            //std::cout << centre_begin << " " << centre_end << " " << sequence_begin << " " << sequence_end << "  3\n";
        }
        // EmptySet();
    }
    
    if (_mg_tag)
        if (i != _centre)
        {
            int name_i = i;
            if (i < _centre)
                name_i++;
            std::ofstream ofi(_MAFinfo.path + std::to_string(name_i) + ".maf", std::ios::binary | std::ios::out);
            //std::ofstream ofsv(_MAFinfo.path + std::to_string(name_i) + ".maf", std::ios::binary | std::ios::out);
            //std::ofstream ofii(_MAFinfo.path + std::to_string(name_i) + ".tmp", std::ios::binary | std::ios::out);
            {
                std::lock_guard<std::mutex> lock(threadPool0->mutex_ns); // �ڷ��ʹ������֮ǰ����
                utils::read_N_tmp(ns, _N_gap[i], i);
            }

            const auto align_start = std::chrono::high_resolution_clock::now();
            //std::cout << chains.size() << " " << chains0.size() << " chains size\n";
            
            getSV_write2file(pairwise_gaps, kband, ofi, i, chains, chains0, array_A, array_B);//д��sv //�ڲ���������ת����Ngap����
            
            //ofsv.close();
            //std::cout << "getSV consumes " << (std::chrono::high_resolution_clock::now() - align_start) << '\n';

            std::vector<std::vector<unsigned char>> ni_seq(2);
            std::vector<std::vector<utils::Insertion>> N_insertions(2);
            std::vector<std::string> name(2);
            std::vector<bool> sign(2);
            std::vector<bool> TU(2);
            std::vector<utils::more_block> more_gap(2);
            //ni_seq
            ni_seq[1] = maf_bin_to_vector(_sequences[i]);
            delete []_sequences[i].seq;//�ͷŵ�ǰ����i
            _sequences[i].length = 0;//�ͷŵ�ǰ����i
            ni_seq[0] = maf_bin_to_vector(_sequences[_centre]);
            //N_insertions
            N_insertions[0] = _N_gap[_centre];
            N_insertions[1].swap(_N_gap[i]);
            //name
            name[0] = _name[_centre];
            name[1] = _name[i];
            //sign
            sign[0] = _sign[_centre];
            sign[1] = _sign[i];
            //TU
            TU[0] = _TU[_centre];
            TU[1] = _TU[i];
            //more_gap
            more_gap[0] = _more_gap[_centre];
            more_gap[1].swap(_more_gap[i]);
            //ofii << name[1] << "\n";
            utils::insertion_gap_more_new(ofi, ni_seq, N_insertions, name, sign, TU, more_gap, _MAFinfo.thresh1, array_A, array_B);
            //ofii << "-1";
            ofi.close();
            //ofii.close();
            //std::string command = "cp " + _MAFinfo.path + std::to_string(name_i) + ".maf" + " " + _MAFinfo.path + std::to_string(name_i) + "_i.maf";
            //int result = std::system(command.c_str());
            //exit(0);
            delete[] array_A;
            delete[] array_B;
            Stream::sort_hebing_plus(_MAFinfo.path + std::to_string(name_i) + ".maf", _name[_centre]);
        }
    
    for (size_t j = 0; j < pairwise_gaps[0].size();)
    {
        if (pairwise_gaps[0][j].number == 0)
            pairwise_gaps[0].erase(pairwise_gaps[0].begin() + j);  // ɾ������� Insertion
        else
            j++;
    }
    for (size_t j = 0; j < pairwise_gaps[1].size();)
    {
        if (pairwise_gaps[1][j].number == 0)
            pairwise_gaps[1].erase(pairwise_gaps[1].begin() + j);  // ɾ������� Insertion
        else
            j++;
    }
    delete kband;
    all_pairwise_gaps[i].swap(pairwise_gaps); //ѭ��һ�κ󣬼���һ��˫��
    EmptySet();
}


void star_alignment::StarAligner::mul_fasta_func(int i, const suffix_array::SuffixArray<nucleic_acid_pseudo::NUMBER>& st,
    std::vector<std::array<std::vector<utils::Insertion>, 2>>& all_pairwise_gaps, std::vector<insert>& SNP_vector, int threshold1, int threshold2) const
{
    int  common_len, common_len0;
    std::vector<unsigned char> veci;
    insert a_gap;
    insert b_gap;
    Kband* kband = new Kband();
    std::fstream io(arguments::out_file_name + "/NoN/" + std::to_string(i), std::ios::binary | std::ios::in | std::ios::out); //�ж�����·���Ϸ���
    if (!io)
    {
        std::cout << "cannot access file " << arguments::out_file_name + "/NoN/" + std::to_string(i) << '\n';
        exit(1);
    }
    std::vector<triple> common_substrings;
    std::vector<triple> common_substrings0;
    if (i == _centre)
    {
        common_substrings.push_back(triple({ 0,0,(int)_centre_len }));
        //utils::read_BYTE_seq(io, veci);
    }
    else if (_lengths[i] != 0)
    {
        utils::read_BYTE_seq(io, veci);
        _sequences[i].seq = utils::BLAST_PackDNA(veci);//д��_sequences��

        //std::cout << i << " " << _name[i] << "JaccardSimilarity\n";
        const auto align_start = std::chrono::high_resolution_clock::now(); //��¼�ȶ���ʼʱ��

        bool similarity01 = utils::JaccardSimilarity(filter1, filter0, veci, _K);
        //std::cout << similarity01 << "\n";
        //std::cout << " consumes " << (std::chrono::high_resolution_clock::now() - align_start) << '\n';
        //if (_lengths[i] > 500)
        //{
        if (similarity01)//++++++++++++++
        {
            _sign[i] = true;//����+
            common_substrings = _optimal_path(st.get_common_substrings(veci.cbegin(), veci.cend(), threshold1, true)); //common_substrings [A.index,B.index,length]
            common_len = 0;

            for (int j = 0; j < common_substrings.size(); j++)
                common_len += common_substrings[j][2];
            //std::cout << common_len << " common_len1\n";
            if (common_len < veci.size() / 3)//��Ҫȡ�油�� 100000 - 10
            {
                for (int zt = 0; zt < veci.size(); zt++)
                    veci[zt] = 3 - veci[zt];
                std::reverse(veci.begin(), veci.end());

                common_len0 = 0;
                common_substrings0 = _optimal_path(st.get_common_substrings(veci.cbegin(), veci.cend(), threshold1, true)); //common_substrings [A.index,B.index,length]
                //utils::set_BYTE_seq_to_noN(veci,i);
                for (int j = 0; j < common_substrings0.size(); j++)
                    common_len0 += common_substrings0[j][2];
                //std::cout << common_len0 << " common_len0\n";

                if (common_len >= common_len0)//+++++++++++++
                {
                    _sign[i] = true;//����+
                    std::vector<triple>().swap(common_substrings0);
                }
                else//-----------------
                {
                    _sign[i] = false;//����-
                    std::vector<triple>().swap(common_substrings);
                    common_substrings.swap(common_substrings0);
                    utils::write_BYTE_seq(io, veci, _name[i].size() + 2, _TU[i]);//�ļ�д��i����
                    delete[]_sequences[i].seq;
                    _sequences[i].seq = utils::BLAST_PackDNA(veci);//����д��_sequences��
                }
            }
        }
        else//-----------------------------------
        {
            _sign[i] = false;//����-
            for (int zt = 0; zt < veci.size(); zt++)
                veci[zt] = 3 - veci[zt];
            std::reverse(veci.begin(), veci.end());
            utils::write_BYTE_seq(io, veci, _name[i].size() + 2, _TU[i]);//�ļ�д��i����  ---
            delete[]_sequences[i].seq;
            _sequences[i].seq = utils::BLAST_PackDNA(veci);//����д��_sequences��   ---

            common_substrings = _optimal_path(st.get_common_substrings(veci.cbegin(), veci.cend(), threshold1, true)); //common_substrings [A.index,B.index,length]
            common_len = 0;

            for (int j = 0; j < common_substrings.size(); j++)
                common_len += common_substrings[j][2];
            //std::cout << common_len << " common_len0\n";

            if (common_len < veci.size() / 3)//��Ҫȡ�油�� 100000 - 10
            {
                for (int zt = 0; zt < veci.size(); zt++) //������
                    veci[zt] = 3 - veci[zt];
                std::reverse(veci.begin(), veci.end());

                common_len0 = 0;
                common_substrings0 = _optimal_path(st.get_common_substrings(veci.cbegin(), veci.cend(), threshold1, true)); //common_substrings [A.index,B.index,length]
                //utils::set_BYTE_seq_to_noN(veci,i);
                for (int j = 0; j < common_substrings0.size(); j++)
                    common_len0 += common_substrings0[j][2];
                //std::cout << common_len0 << " common_len1\n";

                if (common_len >= common_len0)//-------------
                {
                    _sign[i] = false;//����-
                    std::vector<triple>().swap(common_substrings0);
                }
                else//++++++++++++++++++++
                {
                    _sign[i] = true;//����+
                    std::vector<triple>().swap(common_substrings);
                    common_substrings.swap(common_substrings0);
                    utils::write_BYTE_seq(io, veci, _name[i].size() + 2, _TU[i]);//�ļ�д��i����
                    delete[]_sequences[i].seq;
                    _sequences[i].seq = utils::BLAST_PackDNA(veci);//����д��_sequences��
                }

            }
        }
        //}
        /*else
        {
            if (!similarity01)
            {
                _sign[i] = false;//����-
                for (int zt = 0; zt < veci.size(); zt++)
                    veci[zt] = 3 - veci[zt];
                std::reverse(veci.begin(), veci.end());
                utils::write_BYTE_seq(io, veci, _name[i].size() + 2, _TU[i]);//�ļ�д��i����  ---
                delete[]_sequences[i].seq;
                _sequences[i].seq = utils::BLAST_PackDNA(veci);//����д��_sequences��   ---
            }
            std::vector<triple>().swap(common_substrings);
        }*/
        std::vector<unsigned char>().swap(veci); //�ͷ�veci
       
    }
    
    io.close();
    //std::cout << "common_substrings pre-memory" << common_substrings.size() << "\n";
    std::vector<quadra> intervals;
    intervals.reserve(common_substrings.size() + 1);
    //std::cout << "common_substrings memory OK" <<"\n";
    if (common_substrings.empty())
    {
        intervals.push_back(quadra({ 0, (int)_sequences[_centre].length, 0, (int)_sequences[i].length }));
    }
    else
    {
        if (common_substrings[0][0] || common_substrings[0][1])
            intervals.push_back(quadra({ 0, common_substrings[0][0], 0, common_substrings[0][1] }));

        for (size_t j = 0, end_index = common_substrings.size() - 1; j != end_index; ++j)
            if (common_substrings[j][0] + common_substrings[j][2] != common_substrings[j + 1][0] ||
                common_substrings[j][1] + common_substrings[j][2] != common_substrings[j + 1][1])
                intervals.push_back(quadra
                ({
                    common_substrings[j][0] + common_substrings[j][2], common_substrings[j + 1][0],
                    common_substrings[j][1] + common_substrings[j][2], common_substrings[j + 1][1]
                    }));

        if (common_substrings.back()[0] + common_substrings.back()[2] != _centre_len ||
            common_substrings.back()[1] + common_substrings.back()[2] != _lengths[i])
            intervals.push_back(quadra
            ({
                common_substrings.back()[0] + common_substrings.back()[2], (int)_centre_len,
                common_substrings.back()[1] + common_substrings.back()[2], (int)_lengths[i]
                }));
    }
    std::vector<triple>().swap(common_substrings);
    EmptySet();

    std::array<std::vector<utils::Insertion>, 2> pairwise_gaps; //���� ˫��gap
    int jindu = 0;
    //std::cout << "\n NeedlemanWunshReusable:::begin\n";

    for (size_t j = 0; j != intervals.size(); ++j)
    {
        const size_t centre_begin = intervals[j][0];
        const size_t centre_end = intervals[j][1];
        const size_t sequence_begin = intervals[j][2];
        const size_t sequence_end = intervals[j][3];

        auto [lhs_gaps, rhs_gaps] = new_main_Align(kband, _sequences[_centre], centre_begin, centre_end,  //��ͬԴ���򣬶�̬�滮�ȶ�
            _sequences[i], sequence_begin, sequence_end, threshold2, SNP_vector[i], _mg_tag, _more_gap[i], false); //���ε���threshС��Ȼ��k-band

        for (int ii = 0; ii < lhs_gaps.size(); ii++)
        {
            if (a_gap.size() && (pairwise_gaps[0].back().index == std::get<0>(lhs_gaps[ii])))
                pairwise_gaps[0].back().number += std::get<1>(lhs_gaps[ii]);
            else
                pairwise_gaps[0].push_back(utils::Insertion({ (size_t)std::get<0>(lhs_gaps[ii]),(size_t)std::get<1>(lhs_gaps[ii]) }));
        }
        for (int ii = 0; ii < rhs_gaps.size(); ii++)
        {
            if (a_gap.size() && (pairwise_gaps[1].back().index == std::get<0>(rhs_gaps[ii])))
                pairwise_gaps[1].back().number += std::get<1>(rhs_gaps[ii]);
            else
                pairwise_gaps[1].push_back(utils::Insertion({ (size_t)std::get<0>(rhs_gaps[ii]),(size_t)std::get<1>(rhs_gaps[ii]) }));
        }
        insert().swap(lhs_gaps);
        insert().swap(rhs_gaps);
        // EmptySet();
    }
    delete kband;
    if (i != _centre)
        delete[]_sequences[i].seq;
    std::vector<quadra>().swap(intervals);
    all_pairwise_gaps[i].swap(pairwise_gaps); //ѭ��һ�κ󣬼���һ��˫��
    EmptySet();
    return;
}

auto star_alignment::StarAligner::mul_sv_maf_pairwise_align(int threshold1, int threshold2) const -> std::vector<std::array<std::vector<utils::Insertion>, 2>>
{
    cout_cur_time();
    std::cout << "Start: build Suffix Array No." << _centre << "\n";
    suffix_array::SuffixArray<nucleic_acid_pseudo::NUMBER> st(_sequences[_centre], 0, _sequences[_centre].length, nucleic_acid_pseudo::end_mark);//ʵ������׺������st���ȶ�ͬԴ����
    cout_cur_time();
    std::cout << "End  : build Suffix Array" << "\n";
    std::vector<std::array<std::vector<utils::Insertion>, 2>> all_pairwise_gaps(_row); //�������-���ս��
    std::vector<insert> SNP_vector(_row);
    init_scores70();
    std::ifstream ns(arguments::N_file_name, std::ios::binary | std::ios::in); //�ж�N_file_name�Ϸ���
    if (!ns)
    {
        std::cout << "cannot access file " << arguments::in_file_name << '\n';
        exit(1);
    }
    ns.seekg(0, std::ios::beg);
    utils::read_N_tmp(ns, _N_gap[_centre], _centre);
    ns.seekg(0, std::ios::beg);
    cout_cur_time();
    std::cout << "Start: pairwise sequence alignment\n";
    for (int i = 0; i != _row; ++i)
    {
        threadPool0->execute(&star_alignment::StarAligner::mul_sv_maf_func, this, i, std::ref(ns), std::ref(st), std::ref(all_pairwise_gaps), std::ref(SNP_vector), threshold1, threshold2);
    }
    threadPool0->waitFinished();
    cout_cur_time();
    std::cout << "End  : pairwise sequence alignment\n";
    EmptySet();
    ns.close();

    return std::move(all_pairwise_gaps); //˫���бȶԵõ�������gap������Ϊn��vector��ÿ��Ԫ���г���Ϊ2��array��ÿ��Ԫ���������ɸ�Insertion��vector
}

auto star_alignment::StarAligner::mul_pairwise_align(int threshold1, int threshold2) const -> std::vector<std::array<std::vector<utils::Insertion>, 2>>
{
    cout_cur_time();
    std::cout << "Start: build Suffix Array No." << _centre << "\n";
    suffix_array::SuffixArray<nucleic_acid_pseudo::NUMBER> st(_sequences[_centre], 0, _sequences[_centre].length, nucleic_acid_pseudo::end_mark);//ʵ������׺������st���ȶ�ͬԴ����
    //size_t peakMem2 = get_peak_memory(); // ��ȡ�����Դ���ִ�к�ǰ���̵��ڴ�ռ�÷�ֵ
    cout_cur_time();
    std::cout << "End  : build Suffix Array" << "\n";
    std::vector<std::array<std::vector<utils::Insertion>, 2>> all_pairwise_gaps(_row); //�������-���ս��
    std::vector<insert> SNP_vector(_row);
    cout_cur_time();
    std::cout << "Start: pairwise sequence alignment\n";
    for (int i = 0; i != _row; ++i)
        threadPool0->execute(&star_alignment::StarAligner::mul_fasta_func, this, i, std::ref(st), std::ref(all_pairwise_gaps), std::ref(SNP_vector), threshold1, threshold2);
    threadPool0->waitFinished();

    EmptySet();
    cout_cur_time();
    std::cout << "End  : pairwise sequence alignment\n";
    //ns.close();
    
    return std::move(all_pairwise_gaps); //˫���бȶԵõ�������gap������Ϊn��vector��ÿ��Ԫ���г���Ϊ2��array��ÿ��Ԫ���������ɸ�Insertion��vector
}


//��ȡsv��ɸѡ��������ļ�
void star_alignment::StarAligner::getSV_write2file(std::array<std::vector<utils::Insertion>, 2>& pairwise_gaps, Kband* kband, std::ofstream& ofsv, int seq_i, std::vector <std::vector<quadra>>& chains, std::vector <std::vector<quadra>>& chains0, unsigned char* array_A, unsigned char* array_B)const
{
    unsigned int* Fasta_Center = utils::insert_fasta01(pairwise_gaps, _lengths[_centre]);
    ofsv << "##maf version=1 scoring=lastz.v1.04.00\n";
    utils::seq_NCBI2NA A, B;
    utils::m_block more_gap_i;
    insert SNP;
    more_gap_i.tag = 1;
    int ii, k, jn, j0, nsum1, nsum2;

    int pre_jn, pre_nsum2, pre_start2;

    std::vector<utils::Insertion> gapn1, gapn2;
    std::vector<std::vector<unsigned char>> sequences(2);
    int* final_sequences = new int[2];

    std::vector<std::string> name(2);
    std::vector<bool> sign(2);
    std::vector<bool> TU(2);
    //name
    name[0] = _name[_centre];
    name[1] = _name[seq_i];
    //sign
    sign[0] = _sign[_centre];
    sign[1] = _sign[seq_i];
    //TU
    TU[0] = _TU[_centre];
    TU[1] = _TU[seq_i];
    //length
    final_sequences[0] = _lengths[_centre];
    final_sequences[1] = _lengths[seq_i];
    for (ii = 0; ii < _N_gap[_centre].size(); ii++)
        final_sequences[0] += _N_gap[_centre][ii].number;
    for (ii = 0; ii < _N_gap[seq_i].size(); ii++)
        final_sequences[1] += _N_gap[seq_i][ii].number;

    if (_sign[seq_i])//+++++++
    {
        jn = j0 = nsum1 = nsum2 = 0;
        pre_jn = pre_nsum2 = pre_start2 = 0;
        // ԭ�� chains
        for (int i = 0; i < chains.size(); i++)
        {
            auto& intervals = chains[i];
            //�ж��Ƿ�1-n��n-1���ص���С��10%��̰�ģ�����룬����continue
            if (!(AB_exist(Fasta_Center, array_A, array_B, intervals[0][0], intervals.back()[1], intervals[0][2], intervals.back()[3])))
                continue;
                //here
            more_gap_i.start1 = intervals[0][0];
            more_gap_i.start2 = intervals[0][2];
            more_gap_i.end1 = intervals.back()[1];
            more_gap_i.end2 = intervals.back()[3];

            
            
            //std::cout << more_gap_i.start1 << " " << more_gap_i.start2 << " " << more_gap_i.end1 << " " << more_gap_i.end2 << "  se\n";
            A.length = more_gap_i.end1 - more_gap_i.start1;
            A.seq = utils::copy_DNA(_sequences[_centre].seq, more_gap_i.start1, more_gap_i.end1);
            B.length = more_gap_i.end2 - more_gap_i.start2;
            B.seq = utils::copy_DNA(_sequences[seq_i].seq, more_gap_i.start2, more_gap_i.end2);
            //std::cout << "+ copy_DNA  finish\n";
            for (size_t j = 1; j < intervals.size()-1; ++j)
            {
                //std::cout << intervals[j][0] << " " << intervals[j][1] << " " << intervals[j][2] << " " << intervals[j][3] << " \n";
                
                auto [agap_, bgap_] = BWT_MINI::BWT_mini(kband, A, intervals[j][0]- more_gap_i.start1, intervals[j][1] -more_gap_i.start1,
                    B, intervals[j][2]-more_gap_i.start2, intervals[j][3] - more_gap_i.start2, 10000, SNP); //MINI bwt�任��
                more_gap_i.gap1.insert(more_gap_i.gap1.end(), agap_.cbegin(), agap_.cend());
                more_gap_i.gap2.insert(more_gap_i.gap2.end(), bgap_.cbegin(), bgap_.cend());
                
            }
            //std::cout << intervals[0][0] << " " << intervals[0][1] << "+  BWT_mini finish\n";
            std::vector<quadra>().swap(chains[i]);
            sequences[0] = maf_bin_to_vector(A);
            delete []A.seq;//�ͷŵ�ǰ����i
            A.length = 0;//�ͷŵ�ǰ����i
            sequences[1] = maf_bin_to_vector(B);
            delete []B.seq;//�ͷŵ�ǰ����i
            B.length = 0;//�ͷŵ�ǰ����i
            //std::cout << "+  maf_bin_to_vector all finish\n";

            j0 = nsum1 = 0;
            std::vector<utils::Insertion>().swap(gapn1);
            std::vector<utils::Insertion>().swap(gapn2);
            /*if (more_gap_i.start2 == pre_start2)
            {
                jn = pre_jn;
                nsum2 = pre_nsum2;
            }
            pre_jn = jn;
            pre_nsum2 = nsum2;
            pre_start2 = more_gap_i.start2;
            */
            jn = nsum2 = 0;
            while (j0 < _N_gap[_centre].size()  && _N_gap[_centre][j0].index <= more_gap_i.start1)nsum1 += _N_gap[_centre][j0++].number;
            while (jn < _N_gap[seq_i].size()    && _N_gap[seq_i][jn].index <= more_gap_i.start2)nsum2 += _N_gap[seq_i][jn++].number;
            while (j0 < _N_gap[_centre].size()  && _N_gap[_centre][j0].index < more_gap_i.end1)gapn1.push_back(_N_gap[_centre][j0++]);
            while (jn < _N_gap[seq_i].size()    && _N_gap[seq_i][jn].index < more_gap_i.end2)gapn2.push_back(_N_gap[seq_i][jn++]);
            //std::cout << "+  0insertion_gap_out_new_sv0\n";
            insertion_gap_out_new_sv(ofsv, sequences[0], sequences[1], name, sign, TU, more_gap_i, final_sequences, nsum1, nsum2, gapn1, gapn2);
            //std::cout << "+  1insertion_gap_out_new_sv1 finish\n";
            std::vector<std::tuple<int, int>>().swap(more_gap_i.gap1);
            std::vector<std::tuple<int, int>>().swap(more_gap_i.gap2);
        }
        //std::cout << "+  finish\n";
        //ԭ��ȡ����_N_gap������������
        std::vector<utils::Insertion> N_gap_tmp(_N_gap[seq_i].size());
        for (int p = 0; p < _N_gap[seq_i].size(); p++)//N_insertion �ı�
        {
            N_gap_tmp[_N_gap[seq_i].size() - p - 1].index = _lengths[seq_i] - _N_gap[seq_i][p].index;
            N_gap_tmp[_N_gap[seq_i].size() - p - 1].number = _N_gap[seq_i][p].number;
        }
        sign[1] = !sign[1];
        //std::cout << "+ 2 - finish\n";
        // ���� chains0
        jn = j0 = nsum1 = nsum2 = 0;
        pre_jn = pre_nsum2 = pre_start2 = 0;
        for (int i = 0; i < chains0.size(); i++)
        {
            auto& intervals = chains0[i];
            if (!(AB_exist(array_A, array_B, intervals[0][0], intervals.back()[1], _lengths[seq_i]-intervals.back()[3], _lengths[seq_i] - intervals[0][2])))
                continue;
            more_gap_i.start1 = intervals[0][0];
            more_gap_i.start2 = intervals[0][2];
            more_gap_i.end1 = intervals.back()[1];
            more_gap_i.end2 = intervals.back()[3];

            A.length = more_gap_i.end1 - more_gap_i.start1;
            A.seq = utils::copy_DNA(_sequences[_centre].seq, more_gap_i.start1, more_gap_i.end1);
            //std::cout << "- copy_DNA1  finish\n";
            B.length = more_gap_i.end2 - more_gap_i.start2;
            B.seq = utils::copy_DNA_ni(_sequences[seq_i].seq, _lengths[seq_i] - more_gap_i.end2, _lengths[seq_i] - more_gap_i.start2);
            //std::cout << "- copy_DNA2  finish\n";
            for (size_t j = 1; j < intervals.size() - 1; ++j)
            {
                auto [agap_, bgap_] = BWT_MINI::BWT_mini(kband, A, intervals[j][0] - more_gap_i.start1, intervals[j][1] - more_gap_i.start1,
                    B, intervals[j][2] - more_gap_i.start2, intervals[j][3] - more_gap_i.start2, 10000, SNP); //MINI bwt�任��
                more_gap_i.gap1.insert(more_gap_i.gap1.end(), agap_.cbegin(), agap_.cend());
                more_gap_i.gap2.insert(more_gap_i.gap2.end(), bgap_.cbegin(), bgap_.cend());
            }
            //std::cout << intervals[0][0] << " " << intervals[0][1] << "-  BWT_mini finish\n";
            std::vector<quadra>().swap(chains0[i]);
            
            sequences[0] = maf_bin_to_vector(A);
            delete []A.seq;//�ͷŵ�ǰ����i
            A.length = 0;//�ͷŵ�ǰ����i
            sequences[1] = maf_bin_to_vector(B);
            delete []B.seq;//�ͷŵ�ǰ����i
            B.length = 0;//�ͷŵ�ǰ����i
            //std::cout << "-  maf_bin_to_vector all finish\n";

            j0 = nsum1 = 0;
            std::vector<utils::Insertion>().swap(gapn1);
            std::vector<utils::Insertion>().swap(gapn2);
            /*if (more_gap_i.start2 == pre_start2)
            {
                jn = pre_jn;
                nsum2 = pre_nsum2;
            }
            pre_jn = jn;
            pre_nsum2 = nsum2;
            pre_start2 = more_gap_i.start2;
            */
            jn = nsum2 = 0;
            while (j0 < _N_gap[_centre].size() && _N_gap[_centre][j0].index <= more_gap_i.start1)nsum1 += _N_gap[_centre][j0++].number;
            while (jn < N_gap_tmp.size() && N_gap_tmp[jn].index <= more_gap_i.start2)nsum2 += N_gap_tmp[jn++].number;
            while (j0 < _N_gap[_centre].size() && _N_gap[_centre][j0].index < more_gap_i.end1)gapn1.push_back(_N_gap[_centre][j0++]);
            while (jn < N_gap_tmp.size() && N_gap_tmp[jn].index < more_gap_i.end2)gapn2.push_back(N_gap_tmp[jn++]);
            //std::cout << "-  1insertion_gap_out_new_sv0\n";
            insertion_gap_out_new_sv(ofsv, sequences[0], sequences[1], name, sign,TU, more_gap_i, final_sequences, nsum1, nsum2, gapn1, gapn2);
            //std::cout << "-  1insertion_gap_out_new_sv1 finish\n";
            std::vector<std::tuple<int, int>>().swap(more_gap_i.gap1);
            std::vector<std::tuple<int, int>>().swap(more_gap_i.gap2);
        }
        //std::cout << "-  finish\n";
        std::vector<utils::Insertion>().swap(N_gap_tmp);

    }
    else//------
    {
        //test_Start 531
        //std::vector<std::vector<quadra>> tmp;
        //tmp.push_back(chains0[531]);
        //tmp.swap(chains0);

        //test_end

        std::vector<utils::Insertion> N_gap_tmp(_N_gap[seq_i].size());
        for (int p = 0; p < _N_gap[seq_i].size(); p++)//N_insertion �ı�
        {
            N_gap_tmp[_N_gap[seq_i].size() - p - 1].index = _lengths[seq_i] - _N_gap[seq_i][p].index;
            N_gap_tmp[_N_gap[seq_i].size() - p - 1].number = _N_gap[seq_i][p].number;
        }
        N_gap_tmp.swap(_N_gap[seq_i]);//+++N_gap_tmp
                                      //----_N_gap


        jn = j0 = nsum1 = nsum2 = 0;
        pre_jn = pre_nsum2 = pre_start2 = 0;
        // ԭ�� chains0
        for (int i = 0; i < chains0.size(); i++)
        {
            auto& intervals = chains0[i];
            if (!(AB_exist(Fasta_Center, array_A, array_B, intervals[0][0], intervals.back()[1], intervals[0][2], intervals.back()[3])))
                continue;
            more_gap_i.start1 = intervals[0][0];
            more_gap_i.start2 = intervals[0][2];
            more_gap_i.end1 = intervals.back()[1];
            more_gap_i.end2 = intervals.back()[3];

            A.length = more_gap_i.end1 - more_gap_i.start1;
            A.seq = utils::copy_DNA(_sequences[_centre].seq, more_gap_i.start1, more_gap_i.end1);
            B.length = more_gap_i.end2 - more_gap_i.start2;
            B.seq = utils::copy_DNA(_sequences[seq_i].seq, more_gap_i.start2, more_gap_i.end2);

            for (size_t j = 1; j < intervals.size() - 1; ++j)
            {
                auto [agap_, bgap_] = BWT_MINI::BWT_mini(kband, A, intervals[j][0] - more_gap_i.start1, intervals[j][1] - more_gap_i.start1,
                    B, intervals[j][2] - more_gap_i.start2, intervals[j][3] - more_gap_i.start2, 10000, SNP); //MINI bwt�任��
                more_gap_i.gap1.insert(more_gap_i.gap1.end(), agap_.cbegin(), agap_.cend());
                more_gap_i.gap2.insert(more_gap_i.gap2.end(), bgap_.cbegin(), bgap_.cend());
            }
            std::vector<quadra>().swap(chains0[i]);

            sequences[0] = maf_bin_to_vector(A);
            delete []A.seq;//�ͷŵ�ǰ����i
            A.length = 0;//�ͷŵ�ǰ����i
            sequences[1] = maf_bin_to_vector(B);
            delete []B.seq;//�ͷŵ�ǰ����i
            B.length = 0;//�ͷŵ�ǰ����i


            j0 = nsum1 = 0;
            std::vector<utils::Insertion>().swap(gapn1);
            std::vector<utils::Insertion>().swap(gapn2);
            /*if (more_gap_i.start2 == pre_start2)
            {
                jn = pre_jn;
                nsum2 = pre_nsum2;
            }
            pre_jn = jn;
            pre_nsum2 = nsum2;
            pre_start2 = more_gap_i.start2;
            */
            jn = nsum2 = 0;
            while (j0 < _N_gap[_centre].size() && _N_gap[_centre][j0].index <= more_gap_i.start1)nsum1 += _N_gap[_centre][j0++].number;
            while (jn < _N_gap[seq_i].size() && _N_gap[seq_i][jn].index <= more_gap_i.start2)nsum2 += _N_gap[seq_i][jn++].number;
            while (j0 < _N_gap[_centre].size() && _N_gap[_centre][j0].index < more_gap_i.end1)gapn1.push_back(_N_gap[_centre][j0++]);
            while (jn < _N_gap[seq_i].size() && _N_gap[seq_i][jn].index < more_gap_i.end2)gapn2.push_back(_N_gap[seq_i][jn++]);
            //std::cout << i <<" " << more_gap_i.start1 + nsum1 << " " << more_gap_i.start2 + nsum2 << "\n";
            insertion_gap_out_new_sv(ofsv, sequences[0], sequences[1], name, sign,TU, more_gap_i, final_sequences, nsum1, nsum2, gapn1, gapn2);
            std::vector<std::tuple<int, int>>().swap(more_gap_i.gap1);
            std::vector<std::tuple<int, int>>().swap(more_gap_i.gap2);
        }
        //ofsv.close();
        //exit(6);
        //ԭ��ȡ����_N_gapz�Ա�
        sign[1] = !sign[1];

        // ���� chains
        jn = j0 = nsum1 = nsum2 = 0;
        pre_jn = pre_nsum2 = pre_start2 = 0;
        for (int i = 0; i < chains.size(); i++)
        {
            auto& intervals = chains[i];
            if (!(AB_exist(array_A, array_B, intervals[0][0], intervals.back()[1], _lengths[seq_i] - intervals.back()[3], _lengths[seq_i] - intervals[0][2])))
                continue;
            more_gap_i.start1 = intervals[0][0];
            more_gap_i.start2 = intervals[0][2];
            more_gap_i.end1 = intervals.back()[1];
            more_gap_i.end2 = intervals.back()[3];

            A.length = more_gap_i.end1 - more_gap_i.start1;
            A.seq = utils::copy_DNA(_sequences[_centre].seq, more_gap_i.start1, more_gap_i.end1);
            B.length = more_gap_i.end2 - more_gap_i.start2;
            B.seq = utils::copy_DNA_ni(_sequences[seq_i].seq, _lengths[seq_i] - more_gap_i.end2, _lengths[seq_i] - more_gap_i.start2);

            for (size_t j = 1; j < intervals.size() - 1; ++j)
            {
                auto [agap_, bgap_] = BWT_MINI::BWT_mini(kband, A, intervals[j][0] - more_gap_i.start1, intervals[j][1] - more_gap_i.start1,
                    B, intervals[j][2] - more_gap_i.start2, intervals[j][3] - more_gap_i.start2, 10000, SNP); //MINI bwt�任��
                more_gap_i.gap1.insert(more_gap_i.gap1.end(), agap_.cbegin(), agap_.cend());
                more_gap_i.gap2.insert(more_gap_i.gap2.end(), bgap_.cbegin(), bgap_.cend());
            }
            std::vector<quadra>().swap(chains[i]);

            sequences[0] = maf_bin_to_vector(A);
            delete []A.seq;//�ͷŵ�ǰ����i
            A.length = 0;//�ͷŵ�ǰ����i
            sequences[1] = maf_bin_to_vector(B);
            delete []B.seq;//�ͷŵ�ǰ����i
            B.length = 0;//�ͷŵ�ǰ����i


            j0 = nsum1 = 0;
            std::vector<utils::Insertion>().swap(gapn1);
            std::vector<utils::Insertion>().swap(gapn2);
            /*if (more_gap_i.start2 == pre_start2)
            {
                jn = pre_jn;
                nsum2 = pre_nsum2;
            }
            pre_jn = jn;
            pre_nsum2 = nsum2;
            pre_start2 = more_gap_i.start2;
            */
            jn = nsum2 = 0;
            while (j0 < _N_gap[_centre].size() && _N_gap[_centre][j0].index <= more_gap_i.start1)nsum1 += _N_gap[_centre][j0++].number;
            while (jn < N_gap_tmp.size() && N_gap_tmp[jn].index <= more_gap_i.start2)nsum2 += N_gap_tmp[jn++].number;
            while (j0 < _N_gap[_centre].size() && _N_gap[_centre][j0].index < more_gap_i.end1)gapn1.push_back(_N_gap[_centre][j0++]);
            while (jn < N_gap_tmp.size() && N_gap_tmp[jn].index < more_gap_i.end2)gapn2.push_back(N_gap_tmp[jn++]);
            //std::cout << nsum1 << " " << nsum2 << "  nm12..\n";
            insertion_gap_out_new_sv(ofsv, sequences[0], sequences[1], name, sign, TU, more_gap_i, final_sequences, nsum1, nsum2, gapn1, gapn2);
            std::vector<std::tuple<int, int>>().swap(more_gap_i.gap1);
            std::vector<std::tuple<int, int>>().swap(more_gap_i.gap2);
        }
        std::vector<utils::Insertion>().swap(N_gap_tmp);
    }
    std::vector<utils::Insertion>().swap(gapn1);
    std::vector<utils::Insertion>().swap(gapn2);
    delete[] final_sequences;
    delete[] Fasta_Center;
}

//�������¸�,��һ�����Ӵ���ȫ����ͬԴ���Σ�[[A.index��B.index��length]...]��ÿ��B.indexѡ��һ��A.index����Ծ������
auto star_alignment::StarAligner::_MultiReg(const std::vector<triple>& common_substrings) //����·��
    -> std::vector<triple> 
{
    //std::cout << "***common_substrings" << common_substrings.size() << "\n";
    /*for (int i = 0; i < common_substrings.size(); ++i)
        std::cout << common_substrings[i][0] << " " << common_substrings[i][1] << " " << common_substrings[i][2] << "\n";
    */
    std::vector<triple> optimal_common_substrings;
    if (common_substrings.empty()) return optimal_common_substrings;
    int start = common_substrings[0][0];  //��ʼ��
    int end = common_substrings[0][0] + common_substrings[0][2];//
    int i;
    float a_length,tmp, pre_tmp,b_length = common_substrings.rbegin()[0][1] + common_substrings.rbegin()[0][2]; //B�ĳ���
    for (i = 1; i < common_substrings.size(); i++) // �ҳ����к�׺���б��У���С��׺����  start��㣬����׺�� + length_i��end�յ�
    {
        if (common_substrings[i][0] < start) start = common_substrings[i][0];
        if ((common_substrings[i][0]+ common_substrings[i][2]) > end) end = common_substrings[i][0]+ common_substrings[i][2];
    }
    a_length = end - start;
    //std::cout << a_length << " "<< b_length <<"\n";
    i = 0;
    //while ((common_substrings[i][2] < threshold)) i++;
    optimal_common_substrings.push_back(common_substrings[i]);//first
    pre_tmp = common_substrings[i][0] / a_length - common_substrings[i][1] / b_length;//�����һ��Ԫ��
    pre_tmp = fabs(pre_tmp);
    for (i=i+1; i < common_substrings.size(); i++)
    {
        //if (common_substrings[i][2] < threshold)continue;
        if (optimal_common_substrings.rbegin()[0][1] != common_substrings[i][1]) //b.index ��ͬ
        {
            pre_tmp = common_substrings[i][0] / a_length - common_substrings[i][1] / b_length;
            pre_tmp = fabs(pre_tmp);
            optimal_common_substrings.push_back(common_substrings[i]);
        } 
        else
        {
            tmp = common_substrings[i][0] / a_length - common_substrings[i][1] / b_length;
            tmp = fabs(tmp);
            if (tmp < pre_tmp) 
            {
                optimal_common_substrings.rbegin()[0][0] = common_substrings[i][0]; 
                pre_tmp = tmp;
            }
        }
    }
    
    //std::cout << "***optimal_common_substrings" << optimal_common_substrings.size() << "\n";
    //for (int i = 0; i < optimal_common_substrings.size(); ++i)
        //std::cout << optimal_common_substrings[i][0] << " " << optimal_common_substrings[i][1] << " " << optimal_common_substrings[i][2] << "\n";
    return optimal_common_substrings;

}

//�������¸�,�ڶ��������ݶ�̬�滮��ѡ�����ʵĲ��ص���ͬԴ����
std::vector<triple> star_alignment::StarAligner::_optimal_path(const std::vector<triple>& common_substrings) //����·��
  // [A.index��B.index��length]
{
    //for (int i = 0; i < common_substrings.size(); ++i)
    //    std::cout << common_substrings[i][0] << " " << common_substrings[i][1] << " " << common_substrings[i][2] << "\n";
    std::vector<triple> optimal_common_substrings = _MultiReg(common_substrings);//���õ�һ�����
    //std::cout << "***_optimal_path" << optimal_common_substrings.size() << "\n";
    if (optimal_common_substrings.empty()) return optimal_common_substrings;

    //0
    int m = optimal_common_substrings.size();
    int* dp = new int[m];
    int* last = new int[m];
    int* posi = new int[m];
    for (int i = 0; i < m; i++) dp[i] = INT_MAX;
    std::vector<triple> ans_common_substrings;
    int pos = 0, pos1 = 0, k, len;    // ��¼dp��ǰ���һλ���±�
    dp[0] = optimal_common_substrings[0][0] + optimal_common_substrings[0][2];   // dp[0]ֵ��ȻΪa[0]
    last[0] = 0;
    for (int i = 1; i < m; i++)
    {
        if (optimal_common_substrings[i][0] + optimal_common_substrings[i][2] > dp[pos])    // ��a[i]����dp�������ֵ����ֱ�����
        {
            dp[++pos] = optimal_common_substrings[i][0] + optimal_common_substrings[i][2];
            last[i] = pos;
        }
        else    // �����ҵ�dp�е�һ�����ڵ���a[i]��λ�ã���a[i]�滻֮��
        {
            k = std::lower_bound(dp, dp + pos + 1, optimal_common_substrings[i][0] + optimal_common_substrings[i][2]) - dp;
            dp[k] = optimal_common_substrings[i][0] + optimal_common_substrings[i][2];  // ���ֲ���
            last[i] = k;
        }
    }
    len = pos;
    for (int i = m - 1; i >= 0; i--)
        if (last[i] == len)posi[len--] = i;
    ans_common_substrings.push_back(optimal_common_substrings[posi[0]]);
    int total = 0, prev = ans_common_substrings[0][0] + ans_common_substrings[0][2];//total��ʾ��Ҫ�Ƴ�����ĸ��� 
    for (int i = 1; i < pos + 1; i++)
    {
        if (optimal_common_substrings[posi[i]][0] < prev)
        {
            ++total;
        }
        else
        {
            prev = optimal_common_substrings[posi[i]][0] + optimal_common_substrings[posi[i]][2];
            ans_common_substrings.push_back(optimal_common_substrings[posi[i]]);
        }
    }

    //1
    /*
   optimal_common_substrings.swap(ans_common_substrings);
    ans_common_substrings.resize(0);
    m = optimal_common_substrings.size();
    for (int i = 0; i < m; i++) dp[i] = INT_MAX;
    pos = 0, pos1 = 0, k, len;    // ��¼dp��ǰ���һλ���±� 
    dp[0] = optimal_common_substrings[0][1] + optimal_common_substrings[0][2];   // dp[0]ֵ��ȻΪa[0]
    last[0] = 0;
    for (int i = 1; i < m; i++)
    {
        if (optimal_common_substrings[i][1] + optimal_common_substrings[i][2] > dp[pos])    // ��a[i]����dp�������ֵ����ֱ�����
        {
            dp[++pos] = optimal_common_substrings[i][1] + optimal_common_substrings[i][2];
            last[i] = pos;
        }
        else    // �����ҵ�dp�е�һ�����ڵ���a[i]��λ�ã���a[i]�滻֮��
        {
            k = std::lower_bound(dp, dp + pos + 1, optimal_common_substrings[i][1] + optimal_common_substrings[i][2]) - dp;
            dp[k] = optimal_common_substrings[i][1] + optimal_common_substrings[i][2];  // ���ֲ���
            last[i] = k;
        }
    }
    len = pos;
    for (int i = m - 1; i >= 0; i--)
        if (last[i] == len)posi[len--] = i;
    ans_common_substrings.push_back(optimal_common_substrings[posi[0]]);
    total = 0, prev = ans_common_substrings[0][1] + ans_common_substrings[0][2];//total��ʾ��Ҫ�Ƴ�����ĸ��� 
    for (int i = 1; i < pos + 1; i++)
    {
        if (optimal_common_substrings[posi[i]][1] < prev)
        {
            ++total;
        }
        else
        {
            prev = optimal_common_substrings[posi[i]][1] + optimal_common_substrings[posi[i]][2];
            ans_common_substrings.push_back(optimal_common_substrings[posi[i]]);
        }
    }
    //cout << "\n" << total << endl;
    */
    /*
    for (int i = 1; i < ans_common_substrings.size(); ++i)
        if (((ans_common_substrings[i - 1][0] + ans_common_substrings[i - 1][2]) <= ans_common_substrings[i][0]) && ((ans_common_substrings[i - 1][1] + ans_common_substrings[i - 1][2]) <= ans_common_substrings[i][1]))
            continue;
        else
        {
            std::cout << ans_common_substrings[i - 1][0] << " " << ans_common_substrings[i - 1][1] << " " << ans_common_substrings[i - 1][2] << "\n";
            std::cout << ans_common_substrings[i][0] << " " << ans_common_substrings[i][1] << " " << ans_common_substrings[i][2] << "\n\n";
        }
    */    
    std::vector<triple>().swap(optimal_common_substrings);//���һ���ڴ�
    delete[] posi;
    delete[] last;
    delete[] dp;
    //std::cout << "***_optimal_path" << ans_common_substrings.size() << "\n";
    return ans_common_substrings;
}

bool compare_first(const triple& a, const triple& b)
{
    return a[1] < b[1];
}
//get sv chains
std::vector <std::vector<quadra>> star_alignment::StarAligner::find_subchains(std::vector<triple>& common_substrings) {
    /*size_t thresh_dis = 2000;
    size_t thresh_len = 1000;
    double thresh_bi = 0.3;
    double thresh_len_cha = 0.3;
    double thresh_len_cha_i = 0.15;
    int gap_thresh = 20;*/
    
    size_t thresh_len = arguments::sv_thresh_len;//���Ը��û�ָ��
    size_t thresh_dis = arguments::sv_thresh_len < 1000 ? 1000 : arguments::sv_thresh_len;
    double thresh_bi = 0.3;
    double thresh_len_cha = 0.25;
    double thresh_len_cha_i = 0.25;
    int gap_thresh = 20;

    size_t a_start, b_start, len;
    bool single_tag;
    bool tag;
    int A_len, B_len;
    std::vector <int> index;
    std::vector<std::vector<triple>> final_subchains;
    std::vector<subchain> cur_subchain;
    std::map<std::pair< int, int>, std::pair<int, int>> chain_filter;
    std::vector<std::vector<triple>>::reverse_iterator it;
    // iterate through all common substrings
    for (int i = 0; i < common_substrings.size(); i++) {
        a_start = common_substrings[i][0];
        b_start = common_substrings[i][1];
        len = common_substrings[i][2];
        single_tag = true;
        
        // update subchain indices for this common substring
        for (int j = 0; j < cur_subchain.size(); j++)
        {
            //������
            tag = false;
            auto& subchain = cur_subchain[j];
            A_len = a_start + len - subchain.a_start;
            B_len = b_start + len - subchain.b_start;
            if (b_start >= subchain.b_end && a_start >= subchain.a_end
                && b_start <= (subchain.b_end + thresh_dis) //˳�򣬽���
                && a_start <= (subchain.a_end + thresh_dis)
                //&& len > thresh_bi * (b_start - subchain.b_end) //��ֹ���һ��Ԫ�غ�����
                //&& len > thresh_bi * (a_start - subchain.a_end) //��ֹ���һ��Ԫ�غ�����
                && (subchain.len + len) > std::max(A_len, B_len) * thresh_bi //���Ƕ�
                && abs(A_len - B_len) < std::min(A_len, B_len) * thresh_len_cha) //�������Ȳ�
                tag = true;
            if(tag)
            {
                
                if (abs((int)a_start - (int)subchain.a_end - (int)b_start + (int)subchain.b_end) < gap_thresh ||
                    (a_start != subchain.a_end && b_start != subchain.b_end &&
                        ((double)std::max(a_start - subchain.a_end, b_start - subchain.b_end) / std::min(a_start - subchain.a_end, b_start - subchain.b_end)) < (1 + thresh_len_cha_i)))
                {
                    tag = true;
                    //std::cout << (double)std::max(a_start - subchain.a_start, b_start - subchain.b_start) / std::min(a_start - subchain.a_start, b_start - subchain.b_start) <<" true\n";
                }
                else
                    tag = false;
            }
            
            if(tag)
            {
                single_tag = false;
                // add the new interval to this subchain
                subchain.a_end = a_start + len;
                subchain.b_end = b_start + len;
                subchain.len += len;
                subchain.chains.push_back(common_substrings[i]);
            }
            else if (b_start > (subchain.b_end + thresh_dis))
            {
                subchain.state = true; //�������ˣ�ѭ����Ҫȥ�ж��ˡ�
            }
        }
        //��������
        if (single_tag)
        {
            struct subchain tmp;
            cur_subchain.push_back(tmp);
            auto& subchain = cur_subchain.back();
            subchain.state = false;
            subchain.a_start = a_start;
            subchain.b_start = b_start;
            subchain.a_end = a_start + len;
            subchain.b_end = b_start + len;
            subchain.len = len;
            subchain.chains.push_back(common_substrings[i]);
        }

        //�ж�����ȥ����
        for (int j = 0; j < cur_subchain.size();)
        {
            if (cur_subchain[j].state)//����
            {
                if(cur_subchain[j].chains.size()>1)
                {
                    auto& back2chain = cur_subchain[j].chains[cur_subchain[j].chains.size()-2];
                    auto& backchain = cur_subchain[j].chains.back();
                    if (backchain[2] > thresh_bi * (backchain[1] - (back2chain[1] + back2chain[2]))
                        && backchain[2] > thresh_bi * (backchain[0] - (back2chain[0] + back2chain[2]))) //��ֹ���һ��Ԫ�غ�����
                        ;
                    else
                    {
                        cur_subchain[j].a_end = back2chain[0] + back2chain[2];
                        cur_subchain[j].b_end = back2chain[1] + back2chain[2];
                        cur_subchain[j].len -= backchain[2];
                        cur_subchain[j].chains.pop_back();
                    }
                }
                if ((cur_subchain[j].a_end - cur_subchain[j].a_start) > thresh_len
                    && (cur_subchain[j].b_end - cur_subchain[j].b_start) > thresh_len)//Ӧ�üӸ��ȶԣ�����������������ֵ
                {
                    if (chain_filter.count({ cur_subchain[j].chains.back()[0],cur_subchain[j].chains.back()[1] }))
                    {
                        if (chain_filter[{ cur_subchain[j].chains.back()[0], cur_subchain[j].chains.back()[1] }].first < cur_subchain[j].chains.size())
                        {
                            index.push_back(chain_filter[{ cur_subchain[j].chains.back()[0], cur_subchain[j].chains.back()[1] }].second);//����ɾ��
                            chain_filter[{ cur_subchain[j].chains.back()[0], cur_subchain[j].chains.back()[1] }]
                                = { cur_subchain[j].chains.size(),final_subchains.size() };
                            final_subchains.push_back(cur_subchain[j].chains);//����ϸ���
                            //chain_filter[{ cur_subchain[j].chains.back()[0], cur_subchain[j].chains.back()[1] }].first = cur_subchain[j].chains.size();
                            //final_subchains[chain_filter[{ cur_subchain[j].chains.back()[0], cur_subchain[j].chains.back()[1] }].second].swap(cur_subchain[j].chains);
                            //�滻,λ�ò���
                        }
                    }
                    else
                    {
                        chain_filter[{ cur_subchain[j].chains.back()[0], cur_subchain[j].chains.back()[1] }]
                            = { cur_subchain[j].chains.size(),final_subchains.size() };
                        final_subchains.push_back(cur_subchain[j].chains);//����ϸ���

                        // ����ʹ�� reverse_iterator ������� final_subchains
                        //for (it = final_subchains.rbegin(); (it != final_subchains.rend()) && (cur_subchain[j].chains[0][1] < (*it)[0][1]); ++it);
                        //final_subchains.insert(it.base(), cur_subchain[j].chains);

                    }
                }
                cur_subchain.erase(cur_subchain.begin() + j);//��ն���
            }
            else
                j++;
        }
    }
    //�ж�����ȥ����
    for (int j = 0; j < cur_subchain.size();)
    {
        if (1)//����
        {
            if (cur_subchain[j].chains.size() > 1)
            {
                auto& back2chain = cur_subchain[j].chains[cur_subchain[j].chains.size() - 2];
                auto& backchain = cur_subchain[j].chains.back();
                if (backchain[2] > thresh_bi * (backchain[1] - (back2chain[1] + back2chain[2]))
                    && backchain[2] > thresh_bi * (backchain[0] - (back2chain[0] + back2chain[2]))) //��ֹ���һ��Ԫ�غ�����
                    ;
                else
                {
                    cur_subchain[j].a_end = back2chain[0] + back2chain[2];
                    cur_subchain[j].b_end = back2chain[1] + back2chain[2];
                    cur_subchain[j].len -= backchain[2];
                    cur_subchain[j].chains.pop_back();
                }
            }
            if ((cur_subchain[j].a_end - cur_subchain[j].a_start) > thresh_len
                && (cur_subchain[j].b_end - cur_subchain[j].b_start) > thresh_len)//Ӧ�üӸ��ȶԣ�����������������ֵ
            {
                if (chain_filter.count({ cur_subchain[j].chains.back()[0],cur_subchain[j].chains.back()[1] }))
                {
                    if (chain_filter[{ cur_subchain[j].chains.back()[0], cur_subchain[j].chains.back()[1] }].first < cur_subchain[j].chains.size())
                    {
                        index.push_back(chain_filter[{ cur_subchain[j].chains.back()[0], cur_subchain[j].chains.back()[1] }].second);//����ɾ��
                        chain_filter[{ cur_subchain[j].chains.back()[0], cur_subchain[j].chains.back()[1] }]
                            = { cur_subchain[j].chains.size(),final_subchains.size() };
                        final_subchains.push_back(cur_subchain[j].chains);//����ϸ���
                        //chain_filter[{ cur_subchain[j].chains.back()[0], cur_subchain[j].chains.back()[1] }].first = cur_subchain[j].chains.size();
                        //final_subchains[chain_filter[{ cur_subchain[j].chains.back()[0], cur_subchain[j].chains.back()[1] }].second].swap(cur_subchain[j].chains);
                        //�滻,λ�ò���
                    }
                }
                else
                {
                    chain_filter[{ cur_subchain[j].chains.back()[0], cur_subchain[j].chains.back()[1] }]
                        = { cur_subchain[j].chains.size(),final_subchains.size() };
                    final_subchains.push_back(cur_subchain[j].chains);//����ϸ���

                    // ����ʹ�� reverse_iterator ������� final_subchains
                    //for (it = final_subchains.rbegin(); (it != final_subchains.rend()) && (cur_subchain[j].chains[0][1] < (*it)[0][1]); ++it);
                    //final_subchains.insert(it.base(), cur_subchain[j].chains);

                }
            }
            cur_subchain.erase(cur_subchain.begin() + j);//��ն���
        }
        else
            j++;
    }
    //std::cout << index.size() << "  index.size()\n";
    for (auto id = index.rbegin(); id != index.rend(); ++id)
        final_subchains.erase(final_subchains.begin() + (*id));
    
    //paixu
    
    std::stable_sort(final_subchains.begin(), final_subchains.end(),
        [](const auto& a, const auto& b) {
            if (a[0][1] == b[0][1]) {
                return a[0][0] < b[0][0];
            }
            return a[0][1] < b[0][1];
        });
    
    std::vector <std::vector<quadra>> final_intervals(final_subchains.size());
    for (int i = 0; i < final_subchains.size(); i++)
    {
        //auto common_substrings = mini_optimal_path(final_subchains[i]);//ɸѡ��ǰ���ص�
        auto & common_substrings = final_subchains[i];
        auto & intervals = final_intervals[i];
        //first one
        intervals.push_back(quadra({ common_substrings[0][0], common_substrings[0][0], common_substrings[0][1], common_substrings[0][1] }));

        //mid
        for (size_t j = 1; j < common_substrings.size();++j)
            intervals.push_back(quadra ({
                    common_substrings[j-1][0] + common_substrings[j-1][2], common_substrings[j][0],
                    common_substrings[j-1][1] + common_substrings[j-1][2], common_substrings[j][1]
                    }));
        //last one
        intervals.push_back(quadra({ common_substrings.back()[0] + common_substrings.back()[2], common_substrings.back()[0] + common_substrings.back()[2],
                    common_substrings.back()[1] + common_substrings.back()[2], common_substrings.back()[1] + common_substrings.back()[2] }));
        std::vector<triple>().swap(final_subchains[i]);
    }
    
    std::vector<int>().swap(index);
    std::vector<subchain>().swap(cur_subchain);
    return std::move(final_intervals);
}

std::vector<int> _trace_back_bp(const std::vector<triple>& common_substrings, int* p)
{
    std::vector<int> ansi;
    int* tmp = p;
    for (int i = 0; i < common_substrings.size(); i++)
    {
        if (*p < tmp[i])
            p = &tmp[i];
    }
    int j, i = p - tmp;
    ansi.push_back(i);
    p = tmp;
    while (i > 0)
    {
        j = i - 1;
        if (p[i] == common_substrings[i][2]) break;
        while (j >= 0)
        {

            if ((common_substrings[i][0] >= (common_substrings[j][0] + common_substrings[j][2])) && (p[i] == (p[j] + common_substrings[i][2])))
            {
                ansi.push_back(j);
                i = j;
                break;
            }
            j--;
        }
    }
    reverse(ansi.begin(), ansi.end());//��ת
    return ansi;
}
//�ڶ��������ݶ�̬�滮��ѡ�����ʵĲ��ص���ͬԴ����
void _optimal_path_bp(const std::vector<triple>& optimal_common_substrings, std::vector<triple>& ans_common_substrings) //����·��
{
    int m = optimal_common_substrings.size();
    if (m <= 1) 
    { 
        if (m == 1)
            ans_common_substrings.push_back(optimal_common_substrings[0]);
        return; 
    }
    int* p = new int[m];
    for (int i = 0; i < m; i++) p[i] = optimal_common_substrings[i][2];
    for (int i = 1; i < m; i++)
        for (int j = 0; j < i; j++)
            if (optimal_common_substrings[i][0] >= (optimal_common_substrings[j][0] + optimal_common_substrings[j][2]))
                p[i] = (p[i] > (p[j] + optimal_common_substrings[i][2])) ? p[i] : (p[j] + optimal_common_substrings[i][2]);
    std::vector<int> ansi = _trace_back_bp(optimal_common_substrings, p);

   
    for (int i = 0; i < ansi.size(); i++) ans_common_substrings.push_back(optimal_common_substrings[ansi[i]]);
    std::vector<int>().swap(ansi);
    delete[] p;
}

std::vector<triple> star_alignment::StarAligner::sv_optimal_path(std::vector <std::vector<quadra>> & chain, const std::vector<triple>& common_substrings, bool filter) //����·��
{
    std::vector<triple> optimal_common_substrings = _MultiReg(common_substrings);//���õ�һ�����
    
    if (optimal_common_substrings.empty()) return optimal_common_substrings;
    //std::cout << common_substrings.size() << " common_substrings\n";
    //for (int i = 0; i < common_substrings.size(); i++)
     //   std::cout << i << "\t" << common_substrings[i][0] << "\t" << common_substrings[i][1] << "\t" << common_substrings[i][2] << "\n";
    std::vector<triple> ans_common_substrings;
    if (optimal_common_substrings.size() > 10000)
    {//0
        int m = optimal_common_substrings.size();
        int* dp = new int[m];
        int* last = new int[m];
        int* posi = new int[m];
        for (int i = 0; i < m; i++) dp[i] = INT_MAX;

        int pos = 0, pos1 = 0, k, len;    // ��¼dp��ǰ���һλ���±�
        dp[0] = optimal_common_substrings[0][0] + optimal_common_substrings[0][2];   // dp[0]ֵ��ȻΪa[0]
        last[0] = 0;
        for (int i = 1; i < m; i++)
        {
            if (optimal_common_substrings[i][0] + optimal_common_substrings[i][2] > dp[pos])    // ��a[i]����dp�������ֵ����ֱ�����
            {
                dp[++pos] = optimal_common_substrings[i][0] + optimal_common_substrings[i][2];
                last[i] = pos;
            }
            else    // �����ҵ�dp�е�һ�����ڵ���a[i]��λ�ã���a[i]�滻֮��
            {
                k = std::lower_bound(dp, dp + pos + 1, optimal_common_substrings[i][0] + optimal_common_substrings[i][2]) - dp;
                dp[k] = optimal_common_substrings[i][0] + optimal_common_substrings[i][2];  // ���ֲ���
                last[i] = k;
            }
        }
        len = pos;
        for (int i = m - 1; i >= 0; i--)
            if (last[i] == len)posi[len--] = i;
        ans_common_substrings.push_back(optimal_common_substrings[posi[0]]);
        int total = 0, prev = ans_common_substrings[0][0] + ans_common_substrings[0][2];//total��ʾ��Ҫ�Ƴ�����ĸ��� 
        for (int i = 1; i < pos + 1; i++)
        {
            if (optimal_common_substrings[posi[i]][0] < prev)
            {
                ++total;
            }
            else
            {
                prev = optimal_common_substrings[posi[i]][0] + optimal_common_substrings[posi[i]][2];
                ans_common_substrings.push_back(optimal_common_substrings[posi[i]]);
            }
        }

        std::vector<triple>().swap(optimal_common_substrings);//���һ���ڴ�
        delete[] posi;
        delete[] last;
        delete[] dp;
    }
    else
        _optimal_path_bp(optimal_common_substrings, ans_common_substrings);
    std::vector<triple> result1;
    //if (filter)
   // {
        result1.reserve(common_substrings.size() - ans_common_substrings.size());
        std::set_difference(common_substrings.begin(), common_substrings.end(),
            ans_common_substrings.begin(), ans_common_substrings.end(),
            std::back_inserter(result1), compare_first);

    if (result1.size() > 800000)
    {
        int thresh_80w = 100;

        std::unordered_map<int, int> count_map;
        for (const auto& elem : result1) {
            ++count_map[elem[2]];
        }
        std::vector<std::pair<int, int>> vec_count(count_map.begin(), count_map.end());
        std::unordered_map<int, int>().swap(count_map);
        std::sort(vec_count.begin(), vec_count.end(),
            [](const auto& x, const auto& y) {return x.first > y.first; });

        int count_num = 0;
        for (const auto& [elem, count] : vec_count) {
            if (count_num > 800000)
            {
                thresh_80w = std::max(thresh_80w, elem);
                //thresh_80w =  elem ;
                break;
            }
            else count_num += count;
        }
        std::vector<std::pair<int, int>>().swap(vec_count);
        
        std::vector<triple> result2;
        result2.reserve(800000);
        for (int i = 0; i < result1.size(); i++)
            if (result1[i][2] >= thresh_80w)
                result2.push_back(common_substrings[i]);
        std::vector<triple>().swap(result1);

        //std::cout << result2.size()<< " result2.size() "<< thresh_80w << "  thresh\n";

        const auto align_start1 = std::chrono::high_resolution_clock::now();
        //chain = find_subchains(result2);
        //std::cout << chain.size() << "   chain\n";
        //std::cout << "aligning consumes " << (std::chrono::high_resolution_clock::now() - align_start1) << '\n';
    }
    else
    {
        const auto align_start1 = std::chrono::high_resolution_clock::now();
        chain = find_subchains(result1);
        //std::cout << chain.size() << "   100chain\n";
        //std::cout << "aligning consumes " << (std::chrono::high_resolution_clock::now() - align_start1) << '\n';
    }
    //std::cout << ans_common_substrings.size() << " ans_common_substrings\n";
    //for (int i = 0; i < ans_common_substrings.size(); i++)
    //    std::cout << i << "\t" << ans_common_substrings[i][0] << "\t" << ans_common_substrings[i][1] << "\t" << ans_common_substrings[i][2] << "\n";

    //std::cout << result1.size() << " result1\n";
    //for (int i = 0; i < result1.size(); i++)
    //    std::cout << i << "\t" << result1[i][0] << "\t" << result1[i][1] << "\t" << result1[i][2] << "\n";

    return ans_common_substrings;
}

//����MSA-gap���������˫��gap���������final_sequence_gaps
//���룬˫���бȶԵõ�������gap������Ϊn��vector��ÿ��Ԫ���г���Ϊ2��array��ÿ��Ԫ���������ɸ�Insertion��vector
//���������������Ϊn��vector��ÿ��vector�洢���ɸ�Insertion����index+number
auto star_alignment::StarAligner::_merge_results(const std::vector<std::array<std::vector<utils::Insertion>, 2>> &pairwise_gaps) const
        -> std::vector<std::vector<utils::Insertion>>
{
    std::vector<utils::Insertion> final_centre_gaps;
    for (size_t i = 0; i != _row; ++i)
    {
        const auto &curr_centre_gaps = pairwise_gaps[i][0];
        for (size_t lhs_pointer = 0, rhs_pointer = 0; rhs_pointer != curr_centre_gaps.size(); )
        {
            if (lhs_pointer == final_centre_gaps.size())
            {
                final_centre_gaps.insert(final_centre_gaps.cend(), curr_centre_gaps.cbegin() + rhs_pointer, curr_centre_gaps.cend());
                break;
            }

            if (final_centre_gaps[lhs_pointer].index == curr_centre_gaps[rhs_pointer].index)
            {
                if (final_centre_gaps[lhs_pointer].number < curr_centre_gaps[rhs_pointer].number)
                    final_centre_gaps[lhs_pointer].number = curr_centre_gaps[rhs_pointer].number;
                ++lhs_pointer;
                ++rhs_pointer;
            }
            else if (final_centre_gaps[lhs_pointer].index < curr_centre_gaps[rhs_pointer].index)
            {
                ++lhs_pointer;
            }
            else
            {
                final_centre_gaps.insert(final_centre_gaps.cbegin() + lhs_pointer, curr_centre_gaps[rhs_pointer]);
                ++lhs_pointer; // because of the insert above
                ++rhs_pointer;
            }
        }
    }

    std::vector<std::vector<utils::Insertion>> final_sequence_gaps;
    final_sequence_gaps.reserve(_row);
    for (size_t i = 0; i != _row; ++i)
    {
        const auto &curr_centre_gaps = pairwise_gaps[i][0];
        const auto &curr_sequence_gaps = pairwise_gaps[i][1];

        std::vector<utils::Insertion> centre_addition;
        centre_addition.reserve(final_centre_gaps.size());
        utils::Insertion::minus(final_centre_gaps.cbegin(), final_centre_gaps.cend(),
                            curr_centre_gaps.cbegin(),  curr_centre_gaps.cend(),
                            std::back_inserter(centre_addition));

        std::vector<utils::Insertion> sequence_addition;
        for (size_t centre_index = 0, sequence_index = 0, centre_gaps_index = 0, sequence_gaps_index = 0, centre_addition_index = 0;
                centre_addition_index != centre_addition.size(); ++centre_addition_index)
        {
            const auto curr_addition = centre_addition[centre_addition_index]; // current addition pending process

            while (centre_index < curr_addition.index)
            {
                size_t centre_distance = centre_gaps_index < curr_centre_gaps.size() ?
                        curr_centre_gaps[centre_gaps_index].index - centre_index : std::numeric_limits<size_t>::max();
                size_t sequence_distance = sequence_gaps_index < curr_sequence_gaps.size() ?
                        curr_sequence_gaps[sequence_gaps_index].index - sequence_index : std::numeric_limits<size_t>::max();

                size_t step = std::min({ sequence_distance, centre_distance, curr_addition.index - centre_index }); // assure centre_index <= curr_addtion.index
                centre_index += step;
                sequence_index += step;

                if (centre_gaps_index < curr_centre_gaps.size() && curr_centre_gaps[centre_gaps_index].index == centre_index)
                    sequence_index += curr_centre_gaps[centre_gaps_index++].number;

                else if (sequence_gaps_index < curr_sequence_gaps.size() && curr_sequence_gaps[sequence_gaps_index].index == sequence_index)
                    centre_index += curr_sequence_gaps[sequence_gaps_index++].number;
            }

            if (sequence_addition.size() && sequence_index == sequence_addition.back().index)
                sequence_addition.back().number += curr_addition.number;
            else
                sequence_addition.push_back(utils::Insertion({ sequence_index, curr_addition.number }));
        }

        std::vector<utils::Insertion> indels_of_current_sequence;
        indels_of_current_sequence.reserve(curr_sequence_gaps.size() + sequence_addition.size());
        utils::Insertion::plus(curr_sequence_gaps.cbegin(), curr_sequence_gaps.cend(),
                           sequence_addition.cbegin(), sequence_addition.cend(),
                           std::back_inserter(indels_of_current_sequence));
        final_sequence_gaps.push_back(indels_of_current_sequence);

        std::vector<utils::Insertion>().swap(centre_addition);
        std::vector<utils::Insertion>().swap(sequence_addition);
        std::vector<utils::Insertion>().swap(indels_of_current_sequence);
        
    }/*
   std::cout << "2*final_sequence_gaps" << final_sequence_gaps.size() << "\n";
    for (int i = 0; i < final_sequence_gaps.size(); ++i) {
        for (int j = 0; j < final_sequence_gaps[i].size(); ++j) {
            std::cout << final_sequence_gaps[i][j].index << " " << final_sequence_gaps[i][j].number << ", ";
        }
        std::cout << "\n";
    }*/
    std::vector<utils::Insertion>().swap(final_centre_gaps);
    return std::move(final_sequence_gaps);
}