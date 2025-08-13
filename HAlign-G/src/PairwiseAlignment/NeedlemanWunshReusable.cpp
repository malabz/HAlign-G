#include "NeedlemanWunshReusable.hpp"
//kband
Kband::Kband() {
    match = 1;
    mismatch = -2;
    d = 3;
    e = 1;
    my_INT_MIN = 0; // 初始化为合适的值
    Aq = new UCHAR[thresh0 / 4 + 2];
    Bq = new UCHAR[thresh0 / 4 + 2];
    pm = new int* [3];
    pm2 = new int* [3];
    for (int i = 0; i < 3; ++i) {
        pm[i] = new int[thresh0];
        pm2[i] = new int[thresh0];
    }
    //pm = new int[3][thresh0];
    //pm2 = new int[3][thresh0];
    pmt1 = pm;
    pmt2 = pm2;
    pmt = pm;
    bt = new unsigned char* [thresh0];
    for (int i = 0; i < thresh0; ++i) {
        bt[i] = new unsigned char[thresh0];
    }
    //bt = new unsigned char[thresh0][thresh0];
    seq_A.resize(thresh0);
    seq_B.resize(thresh0);
}
Kband::~Kband() {
    delete[] Aq;
    delete[] Bq;
    for (int i = 0; i < 3; ++i) {
        delete[] pmt1[i];
        delete[] pmt2[i];
    }
    delete[] pmt1;
    delete[] pmt2;
    for (int i = 0; i < thresh0; ++i) 
        delete[] bt[i];
    delete[] bt;
    char_vector_type().swap(seq_A);
    char_vector_type().swap(seq_B);
}
inline int Kband::score(unsigned char xi, unsigned char yi)
{
    if (xi == yi)
        return match;
    else
        return mismatch;
}
inline bool Kband::InsiderStrip(int i, int j, int k, int diff)
{
    return ((-k <= (j - i)) && ((j - i) <= (k + diff)));
}
inline int Kband::index(int i, int l)
{
    return (i + l) % l;
}
inline int Kband::maxi(int a, int b)
{
    if (a > b)return a;
    else return b;
}
inline int Kband::maxi(int a, int b, int c)
{
    int max; if (a > b)max = a; else max = b;
    if (max > c)return max; else return c;
}
void Kband::Init(int m, int k, int diff)
{
    for (int i = 0; i < (m + 1); i++)
    {
        for (int j = 0; j < (diff + 2 * k + 1); j++)
            bt[i][j] = '\0';
    }
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < (diff + 2 * k + 1); j++)
            pm[i][j] = my_INT_MIN;
    }
    pm[0][k] = 0;
    bt[0][k] = '\16';
    for (int j = 0; j < (diff + k + 1); j++)
    {
        pm[1][j + k] = -d - e * (j - 1);
        bt[0][j + k] = (char)8;
    }
    for (int i = 0; i < (k + 1); i++)
        bt[i][k - i] = '\3';
}
void Kband::InitTwo(int ii, int k, int diff)
{
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < (diff + 2 * k + 1); j++)
            pm2[i][j] = my_INT_MIN;
    }
    if (ii < k + 1)
        pm2[2][index(k - ii, diff + 2 * k + 1)] = -d - e * (ii - 1);
}
int Kband::ChooseWay(int p0, int p1, int p2, bool state)
{
    if (p0 >= p1)
    {
        if (p0 >= p2)
            return state ? 16 : 0;
        else
            return state ? 48 : 2;
    }
    else if (p1 >= p2)
        return state ? 32 : 1;
    else
        return state ? 48 : 2;
}
inline int Kband::parse(int b, int s)
{
    //b = (int)b;
    b = (b >> (4 - s * 2));
    return (b & 3) - 1;
}
inline UCHAR Kband::get_char1(utils::seq_NCBI2NA& Seq, int i)
{
    return NCBI2NA_UNPACK_BASE(Seq.seq[i / 4], (3 - i % 4));
};
std::tuple<insert, insert>
    Kband::PSA_AGP_Kband3(utils::seq_NCBI2NA& sequence1, int a_begin, int a_end, utils::seq_NCBI2NA& sequence2, int b_begin, int b_end,
        insert& SNP, int cmatch, int cmismatch, int cd, int ce)
{

    match = cmatch;
    mismatch = cmismatch;
    d = cd;
    e = ce;
    pm = pmt1;
    pm2 = pmt2;
    int i = 0, j, z, diff, k = 1, m, n, b_j, l, old, bt1, bt2, bt3, newi, l2, channel;
    bool state_ex = false;//交换标识
    int a_len = a_end - a_begin;
    int b_len = b_end - b_begin;
    //std::cout << a_len << " " << b_len << "     1\n";
    insert a_gap;
    insert b_gap;
    if (a_len == 0)                             //a为0
    {
        a_gap.push_back(in(a_begin, b_len));
        return std::make_tuple(std::move(a_gap), std::move(b_gap));
    }
    else if (b_len == 0)                        //b为0
    {
        b_gap.push_back(in(b_begin, a_len));
        return std::make_tuple(std::move(a_gap), std::move(b_gap));
    }

    A.length = a_len;
    A.seq = utils::copy_DNA(sequence1.seq, Aq, a_begin, a_end);
    B.length = b_len;
    B.seq = utils::copy_DNA(sequence2.seq, Bq, b_begin, b_end);
    //utils::DNA_cout(A);
    //utils::DNA_cout(B);
    if (a_len > b_len)  //保证  B长，A短
    {
        auto tmp = A; A = B; B = tmp;
        diff = a_len - b_len;
        i = a_len; a_len = b_len; b_len = i;
        state_ex = true; //交换标识
    }
    else
        diff = b_len - a_len;
    m = a_len, n = b_len;
    my_INT_MIN = old = -d * n;

    while (k <= m)
    {// init
        //Init(bt, pm, m, k, diff);
        pm = pmt1;
        pm2 = pmt2;
        for (i = 0; i < (m + 1); i++)
        {
            for (int j = 0; j < (diff + 2 * k + 1); j++)
                bt[i][j] = '\0';
        }
        for (i = 0; i < 3; i++)
        {
            for (int j = 0; j < (diff + 2 * k + 1); j++)
                pm[i][j] = my_INT_MIN;
        }
        pm[0][k] = 0;
        bt[0][k] = '\16';
        for (j = 0; j < (diff + k + 1); j++)
        {
            pm[1][j + k] = -d - e * (j - 1);
            bt[0][j + k] = (char)8;
        }
        for (i = 0; i < (k + 1); i++)
            bt[i][k - i] = '\3';
        l = diff + 2 * k + 1;
        //end-init
        for (i = 1; i < (m + 1); i++)
        {
            //InitTwo(pm2, i, k, diff);
            for (int q = 0; q < 3; q++)
            {
                for (j = 0; j < (diff + 2 * k + 1); j++)
                    pm2[q][j] = my_INT_MIN;
            }
            if (i < k + 1)
                pm2[2][index(k - i, diff + 2 * k + 1)] = -d - e * (i - 1);
            l2 = diff + 2 * k + 1;
            //end-init
            for (int z = -k; z < (diff + k + 1); z++)
            {
                j = z;
                if ((1 <= (j + i)) && ((j + i) <= n))
                {
                    j = j + k;
                    bt1 = bt2 = bt3 = 0;
                    bt1 = ChooseWay(pm[0][index(j, l)], pm[1][index(j, l)], pm[2][index(j, l)]);
                    pm2[0][index(j, l2)] = maxi(pm[0][index(j, l)], pm[1][index(j, l)], pm[2][index(j, l)]) + score(get_char1(A, i - 1), get_char1(B, j + i - k - 1));
                    if (InsiderStrip(i, j + i - k - 1, k, diff))// x : B[j] ~_
                    {
                        pm2[1][index(j, l2)] = maxi(pm2[0][index(j - 1, l2)] - d, pm2[1][index(j - 1, l2)] - e);
                        if ((pm2[0][index(j - 1, l2)] - d) > (pm2[1][index(j - 1, l2)] - e)) bt2 = 4;
                        else bt2 = 8;
                    }
                    if (InsiderStrip(i - 1, j + i - k, k, diff))// y : A[i] ~_
                    {
                        pm2[2][index(j, l2)] = maxi(pm[0][index(j + 1, l)] - d, pm[2][index(j + 1, l)] - e);
                        if ((pm[0][index(j + 1, l)] - d) > (pm[2][index(j + 1, l)] - e)) bt3 = 1;
                        else bt3 = 3;
                    }
                    bt[i][index(j, l)] = (char)(bt1 + bt2 + bt3);
                }
            }
            pmt = pm;
            pm = pm2;
            pm2 = pmt;
        }
        newi = maxi(pm[0][diff + k], pm[1][diff + k], pm[2][diff + k]);
        if (old == newi || (k * 2) > m) break;
        else { old = newi; k *= 2; }
    }
    channel = ChooseWay(pm[0][diff + k], pm[1][diff + k], pm[2][diff + k], false);

    //traceback
    i = m;
    b_j = n;
    j = diff + k;

    seq_A.clear();
    seq_B.clear();

    while (i > 0 || j > k)
    {
        if (channel == 0)
        {
            channel = parse(bt[i][j], 0);
            seq_A.push_back(get_char1(A, --i));
            seq_B.push_back(get_char1(B, --b_j));
        }
        else if (channel == 1)
        {
            channel = parse(bt[i][j], 1);
            seq_A.push_back('\7');
            seq_B.push_back(get_char1(B, b_j - 1));
            b_j--;
            j--;
        }
        else if (channel == 2)
        {
            channel = parse(bt[i][j], 2);
            seq_A.push_back(get_char1(A, i - 1));
            seq_B.push_back('\7');
            i--;
            j++;
        }
        else
        {
            std::cout << "channel error!\n";
            exit(-1);
        }
    }
    //std::cout << seq_A.size() << " " << seq_B.size() << "\n";
    int j1 = 0, j2 = 0, num1 = 0, num2 = 0, match_num = 0;
    if (state_ex)
    {
        for (i = seq_A.size() - 1; i >= 0; i--)
        {
            if (seq_A[i] == '\7') { num1++; }
            else { if (num1 != 0)b_gap.push_back(in(b_begin + j1, num1)); num1 = 0; j1++; }
            if (seq_B[i] == '\7') { num2++; }
            else
            {
                if (num2 != 0)a_gap.push_back(in(a_begin + j2, num2)); num2 = 0; j2++;
                if (seq_A[i] == seq_B[i]) match_num++;
            }
        }
        if (num1 != 0)b_gap.push_back(in(b_begin + j1, num1));
        if (num2 != 0)a_gap.push_back(in(a_begin + j2, num2));
        /*
        if (b_len <= 10 && (b_len - a_len) < 4)
        {
            if (seq_A.back() != '\7' && seq_B.back() != '\7' && (seq_A.back() != seq_B.back()))
                SNP.push_back(std::make_tuple(a_begin, b_begin));
            if (seq_A[0] != '\7' && seq_B[0] != '\7' && (seq_A[0] != seq_B[0]))
                SNP.push_back(std::make_tuple(a_begin + b_len - 1, b_begin + a_len - 1));
        }
        else if (b_len > 10 && (b_len - a_len) < 10 && match_num > (int)(a_len * 0.8))
        {
            //std::cout << b_len << " " << a_len << " " << match_num << "\n";
            j1 = j2 = 0;
            for (i = seq_A.size() - 1; i >= 0; i--)
            {
                if (seq_A[i] != '\7' && seq_B[i] != '\7')
                {
                    if (seq_A[i] != seq_B[i])
                        SNP.push_back(std::make_tuple(a_begin + j2, b_begin + j1));
                    j1++; j2++;
                }
                else
                {
                    if (seq_A[i] != '\7') j1++;
                    if (seq_B[i] != '\7') j2++;
                }
            }
        }
        */
    }
    else
    {
        for (i = seq_A.size() - 1; i >= 0; i--)
        {
            if (seq_A[i] == '\7') { num1++; }
            else { if (num1 != 0)a_gap.push_back(in(a_begin + j1, num1)); num1 = 0; j1++; }
            if (seq_B[i] == '\7') { num2++; }
            else
            {
                if (num2 != 0)b_gap.push_back(in(b_begin + j2, num2)); num2 = 0; j2++;
                if (seq_A[i] == seq_B[i]) match_num++;
            }
        }
        if (num1 != 0)a_gap.push_back(in(a_begin + j1, num1));
        if (num2 != 0)b_gap.push_back(in(b_begin + j2, num2));
        /*
        if (a_len == 1 && b_len == 1)
            SNP.push_back(std::make_tuple(a_begin, b_begin));
        else if (b_len <= 10 && (b_len - a_len) < 4)
        {
            if (seq_A.back() != '\7' && seq_B.back() != '\7' && (seq_A.back() != seq_B.back()))
                SNP.push_back(std::make_tuple(a_begin, b_begin));
            if (seq_A[0] != '\7' && seq_B[0] != '\7' && (seq_A[0] != seq_B[0]))
                SNP.push_back(std::make_tuple(a_begin + a_len - 1, b_begin + b_len - 1));
        }
        else if ((b_len > 10) && ((b_len - a_len) < 10) && (match_num > (int)(a_len * 0.8)))
        {
            j1 = j2 = 0;
            //std::cout << b_len << " " << a_len << " " << match_num << "\n";
            for (i = seq_A.size() - 1; i >= 0; i--)
            {
                if ((seq_A[i] != '\7') && (seq_B[i] != '\7'))
                {
                    if (seq_A[i] != seq_B[i])
                        SNP.push_back(std::make_tuple(a_begin + j1, b_begin + j2));
                    j1++; j2++;
                }
                else
                {
                    if (seq_A[i] != '\7') j1++;
                    if (seq_B[i] != '\7') j2++;
                }
            }
        }
        */
    }

    //std::cout << a_len << " " << b_len << "     2\n";
    //EmptySet();
    return std::make_tuple(std::move(a_gap), std::move(b_gap));

}



