#include <tuple>
#include <fstream>
#include <filesystem>
#include <string>
#include <iostream>
#include "sequence.hpp"
using namespace std;

int main()
{
	auto chr1 = std::make_unique<Sequence>("C:\\Users\\13508\\Desktop\\1.fasta");
	size_t x = 633471;
	for(size_t i = x - 100; i <= x + 100; ++ i) std::cout << (*chr1)[i];
	std::cout << std::endl;
	return 0;
}