#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>


std::string filterSequence(const std::string& sequence) {
    std::string filteredSeq;
    for (char c : sequence) {
        if (c == 'A' || c == 'C' || c == 'G' || c == 'T' || c == 'a' || c == 'c' || c == 'g' || c == 't') {
            filteredSeq += c;
        }
    }
    return filteredSeq;
}

void processFile(const std::string& filePath) {
    std::ifstream inputFile(filePath);
    std::ofstream outputFile(filePath + ".filtered");

    std::string line, header, sequence;
    while (std::getline(inputFile, line)) {
        if (line.empty()) continue;

        if (line[0] == '>') {
            // Write previous sequence (if any) to output file
            if (!sequence.empty()) {
                std::string filteredSeq = filterSequence(sequence);
                outputFile << ">" << header << "\n" << filteredSeq << "\n";
            }

            // Start processing new header
            header = line.substr(1);
            sequence = "";
        }
        else {
            // Append sequence line
            sequence += line;
        }
    }

    // Write the last sequence to output file
    if (!sequence.empty()) {
        std::string filteredSeq = filterSequence(sequence);
        outputFile << ">" << header << "\n" << filteredSeq << "\n";
    }

    inputFile.close();
    outputFile.close();

    // Replace original file with filtered file
    std::filesystem::remove(filePath);
    std::filesystem::rename(filePath + ".filtered", filePath);
}

void processFolder(const std::string& folderPath) {
    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            std::string filePath = entry.path().string();
            processFile(filePath);
        }
    }
}

int main() {
    std::string path;
    std::cout << "Enter a file path or folder path: ";
    std::getline(std::cin, path);

    if (std::filesystem::is_regular_file(path)) {
        processFile(path);
    }
    else if (std::filesystem::is_directory(path)) {
        processFolder(path);
    }
    else {
        std::cout << "Invalid path. Please provide a valid file or folder path." << std::endl;
    }

    return 0;
}
