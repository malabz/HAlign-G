#pragma once
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <chrono>
#include <array>
#include <iostream>
#include <map>
#include <iterator>
#include <memory>
#include <vector>
#include <array>
#include <string>
#include<algorithm>
#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <limits.h>
#include <unordered_map>
#include <list>

#include <unistd.h>
#include <sys/wait.h>

#include "Arguments.hpp"
#include "Pseudo.hpp"
#include "Insertion.hpp"
#include "sequence.h"
#include "Utils.hpp"

#include "../SuffixArray/SuffixArray.hpp"  //利用后缀树
#include "../PairwiseAlignment/NeedlemanWunshReusable.hpp"


struct Mum_struct {
    size_t file1;
    size_t file2;
    size_t start1;
    size_t end1;
    size_t start2;
    size_t end2;
    int tag;
    std::vector <long> string;
};
struct Mum_struct_insert {
    size_t file1;
    size_t file2;
    size_t start1;
    size_t end1;
    size_t start2;
    size_t end2;
    int tag;
    std::vector <utils::Insertion> insertA;
    std::vector <utils::Insertion> insertB;
};
using mquadra = std::array<int, 5>;
using quadras = std::vector<std::array<int, 5>>;

void mum_main(std::vector<std::string>& files, std::vector<std::vector<size_t>>& Length_All, std::vector<std::vector<std::string>>& Name_All, int inThreads, bool tag_load);
void mum_main_psa(int I, std::vector<std::string>& files, std::vector<std::vector<size_t>>& Length_All, std::vector<std::vector<std::string>>& Name_All, int inThreads, bool tag_load);
void read_delta_get_struct(std::string delta_filename, std::vector<std::vector<Mum_struct>>& Final_Structs, std::vector<std::string>& NameA, std::vector<std::string>& NameB);
void get_inserts(std::vector<utils::Insertion>& inserts_1, std::vector<utils::Insertion>& inserts_2, std::vector <long>& numbers, size_t start1, size_t start2);
void max_weight_increasing_subsequence(std::vector<Mum_struct>& struct_list);

std::string getFileName(const std::string& filePath);
void nibu(std::string& str);
void align_delta_sv(std::string& aligned_reference, std::string& aligned_query, std::vector<long>& numbers);
void BinaryMUM(int I, std::vector<std::string>& files, int inThreads, bool FAorMA = true);
void BinaryMUMSave(int I, std::vector<std::string>& files, int inThreads, bool FAorMA = true);
void BinaryMUMLoad(int I, std::vector<std::string>& files, int inThreads, bool FAorMA = true);
int findLongestPrefixIndex( std::string& tmpline1,  std::vector<std::string>& NameA);