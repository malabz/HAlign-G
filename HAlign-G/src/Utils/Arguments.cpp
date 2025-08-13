#include "Arguments.hpp"
////输入输出路径，参数
//std::string arguments::in_file_name = "/home/zqzhoutong/stmsa-BWT/data/people13.fasta";
//std::string arguments::out_file_name = "/home/zqzhoutong/stmsa-BWT/data/test/people13ed.fasta";
std::string arguments::in_file_name;
std::string arguments::out_file_name;
std::string arguments::N_file_name;
std::string arguments::score_file;
std::string arguments::snp_file_name;
int arguments::seq_num = 0;
int arguments::File_i = 0;
size_t arguments::sv_thresh_len = 50;
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