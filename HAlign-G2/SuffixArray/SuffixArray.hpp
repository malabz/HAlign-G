#pragma once
#include "../Utils/Utils.hpp"
#include "divsufsort.h"
#include "../Utils/Arguments.hpp"

#include <iostream>
#include <vector>

#include <iterator>
#include <algorithm>
#include <cstring>
#include <array>
#include <limits>
#include <unordered_map>
#include <set>

namespace suffix_array
{

    template<size_t width> //后缀树类
    class SuffixArray
    {
    public:
        using triple = std::array<int, 4>;

        SuffixArray(utils::seq_NCBI2NA& sequence, int first, size_t last, const unsigned char end_mark)
            : length(last - first + 1)
            , dis(1)
            , reword(_copy_reword(sequence, first, last, end_mark))
            //, SA(build_sa())
            , SA(new int32_t[length])
        {
            divsufsort(reword, SA, length, 4);
            endp = build_b();
            delete[] reword;
        }

        SuffixArray(const std::vector<unsigned char>& sequence, int first, size_t last, const unsigned char end_mark)
            : length(last - first + 1)
            , dis(1)
            , reword(vec_copy_reword(sequence, first, last, end_mark))
            //, SA(build_sa())
            , SA(new int32_t[length])
        {
            divsufsort(reword, SA, length, 4);
            endp = build_b();
            delete[] reword;
        }

        ~SuffixArray()
        {
            delete[] O;
            delete[] SA;
            delete[] B;
            delete[] begin;
        }
        
        template<typename InputIterator>
        std::vector<int> search_for_prefix(InputIterator first, InputIterator last, size_t threshold) const
        {
            int common_prefix_length = 0;
            int lbegin, lend, start, end, len_sub = last - first;
            char sub = *(first++) + 1; //第0个
            start = begin[(int)sub - 1];
            end = begin[(int)sub] - 1;
            //std::cout << start << " " << end << " s-e?\n";
            while (first < last)
            {
                sub = *(first)+1;
                lbegin = find(sub, start, end);
                lend = rfind(sub, start, end);
                //std::cout << lbegin << " " << lend << "?\n";
                if (lbegin == -1)
                    break;
                start = begin[(int)sub - 1] + O_index_num(lbegin, sub) - 1;
                end = begin[(int)sub - 1] + O_index_num(lend, sub) - 1;
                //std::cout << start << " " << end << " ??\n";
                first++;
            }

            common_prefix_length = last - first;
            common_prefix_length = len_sub - common_prefix_length;
            if (common_prefix_length < threshold)
                return std::vector<int>();
            std::vector<int> starts{ common_prefix_length };
            for (int i = start; i <= end; i++)
            {
                //std::cout << SA[i] << " ";
                starts.push_back(length - 1 - SA[i] - common_prefix_length);
            }
            /* for (int i = 0; i < starts.size(); i++)
                 std::cout << starts[i] << " ";
             std::cout << "\n";*/
            return std::move(starts);
        }

        //外调入口，不动
        template<typename RandomAccessIterator>
        std::vector<triple> get_common_substrings_other(RandomAccessIterator first, RandomAccessIterator last, size_t threshold, bool tag_center) const
        {
            const auto align_start = std::chrono::high_resolution_clock::now(); //记录比对起始时间

            std::vector<triple> common_substrings; //三元组向量 用于返回
            const size_t rhs_len = last - first;

            int jindu = 0;
            for (size_t rhs_index = 0; rhs_index < rhs_len; )
            {
                auto found = search_for_prefix(first + rhs_index, last, threshold); //主调，寻找前缀,返回 【length，A.index0，A.index1...】
                if (found.empty())// || (tag_center && found.size() > 2))
                {
                    ++rhs_index;
                }
                else
                {
                    for (size_t i = 1; i != found.size(); ++i)
                        common_substrings.push_back(triple({ found[i], (int)rhs_index, found[0] })); //common_substrings增加
                    //if (tag_center)
                    //    rhs_index += found[0];// - threshold + 1;?????
                    //else
                    rhs_index += found[0];
                }
            }

            //std::cout << common_substrings.size() << "common_substrings  15\n";
            
            //exit(1);

            //if(tag_center)
           // std::cout << " finding " << (std::chrono::high_resolution_clock::now() - align_start) << '\n';
            //std::cout << "\nget_common_substrings:::end\n";
            return std::move(common_substrings);
        }

