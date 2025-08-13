#include <iostream>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

struct Sequence {
    std::string name;
    std::string sequence;
};

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cout << "Usage: program_name inputfile ref outfile" << std::endl;
        return 1;
    }

    std::string inputsfile = argv[1];
    std::string ref = argv[2];
    std::string outfile = argv[3];

    std::ifstream file(inputsfile);  // 打开fasta文件

    std::vector<Sequence> sequences;  // 存储序列的向量
    std::string REF;
    std::string line;
    std::string currentSequence;
    std::string currentName;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;  // 跳过空行
        }
        if (line[0] == '>') {
            // 如果当前行是序列名行，则保存之前的序列和序列名
            if (!currentSequence.empty()) {
                if (currentName == ref)
                {
                    REF = currentSequence;
                }
                else
                {
                    Sequence seq;
                    seq.name = currentName;
                    seq.sequence = currentSequence;
                    sequences.push_back(seq);
                }
                
                currentSequence.clear();
            }
            // 提取序列名并去除空白符
            currentName = line.substr(1);
            currentName.erase(std::remove_if(currentName.begin(), currentName.end(), ::isspace), currentName.end());
        }
        else {
            // 将当前行的序列拼接到currentSequence变量中
            currentSequence += line;
        }
    }

    // 保存最后一个序列
    if (!currentSequence.empty()) {
        if (currentName == ref)
        {
            REF = currentSequence;
        }
        else
        {
            Sequence seq;
            seq.name = currentName;
            seq.sequence = currentSequence;
            sequences.push_back(seq);
        }
    }
    file.close();    // 关闭输入文件
    // 打开输出文件
    std::ofstream output(outfile);
    if (!output) {
        std::cout << "Error opening output file." << std::endl;
        return 1;
    }
    int count = 0;
    int acgt = 0;
    // 写入序列名和序列到输出文件
    for (auto& seq : sequences) {
        count = 0;
        acgt = 0;
        for (int i = 0; i < REF.size(); i++)
        {
            if (REF[i] == seq.sequence[i] && REF[i] != 'N' && REF[i] != 'n' && REF[i] != '-')
                count++;
            if (seq.sequence[i] == 'a' || seq.sequence[i] == 'c' || seq.sequence[i] == 'g' || seq.sequence[i] == 't'
                || seq.sequence[i] == 'A' || seq.sequence[i] == 'C' || seq.sequence[i] == 'G' || seq.sequence[i] == 'T')
                acgt++;
        }
        seq.sequence.erase(std::remove(seq.sequence.begin(), seq.sequence.end(), '-'), seq.sequence.end());
        output << "***  "<<(double)count/acgt <<"\t" << count << "\t" << acgt << "\t" << seq.sequence.size() << "\t" << seq.name << std::endl;
    }
    output.close();  // 关闭输出文件

    return 0;
}
