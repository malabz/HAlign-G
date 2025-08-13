#pragma once

#include <vector>
#include <algorithm>
#include <string>
#include <tuple>
#include <cmath>
#include "../SuffixArray/SuffixArray.hpp"
#include "../Utils/Utils.hpp"

const int thresh0 = 10001;

using RandomAccessIterator = std::vector<unsigned char>::const_iterator;
using gap_vector_type = std::vector<int>;
using char_vector_type = std::vector<unsigned char>;
using triple = std::array<int, 4>;
using quadra = std::array<int, 5>;
using insert = std::vector<std::tuple<int, int>>;
using in = std::tuple<int, int>;

class Kband {
    public:
        int match;
        int mismatch;
        int d;
        int e;
        int my_INT_MIN;
        utils::seq_NCBI2NA A, B;
        UCHAR* Aq;
        UCHAR* Bq;
        int** pm;
        int** pm2;
        int** pmt1;
        int** pmt2;
        int** pmt;
        //int(*pm)[thresh0];
        //int(*pm2)[thresh0];
        //int(*pmt1)[thresh0];
        //int(*pmt2)[thresh0];
        //int(*pmt)[thresh0];
        //unsigned char(*bt)[thresh0];
        unsigned char** bt;
        std::vector<unsigned char> seq_A;
        std::vector<unsigned char> seq_B;

    public:
        Kband();

        ~Kband();
        inline int score(unsigned char xi, unsigned char yi);
        inline bool InsiderStrip(int i, int j, int k, int diff = 0);
        inline int index(int i, int l);
        inline int maxi(int a, int b);
        inline int maxi(int a, int b, int c);
        void Init(int m, int k, int diff);
        void InitTwo(int ii, int k, int diff);
        int ChooseWay(int p0, int p1, int p2, bool state = true);
        inline int parse(int b, int s);
        inline UCHAR get_char1(utils::seq_NCBI2NA& Seq, int i);

        std::tuple<insert, insert>
            PSA_AGP_Kband3(utils::seq_NCBI2NA& sequence1, int a_begin, int a_end, utils::seq_NCBI2NA& sequence2, int b_begin, int b_end,
                insert& SNP, int cmatch = 1, int cmismatch = -2, int cd = 3, int ce = 1);
        std::tuple<insert, insert>
            mum_PSA_AGP_Kband3(const std::vector<unsigned char>& sequence1, int a_begin, int a_end, const std::vector<unsigned char>& sequence2, int b_begin, int b_end,
               int cmatch = 1, int cmismatch = -2, int cd = 3, int ce = 1);
}
;
/**/
namespace BWT_MINI //星比对命名空间
{
    //BWT套件！！！
    //第一步，从传入全部的同源区段，[[A.index，B.index，length]...]，每个B.index选出一个A.index，相对距离最近
    std::vector<triple> _MultiReg(const std::vector<triple>& common_substrings);
    //回溯
    std::vector<int> _trace_back(const std::vector<triple>& common_substrings, int* p);
    //第二步，依据动态规划，选出合适的不重叠的同源区段
    std::vector<triple> _optimal_path(const std::vector<triple>& common_substrings);

    std::tuple<insert, insert>
        BWT_mini(Kband* kband, utils::seq_NCBI2NA& sequence1, int a_begin, int a_end, utils::seq_NCBI2NA& sequence2, int b_begin, int b_end,
            int thresh, insert& SNP, int d = 3, int e = 1);
    std::tuple<insert, insert>
        mum_BWT_mini(Kband* kband, const std::vector<unsigned char>& sequence1, int a_begin, int a_end, const std::vector<unsigned char>& sequence2, int b_begin, int b_end,
            int thresh, int d = 3, int e = 1);
}
std::tuple<insert, insert>
mum_main_Align(Kband* kband, const std::unique_ptr<Sequence>& sequence1, int a_begin, int a_end, const std::unique_ptr<Sequence>& sequence2, int b_begin, int b_end,
    int thresh, int d = 3, int e = 1);




































//顶层调用，递归 返回匹配好的串vector
std::tuple<insert, insert>
new_main_Align(Kband* kband, utils::seq_NCBI2NA& sequence1, int a_begin, int a_end, utils::seq_NCBI2NA& sequence2, int b_begin, int b_end,
    int thresh, insert& SNP_i, bool _mg_tag, utils::more_block& more_gap, bool tag01, int d = 3, int e = 1);

//处理单个逆补串簇
void
nibu_Align(Kband* kband, utils::seq_NCBI2NA& sequence1, utils::seq_NCBI2NA& sequence2, int thresh, std::reverse_iterator<std::vector<std::vector<triple>>::iterator>& C_Strer,
    insert& SNP_i, utils::more_block& more_gap, std::array<int, 5>& interval, int d = 3, int e = 1);

//处理0集合
std::tuple<insert, insert>
new_mul_main_Align(Kband* kband, utils::seq_NCBI2NA& sequence1, int a_begin, int a_end, utils::seq_NCBI2NA& sequence2, int b_begin, int b_end,
    int thresh, insert& SNP_i, bool _mg_tag, utils::more_block& more_gap, std::vector<std::array<int, 5>>& intervals,
    std::reverse_iterator<std::vector<std::vector<triple>>::iterator>& C_String, int d = 3, int e = 1);