        template<typename RandomAccessIterator>
        std::vector<triple> get_common_substrings_back(RandomAccessIterator first, RandomAccessIterator last, size_t threshold, bool tag_center) const
        {
            const auto align_start = std::chrono::high_resolution_clock::now(); //记录比对起始时间

            std::vector<triple> common_substrings; //三元组向量 用于返回
            const size_t rhs_len = last - first;

            int jindu = 0;
            for (size_t rhs_index = 0; rhs_index < rhs_len; )
            {
                auto found = search_for_prefix(first + rhs_index, last, threshold); //主调，寻找前缀,返回 【length，A.index0，A.index1...】
                if (found.empty())// || (tag_center && found.size() > 2))
                {
                    rhs_index+=100;
                }
                else
                {
                    for (size_t i = 1; i != found.size(); ++i)
                        common_substrings.push_back(triple({ found[i], (int)rhs_index, found[0] })); //common_substrings增加
                    //if (tag_center)
                    //    rhs_index += found[0];// - threshold + 1;?????
                    //else
                    rhs_index += found[0];
                }
            }
            //std::cout << " finding back " << (std::chrono::high_resolution_clock::now() - align_start) << '\n';
            //std::cout << "\nget_common_substrings:::end\n";
            return std::move(common_substrings);
        }

        
        //main_maf
        template<typename InputIterator>
        std::vector<std::pair<int, int>> search_for_prefix_main_maf(InputIterator first, InputIterator last, size_t threshold) const
        {
            int thresh = threshold + 5;
            int common_prefix_length = 0, tmp_len = 1, cha;
            int lbegin, lend, start, end, len_sub = last - first, j;
            char sub = *(first++) + 1; //第0个
            start = begin[(int)sub - 1];
            end = begin[(int)sub] - 1;
            //std::cout << start << " " << end << " s-e?\n";
            std::unordered_map<int, int> pre;
            while (first < last)
            {
                if (tmp_len == thresh)
                {
                    //std::cout << (end - start + 1) << " all\n";
                    for (int i = start, j = 0; i <= end; i++, j++)
                    {
                        //std::cout << SA[i] << " ";
                        pre[length - 1 - SA[i] - tmp_len] = tmp_len;//start
                        //pre[j][1] = tmp_len;                     //len
                        //std::cout << pre[j][0] << " " << pre[j][1] << " pre\n";
                    }
                }

                sub = *(first)+1;
                lbegin = find(sub, start, end);
                lend = rfind(sub, start, end);
                if (lbegin == -1)
                {
                    if (tmp_len < thresh)
                        break;

                    for (int i = start; i <= end; i++)
                    {
                        pre[length - 1 - SA[i] - tmp_len] = tmp_len;
                    }

                    break;
                }

                if (tmp_len > thresh)
                {
                    for (int i = start; i <= end; i++)
                    {
                        pre[length - 1 - SA[i] - tmp_len] = tmp_len;
                    }
                }

                start = begin[(int)sub - 1] + O_index_num(lbegin, sub) - 1;
                end = begin[(int)sub - 1] + O_index_num(lend, sub) - 1;
                tmp_len++;
                first++;
            }

            common_prefix_length = last - first;
            common_prefix_length = len_sub - common_prefix_length;

            if (common_prefix_length < threshold)
                return  std::vector<std::pair<int, int>>();
            /*std::vector<int> starts((end - start + 1 + pre.size()) * 2);
            int id = 0;
            for (int i = start; i <= end; i++)
            {
                starts[id++] = common_prefix_length;//0len
                starts[id++] = length - 1 - SA[i] - common_prefix_length;//1start
            }*/
            std::vector<std::pair<int, int>> vec(pre.begin(), pre.end());
            pre.clear();
            // 按照值从da到xiao进行排序
            std::sort(vec.begin(), vec.end(), [](const auto& left, const auto& right) {
                return left.second > right.second;
                });

            //std::vector<std::pair<int, int>>().swap(vec);
            return std::move(vec);//start,len
        }

