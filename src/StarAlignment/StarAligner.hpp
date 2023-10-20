#pragma once
#include "../SuffixArray/SuffixArray.hpp"  //利用后缀树
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

namespace star_alignment //星比对命名空间
{

    class StarAligner//星比对类
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
            size_t a_start, b_start, a_end, b_end; //闭区间
            size_t nw_start, nw_end;
        };
        static void get_common_substrings_vector(Substrings& C_Strings, std::vector<triple>& common_substrings, bool sign, size_t read_len);
        //先调用类初始化，再调用_get_gaps
        //传入int序列数据；
        //返回最后结果，长度为n的vector，每个vector存储若干个Insertion，有index+number
        static std::vector<std::vector<utils::Insertion>> get_gaps(std::vector<utils::seq_NCBI2NA> &sequences, std::vector<std::vector<utils::Insertion>>& N_gap, 
            std::vector<std::string>& name, std::vector<bool>& sign, std::vector<size_t>& Length, std::vector<bool>& TU, std::vector<utils::more_block>& more_gap, bool mg_tag, utils::MAF_info& MAFinfo,
            int threshold1 = 15, int threshold2 = 10000, int center=-1);
        
        //依据动态规划选择最长不重叠同源区段
        //传入全部的同源区段，[[A.index，B.index，length]...] B.index可能相同即[[[A.index0,A.index1...],B.index,length],...]
        //返回选择好的同源区段[[A.index，B.index，length]...] B.index各不相同即[[A.index,B.index,length],...]
        static std::vector<triple> _optimal_path(const std::vector<triple> &common_substrings);//最优路径
        static std::vector<triple> sv_optimal_path(std::vector <std::vector<quadra>>& chain, const std::vector<triple>& common_substrings, bool filter); //(最优路径+_sv_chain)
        static std::vector <std::vector<quadra>> find_subchains(std::vector<triple>& common_substrings);
        //分两步新改,第一步，从传入全部的同源区段，[[A.index，B.index，length]...]，每个B.index选出一个A.index，相对距离最近
        static std::vector<triple> _MultiReg(const std::vector<triple>& common_substrings);
        void mul_sv_maf_func(int i, std::ifstream& ns, const suffix_array::SuffixArray<nucleic_acid_pseudo::NUMBER>& st,
            std::vector<std::array<std::vector<utils::Insertion>, 2>>& all_pairwise_gaps, std::vector<insert>& SNP_vector, int threshold1, int threshold2) const;
        void mul_fasta_func(int i, const suffix_array::SuffixArray<nucleic_acid_pseudo::NUMBER>& st,
            std::vector<std::array<std::vector<utils::Insertion>, 2>>& all_pairwise_gaps, std::vector<insert>& SNP_vector, int threshold1, int threshold2) const;

    private:
        StarAligner(std::vector<utils::seq_NCBI2NA> &sequences, std::vector<std::vector<utils::Insertion>>& N_gap, std::vector<std::string> &name, std::vector<bool> &sign, 
            std::vector<size_t>& Length, std::vector<bool>& TU, std::vector<utils::more_block>& more_gap, bool mg_tag, utils::MAF_info& MAFinfo, int center);
        ~StarAligner();
        
        //类已经初始化，不需要传入变量。
        //返回最后结果，长度为n的vector，每个vector存储若干个Insertion，有index+number
        std::vector<std::vector<utils::Insertion>> _get_gaps(int threshold1, int threshold2) const;

        std::vector<size_t> _set_lengths();
        size_t _set_centre(int center) ;
        // main steps
        //依据sufferTree进行双比，返回all_pairwise_gaps
        //无需传入，类已经初始化
        //返回，双序列比对得到的两两gap，长度为n的vector，每个元素有长度为2的array，每个元素是有若干个Insertion的vector
        std::vector<std::array<std::vector<utils::Insertion>, 2>> mul_pairwise_align(int threshold1, int threshold2) const;
        std::vector<std::array<std::vector<utils::Insertion>, 2>> mul_sv_maf_pairwise_align(int threshold1, int threshold2) const;
        void getSV_write2file(std::array<std::vector<utils::Insertion>, 2>& pairwise_gaps, Kband* kband, std::ofstream& ofsv, int seq_i, std::vector <std::vector<quadra>>& chains, std::vector <std::vector<quadra>>& chains0, unsigned char* array_A, unsigned char* array_B) const;
        //整合MSA-gap结果，传入双比gap结果，返回final_sequence_gaps
        //传入，双序列比对得到的两两gap，长度为n的vector，每个元素有长度为2的array，每个元素是有若干个Insertion的vector
        //返回最后结果，长度为n的vector，每个vector存储若干个Insertion，有index+number
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