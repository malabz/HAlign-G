#include "../mummer/delta.hh"
#include "../mummer/tigrinc.hh"
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <filesystem>  // C++17 or later
using namespace std;

// Helper function to extract the two keys from the block header line
std::pair<std::string, std::string> extract_keys(const std::string& header) {
    size_t start = header.find('>') + 1;
    size_t end1 = header.find(' ', start);
    size_t start2 = end1 + 1;
    size_t end2 = header.find(' ', start2);

    std::string first_key = header.substr(start, end1 - start);
    std::string second_key = header.substr(start2, end2 - start2);

    return std::make_pair(first_key, second_key);
}

int filter_sub(std::string OPT_AlignName, int filter)
{
    bool           OPT_QLIS = false;     // do query based LIS
    bool           OPT_RLIS = false;     // do reference based LIS
    bool           OPT_GLIS = false;     // do global LIS
    bool           OPT_1to1 = false;     // do 1-to-1 alignment
    bool           OPT_MtoM = false;     // do M-to-M alignment
    long int       OPT_MinLength = 0;         // minimum alignment length
    float          OPT_MinIdentity = 0.0;       // minimum %identity
    float          OPT_MinUnique = 0.0;       // minimum %unique
    float          OPT_MaxOverlap = 100.0;     // maximum olap as % of align len
    float          OPT_Epsilon = -1.0;      // negligible alignment score
    srand(1);
    if(filter==2)
        OPT_1to1 = true;
    else if (filter==1)
        OPT_QLIS = true;
    string outfile = OPT_AlignName + ".delta";

    DeltaGraph_t graph;
    graph.build(OPT_AlignName, true);

    //-- 1-to-1
    if (OPT_1to1)
        graph.flag1to1(OPT_Epsilon, OPT_MaxOverlap);
    //-- Query-based LIS
    if (OPT_QLIS)
        graph.flagQLIS(OPT_Epsilon, OPT_MaxOverlap);

    std::ofstream fout(outfile, std::ios::binary | std::ios::out);
    graph.outputDelta(fout);
    fout.close();
    return 0;
}

// Function to sort and write the blocks into separate files
void sort_delta(std::string file, int filter) {
    if (filter == 0)
        return;
    std::ifstream infile(file);
    if (!infile) {
        std::cerr << "Error: Cannot open file " << file << std::endl;
        return;
    }
    //std::cout << file << " sort_delta finish\n";
    std::string line;
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> blocks;
    std::vector<std::string> current_block;

    // Read the first two lines as header
    for (int i = 0; i < 2 && std::getline(infile, line); ++i) {
        header.push_back(line);
    }
   // std::cout << file << " sort_delta 1\n";
    // Read the file and group blocks
    while (std::getline(infile, line)) {
        if (line.empty()) continue;

        if (line[0] == '>') {
            if (!current_block.empty()) {
                blocks.push_back(current_block);
                current_block.clear();
            }
        }
        current_block.push_back(line);
    }
    infile.close();
    // Don't forget to push the last block
    if (!current_block.empty()) {
        blocks.push_back(current_block);
    }
    //std::cout << file << " sort_delta 2\n";
    // Sort blocks based on the two keys extracted from the block header
    std::sort(blocks.begin(), blocks.end(), [](const std::vector<std::string>& a, const std::vector<std::string>& b) {
        auto key_a = extract_keys(a[0]);
        auto key_b = extract_keys(b[0]);
        return key_a < key_b;
        });
   // std::cout << file << " sort_delta 3\n";
    // Create the output directory
    std::string output_dir = file + "_f/";
    std::filesystem::create_directory(output_dir);
   // std::cout << file << " sort_delta 4\n";
    // Write the sorted blocks into separate files
    std::ofstream outfile;
    std::pair<std::string, std::string> last_key("", "");
    int file_index = -1;
    //std::cout << file << " sort_delta 5\n";
    for (const auto& block : blocks) {
        auto current_key = extract_keys(block[0]);

        // If the key changes, start a new file
        if (current_key != last_key) {
            if (outfile.is_open()) {
                outfile.close();
                filter_sub(output_dir + std::to_string(file_index),filter);
            }
            outfile.open(output_dir + std::to_string(++file_index));
            if (!outfile) {
                std::cerr << "Error: Cannot create file " << output_dir + std::to_string(file_index) << std::endl;
                return;
            }

            // Write the header to the new file
            for (const auto& h : header) {
                outfile << h << std::endl;
            }

            last_key = current_key;
        }

        // Write the current block to the file
        for (const auto& line : block) {
            outfile << line << std::endl;
        }
    }
    //std::cout << file << " sort_delta 6\n";
    // Close the last file
    if (outfile.is_open()) {
        outfile.close();
        filter_sub(output_dir + std::to_string(file_index),filter);
    }
    
    outfile.open(file);
    if (!outfile) {
        std::cerr << "Error: Cannot create file " << file << std::endl;
        return;
    }
    //std::cout << file << " sort_delta 7\n";
    for (const auto& h : header) {
        outfile << h << std::endl;
    }
    for (int i = 0; i <= file_index; i++)
    {
        infile.open(output_dir + std::to_string(i)+".delta");
        if (!infile) {
            std::cerr << "Error: Cannot open file " << output_dir + std::to_string(i) + ".delta" << std::endl;
            return;
        }
        int hd = 0;
        while (std::getline(infile, line)) {
            hd++;
            if (hd <= 2)
                continue;
            outfile << line << std::endl;
        }
        infile.close();
    }
    outfile.close();
   // std::cout << file << " sort_delta finish\n";
}