        template<typename RandomAccessIterator>
        std::vector<triple> get_common_substrings_main_maf(RandomAccessIterator first, RandomAccessIterator last, size_t threshold, bool tag_center) const
        {
            const auto align_start = std::chrono::high_resolution_clock::now();
            std::vector<triple> common_substrings; //三元组向量 用于返回
            //common_substrings.reserve(200000);
            std::vector<triple> all_common_substrings;
            //all_common_substrings.reserve(18000000);
            std::set<std::pair<int, int>> all_Set;
            int old_size, new_size;
            const size_t rhs_len = last - first;

            int jindu = 0;
            int max_l = 0;
            int pre_end = -1;
            int z, t;
            for (size_t rhs_index = 0; rhs_index < rhs_len; )
            {
                auto found = search_for_prefix_main_maf(first + rhs_index, last, threshold); //主调，寻找前缀,返回 【length，A.index0，A.index1...】
                if (found.empty())// || (tag_center && found.size() > 2))
                {
                    ++rhs_index;
                }
                else//start0,len0; start1,len1,,,,,
                {
                    max_l = found[0].second;
                    z = all_common_substrings.size();
                    t = common_substrings.size();
                    for (size_t i = 0; i < found.size(); i++)
                    { 
                        if (found[i].second >= threshold && found[i].second == max_l && (int)rhs_index >= pre_end)
                            common_substrings.push_back(triple({ found[i].first, (int)rhs_index, found[i].second })); //common_substrings增加
                        
                        old_size = all_Set.size();
                        all_Set.insert({ found[i].first + found[i].second ,(int)rhs_index + found[i].second });
                        if(all_Set.size()>old_size)
                            all_common_substrings.push_back(triple({ found[i].first, (int)rhs_index, found[i].second }));//全要 A.start, B.start,len
                    }
                    if(common_substrings.size()>0)
                        pre_end = common_substrings.back()[1] + common_substrings.back()[2];
                   rhs_index += max_l;
                    //if (max_l > 200)
                     //  rhs_index += max_l;
                    //else
                     //  rhs_index += max_l - threshold + 1;

                }
            }
            std::cout << common_substrings.size() << "common_substrings\n";
            std::cout << all_common_substrings.size() << "all_common_substrings\n";
            std::cout << "finding consumes " << (std::chrono::high_resolution_clock::now() - align_start) << '\n';
            //std::string filename;
            std::set<std::pair<int, int>>().swap(all_Set);
            
            std::ofstream out_file(arguments::out_file_name + "/substrings0.txt", std::ios::binary | std::ios::app);
            out_file << common_substrings.size() << "common_substrings\n";
            for (int i = 0; i < common_substrings.size(); i++)
                out_file << common_substrings[i][0] << " " << common_substrings[i][1] << " " << common_substrings[i][2] << "\n";
            out_file << "\n";
           
            out_file << all_common_substrings.size() << "all_common_substrings\n";
            for (int i = 0; i < all_common_substrings.size(); i++)
                out_file << all_common_substrings[i][0] << " " << all_common_substrings[i][1] << " " << all_common_substrings[i][2] << "\n";
            out_file << "\n";
            // 关闭输出文件
            out_file.close();
            
            return std::move(common_substrings);
        }


        template<typename RandomAccessIterator>
        std::vector<triple> get_common_substrings(RandomAccessIterator first, RandomAccessIterator last, size_t threshold, bool tag_center) const
        {
            return get_common_substrings_other(first, last, threshold, tag_center); //不寻找相近字
        }

