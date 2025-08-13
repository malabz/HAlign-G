#pragma once
//读入，写回，预处理，后处理，统一化

#include "Pseudo.hpp"
#include "Insertion.hpp"
#include "sequence.h"
#include <cctype>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <sstream> // 用于 stringstream
#include <chrono>
#include <array>
#include <iostream>
#include <map>
#include <iterator>
#include <memory>
#pragma once
#include "../multi-thread/multi.hpp"
extern "C"
{
#include "../multiz/multiz.h"
}
#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#include <process.h>
#include <io.h>
inline void EmptySet()
{
    EmptyWorkingSet(GetCurrentProcess());
}
void getFiles_win(std::string path, std::vector<std::string>& files);
#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/types.h>
#include <dirent.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/resource.h>
#include <pthread.h>
inline void EmptySet()
{
    malloc_trim(0);
}
void getFiles_linux(std::string path, std::vector<std::string>& filenames);
#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>
#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>
#endif
#else
#error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif

#undef max
#undef min
#define mMAX(x,y) ((x) > (y) ? (x) : (y))
#define mMIN(x,y) ((x) < (y) ? (x) : (y))

size_t getPeakRSS();

bool deleteFile(std::string path);
bool deleteDirectory(std::string path);
void renameAndMoveFile(const std::string& filePath);
void renameFile(const std::string& oldPath, const std::string& newPath);

//测试内存峰值
inline void GetMemoryUsage()
{
    int mem = getPeakRSS() / 1024.0 / 1024.0;
    std::cout << "****process mem****" << std::endl;
    std::cout << "current pid: " << getpid() << std::endl;
    std::cout << "memory usage: " << mem << "MB" << std::endl;
}

#define NCBI2NA_UNPACK_BASE(x, N) (((x)>>(2*(N))) & 3)
#define UCHAR unsigned char

namespace utils
{
    struct maf
    {
        float score;
        int seq_num;
        int seq_len;
        std::vector<std::string> seqs;
    };
    struct MAF_info
    {
        std::string path;
        int thresh1; //长度100
        int thresh2; //条数1
        int thresh3; //分数95
    };
    struct block
    {
        int name;
        size_t start;
        size_t length;
        //unsigned char* seqi;
        //std::string sign;
        //size_t all_length;
        std::vector<unsigned char> seqi;
    };
    struct PSA_ni_block
    {
        size_t start[2];
        size_t end[2];
        size_t length[2];
        bool sign;
        std::vector<unsigned char> a_seq;
        std::vector<unsigned char> b_seq;
        PSA_ni_block* next = NULL;
    };
    struct maf_2_block
    {
        size_t start1;
        size_t end1;
        size_t start2;
        size_t end2;
        bool sign;
    };
    struct maf_sv_block
    {
        size_t start1;
        size_t end1;
        size_t start2;
        size_t end2;
        bool sign;
        int score;
    };
    struct MSA_ni_block
    {
        size_t start[2];
        size_t end[2];
        MSA_ni_block* next = NULL;
    };
    struct in_block
    {
        float score;
        float score_100;
        size_t start;
        size_t end;
        std::vector<size_t> name;
        std::vector<size_t> length;
        std::vector<size_t> si;
        in_block* next = NULL;
    };
    struct m_block
    {
        size_t start1;
        size_t end1;
        size_t start2;
        size_t end2;
        bool tag = 0;//1-全要
        std::vector<std::tuple<int, int>> gap1;
        std::vector<std::tuple<int, int>> gap2;
    };
    using more_block = std::vector<m_block>;
    struct MAF_block
    {
        float score;
        int tag_num;  //记录有tag的列数，连续性
        std::vector<block> seq;
    };

    struct seq_NCBI2NA
    {
        size_t length = 0;
        unsigned char* seq = NULL;
    };
    std::vector<unsigned char> bin_to_vector(utils::seq_NCBI2NA& Seq, int start, int end);
    std::vector<unsigned char> maf_bin_to_vector(utils::seq_NCBI2NA& Seq);
    inline UCHAR get_char(seq_NCBI2NA& Seq, int i);
    void push_char(seq_NCBI2NA& Seq, UCHAR c);
    UCHAR* BLAST_PackDNA(std::string buffer);
    UCHAR* BLAST_PackDNA(std::vector<unsigned char>& seq);
    UCHAR* BLAST_PackDNA(std::string buffer, UCHAR* new_buffer);
    UCHAR* copy_DNA(UCHAR* buffer, int start, int end);
    UCHAR* copy_DNA_ni(UCHAR* buffer, int start, int end);  //先按正串索引取正串，再逆补。注意传入时需要把逆串索引转为正串索引
    UCHAR* copy_DNA(UCHAR* buffer, UCHAR* new_buffer, int start, int end);
    void DNA_cout(seq_NCBI2NA& Seq);