//BWT_MINI
std::vector<triple> BWT_MINI::_MultiReg(const std::vector<triple>& common_substrings) //最优路径
{
    std::vector<triple> optimal_common_substrings;
    if (common_substrings.empty()) return optimal_common_substrings;
    int start = common_substrings[0][0];  //初始化
    int end = common_substrings[0][0] + common_substrings[0][2];//
    int i;
    float a_length, tmp, pre_tmp, b_length = common_substrings.rbegin()[0][1] + common_substrings.rbegin()[0][2]; //B的长度
    for (i = 1; i < common_substrings.size(); i++) // 找出所有后缀号列表中，最小后缀号做  start起点，最大后缀号 + length_i做end终点
    {
        if (common_substrings[i][0] < start) start = common_substrings[i][0];
        if ((common_substrings[i][0] + common_substrings[i][2]) > end) end = common_substrings[i][0] + common_substrings[i][2];
    }
    a_length = end - start;
    i = 0;
    optimal_common_substrings.push_back(common_substrings[i]);//first
    pre_tmp = common_substrings[i][0] / a_length - common_substrings[i][1] / b_length;//加入第一个元素
    pre_tmp = fabs(pre_tmp);
    for (i = i + 1; i < common_substrings.size(); i++)
    {
        if (optimal_common_substrings.rbegin()[0][1] != common_substrings[i][1]) //b.index 不同
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
    return optimal_common_substrings;
}
//回溯
std::vector<int> BWT_MINI::_trace_back(const std::vector<triple>& common_substrings, int* p)
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
    reverse(ansi.begin(), ansi.end());//反转
    return ansi;
}
//第二步，依据动态规划，选出合适的不重叠的同源区段
std::vector<triple> BWT_MINI::_optimal_path(const std::vector<triple>& common_substrings) //最优路径
{
    std::vector<triple> optimal_common_substrings = _MultiReg(common_substrings);//调用第一步结
    int m = optimal_common_substrings.size();
    if (m <= 1) return optimal_common_substrings;
    int* p = new int[m];
    for (int i = 0; i < m; i++) p[i] = optimal_common_substrings[i][2];
    for (int i = 1; i < m; i++)
        for (int j = 0; j < i; j++)
            if (optimal_common_substrings[i][0] >= (optimal_common_substrings[j][0] + optimal_common_substrings[j][2]))
                p[i] = (p[i] > (p[j] + optimal_common_substrings[i][2])) ? p[i] : (p[j] + optimal_common_substrings[i][2]);
    std::vector<int> ansi = _trace_back(optimal_common_substrings, p);

    std::vector<triple> ans_common_substrings;
    for (int i = 0; i < ansi.size(); i++) ans_common_substrings.push_back(optimal_common_substrings[ansi[i]]);
    std::vector<triple>().swap(optimal_common_substrings);//清空一块内存
    std::vector<int>().swap(ansi);
    delete[] p;
    return ans_common_substrings;
}