int filter_main(char* filename, int filter)
{
    string OPT_AlignName = std::string(filename);
    OPT_AlignName += ".delta";
    sort_delta(OPT_AlignName, filter);
    return 0;
}

#include <iostream>
#include <fstream>
#include <climits>
#include <cstdlib>
#include <pthread.h>
#include <thread>
#include <memory>
#include "../mummer/nucmer.hpp"
#include "../thread_pipe/thread_pipe.hpp"
#include "nucmer_cmdline.hpp"

#undef _OPENMP
#ifdef _OPENMP
#include <omp.h>
#endif

struct getrealpath {
    const char* path, * res;
    getrealpath(const char* p) : path(p), res(realpath(p, nullptr)) { }
    ~getrealpath() { free((void*)res); }
    operator const char* () const { return res ? res : path; }
};

typedef std::vector<const char*>::const_iterator         path_iterator;
typedef jellyfish::stream_manager<path_iterator>         stream_manager;
typedef jellyfish::whole_sequence_parser<stream_manager> sequence_parser;

void query_thread(mummer::nucmer::FileAligner* aligner, sequence_parser* parser,
    thread_pipe::ostream_buffered* printer, const nucmer_cmdline* args) {
    auto output_it = printer->begin();
    const bool sam = args->sam_short_given || args->sam_long_given;

    auto print_function = [&](std::vector<mummer::postnuc::Alignment>&& als,
        const mummer::nucmer::FastaRecordPtr& Af, const mummer::nucmer::FastaRecordSeq& Bf) {
            assert(Af.Id()[strlen(Af.Id()) - 1] != ' ');
            assert(Bf.Id().back() != ' ');
            if (!sam)
                mummer::postnuc::printDeltaAlignments(als, Af.Id(), Af.len(), Bf.Id(), Bf.len(), *output_it, args->minalign_arg);
            else
                mummer::postnuc::printSAMAlignments(als, Af, Bf, *output_it, args->sam_long_given, args->minalign_arg);
            if (output_it->tellp() > 1024)
                ++output_it;
    };
    aligner->thread_align_file(*parser, print_function);
    output_it.done();
}

void query_long(mummer::nucmer::FileAligner* aligner, sequence_parser* parser,
    thread_pipe::ostream_buffered* printer, const nucmer_cmdline* args) {
    auto output_it = printer->begin();
    auto print_function = [&](std::vector<mummer::postnuc::Alignment>&& als,
        const mummer::nucmer::FastaRecordPtr& Af, const mummer::nucmer::FastaRecordSeq& Bf) {
            mummer::postnuc::printDeltaAlignments(als, Af.Id(), Af.len(), Bf.Id(), Bf.len(), *output_it, args->minalign_arg);
            if (output_it->tellp() > 1024)
                ++output_it;
    };

    while (true) {
        sequence_parser::job j(*parser);
        if (j.is_empty()) break;
        for (size_t i = 0; i < j->nb_filled; ++i) {
            mummer::nucmer::FastaRecordSeq Query(j->data[i].seq.c_str(), j->data[i].seq.length(), j->data[i].header.c_str());
            aligner->align_long_sequences(Query, print_function);
        }
    }
    output_it.done();
}