    unsigned int* insert_fasta01(std::array<std::vector<utils::Insertion>, 2>& insertions, size_t L0);
    int* Compare_two(std::string& s1, std::string& s2, int d = 0, int e = 0, int m = 1, int mis = 0);
    int get_next_maf(std::ifstream& mfs, struct maf_2_block& mafi, int d = 0, int e = 0, int m = 1, int mis = 0);
    int get_next_sv_maf(std::ifstream& mfs, struct maf_sv_block& mafi, std::string sign_i, int d = 0, int e = 0, int m = 1, int mis = 0);
    int* maf_Compare_two(std::ifstream& mfs, std::ifstream& mfsv, std::string& s1, std::string& s2, bool signi, int d = 0, int e = 0, int m = 1, int mis = 0);
    std::string remove_white_spaces(const std::string& str); //去掉空格

    unsigned char to_pseudo(unsigned char c); //预处理，char->int
    void to_pseudo(std::string& str, std::vector<unsigned char>& seq);
    void to_pseudo(std::string& str);
    std::string from_pseudo(const std::vector<unsigned char>& pseu);
    

    template<typename InputIterator, typename OutputIterator>
    void transform_to_pseudo(InputIterator src_first, InputIterator src_last, OutputIterator des)   //预处理，char->int
    {
        std::vector<unsigned char>(*op)(const std::string&) = &to_pseudo;
        std::transform(src_first, src_last, des, op);
    }

    template<typename InputIterator, typename OutputIterator>
    void transform_from_pseudo(InputIterator src_first, InputIterator src_last, OutputIterator des) //后处理，int->char
    {
        std::string(*op)(const std::vector<unsigned char> &) = &from_pseudo;
        std::transform(src_first, src_last, des, op);
    }

    template<typename InputIterator>
    InputIterator iter_of_max(InputIterator first, InputIterator last) //找到最大的元素
    {
        auto result = first;

        for (; first != last; ++first) if (*result < *first) result = first;
        return result;
    }

    void read_to_pseudo(std::istream& is, std::ofstream& nf, std::vector<std::string>& name, std::vector<size_t>& Length, std::vector<bool>& TU);
    void read_to_pseudo(std::istream& is, std::vector<std::string>& name, std::vector<size_t>& Length, std::vector<size_t>& non_Length, std::vector<bool>& TU);


    void get_BIN_seq_from_noN(int id, std::string& cur_line);
    void read_BYTE_seq(std::fstream& io, std::vector<unsigned char>& seq);
    void write_BYTE_seq(std::fstream& io, std::vector<unsigned char>& seq, int seek, bool TU);

    PSA_ni_block* insertion_gap_out(std::ostream& os, int seqi, std::vector<std::vector<unsigned char>>& sequences, std::vector<std::string>& name, std::vector<bool>& sign, utils::m_block& more_block, int nsum1, int nsum2, std::vector<Insertion>& N_insertion1, std::vector<Insertion>& N_insertion2, PSA_ni_block* PSA_head, int thresh1);
    PSA_ni_block* insertion_gap_more(std::ostream& os, std::vector<std::vector<unsigned char>>& sequences, const std::vector<std::vector<Insertion>>& N_insertions, std::vector<std::string>& name, std::vector<bool>& sign, std::vector<utils::more_block>& more_gap, int thresh1);
    void insertion_gap_out_new(std::ostream& os, int seqi, std::vector<std::vector<unsigned char>>& sequences, std::vector<std::string>& name, std::vector<bool>& sign, std::vector<bool>& TU, utils::m_block& more_block, int* final_sequences, int nsum1, int nsum2, std::vector<Insertion>& N_insertion1, std::vector<Insertion>& N_insertion2, int thresh1);
    void insertion_gap_out_new_sv(std::ostream& os, std::vector<unsigned char>& A, std::vector<unsigned char>& B, std::vector<std::string>& name,
        std::vector<bool>& sign, std::vector<bool>& TU, utils::m_block& more_block, int* final_sequences, int nsum1, int nsum2, std::vector<Insertion>& N_insertion1, std::vector<Insertion>& N_insertion2);
    void insertion_gap_out_new_mum(std::vector<unsigned char>& A, std::vector<unsigned char>& B, std::tuple<std::vector<std::tuple<int, int>>, std::vector<std::tuple<int, int>>>& tmp_insert, std::vector<Insertion>& N_insertion1, std::vector<Insertion>& N_insertion2, int nsum1, int nsum2);
    void insertion_gap_more_new(std::ostream& os, std::vector<std::vector<unsigned char>>& sequences, std::vector<std::vector<Insertion>>& N_insertions, std::vector<std::string>& name, std::vector<bool>& sign, std::vector<bool>& TU, std::vector<utils::more_block>& more_gap, int thresh1, unsigned char* array_A, unsigned char* array_B);
    size_t mum_insert_and_write_fasta(std::ostream& os, std::vector<std::vector<Insertion>>& insertions, std::vector<std::string>& name, std::vector<bool>& sign, std::vector<size_t>& Length, bool mg_tag);

}
void cout_cur_time();
int my_mk_dir(std::string output_dir);
bool AB_exist(unsigned char* array_A, unsigned char* array_B, size_t start_A, size_t end_A, size_t start_B, size_t end_B);
bool AB_exist(unsigned int* Fasta_Center, unsigned char* array_A, unsigned char* array_B, size_t start_A, size_t end_A, size_t start_B, size_t end_B);
bool AB_exist_ni(unsigned char* array_A, unsigned char* array_B, size_t start_A, size_t end_A, size_t start_B, size_t end_B);
struct TupleCompare {
    bool operator()(const std::tuple<int, int, int>& lhs, const std::tuple<int, int, int>& rhs) const;
};
namespace Stream
{
    struct maf_two
    {
        size_t start;
        size_t end;
    };
    struct maf_three
    {
        size_t imaf;
        size_t start;
        size_t end;
    };
    struct maf_four
    {
        size_t imaf;
        size_t start;
        size_t end;
        size_t score;
    };
    struct CompareStart {
        bool operator()(const maf_four& a, const maf_four& b) const {
            return a.start < b.start;
        }
    };
    void main_maf(int num, std::vector<std::string>& name, std::vector<size_t>& Length, std::vector<bool>& sign, utils::MAF_info& MAFinfo);
    //void main_maf1(int num, std::vector<std::string>& name, std::vector<size_t>& Length, std::vector<bool>& sign, utils::MAF_info& MAFinfo);