std::tuple<insert, insert>
    BWT_MINI::BWT_mini(Kband* kband, utils::seq_NCBI2NA& sequence1, int a_begin, int a_end, utils::seq_NCBI2NA& sequence2, int b_begin, int b_end,
        int thresh, insert& SNP, int d, int e)
{
    int a_len = a_end - a_begin;
    int b_len = b_end - b_begin;
    std::tuple<insert, insert> before;
    std::tuple<insert, insert> after;
    int a, b, c, dd;
    //std::cout << a_len << " " << b_len << "--len\n";
    insert a_gap;
    insert b_gap;
    if (a_len == 0)                             //a为0
    {
        a_gap.push_back(in(a_begin, b_len));
        return std::make_tuple(std::move(a_gap), std::move(b_gap));
    }
    else if (b_len == 0)                        //b为0
    {
        b_gap.push_back(in(b_begin, a_len));
        return std::make_tuple(std::move(a_gap), std::move(b_gap));
    }
    else if ((a_len < thresh) && (b_len < thresh)) //A，B都小于阈值
    {
        return kband->PSA_AGP_Kband3(sequence1, a_begin, a_end, sequence2, b_begin, b_end, SNP);
    }
    else if (b_len <= a_len)  //保持A长
    {
        if ((a_len / b_len > 1000) && (b_len < 1000)) //离谱差异，插空补长
        {
            b_gap.push_back(in(b_begin, a_len - b_len));
            return std::make_tuple(std::move(a_gap), std::move(b_gap));
        }
        suffix_array::SuffixArray<nucleic_acid_pseudo::NUMBER> mini_bwt(sequence1, a_begin, a_end, nucleic_acid_pseudo::end_mark);//实例化后缀树对象st，比对同源区域
        std::vector<unsigned char> veci = utils::bin_to_vector(sequence2, b_begin, b_end);
        std::vector<triple> common_substrings = _optimal_path(mini_bwt.get_common_substrings(veci.cbegin(), veci.cend(), 5, false)); //common_substrings [A.index,B.index,length]
        if (common_substrings.size() == 0)
            common_substrings = _optimal_path(mini_bwt.get_common_substrings(veci.cbegin(), veci.cend(), 1, false));
        std::vector<unsigned char>().swap(veci);
        std::vector<quadra> intervals;
        intervals.reserve(common_substrings.size() + 1);
        if (common_substrings.empty())
        {
            intervals.push_back(quadra({ 0, a_len, 0, b_len }));
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

            if (common_substrings.back()[0] + common_substrings.back()[2] != a_len ||
                common_substrings.back()[1] + common_substrings.back()[2] != b_len)
                intervals.push_back(quadra
                ({
                    common_substrings.back()[0] + common_substrings.back()[2], a_len,
                    common_substrings.back()[1] + common_substrings.back()[2], b_len
                    }));
        }
        //三元组同源序列
        std::vector<triple>().swap(common_substrings);
        //四元组 mis片段
        for (size_t j = 0; j != intervals.size(); ++j)
        {
            const int centre_begin = intervals[j][0] + a_begin;
            const int centre_end = intervals[j][1] + a_begin;
            const int sequence_begin = intervals[j][2] + b_begin;
            const int sequence_end = intervals[j][3] + b_begin;
            auto [lhs_gaps, rhs_gaps] = BWT_mini(kband, sequence1, centre_begin, centre_end, sequence2, sequence_begin, sequence_end, thresh, SNP); //分治到比thresh小，然后k-band
            a_gap.insert(a_gap.end(), lhs_gaps.begin(), lhs_gaps.end());
            b_gap.insert(b_gap.end(), rhs_gaps.begin(), rhs_gaps.end());
            insert().swap(lhs_gaps);
            insert().swap(rhs_gaps);
        }
        std::vector<quadra>().swap(intervals);
        //EmptySet();
        return std::make_tuple(std::move(a_gap), std::move(b_gap));//返回000005000  gap-vecter
    }
    else
    {
        if ((b_len / a_len > 1000) && (a_len < 1000)) //离谱差异，插空补长
        {
            a_gap.push_back(in(a_begin, b_len - a_len));
            return std::make_tuple(std::move(a_gap), std::move(b_gap));
        }
        suffix_array::SuffixArray<nucleic_acid_pseudo::NUMBER> mini_bwt(sequence2, b_begin, b_end, nucleic_acid_pseudo::end_mark);//实例化后缀树对象st，比对同源区域
        std::vector<unsigned char> veci = utils::bin_to_vector(sequence1, a_begin, a_end);
        std::vector<triple> common_substrings = _optimal_path(mini_bwt.get_common_substrings(veci.cbegin(), veci.cend(), 5, false)); //common_substrings [A.index,B.index,length]
        if (common_substrings.size() == 0)
            common_substrings = _optimal_path(mini_bwt.get_common_substrings(veci.cbegin(), veci.cend(), 1, false));
        std::vector<unsigned char>().swap(veci);
        std::vector<quadra> intervals;
        intervals.reserve(common_substrings.size() + 1);
        if (common_substrings.empty())
        {
            intervals.push_back(quadra({ 0, b_len, 0, a_len }));
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

            if (common_substrings.back()[0] + common_substrings.back()[2] != b_len ||
                common_substrings.back()[1] + common_substrings.back()[2] != a_len)
                intervals.push_back(quadra
                ({
                    common_substrings.back()[0] + common_substrings.back()[2], b_len,
                    common_substrings.back()[1] + common_substrings.back()[2], a_len
                    }));
        }
        //三元组同源序列
        std::vector<triple>().swap(common_substrings);
        //四元组 mis片段

        for (size_t j = 0; j != intervals.size(); ++j)
        {
            const int centre_begin = intervals[j][0] + b_begin;
            const int centre_end = intervals[j][1] + b_begin;
            const int sequence_begin = intervals[j][2] + a_begin;
            const int sequence_end = intervals[j][3] + a_begin;
            auto [lhs_gaps, rhs_gaps] = BWT_mini(kband, sequence1, sequence_begin, sequence_end, sequence2, centre_begin, centre_end, thresh, SNP); //分治到比thresh小，然后k-band
            a_gap.insert(a_gap.end(), lhs_gaps.begin(), lhs_gaps.end());
            b_gap.insert(b_gap.end(), rhs_gaps.begin(), rhs_gaps.end());
            insert().swap(lhs_gaps);
            insert().swap(rhs_gaps);
        }

        std::vector<quadra>().swap(intervals);
        EmptySet();
        return std::make_tuple(std::move(a_gap), std::move(b_gap));//返回000005000  gap-vecter
    }

}