        int find(char now, int start, int end) const//从左到右查
        {
            for (int i = start; i <= end; i++)
                if (B[i] == now)
                    return i;
            return -1;
        }

        int rfind(char now, int start, int end) const//从右到左查
        {
            for (int i = end; i >= start; i--)
                if (B[i] == now)
                    return i;
            return -1;
        }

        int O_index_num(int x, char now) const//查找 第几个A
        {
            int num, i, quotient = x / dis;
            if (((x - quotient * dis) <= (dis / 2)) || (quotient == (Osize - 1))) //从quotient向后查
            {
                num = O[quotient][(int32_t)now - 1];
                for (i = quotient * dis + 1; i <= x; i++)
                    if (B[i] == now)
                        num++;
            }
            else             //从quotient+1向前查
            {
                num = O[quotient + 1][(int32_t)now - 1];
                for (i = (quotient + 1) * dis; i > x; i--)
                    if (B[i] == now)
                        num--;
            }
            return num;
        }


    private:
        //初始化 A逆- reword
        unsigned char* _copy_reword(utils::seq_NCBI2NA& sequence, int first, size_t last, const unsigned char end_mark)
        {
            unsigned char* result = new unsigned char[length];
            int i = length - 2;
            int j = first;
            while (i >= 0)
            {
                result[i] = NCBI2NA_UNPACK_BASE(sequence.seq[j / 4], (3 - j % 4)) + 1;
                i--;
                j++;
            }
            result[length - 1] = end_mark;
            return result;
        }
        
        unsigned char* vec_copy_reword(const std::vector<unsigned char>& sequence, int first, size_t last, const unsigned char end_mark)
        {
            unsigned char* result = new unsigned char[length];
            int i = length - 2;
            int j = first;
            while (i >= 0)
            {
                result[i] = sequence[j] + 1;
                
                i--;
                j++;
            }
            result[length - 1] = end_mark;
            return result;
        }

        
        //初始化倍增排序后求得 后缀数组SA
        /*
        int* build_sa()
        {
            //m 是编号的最大值
            int* sa = new int[length];
            int* t = new int[length];
            int* t2 = new int[length];
            int* c = new int[length];
            int n = length, m = 5;
            int i, * x = t, * y = t2;
            for (i = 0; i < m; ++i) c[i] = 0;
            for (i = 0; i < n; ++i) c[x[i] = (int)reword[i]] ++;
            for (i = 1; i < m; ++i) c[i] += c[i - 1];
            for (i = n - 1; i >= 0; i--) sa[--c[x[i]]] = i;
            for (int k = 1; k <= n; k <<= 1) {
                int p = 0;
                for (i = n - k; i < n; i++) y[p++] = i;
                for (i = 0; i < n; i++) if (sa[i] >= k)
                    y[p++] = sa[i] - k;
                for (i = 0; i < m; ++i) c[i] = 0;
                for (i = 0; i < n; ++i) c[x[y[i]]] ++;
                for (i = 1; i < m; ++i) c[i] += c[i - 1];
                for (i = n - 1; i >= 0; i--)
                    sa[--c[x[y[i]]]] = y[i];
                std::swap(x, y);
                p = 1; x[sa[0]] = 0;
                for (i = 1; i < n; ++i)
                    x[sa[i]] = y[sa[i - 1]] == y[sa[i]] && y[sa[i - 1] + k] == y[sa[i] + k] ? p - 1 : p++;
                if (p >= n) break;
                m = p;
            }

            delete [] x;
            delete [] y;
            delete [] c;
            return sa;
        }
*/