    void main_maf2(int num, std::vector<std::string>& name, std::vector<size_t>& Length, std::vector<bool>& sign, utils::MAF_info& MAFinfo);
    void star_hebing(char* str_fa, char* str_maf, std::vector<maf_two>& insert_fa, std::vector<maf_two>& insert_maf);
    void sort_hebing(std::string fileName, std::string ref_name);
    void sort_hebing(std::string sourceName, std::string fileName, std::string ref_name);
    void sort_hebing2(struct mafAli* cp_list, std::string fileName, std::string ref_name);
    void cut_maf(struct mafAli* cp_list, std::string file_name, std::string ref_name);
    void cut_maf1000(std::string infile_name, FILE* fpw1, size_t max_lenght = 1000);
    void get_strand(std::vector<std::vector<char>>& strand, std::vector<std::string>& file_name, std::vector<std::vector<std::string>>& Name_All, std::string infile_name);
    struct mafComp* sortAndReconnect(std::map<std::tuple<int, int, int>, struct mafComp*, TupleCompare>& mapping);
    void sort_maf1000(std::vector<std::vector<char>>& strand, std::vector<std::string>& file_name, std::vector<std::vector<std::string>>& Name_All, std::string infile_name, FILE* fpw1, size_t max_lenght=1000);
    void sort_hebing_plus(std::string fileName, std::string ref_name);
    void filter_100_maf(std::vector<std::unique_ptr<Sequence>>& sequence, std::vector<std::string>& name, std::vector<size_t>& Length, std::vector<bool>& sign, utils::MAF_info& MAFinfo);
    utils::in_block* qiansu(std::vector<std::unique_ptr<Sequence>>& sequences, utils::in_block* pre, utils::in_block* now, bool* gap_tag, int* gap_numi, int thresh);
    void housu(int end, std::vector<std::unique_ptr<Sequence>>& sequences, utils::in_block* now, bool* gap_tag, int* gap_numi, int thresh);
    utils::MSA_ni_block* read_ni_maf(int& seqi, std::vector<std::string>& name, std::string path);
    utils::in_block* my_memcpy(utils::in_block* now);
}
void reverseString(char* str);
size_t maf_i_score(char* ref, char* newString);
template<typename Representation, typename Period>
std::ostream& operator<<(std::ostream& os, std::chrono::duration<Representation, Period> duration) //时间消耗
{
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << "ms";
    return os;
}
void processFastaFile(int index, const std::string& filename, std::vector<size_t>& sequence_lengths, std::vector<std::string>& Name_All);
void processFastaFile(int index, const std::string& filename, std::vector<size_t>& sequence_lengths, std::vector<std::string>& Name_All);
void writeSortedChunk(struct mafAli* head, const char* filename);
void sortAndSaveChunks(char* fileName, const char* dirName, size_t chunkSize, int* chunkCount);
void mergeChunks(int chunkCount, const char* dirName, char* outputFile);
void sort_maf_file(char* filepath, int chunkSize = 1000000);