extern "C"
{
#include "multiz/multiz.h"
#include "multiz/maf.h"
}

#include <iostream>
#include <vector>
#include <queue>
#include <iostream>
#include <iostream>
#include <cstring>
#include <stdlib.h>
#include<iostream>
#include<sstream>

using namespace std;

int main(int argc, char* argv[]) {
    char file_name1[1000];
    char file_name2[1000];
    char seq_name1[1000];
    char seq_name2[1000];
    int MAFthresh1;
    int MAFthresh2;
    int MAFthresh3;
    if (argc == 8) {
        std::cout << "Usage: ./program file_name1 file_name2 seq_name1 seq_name2  MAFthresh1 MAFthresh2 MAFthresh3\n";
        strncpy(file_name1, argv[1], sizeof(file_name1));
        strncpy(file_name2, argv[2], sizeof(file_name2));
        strncpy(seq_name1, argv[3], sizeof(seq_name1));
        strncpy(seq_name2, argv[4], sizeof(seq_name2));
        MAFthresh1 = std::stoi(argv[5]);
        MAFthresh2 = std::stoi(argv[6]);
        MAFthresh3 = std::stoi(argv[7]);
        mafRead_filter_writeAll2(file_name1, file_name2, seq_name1, seq_name2, MAFthresh1, MAFthresh2, MAFthresh3);
    }

    if (argc == 6) {
        std::cout << "Usage: ./program file_name1 file_name2  MAFthresh1 MAFthresh2 MAFthresh3\n";
        strncpy(file_name1, argv[1], sizeof(file_name1));
        strncpy(file_name2, argv[2], sizeof(file_name2));
        
        MAFthresh1 = std::stoi(argv[3]);
        MAFthresh2 = std::stoi(argv[4]);
        MAFthresh3 = std::stoi(argv[5]);
        mafRead_filter_writeAll(file_name1, file_name2, MAFthresh1, MAFthresh2, MAFthresh3);
    }


    

    return 0;
}