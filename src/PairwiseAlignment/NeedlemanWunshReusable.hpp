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
}
;
/**/
namespace BWT_MINI //�Ǳȶ������ռ�
{
    //BWT�׼�������
    //��һ�����Ӵ���ȫ����ͬԴ���Σ�[[A.index��B.index��length]...]��ÿ��B.indexѡ��һ��A.index����Ծ������
    std::vector<triple> _MultiReg(const std::vector<triple>& common_substrings);
    //����
    std::vector<int> _trace_back(const std::vector<triple>& common_substrings, int* p);
    //�ڶ��������ݶ�̬�滮��ѡ�����ʵĲ��ص���ͬԴ����
    std::vector<triple> _optimal_path(const std::vector<triple>& common_substrings);

    std::tuple<insert, insert>
        BWT_mini(Kband* kband, utils::seq_NCBI2NA& sequence1, int a_begin, int a_end, utils::seq_NCBI2NA& sequence2, int b_begin, int b_end,
            int thresh, insert& SNP, int d = 3, int e = 1);
}

//������ã��ݹ� ����ƥ��õĴ�vector
std::tuple<insert, insert>
new_main_Align(Kband* kband, utils::seq_NCBI2NA& sequence1, int a_begin, int a_end, utils::seq_NCBI2NA& sequence2, int b_begin, int b_end,
    int thresh, insert& SNP_i, bool _mg_tag, utils::more_block& more_gap, bool tag01, int d = 3, int e = 1);

//�������油����
void
nibu_Align(Kband* kband, utils::seq_NCBI2NA& sequence1, utils::seq_NCBI2NA& sequence2, int thresh, std::reverse_iterator<std::vector<std::vector<triple>>::iterator>& C_Strer,
    insert& SNP_i, utils::more_block& more_gap, std::array<int, 5>& interval, int d = 3, int e = 1);

//����0����
std::tuple<insert, insert>
new_mul_main_Align(Kband* kband, utils::seq_NCBI2NA& sequence1, int a_begin, int a_end, utils::seq_NCBI2NA& sequence2, int b_begin, int b_end,
    int thresh, insert& SNP_i, bool _mg_tag, utils::more_block& more_gap, std::vector<std::array<int, 5>>& intervals,
    std::reverse_iterator<std::vector<std::vector<triple>>::iterator>& C_String, int d = 3, int e = 1);