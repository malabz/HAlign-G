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
    SmpCommandLine userCommands(argc, argv);
    bool FAorMA = 1;//输出为fasta=0， 输出为maf=1
    int numThreads = 8;
    utils::MAF_info MAFinfo;
    int multiz_num = 0;
    bool mg_tag = false; //1-maf 0-fasta
    int center = -1;//默认-1；
    std::string center_name = "";
    const auto start_point = std::chrono::high_resolution_clock::now(); //记录总起始时间

    bool result_maf = 1;//1输出重复， 0输出唯一                 *****************
    int thresh2 = 10000; //10000    未必对区域分割到*以下       *****************
    int MAFthresh1 = 1;//最短长度
    int MAFthresh2 = 2;//最小行数
    int MAFthresh3 = 0;//最低分数


    // Firstly extract all flagged argumants (i.e. argument identified by a leading hyphen flag)
    center_name = userCommands.getString("r", "reference", "[Longest]", "The reference sequence name [Please delete all whitespace]");
    numThreads = userCommands.getInteger("p", "threads", 1, "The number of threads");
    thresh2 = userCommands.getInteger("dc", "divide_conquer", 10000, "The divide & conquer Kband threshold");
    arguments::in_file_name = userCommands.getString(1, "", " Input file/folder path[Please use .fasta as the file suffix or a forder]");
    arguments::out_file_name = userCommands.getString(2, "", " Output file path[Please use .maf or .fasta as the file suffix]");
    
    //center_name = "1ref";
    //手动指定
    //arguments::in_file_name = "C:/Users/13508/Desktop/test-data/";
    //arguments::out_file_name = "C:/Users/13508/Desktop/test-data.maf";

    
    if (userCommands.helpMessageWanted() || argc == 1)
    {
        userCommands.showHelpMessage();
        std::cout << "http://lab.malab.cn/soft/halign/\n";
        exit(0);
    }
    if (argc < 3)
    {
        userCommands.showHelpMessage();
        std::cout << "\n";
        std::cout << "http://lab.malab.cn/soft/halign/\n";
        std::cout << "Insufficient input parameter!\n";
        exit(1);
    }
    
    std::filesystem::path absolutePath = std::filesystem::absolute(arguments::in_file_name);
    if (std::filesystem::exists(absolutePath)) {
        arguments::in_file_name = absolutePath.generic_string();
        std::replace(arguments::in_file_name.begin(), arguments::in_file_name.end(), '\\', '/');
        if (std::filesystem::is_directory(absolutePath)) {
            if (arguments::in_file_name.back() != '/')
                arguments::in_file_name += '/';
            //std::cout << arguments::in_file_name << std::endl;
            //std::cout << "The input path represents a directory." << std::endl;
        }
        else if (std::filesystem::is_regular_file(absolutePath) && absolutePath.extension().string() == ".fasta") {
            //std::cout << arguments::in_file_name << std::endl;
            //std::cout << "The input path represents a fasta file." << std::endl;
        }
        else {
            std::cout << "The input file/folder path does not represent a .fasta file or a directory." << std::endl;
            exit(1);
        }
    }
    else {
        std::cout << "The input file/folder path does not exist." << std::endl;
        exit(1);
    }
    absolutePath = std::filesystem::absolute(arguments::out_file_name);
    arguments::out_file_name = absolutePath.generic_string();
    std::replace(arguments::out_file_name.begin(), arguments::out_file_name.end(), '\\', '/');

    
    // Print the extracted arguments:
    cout << "[  Input_name  ] : " << arguments::in_file_name << endl;
    cout << "[  Output_name ] : " << arguments::out_file_name << endl;
    cout << "[   Reference  ] = " << center_name << endl;
    cout << "[    Threads   ] = " << numThreads << endl;
    cout << "[Divide_conquer] = " << thresh2 << endl;
    
    threadPool0 = new ThreadPool(numThreads);

    std::vector<std::string> name;
    std::vector<size_t> Length;
    std::vector<size_t> non_Length;
    std::vector<bool> TU; //T1 U0
    if ((arguments::out_file_name.substr(arguments::out_file_name.find_last_of('.') + 1).compare("maf")) == 0)
    {
        mg_tag = true;
        FAorMA = 1;
        MAFinfo.path = arguments::out_file_name + "/";
        if (my_mk_dir(MAFinfo.path) != 0)
        {
            std::cout << MAFinfo.path << "; Folder creation error\n";
            exit(-1);
        }
        if (my_mk_dir(MAFinfo.path + "fasta/") != 0)
        {
            std::cout << MAFinfo.path + "fasta/" << "; Folder creation error\n";
            exit(-1);
        }
        if (my_mk_dir(MAFinfo.path + "maf/") != 0)
        {
            std::cout << MAFinfo.path + "maf/" << "; Folder creation error\n";
            exit(-1);
        }
        MAFinfo.path = arguments::out_file_name + "/maf/";
        MAFinfo.thresh1 = 100; //最短长度
        MAFinfo.thresh2 = 1;   //最小行数
        MAFinfo.thresh3 = 94;  //最低分数
    }
    else if ((arguments::out_file_name.substr(arguments::out_file_name.find_last_of('.') + 1).compare("fasta")) == 0)
    {
        mg_tag = false;
        FAorMA = 0;
        if (my_mk_dir(arguments::out_file_name + "/") != 0)
        {
            std::cout << "Folder creation error\n";
            exit(-1);
        }
	if (my_mk_dir(arguments::out_file_name + "/maf/") != 0)
	{
            std::cout << arguments::out_file_name + "/maf/" << "; Folder creation error\n";
	    exit(-1);
	}
    }
    else {
        std::cout << "The output file path is wrong.[Please use .maf or .fasta as the file suffix]" << std::endl;
        exit(1);
    }

    if (my_mk_dir(arguments::out_file_name + "/NoN/") != 0)
    {
        std::cout << "Folder creation error\n";
        exit(-1);
    }

    arguments::score_file = arguments::out_file_name + "/score.txt";

    cout_cur_time();
    std::cout << "Start: Read and data preprocessing: ";
    if (arguments::in_file_name[arguments::in_file_name.size() - 1] == '/')
    {
        std::vector<std::string> files;
#if defined(_WIN32)
        getFiles_win(arguments::in_file_name, files);
#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
        getFiles_linux(arguments::in_file_name, files);
#endif  
        if (files.size() == 0)
        {
            std::cout << "\nThe input folder is empty!" << std::endl;
            exit(-1);
        }
        std::sort(files.begin(), files.end());
        std::cout << files.size() << " files\n";
        for (int i = 0; i < files.size(); i++)
        {
            std::ifstream ifs(files[i].c_str(), std::ios::binary | std::ios::in);
            utils::read_to_pseudo(ifs, name, Length, non_Length, TU);
            ifs.clear();
        }
    }
    else if ((arguments::in_file_name.substr(arguments::in_file_name.find_last_of('.') + 1).compare("fasta")) == 0)
    {
        std::cout << "1 files\n";
        std::ifstream ifs(arguments::in_file_name, std::ios::binary | std::ios::in); //判断输入路径合法否
        if (!ifs)
        {
            std::cout << "cannot access file " << arguments::in_file_name << '\n';
            exit(1);
        }
        utils::read_to_pseudo(ifs, name, Length, non_Length, TU);
        ifs.clear();
    }

    //确定中心序列
    if (name.size() < 2)
    {
        std::cout << "The number of input sequences is less than two!\n";
        exit(1);  //数据w条数少于2，退出
    }
    auto maxIt = std::max_element(non_Length.begin(), non_Length.end());
    size_t maxIndex = std::distance(non_Length.begin(), maxIt);
    if (center_name != "Longest")
        for (size_t i = 0; i != name.size(); ++i)
            if (name[i] == center_name)
            {
                maxIndex = i;//指定中心序列
                break;
            }
    if (maxIndex != 0)
    {
        std::swap(non_Length[0], non_Length[maxIndex]);
        std::swap(Length[0], Length[maxIndex]);
        std::swap(name[0], name[maxIndex]);

        auto tagTU = TU[maxIndex];
        TU[maxIndex] = TU[0];
        TU[0] = tagTU;

        renameFile(arguments::out_file_name + "/NoN/0", arguments::out_file_name + "/NoN/ztmp");
        renameFile(arguments::out_file_name + "/NoN/" + std::to_string(maxIndex), arguments::out_file_name + "/NoN/0");
        renameFile(arguments::out_file_name + "/NoN/ztmp", arguments::out_file_name + "/NoN/"+ std::to_string(maxIndex));
    }

    cout_cur_time();
    std::cout << "End  : " << Length.size() << " sequences were discovered\n";
    
    std::vector<bool> sign;
    for (int i = 0; i < name.size(); i++)
        sign.push_back(true);
    //for (int i = 0; i < name.size(); i++)
    //    std::cout << name[i] << "\t" << Length[i] << "\t" << TU[i] << "\n";
    
    cout_cur_time();
    std::cout << "Start: pairwise sequence alignment with mummer4.0\n";
    auto insertions = mum_main(name, Length, non_Length, TU, sign, FAorMA, thresh2);
    cout_cur_time();
    std::cout << "End  : pairwise sequence alignment with mummer4.0\n";
    size_t centre_index = 0;//中心序列  永远为0
    multiz_num = name.size() - 1;
    
    threadPool2 = threadPool0;

    if ((arguments::out_file_name.substr(arguments::out_file_name.find_last_of('.') + 1).compare("fasta")) == 0)
    {
        //插入
        cout_cur_time();
        std::cout << "Start: Build alignment and output\n";
        std::ofstream ofs(arguments::out_file_name + "/result.fasta", std::ios::binary | std::ios::out); //判断输出路径合法否
        if (!ofs)
        {
            std::cout << "cannot write file " << arguments::out_file_name + "/result.fasta" << '\n';
            exit(1);
        }
        auto all_size = utils::mum_insert_and_write_fasta(ofs, insertions, name, sign, Length, mg_tag);
        ofs.close();

        std::ifstream ifs(arguments::out_file_name + "/result.fasta", std::ios::binary | std::ios::in); //判断输入路径合法否
        if (!ifs)
        {
            std::cout << "cannot access file " << arguments::out_file_name + "/result.fasta" << '\n';
            exit(1);
        }
        std::string cur_line, center_line;
        for (int i = 0; i < centre_index; i++)
            ifs.seekg(name[i].size() + 7 + all_size, std::ios::cur);
        ifs.seekg(name[centre_index].size() + 6, std::ios::cur);
        ifs >> center_line;
        ifs.seekg(0, std::ios::beg);

        std::ofstream tmpo(arguments::score_file, std::ios::binary | std::ios::out);
        tmpo << "I" << "\t" << "Sign_i\tMatch_num" << "\t" << "NoN_Len" << "\t" << "Length" << "\t" << "Match/noN_Len" << "\t" << "Name" << "\n";
        std::vector<utils::Insertion> N_insert;
        for (int i = 0; i < Length.size(); i++)
        {
            if (i != centre_index)
            {
                ifs.seekg(name[i].size() + 6, std::ios::cur);
                ifs >> cur_line;
                auto score_two = utils::Compare_two(center_line, cur_line, 0, 0, 1, 0);
                tmpo << i << "\t" << sign[i] << "\t" << score_two[0] << "\t" << non_Length[i] << "\t" << Length[i] << "\t" << score_two[0] * 1.0 / non_Length[i] << "\t" << name[i] << "\n";
                delete[]score_two;
            }
            else
            {
                tmpo << centre_index << "\t" << "1" << "\t" << "--" << "\t" << non_Length[i] << "\t" << Length[i] << "\t" << "--" << "\t" << name[centre_index] << "\n";
                ifs.seekg(name[i].size() + 7 + all_size, std::ios::cur);
            }
        }
        tmpo.close();
        ifs.close();
        cout_cur_time();
        std::cout << "End  : Build alignment and output\n";
    }
    else if ((arguments::out_file_name.substr(arguments::out_file_name.find_last_of('.') + 1).compare("maf")) == 0)
    {
        cout_cur_time();
        std::cout << "Start: Build alignment and output\n";
        //插入并输出fasta
        std::ofstream ofs(arguments::out_file_name + "/result.fasta", std::ios::binary | std::ios::out); //判断输出路径合法否
        if (!ofs)
        {
            std::cout << "cannot write file " << arguments::out_file_name + "/result.fasta" << '\n';
            exit(1);
        }
        auto all_size = utils::mum_insert_and_write_fasta(ofs, insertions, name, sign, Length, mg_tag);
        std::cout << "                    | Info : Multiple sequence alignment length = " << all_size << "\n";//输出内存耗费
        ofs.close();
        cout_cur_time();
        std::cout << "End  : Build alignment and output\n";
        
	const auto P1 = std::chrono::high_resolution_clock::now();
        //读取fasta并筛选大maf
        cout_cur_time();
        std::cout << "Start: Multiz merges...\n";

       //合并小maf
        const auto align_2 = std::chrono::high_resolution_clock::now();
        const auto align_sv = std::chrono::high_resolution_clock::now();
        my_mul_main(multiz_num, (char*)(MAFinfo.path).data());
        //mul_main(multiz_num, (char*)(MAFinfo.path).data());
        std::cout << "                    | Info : multiz process result.maf...\n";
        const auto maf_point = std::chrono::high_resolution_clock::now();

        Stream::main_maf2(centre_index, name, Length, sign, MAFinfo);
        std::cout << "                    | Info : MGA.maf consumes   : " << (std::chrono::high_resolution_clock::now() - maf_point) << "\n"; //输出总耗费时间
        std::cout << "                    | Info : MGA.maf mem usage  : " << getPeakRSS() << " B" << std::endl;//输出内存耗费
        renameAndMoveFile(arguments::out_file_name + "/maf/small.maf");
        deleteDirectory(arguments::out_file_name + "/fasta");
        deleteDirectory(arguments::out_file_name + "/maf");
        cout_cur_time();
        std::cout << "End  : Multiz merges...\n";
    }

    delete threadPool2;
    deleteDirectory(arguments::out_file_name + "/NoN");
    std::cout << "                    | Info : Current pid   : " << getpid() << std::endl;
    std::cout << "                    | Info : Time consumes : " << (std::chrono::high_resolution_clock::now() - start_point) << "\n"; //输出总耗费时间
    std::cout << "                    | Info : Memory usage  : " << getPeakRSS() << " B" << std::endl;//输出内存耗费
    std::cout << "                    | Info : Finished\n";
    std::cout << "http://lab.malab.cn/soft/halign/\n";
    return 0;
}