        inline bool leq(int a1, int a2, int b1, int b2) { // lexic. orderfor pairs
            return(a1 < b1 || a1 == b1 && a2 <= b2);
        }                                                  // and triples
        inline bool leq(int a1, int a2, int a3, int b1, int b2, int b3) {
            return(a1 < b1 || a1 == b1 && leq(a2, a3, b2, b3));
        }
        // stably sort a[0..n-1] to b[0..n-1] with keys in 0..K fromr
        static void radixPass(int* a, int* b, int* r, int n, int K)
        {// count occurrences
            int* c = new int[K + 1];                          // counter array
            for (int i = 0; i <= K; i++) c[i] = 0;         // resetcounters
            for (int i = 0; i < n; i++) c[r[a[i]]]++;    // countoccurences
            for (int i = 0, sum = 0; i <= K; i++) { // exclusive prefix sums
                int t = c[i];  c[i] = sum;  sum += t;
            }
            for (int i = 0; i < n; i++) b[c[r[a[i]]]++] = a[i];      //sort
            delete[] c;
        }
        // find the suffix array SA of s[0..n-1] in {1..K}^n
        // require s[n]=s[n+1]=s[n+2]=0, n>=2
        void suffixArray(int* s, int* SA, int n, int K) {
            int n0 = (n + 2) / 3, n1 = (n + 1) / 3, n2 = n / 3, n02 = n0 + n2;
            //n0是字符串中模为的下标的个数，n1，n2依此类推
            int* s12 = new int[n02 + 3]; s12[n02] = s12[n02 + 1] = s12[n02 + 2] = 0;
            int* SA12 = new int[n02 + 3]; SA12[n02] = SA12[n02 + 1] = SA12[n02 + 2] = 0;
            int* s0 = new int[n0];
            int* SA0 = new int[n0];

            // generatepositions of mod 1 and mod  2 suffixes
            // the"+(n0-n1)" adds a dummy mod 1 suffix if n%3 == 1
            for (int i = 0, j = 0; i < n + (n0 - n1); i++) if (i % 3 != 0) s12[j++] = i;
            //将所有模不为的下标存入s12中

            // lsb radix sortthe mod 1 and mod 2 triples
            radixPass(s12, SA12, s + 2, n02, K);
            radixPass(SA12, s12, s + 1, n02, K);
            radixPass(s12, SA12, s, n02, K);
            //radixPass实际是一个计数排序
            //对后缀的前三个字符进行三次计数排序完成了对SA12数组的基数排序
            //这个排序是初步的，没有将SA12数组真正地排好序，因为：
            //若SA12数组中几个后缀的前三个字符相等，则起始位置靠后的排在后面

            // findlexicographic names of triples
            int name = 0, c0 = -1, c1 = -1, c2 = -1;
            for (int i = 0; i < n02; i++) {
                if (s[SA12[i]] != c0 || s[SA12[i] + 1] != c1 || s[SA12[i] + 2] != c2) {
                    name++; c0 = s[SA12[i]];  c1 = s[SA12[i] + 1];  c2 = s[SA12[i] + 2];
                }
                //name是计算后缀数组SA12中前三个字符不完全相同的后缀个数
                //这么判断的原因是：SA12有序，故只有相邻后缀的前三个字符才可能相同
                if (SA12[i] % 3 == 1) { s12[SA12[i] / 3] = name; }// left half
                else { s12[SA12[i] / 3 + n0] = name; } // right half
                 //SA12[i]模不是就是，s12保存的是后缀数组SA12中前三个字符的排位
            }

            // recurse if namesare not yet unique
            if (name < n02) {
                //如果name等于n02，意味着SA12前三个字母均不相等，即SA12已有序
                //否则，根据s12的后缀数组与SA12等价，对s12的后缀数组进行排序即可
                suffixArray(s12, SA12, n02, name);
                // store uniquenames in s12 using the suffix array
                for (int i = 0; i < n02; i++) s12[SA12[i]] = i + 1;
            }
            else // generate the suffix array of s12 directly
                for (int i = 0; i < n02; i++) SA12[s12[i] - 1] = i;
            //s12保存的是后缀数组SA12中前三个字符的排位
            //在所有后缀前三个字符都不一样的情况下，s12就是后缀的排位

            //至此SA12排序完毕
            //SA12[i]是第i小的后缀的序号(序号从到n02)，s12[i]是序号为i的后缀的排位
            //使用后缀序号而不是实际位置的原因是递归调用suffixArray时不能保留该信息

            // stably sort themod 0 suffixes from SA12 by their first character
            for (int i = 0, j = 0; i < n02; i++) if (SA12[i] < n0) s0[j++] = 3 * SA12[i];
            //将SA12中所有的模为的后缀的实际位置减去按序存储在s0中
            //注意后缀序号到实际位置的转化需将前者乘
            //这意味着首先已经利用模为的后缀对SA0进行了初步排序
            //只需要采用一次计数排序即可对SA0完成基数排序的最后一步
            radixPass(s0, SA0, s, n0, K);

            //最后一步，对有序表SA12和SA0进行归并
            // merge sorted SA0suffixes and sorted SA12 suffixes
            for (int p = 0, t = n0 - n1, k = 0; k < n; k++) {
#define GetI() (SA12[t] < n0 ? SA12[t] * 3 + 1 : (SA12[t] - n0) *3 + 2)
                int i = GetI(); // pos of current offset 12 suffix
                int j = SA0[p]; // pos of current offset 0  suffix
                if (SA12[t] < n0 ?
                    leq(s[i], s12[SA12[t] + n0], s[j], s12[j / 3]) :
                    leq(s[i], s[i + 1], s12[SA12[t] - n0 + 1], s[j], s[j + 1], s12[j / 3 + n0]))
                { // suffix fromSA12 is smaller
                    SA[k] = i;  t++;
                    if (t == n02) { // done --- only SA0 suffixes left
                        for (k++; p < n0; p++, k++) SA[k] = SA0[p];
                    }
                }
                else {
                    SA[k] = j;  p++;
                    if (p == n0) { // done--- only SA12 suffixes left
                        for (k++; t < n02; t++, k++) SA[k] = GetI();
                    }
                }
            }
            delete[]s12; delete[] SA12; delete[] SA0; delete[] s0;
        }

