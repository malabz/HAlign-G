#if defined(_WIN32)
#endif

#include "Utils/CommandLine.hpp"
#include "Utils/mum_main.hpp"
#include <tuple>
#include <fstream>
#include <filesystem>
#include <string>

using namespace std;

std::map<unsigned char, unsigned char> dick = { {'A', '\1'}, {'C', '\2'}, {'G', '\3'}, {'T', '\4'}, {'U', '\4'}, {'N', '\5'}, {'-', '\7'} };
int main(int argc, char* argv[])
{
    // 提取参数
    char* path = argv[1];
    std::string path_str(path);
    int filenum = std::stoi(argv[2]);
    int chr = std::stoi(argv[3]);
    int chunk = std::stoi(argv[4]);
    init_scores70();
    my_mul_main(path, filenum, chr, chunk);

    return 0;
}
