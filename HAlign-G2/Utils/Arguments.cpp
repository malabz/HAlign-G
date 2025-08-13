#include "Arguments.hpp"
////输入输出路径，参数
std::string arguments::in_file_name;
std::string arguments::out_file_name;
std::string arguments::score_file;
int arguments::seq_num = 0;
int arguments::File_i = 0;
int arguments::que_len = 1000000;
std::vector<unsigned char*> arguments::que;
int arguments::q_start = 0;
int arguments::q_end = 0;
int arguments::q_head = 0;
int arguments::fasta_len = 0;
//std::string arguments::in_file_name ;
//std::string arguments::out_file_name ;
bool arguments::output_matrix;
bool arguments::tag;