        int* build_sa()
        {
            int* s = new int[length + 3];
            int* sa = new int[length + 3];
            for (int i = 0; i < length; i++) s[i] = (int)reword[i];
            s[length] = s[length + 1] = s[length + 2] = sa[length] = sa[length + 1] = sa[length + 2] = 0;
            suffixArray(s, sa, length, 5);
            delete[] s;
            return sa;
        }
        //初始化，最后一列B，ACGT在F列的断点begin，#在B中的位置，对B隔dis段计数O
        int build_b()
        {
            quadra* o = new quadra[length / dis + 2]();
            Osize = length / dis + 2;
            int* _begin = new int[5];
            _begin[4] = length;
            unsigned char* b = new unsigned char[length];
            quadra num = { 0,0,0,0 };
            int  i, e = 0;
            for (i = 0; i < length; i++)
            {
                b[i] = reword[(SA[i] + length - 1) % length];
                if (((int)b[i]) == 0)
                    e = i;
                else
                    num[(int)b[i] - 1]++;
                if (i % dis == 0)
                    o[i / dis] = num;
            }
            _begin[0] = 1;
            _begin[1] = num[0] + 1;
            _begin[2] = num[0] + num[1] + 1;
            _begin[3] = num[0] + num[1] + num[2] + 1;
            B = b;  //初始化B
            begin = _begin; //初始化begin断点
            O = o;
            return e; //初始化 end #位置
        }

    private:
        using quadra = std::array<int32_t, 4>;
        const int dis;
        int endp;
        int Osize;
    public:
        const size_t length;                        // including the ending '$'
        const unsigned char* reword;          // 原串逆+'$'
        int32_t* SA;                              //后缀数组
        const unsigned char* B;               //L列
        const quadra* O;
        const int* begin;
    };
}
