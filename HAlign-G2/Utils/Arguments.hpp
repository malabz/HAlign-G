#pragma once
//输入输出路径，参数
#include <string>
#include <vector>
namespace arguments
{
    extern std::string in_file_name;
    extern std::string out_file_name;
    extern std::string score_file;
    extern int seq_num;
    extern int File_i;
    extern int que_len;
    extern bool output_matrix;
    extern std::vector<unsigned char*> que;
    extern int q_start, q_end, q_head, fasta_len;
    extern bool tag;
}