//顶层调用，递归 返回匹配好的串vector
std::tuple<insert, insert>
new_main_Align(Kband* kband, utils::seq_NCBI2NA& sequence1, int a_begin, int a_end, utils::seq_NCBI2NA& sequence2, int b_begin, int b_end,
    int thresh, insert& SNP_i, bool _mg_tag, utils::more_block& more_gap, bool tag01, int d, int e)
{
    int a_len = a_end - a_begin;
    int b_len = b_end - b_begin;
    insert a_gaps;
    insert b_gaps;
    int gap_num = 0, i = 0;

    if (a_len == 0)                             //a为0
    {
        a_gaps.push_back(in(a_begin, b_len));
        return std::make_tuple(std::move(a_gaps), std::move(b_gaps));
    }
    else if (b_len == 0)                        //b为0
    {
        b_gaps.push_back(in(b_begin, a_len));
        return std::make_tuple(std::move(a_gaps), std::move(b_gaps));
    }
    else if ((a_len < thresh) && (b_len < thresh)) //A，B都小于阈值
    {
        return kband->PSA_AGP_Kband3(sequence1, a_begin, a_end, sequence2, b_begin, b_end, SNP_i);
    }
    else
    {
        return BWT_MINI::BWT_mini(kband, sequence1, a_begin, a_end, sequence2, b_begin, b_end, thresh, SNP_i); //MINI bwt变换求
    }
}

