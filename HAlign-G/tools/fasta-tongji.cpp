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

    std::ifstream file(inputsfile);  // ��fasta�ļ�

    std::vector<Sequence> sequences;  // �洢���е�����
    std::string REF;
    std::string line;
    std::string currentSequence;
    std::string currentName;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;  // ��������
        }
        if (line[0] == '>') {
            // �����ǰ�����������У��򱣴�֮ǰ�����к�������
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
            // ��ȡ��������ȥ���հ׷�
            currentName = line.substr(1);
            currentName.erase(std::remove_if(currentName.begin(), currentName.end(), ::isspace), currentName.end());
        }
        else {
            // ����ǰ�е�����ƴ�ӵ�currentSequence������
            currentSequence += line;
        }
    }

    // �������һ������
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
    file.close();    // �ر������ļ�
    // ������ļ�
    std::ofstream output(outfile);
    if (!output) {
        std::cout << "Error opening output file." << std::endl;
        return 1;
    }
    int count = 0;
    int acgt = 0;
    // д�������������е�����ļ�
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
    output.close();  // �ر�����ļ�

    return 0;
}