//   ./nuc,path1,path2,out
//    ./xx 1.fasta 2.fasta 1_2ans
int main(int argc, char* argv[]) {
   /*char* filename = new char[strlen(argv[1]) + 1];
    strcpy(filename, argv[1]);
    //filter_start
    filter_main(filename);
    return 0;*/

    char** new_argv = new char* [9];
    
    for (int i = 0; i < 3; ++i) {
        new_argv[i] = new char[strlen(argv[i]) + 1];
        strcpy(new_argv[i], argv[i]);
    }
    new_argv[3] = new char[9];
    strcpy(new_argv[3], "--prefix");
    new_argv[4] = new char[7];
    strcpy(new_argv[4], "nucmer");
    new_argv[5] = new char[10];
    strcpy(new_argv[5], "--threads");
    new_argv[6] = new char[3];
    std::string threadnum = argv[4];
    //std::cout << "threadnum: " << threadnum << std::endl;
    if (std::stoi(threadnum) > 8)
        strcpy(new_argv[6], argv[4]);
    else
        strcpy(new_argv[6], "8");
    new_argv[7] = new char[strlen(argv[5]) + 1];
    strcpy(new_argv[7], argv[5]);
    new_argv[8] = new char[strlen(argv[6]) + 1];
    strcpy(new_argv[8], argv[6]);
    
    std::string filter_Str = argv[7];
    int filter = std::stoi(filter_Str);//É¸Ñ¡µÈ¼¶

    std::ios::sync_with_stdio(false);
    nucmer_cmdline args(9, new_argv);



    mummer::nucmer::Options opts;
    opts.breaklen(args.breaklen_arg)
        .mincluster(args.mincluster_arg)
        .diagdiff(args.diagdiff_arg)
        .diagfactor(args.diagfactor_arg)
        .maxgap(args.maxgap_arg)
        .minmatch(args.minmatch_arg);
    if (args.noextend_flag) opts.noextend();
    if (args.nooptimize_flag) opts.nooptimize();
    if (args.nosimplify_flag) opts.nosimplify();
    if (args.forward_flag) opts.forward();
    if (args.reverse_flag) opts.reverse();
    if (args.mum_flag) opts.mum();
    if (args.maxmatch_flag) opts.maxmatch();

    std::string arg3_string(argv[3]);
    const std::string output_file = arg3_string + ".delta";
   // const std::string output_file = arg3_string + ".delta";
    std::ofstream os;
    if (!args.qry_arg.empty()) {
        if (args.qry_arg.size() != 1 && !(args.sam_short_given || args.sam_long_given))
            nucmer_cmdline::error() << "Multiple query file is only supported with the SAM output format";
        os.open(output_file);
        if (!os.good())
            nucmer_cmdline::error() << "Failed to open output file '" << output_file << '\'';

        getrealpath real_ref(args.ref_arg), real_qry(args.qry_arg[0]);
    }
    os << argv[1] << " " << argv[2] << "\nNUCMER\n";
    thread_pipe::ostream_buffered output(os);

    std::unique_ptr<mummer::nucmer::FileAligner> aligner;
    std::ifstream reference;

    if (args.load_given) {
        mummer::nucmer::sequence_info reference_info(args.ref_arg);
        mummer::mummer::sparseSA SA(reference_info.sequence, args.load_arg);
        aligner.reset(new mummer::nucmer::FileAligner(std::move(reference_info), std::move(SA), opts));
    }
    else {
        reference.open(args.ref_arg);
        if (!reference.good())
            nucmer_cmdline::error() << "Failed to open reference file '" << args.ref_arg << "'";
    }
    const size_t batch_size = args.batch_given ? args.batch_arg : std::numeric_limits<size_t>::max();
    do {
        if (!args.load_given)
            aligner.reset(new mummer::nucmer::FileAligner(reference, batch_size, opts));

        if (args.save_given && !aligner->sa().save(args.save_arg))
            nucmer_cmdline::error() << "Can't save the suffix array to '" << args.save_arg << "'";

        stream_manager     streams(args.qry_arg.cbegin(), args.qry_arg.cend());
        const unsigned int nb_threads = args.threads_given ? args.threads_arg : 2;
#ifdef _OPENMP
        if (args.threads_given) omp_set_num_threads(nb_threads);
#endif // _OPENMP

        if (!args.genome_flag) {
            sequence_parser    parser(4 * nb_threads, 10, args.max_chunk_arg, 1, streams);

#ifdef _OPENMP
#pragma omp parallel
            {
                query_thread(aligner.get(), &parser, &output, &args);
            }
#else // _OPENMP
            std::vector<std::thread> threads;
            for (unsigned int i = 0; i < nb_threads; ++i)
                threads.push_back(std::thread(query_thread, aligner.get(), &parser, &output, &args));

            for (auto& th : threads)
                th.join();
#endif // _OPENMP
        }
        else {
            // Genome flag on
            sequence_parser    parser(4, 1, 1, streams);
            query_long(aligner.get(), &parser, &output, &args);
        }
    } while (!args.load_given && reference.peek() != EOF);
    output.close();
    os.close();

    for (int i = 0; i < 7; ++i)
        delete[] new_argv[i];
    delete[] new_argv;

    char* filename = new char[strlen(argv[3]) + 1];
    strcpy(filename, argv[3]);
    
    filter_main(filename,filter);

    delete[] filename;

    return 0;
}