//处理单个逆补串簇
void
nibu_Align(Kband* kband, utils::seq_NCBI2NA& sequence1, utils::seq_NCBI2NA& sequence2, int thresh, std::reverse_iterator<std::vector<std::vector<triple>>::iterator>& C_Strer,
    insert& SNP_i, utils::more_block& more_gap, std::array<int, 5>& interval, int d, int e)
{
    int a_begin = interval[0];
    int a_end = interval[1];
    int b_begin = interval[2];
    int b_end = interval[3];

    //std::cout << a_begin<<" nibu_Align \n";
    std::vector<triple> C_String = *C_Strer;
    //std::cout << " nibu_Align \n";
    //std::cout << interval[0] << " " << interval[1] << " " << interval[2] << " " << interval[3] << " " << interval[4] << " interval\n";
    //std::cout << C_String[0][0] << " " << C_String.back()[0] + C_String.back()[2] << " " << sequence2.length - C_String.back()[1] - C_String.back()[2] << " " << sequence2.length - C_String[0][1] << " C_String\n\n";
    C_Strer++;
    int Blength = sequence2.length;
    insert SNP;

    //utils::m_block Block;
    more_gap.push_back(utils::m_block());
    more_gap.back().start1 = a_begin;
    more_gap.back().start2 = b_begin;
    more_gap.back().end1 = a_end;
    more_gap.back().end2 = b_end;
    more_gap.back().tag = 1;
    if (C_String.size() == 1)
        return;
    utils::seq_NCBI2NA A, B;
    A.length = a_end - a_begin;
    A.seq = utils::copy_DNA(sequence1.seq, a_begin, a_end);
    B.length = b_end - b_begin;
    std::string back_sequences(B.length, '\0');
    int zt = 0;
    for (int j = b_end - 1; j >= b_begin; j--)
        back_sequences[zt++] = 3 - NCBI2NA_UNPACK_BASE(sequence2.seq[j / 4], (3 - j % 4));
    B.seq = utils::BLAST_PackDNA(back_sequences);
    std::string().swap(back_sequences);

    b_begin = Blength - b_end;
    //b_begin = C_String[0][1];
    //std::cout << "nibu_Align " << b_begin << " " << C_String[0][1] << "\n";
    for (int i = 1; i < C_String.size(); i++)
    {
        auto [agap_, bgap_] = BWT_MINI::BWT_mini(kband, A, C_String[i - 1][0] + C_String[i - 1][2] - a_begin, C_String[i][0] - a_begin,
            B, C_String[i - 1][1] + C_String[i - 1][2] - b_begin, C_String[i][1] - b_begin, thresh, SNP); //MINI bwt变换求

        more_gap.back().gap1.insert(more_gap.back().gap1.end(), agap_.cbegin(), agap_.cend());
        more_gap.back().gap2.insert(more_gap.back().gap2.end(), bgap_.cbegin(), bgap_.cend());
    }


    delete[] A.seq;
    delete[] B.seq;
}

