#pragma once
#include "../SuffixArray/SuffixArray.hpp"  //���ú�׺��
#include "../Utils/Pseudo.hpp"
#include "../Utils/Utils.hpp"
#include "../Utils/Arguments.hpp"

#include "../PairwiseAlignment/NeedlemanWunshReusable.hpp"
#include <vector>
#include <array>
#include <string>
#include<algorithm>
#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <limits.h>

namespace star_alignment //�Ǳȶ������ռ�
{

    class StarAligner//�Ǳȶ���
    {
    public:
        using double2 = std::array<int, 2>;
        using triple = std::array<int, 4>;
        using quadra = std::array<int, 5>;
        using sequence_type = std::vector<unsigned char>;
        using quadras = std::vector<std::array<int, 5>>;
    public:
        struct subchain {
            size_t a_start, a_end, b_start, b_end, len;
            bool state;
            std::vector<triple> chains;
        };
        struct Substrings
        {
            bool sign;
            std::vector<std::vector<triple>> Subs;
            size_t add_len;
        };
        struct Sub_align
        {
            bool sign;
            size_t a_start, b_start, a_end, b_end; //������
            size_t nw_start, nw_end;
        };
        static void get_common_substrings_vector(Substrings& C_Strings, std::vector<triple>& common_substrings, bool sign, size_t read_len);
        //�ȵ������ʼ�����ٵ���_get_gaps
        //����int�������ݣ�
        //���������������Ϊn��vector��ÿ��vector�洢���ɸ�Insertion����index+number
        static std::vector<std::vector<utils::Insertion>> get_gaps(std::vector<utils::seq_NCBI2NA> &sequences, std::vector<std::vector<utils::Insertion>>& N_gap, 
            std::vector<std::string>& name, std::vector<bool>& sign, std::vector<size_t>& Length, std::vector<bool>& TU, std::vector<utils::more_block>& more_gap, bool mg_tag, utils::MAF_info& MAFinfo,
            int threshold1 = 15, int threshold2 = 10000, int center=-1);
        
        //���ݶ�̬�滮ѡ������ص�ͬԴ����
        //����ȫ����ͬԴ���Σ�[[A.index��B.index��length]...] B.index������ͬ��[[[A.index0,A.index1...],B.index,length],...]
        //����ѡ��õ�ͬԴ����[[A.index��B.index��length]...] B.index������ͬ��[[A.index,B.index,length],...]
        static std::vector<triple> _optimal_path(const std::vector<triple> &common_substrings);//����·��
        static std::vector<triple> sv_optimal_path(std::vector <std::vector<quadra>>& chain, const std::vector<triple>& common_substrings, bool filter); //(����·��+_sv_chain)
        static std::vector <std::vector<quadra>> find_subchains(std::vector<triple>& common_substrings);
        //�������¸�,��һ�����Ӵ���ȫ����ͬԴ���Σ�[[A.index��B.index��length]...]��ÿ��B.indexѡ��һ��A.index����Ծ������
        static std::vector<triple> _MultiReg(const std::vector<triple>& common_substrings);
        void mul_sv_maf_func(int i, std::ifstream& ns, const suffix_array::SuffixArray<nucleic_acid_pseudo::NUMBER>& st,
            std::vector<std::array<std::vector<utils::Insertion>, 2>>& all_pairwise_gaps, std::vector<insert>& SNP_vector, int threshold1, int threshold2) const;
        void mul_fasta_func(int i, const suffix_array::SuffixArray<nucleic_acid_pseudo::NUMBER>& st,
            std::vector<std::array<std::vector<utils::Insertion>, 2>>& all_pairwise_gaps, std::vector<insert>& SNP_vector, int threshold1, int threshold2) const;

    private:
        StarAligner(std::vector<utils::seq_NCBI2NA> &sequences, std::vector<std::vector<utils::Insertion>>& N_gap, std::vector<std::string> &name, std::vector<bool> &sign, 
            std::vector<size_t>& Length, std::vector<bool>& TU, std::vector<utils::more_block>& more_gap, bool mg_tag, utils::MAF_info& MAFinfo, int center);
        ~StarAligner();
        
        //���Ѿ���ʼ��������Ҫ���������
        //���������������Ϊn��vector��ÿ��vector�洢���ɸ�Insertion����index+number
        std::vector<std::vector<utils::Insertion>> _get_gaps(int threshold1, int threshold2) const;

        std::vector<size_t> _set_lengths();
        size_t _set_centre(int center) ;
        // main steps
        //����sufferTree����˫�ȣ�����all_pairwise_gaps
        //���贫�룬���Ѿ���ʼ��
        //���أ�˫���бȶԵõ�������gap������Ϊn��vector��ÿ��Ԫ���г���Ϊ2��array��ÿ��Ԫ���������ɸ�Insertion��vector
        std::vector<std::array<std::vector<utils::Insertion>, 2>> mul_pairwise_align(int threshold1, int threshold2) const;
        std::vector<std::array<std::vector<utils::Insertion>, 2>> mul_sv_maf_pairwise_align(int threshold1, int threshold2) const;
        void getSV_write2file(std::array<std::vector<utils::Insertion>, 2>& pairwise_gaps, Kband* kband, std::ofstream& ofsv, int seq_i, std::vector <std::vector<quadra>>& chains, std::vector <std::vector<quadra>>& chains0, unsigned char* array_A, unsigned char* array_B) const;
        //����MSA-gap���������˫��gap���������final_sequence_gaps
        //���룬˫���бȶԵõ�������gap������Ϊn��vector��ÿ��Ԫ���г���Ϊ2��array��ÿ��Ԫ���������ɸ�Insertion��vector
        //���������������Ϊn��vector��ÿ��vector�洢���ɸ�Insertion����index+number
        std::vector<std::vector<utils::Insertion>> _merge_results(const std::vector<std::array<std::vector<utils::Insertion>, 2>> &pairwise_gaps) const;
        static void hebing_substrings(Substrings& C_Strings, std::vector<quadras>& new_intervals, std::vector<quadra>& intervals, 
            std::vector<triple>& common_substrings, std::vector<triple>& common_substrings0, size_t _centre_len, size_t length, bool sign);
        
        std::vector<utils::seq_NCBI2NA> &_sequences;
        bloom_filter* filter1, * filter0;
        int _K;
        std::vector<std::string> &_name;
        std::vector<bool> &_sign;
        std::vector<std::vector<utils::Insertion>>& _N_gap;
        std::vector<utils::more_block>& _more_gap;
        bool _mg_tag;
        const size_t _row;
        std::vector<size_t>& _lengths;
        std::vector<bool>& _TU;
        utils::MAF_info _MAFinfo;
        size_t _centre;
        const size_t _centre_len;
        size_t _max_len;
    };

}
std::vector<int> _trace_back_bp(const std::vector<triple>& common_substrings, int* p);
void _optimal_path_bp(const std::vector<triple>& optimal_common_substrings, std::vector<triple>& ans_common_substrings);