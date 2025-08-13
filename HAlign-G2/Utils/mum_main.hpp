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
#include <unistd.h>
#include <sys/wait.h>

#include "Arguments.hpp"
#include "Pseudo.hpp"
#include "Insertion.hpp"
#include "sequence.h"
#include "Utils.hpp"

#include "../SuffixArray/SuffixArray.hpp"  //���ú�׺��
#include "../PairwiseAlignment/NeedlemanWunshReusable.hpp"


struct Mum_struct {
    size_t start1;
    size_t end1;
    size_t start2;
    size_t end2;
    int tag;
    std::vector <long> string;
};
struct Mum_struct_insert {
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

std::vector<std::vector<utils::Insertion>> mum_main(std::vector<std::string>& name, std::vector<size_t>& Length, std::vector<size_t>& non_Length, std::vector<bool>& TU, std::vector<bool>& sign, bool FAorMA, size_t threshold2, size_t inThreads, int filter_level);
void mum_main_psa(int I, const std::unique_ptr<Sequence>& seqA, std::vector<std::array<std::vector<utils::Insertion>, 2>>& all_pairwise_gaps, std::vector<std::string>& name, std::vector<size_t>& Length, std::vector<bool>& TU, std::vector<bool>& sign, bool FAorMA, size_t threshold2, size_t inThreads, int filter_level);
bool read_delta_get_struct(std::vector<Mum_struct>& Structs, std::string delta_filename, std::string seq_file, size_t lengthA, size_t lengthB, bool TU);
void get_inserts(std::vector<utils::Insertion>& inserts_1, std::vector<utils::Insertion>& inserts_2, std::vector <long>& numbers, size_t start1, size_t start2);
void max_weight_increasing_subsequence(std::vector<Mum_struct>& struct_list);
void cut_and_get_inserts(std::vector<Mum_struct_insert>& new_SV, std::vector<utils::Insertion>& Ainsert, std::vector<utils::Insertion>& Binsert,
    size_t& new_end1, size_t& new_end2, std::vector<long>& numbers, size_t start1, size_t start2,
    size_t end1, size_t end2, size_t next_start1, size_t next_start2);
void nibu(std::string& str, bool TU);
void align_delta_sv(std::string& aligned_reference, std::string& aligned_query, std::vector<long>& numbers);
void BinaryMUM(int I, bool FAorMA);