//处理0集合
std::tuple<insert, insert>
new_mul_main_Align(Kband* kband, utils::seq_NCBI2NA& sequence1, int a_begin, int a_end, utils::seq_NCBI2NA& sequence2, int b_begin, int b_end,
    int thresh, insert& SNP_i, bool _mg_tag, utils::more_block& more_gap, std::vector<std::array<int, 5>>& intervals,
    std::reverse_iterator<std::vector<std::vector<triple>>::iterator>& C_String, int d, int e)
{
    int a_len = a_end - a_begin;
    int b_len = b_end - b_begin;
    //std::cout << a_len << " " << b_len << "    len1\n";
    std::vector<std::array<int, 2>> dB;
    insert  a_gaps_more, b_gaps_more;
    std::tuple<insert, insert> gaps;
    insert& a_gaps = std::get<0>(gaps);
    insert& b_gaps = std::get<1>(gaps);
    int index = 0, i = 0, j, sum, gap_num;
    std::vector<unsigned char> seq_A;
    std::vector<unsigned char> seq_B;
    if (a_len == 0)
    {
        for (i = 0; i < intervals.size(); i++)//传入函数处理逆补
            nibu_Align(kband, sequence1, sequence2, thresh, C_String, SNP_i, more_gap, intervals[i]);
        a_gaps.push_back(in(a_end, b_len - a_len));
        return std::make_tuple(std::move(a_gaps), std::move(b_gaps));
    }

    seq_A = utils::bin_to_vector(sequence1, a_begin, a_end);
    seq_B = utils::bin_to_vector(sequence2, b_begin, b_end);
    //std::cout << "error:0\n";
    i = intervals.size() - 1;
    j = 0;
    while (i >= 0)//????
    {
        if (intervals[i][4] == 0)//- 与 B
        {
            //std::cout << b_begin << " " << b_end << " " << intervals[i][2] - b_begin << " " << intervals[i][3] - b_begin << " erase\n";
            seq_B.erase(seq_B.begin() + (intervals[i][2] - b_begin), seq_B.begin() + (intervals[i][3] - b_begin));
            j = seq_B.begin() - seq_B.end() + (intervals[i][2] - b_begin);//相对于结尾的偏移，负的
            dB.insert(dB.begin(), std::array<int, 2>({ j,intervals[i][3] - intervals[i][2] }));
        }
        else
        {
            std::cout << "error: intervals[i][4] != 0\n";
            exit(-1);
        }
        i--;
    }
    //std::cout << "error:1\n";
    for (i = 0; i < intervals.size(); i++)//传入函数处理逆补
        nibu_Align(kband, sequence1, sequence2, thresh, C_String, SNP_i, more_gap, intervals[i]);
    //std::cout << "error:2\n";

    if (seq_B.size() == 0)
    {
        if (b_len > a_len)
            a_gaps.push_back(in(a_end, b_len - a_len));
        else
            b_gaps.push_back(in(b_end, a_len - b_len));
        return gaps;
    }

    utils::seq_NCBI2NA A, B;
    a_len = A.length = seq_A.size();
    A.seq = utils::BLAST_PackDNA(seq_A);
    std::vector<unsigned char>().swap(seq_A);
    b_len = B.length = seq_B.size();
    B.seq = utils::BLAST_PackDNA(seq_B);
    std::vector<unsigned char>().swap(seq_B);
    int wr = 0;
    for (i = 0; i < dB.size(); i++)//在dA[i][0]后面插入dA[i][1]个碱基或者gap
    {
        dB[i][0] += b_len;//距离开头的偏移,转为正的
        wr += dB[i][1];
    }
    //std::cout << a_len << " " << b_len << " " << wr << "    wr\n";
    //比对
    if (a_len == 0)                             //a为0
        a_gaps.push_back(in(0, b_len));
    else if (b_len == 0)                        //b为0
        b_gaps.push_back(in(0, a_len));
    else if ((a_len < thresh) && (b_len < thresh)) //A，B都小于阈值
        gaps = kband->PSA_AGP_Kband3(A, 0, a_len, B, 0, b_len, SNP_i);
    else
        gaps = BWT_MINI::BWT_mini(kband, A, 0, a_len, B, 0, b_len, thresh, SNP_i); //MINI bwt变换求
    delete[] A.seq;
    delete[] B.seq;
    //std::cout << "error:3\n";

    int an = 0, bn = 0;
    for (int i = 0; i < a_gaps.size(); i++)
    {
        an += std::get<1>(a_gaps[i]);

    }
    for (int i = 0; i < b_gaps.size(); i++)
    {
        bn += std::get<1>(b_gaps[i]);
    }


    //得到a_more_gap
    index = 0; i = 0; j = 0; sum = 0; gap_num = 0;
    while (index < dB.size())
    {
        sum = 0;
        while (i < b_gaps.size() && std::get<0>(b_gaps[i]) < dB[index][0])
            gap_num += std::get<1>(b_gaps[i++]);
        sum = gap_num + dB[index][0];
        j = 0;
        while (sum >= 0 && j < a_gaps.size())
        {
            sum -= std::get<1>(a_gaps[j]);
            if (sum <= std::get<0>(a_gaps[j]))
            {
                sum += std::get<1>(a_gaps[j]);
                if (sum >= std::get<0>(a_gaps[j]))
                {
                    a_gaps_more.push_back(in(std::get<0>(a_gaps[j]), dB[index][1]));
                    sum = -1;
                    break;
                }
                else
                {
                    a_gaps_more.push_back(in(sum, dB[index][1]));
                    sum = -1;
                    break;
                }

            }
            j++;
        }if (sum != -1)a_gaps_more.push_back(in(sum, dB[index][1]));
        index++;
    }
    //std::cout << "error:4\n";
    //结合a_gap和a_gap_more
    i = 0; j = 0;
    while (i < a_gaps.size() && j < a_gaps_more.size())
    {
        if (std::get<0>(a_gaps[i]) < std::get<0>(a_gaps_more[j]))
            i++;
        else if (std::get<0>(a_gaps[i]) == std::get<0>(a_gaps_more[j]))
        {
            std::get<1>(a_gaps[i]) += std::get<1>(a_gaps_more[j]);
            j++;
        }
        else
        {
            a_gaps.insert(a_gaps.begin() + i, a_gaps_more[j]);
            i++;
            j++;
        }
    }
    while (j < a_gaps_more.size())
        a_gaps.push_back(a_gaps_more[j++]);
    insert().swap(a_gaps_more);
    //std::cout << "error:5\n";
    //转换dB  用于转换b_gap的index
    for (i = dB.size() - 1; i >= 0; i--)
        for (j = i - 1; j >= 0; j--)
            dB[i][1] += dB[j][1];
    //std::cout << "error:6\n";
    //转换b_gap的index
    i = 0; j = 0;
    while (i < b_gaps.size())
    {
        if (((j < dB.size()) && (std::get<0>(b_gaps[i]) < dB[j][0])) || (j == dB.size()))
        {
            if (j != 0)
                std::get<0>(b_gaps[i]) += dB[j - 1][1];
            i++;
        }
        else
            j++;
    }
    std::vector<std::array<int, 2>>().swap(dB);
    //std::cout << "error:7\n";
    //转换ab_gap的index,利用begin
    for (i = 0; i < a_gaps.size(); i++)
        std::get<0>(a_gaps[i]) += a_begin;
    for (i = 0; i < b_gaps.size(); i++)
        std::get<0>(b_gaps[i]) += b_begin;
    //std::cout << "error:8\n";
    an = 0, bn = 0;
    for (int i = 0; i < a_gaps.size(); i++)
    {
        an += std::get<1>(a_gaps[i]);

    }
    for (int i = 0; i < b_gaps.size(); i++)
    {
        bn += std::get<1>(b_gaps[i]);
    }
    return gaps;
}