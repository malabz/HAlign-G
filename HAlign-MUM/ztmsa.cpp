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

void mMultiz(string xx, int multiz_num, int i)
{
	std::string mymultiz_cmd = "mymultiz " + xx + " " + std::to_string(multiz_num) + " " + std::to_string(i) + " 1000000";
	int result = std::system(mymultiz_cmd.c_str());
	if (result != 0)
		std::cerr << "'" << mymultiz_cmd << "' failed with return code: " << result << std::endl;
	FILE* fpw1;
	{
		std::lock_guard<std::mutex> lock(threadPool1->mutex_fp);
		fpw1 = fopen((arguments::out_file_name + "/" + std::to_string(i) + "_.maf").c_str(), "w");
	}

	fprintf(fpw1, "##maf version=1 scoring=N/A\n");
	Stream::cut_maf1000(arguments::out_file_name + "/" + std::to_string(i) + ".maf", fpw1, 100);
	fclose(fpw1);
	deleteFile(arguments::out_file_name + "/" + std::to_string(i) + ".maf");


	std::string mafDuplicateFilter_cmd = "mafDuplicateFilter --maf " + arguments::out_file_name + "/" + std::to_string(i) + "_.maf  > " + arguments::out_file_name + "/" + std::to_string(i) + "_nodup.maf";
	result = std::system(mafDuplicateFilter_cmd.c_str());
	if (result != 0)
		std::cerr << "Fail to run mafDuplicateFilter , exit code: " << result << std::endl;
	deleteFile(arguments::out_file_name + "/" + std::to_string(i) + "_.maf");
	/*
	std::string maf2hal_cmd = "maf2hal " + arguments::out_file_name + "/" + std::to_string(i) + "_nodup.maf  " + arguments::out_file_name + "/" + std::to_string(i) + ".hal  > " + arguments::out_file_name + "/tmp.out";
	result = std::system(maf2hal_cmd.c_str());
	if (result != 0)
		std::cerr << "Fail to convert maf to hal file, exit code: " << result << std::endl;
	*/
	return;
}

int get_Threadnum_from_LargestFileSizeInGB(int mem, int thread_num, int filenum_1) {
	std::string directory = arguments::out_file_name + "/maf";
	uintmax_t max_size = 0;
	// 遍历目录及其子目录
	for (auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
		if (entry.is_regular_file()) {
			uintmax_t file_size = entry.file_size();
			// 更新最大文件大小
			if (file_size > max_size) {
				max_size = file_size;
			}
		}
	}
	int max_file = mem / (static_cast<double>(max_size) / 1024 / 1024 / 1024 * filenum_1 * 10);
	return min(max_file, thread_num);
}
int getTotalMemory() {
	std::ifstream meminfo("/proc/meminfo");
	std::string line;
	long long total_memory = 0;

	if (meminfo.is_open()) {
		while (std::getline(meminfo, line)) {
			if (line.find("MemTotal:") == 0) {
				// 解析MemTotal行，获取总内存大小
				std::sscanf(line.c_str(), "MemTotal: %lld kB", &total_memory);
				break;
			}
		}
		meminfo.close();
	}

	return  static_cast<double>(total_memory) / (1024 * 1024); // 单位为kB
}

int main(int argc, char* argv[])
{
	SmpCommandLine userCommands(argc, argv);
	bool FAorMA = 1;//输出为fasta=0， 输出为maf=1
	int numThreads = 8;
	utils::MAF_info MAFinfo;
	int multiz_num = 0;
	int  Chrs_num = 0;
	bool mg_tag = false; //1-maf 0-fasta
	int center = -1;//默认-1；
	std::string center_name = "";
	const auto start_point = std::chrono::high_resolution_clock::now(); //记录总起始时间

	bool result_maf = 1;//1输出重复， 0输出唯一                 *****************
	int thresh2 = 10000; //10000    未必对区域分割到*以下       *****************
	int MAFthresh1 = 1;//最短长度
	int MAFthresh2 = 2;//最小行数
	int MAFthresh3 = 0;//最低分数

	int num_threads_sys = sysconf(_SC_NPROCESSORS_ONLN);
	if (num_threads_sys == -1) num_threads_sys = 1;
	// Firstly extract all flagged argumants (i.e. argument identified by a leading hyphen flag)
	center_name = userCommands.getString("r", "reference", "[Longest]", "The reference file name");
	numThreads = userCommands.getInteger("t", "threads", num_threads_sys, "The number of threads");
	bool tag_load = userCommands.getInteger("l", "load", false, "Whether the second run and the result directory has a postfix structure file");
	int filter = userCommands.getInteger("f", "filter", 2, "Filter-level: 0-None, 1-General, 2-Strict");
	int mem_size = userCommands.getInteger("m", "memory", getTotalMemory(), "Maximum available memory / GB");
	arguments::in_file_name = userCommands.getString(1, "", " Input folder path");
	arguments::out_file_name = userCommands.getString(2, "", " Output file path[Please use .maf or .hal as the file suffix]");

	if (filter < 0 || filter>2)
	{
		std::cout << "Filter error\n";
		exit(-1);
	}

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
	//cout << "[Divide_conquer] = " << thresh2 << endl;

	if ((arguments::out_file_name.substr(arguments::out_file_name.find_last_of('.') + 1).compare("maf")) == 0);
	else if ((arguments::out_file_name.substr(arguments::out_file_name.find_last_of('.') + 1).compare("hal")) == 0);
	else {
		std::cout << "The output file path is wrong.[Please use .maf or .hal as the file suffix]" << std::endl;
		exit(1);
	}
	mg_tag = true;
	FAorMA = 1;
	MAFinfo.path = arguments::out_file_name + "/";
	if (my_mk_dir(MAFinfo.path) != 0)
	{
		std::cout << MAFinfo.path << "; Folder creation error\n";
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


	cout_cur_time();
	std::vector<std::string> files;
	std::cout << "Start: Read and data preprocessing: ";
	if (arguments::in_file_name[arguments::in_file_name.size() - 1] == '/')
	{

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
	}
	//确定中心序列
	if (files.size() < 2)
	{
		std::cout << "The number of input files is less than two!\n";
		exit(1);  //数据w条数少于2，退出
	}
	int max_file = 0;
	size_t max_size;
	center_name.erase(std::remove_if(center_name.begin(), center_name.end(), [](unsigned char x) {
		return std::isspace(x);
		}), center_name.end());
	bool findtag = false;
	if (center_name != "[Longest]")
	{
		for (int i = 0; i < files.size(); i++)
		{
			std::string now_name = std::filesystem::path(files[i]).filename().string();
			now_name.erase(std::remove_if(now_name.begin(), now_name.end(), [](unsigned char x) {
				return std::isspace(x);
				}), now_name.end());
			if (now_name == center_name)
			{
				findtag = true;
				max_file = i;
				break;
			}
		}
		if (!findtag)
		{
			std::cout << "Error: No genome named '" << center_name << "' could be found !\n";
			exit(1);
		}
	}
	if (!findtag)
	{
		max_size = std::filesystem::file_size(files[0]);
		for (int i = 1; i < files.size(); i++)
		{
			if (std::filesystem::file_size(files[i]) > max_size)
			{
				max_file = i;
				max_size = std::filesystem::file_size(files[i]);
			}
		}
	}
	if (max_file != 0)
		std::swap(files[max_file], files[0]);
	std::cout << "                    | Info : Center: " << std::filesystem::path(files[0]).filename().string() << "\t | size:" << std::filesystem::file_size(files[0]) << " B\n";

	if (my_mk_dir(arguments::out_file_name + "/NoN/") != 0)
	{
		std::cout << "Folder creation error\n";
		exit(-1);
	}

	if (my_mk_dir(arguments::out_file_name + "/save/") != 0)
	{
		std::cout << "Folder creation error\n";
		exit(-1);
	}
	std::vector<std::vector<size_t>> Length_All(files.size());
	std::vector<std::vector<std::string>> Name_All(files.size());
	for (int i = 0; i < files.size(); i++)
	{
		processFastaFile(i, files[i], Length_All[i], Name_All[i]);
	}
	arguments::seq_num = numThreads;
	size_t  inThreads = 1;
	size_t numThreadsCeil = numThreads;

	int max_out = max(1, mem_size / 90);
	if (numThreadsCeil > (files.size() - 1))
	{
		if ((files.size() - 1) > max_out)
		{
			inThreads = static_cast<int>(std::ceil(1.0 * numThreads / max_out));
			numThreadsCeil = max_out;
		}
		else
		{
			inThreads = static_cast<int>(std::ceil(1.0 * numThreads / (files.size() - 1)));
			numThreadsCeil = files.size() - 1;
		}
	}
	threadPool0 = new ThreadPool(numThreadsCeil);

	cout_cur_time();
	std::cout << "End  : " << Length_All.size() << " genomes were discovered\n";
	const auto align_start = std::chrono::high_resolution_clock::now();
	cout_cur_time();
	std::cout << "Start: pairwise genome alignment with mummer4.0\n";
	std::cout << "                    | Info : Thread num of mynuc : " << inThreads << "\n";
	std::cout << "                    | Info : Thread num of halign : " << numThreadsCeil << "\n";
	//main 
	mum_main(files, Length_All, Name_All, inThreads, tag_load, filter);
	cout_cur_time();
	std::cout << "End  : pairwise genome alignment with mummer4.0\n";
	std::cout << "                    | Info : PSA time consumes : " << (std::chrono::high_resolution_clock::now() - align_start) << "\n"; //输出比对耗费时间
	std::cout << "                    | Info : PSA memory peak   : " << getPeakRSS() << " B\n";//输出内存耗费

	delete threadPool0;

	deleteDirectory(arguments::out_file_name + "/NoN");
	//MAF 合并
	multiz_num = files.size() - 1;
	Chrs_num = Length_All[0].size();

	const auto maf_point = std::chrono::high_resolution_clock::now();
	cout_cur_time();
	std::cout << "Start: Multiz merges...\n";
	std::string xx = arguments::out_file_name + "/";
	//threadPool1 = new ThreadPool((numThreads > 8) ? 8 : numThreads);
	int thread_num1 = get_Threadnum_from_LargestFileSizeInGB(mem_size, numThreads, files.size() - 1);
	std::cout << "                    | Info : Thread num of mymultiz : " << thread_num1 << "\n";
	threadPool1 = new ThreadPool(thread_num1);
	for (int i = 0; i < Chrs_num; i++)
	{
		threadPool1->execute(&mMultiz, xx, multiz_num, i);
		//mMultiz(xx, multiz_num, i);
	}
	threadPool1->waitFinished();
	delete threadPool1;
	deleteDirectory(arguments::out_file_name + "/maf");
	//exit(0);
	/*
	std::vector<std::string> file_name;
	for (int i = 0; i < files.size(); i++)
		file_name.push_back(getFileName(files[i]));
	std::vector<std::vector<char>> strand(files.size());
	for (int i = 0; i < Name_All.size(); i++)
	{
		for (int j = 0; j < Name_All[i].size(); j++)
		{
			Name_All[i][j].erase(std::remove(Name_All[i][j].begin(), Name_All[i][j].end(), '.'), Name_All[i][j].end());
			Name_All[i][j].erase(std::remove(Name_All[i][j].begin(), Name_All[i][j].end(), ' '), Name_All[i][j].end());
			strand[i].push_back('+');
		}
	}*/
	/*
	FILE* fpw1 = fopen((arguments::out_file_name + "/result.maf").c_str(), "w");
	fprintf(fpw1, "##maf version=1 scoring=N/A\n");
	for (int i = 0; i < Chrs_num; i++) {
		Stream::cut_maf1000(arguments::out_file_name + "/" + std::to_string(i) + "_nodup.maf", fpw1, 500);
		//三级排序  去重后质量更低了
		//Stream::get_strand(strand, file_name, Name_All, arguments::out_file_name + "/" + std::to_string(i) + ".maf");
		//Stream::sort_maf1000(strand, file_name, Name_All, arguments::out_file_name + "/" + std::to_string(i) + ".maf", fpw1);
		deleteFile(arguments::out_file_name + "/" + std::to_string(i) + "_nodup.maf");
	}
	fclose(fpw1);
	cout_cur_time();
	std::cout << "End  : Multiz merges...\n";*/

	//写入一个文件
	std::string tmpline;
	std::ofstream output(arguments::out_file_name + "/result.maf", std::ios::out); // 创建文件或覆盖已有文件
	
	if (!output) {
		std::cerr << "Error opening output file: " << arguments::out_file_name + "/result.maf" << std::endl;
		return -1;
	}
	output << "##maf version=1 scoring=N/A\n";
	for (int i = 0; i < Chrs_num; i++) {
		std::ifstream input(arguments::out_file_name + "/" + std::to_string(i) + "_nodup.maf");
		if (!input) {
			std::cerr << "Error opening input file: " << arguments::out_file_name + "/" + std::to_string(i) + "_nodup.maf" << std::endl;
			return -1;
		}
		std::getline(input, tmpline); // 读取并丢弃第一行
		// 将剩余内容写入目标文件
		output << input.rdbuf(); // 将输入文件的内容写入目标文件
		input.close();
		deleteFile(arguments::out_file_name + "/" + std::to_string(i) + "_nodup.maf");
	}
	output.close();
	cout_cur_time();
	std::cout << "End  : Multiz merges...\n";
	if ((arguments::out_file_name.substr(arguments::out_file_name.find_last_of('.') + 1).compare("hal")) == 0)
	{
		try {
			// 构建maf2hal arguments::out_file_name + "/result.maf" arguments::out_file_name + "/result.hal"命令并且运行
			// 一定要把maf2hal放在环境变量中！！！
			//std::string mafDuplicateFilter_cmd = "mafDuplicateFilter --maf " + arguments::out_file_name + "/result_all.maf  > " + arguments::out_file_name + "/result_nodup.maf ";
			std::string maf2hal_cmd = "maf2hal " + arguments::out_file_name + "/result.maf  " + arguments::out_file_name + "/result.hal >" + arguments::out_file_name + "/tmp.out";
			/*cout_cur_time();
			std::cout << "Start: mafDuplicateFilter\n";
			int result = std::system(mafDuplicateFilter_cmd.c_str());
			if (result != 0) {
				std::cerr << "Fail to run mafDuplicateFilter , exit code: " << result << std::endl;
			}
			else {
				cout_cur_time();
				std::cout << "End  : mafDuplicateFilter\n";
			}*/
			cout_cur_time();
			std::cout << "Start: MAF to HAL\n";
			int result = std::system(maf2hal_cmd.c_str());
			if (result != 0) {
				std::cerr << "Fail to convert maf to hal file, exit code: " << result << std::endl;
			}
			else {
				cout_cur_time();
				std::cout << "End  : MAF to HAL\n";
			}

		}
		catch (const std::exception& e) {
			std::cerr << "Fail to convert maf to hal file\n";
			std::cerr << e.what() << std::endl;
		}

		std::cout << "                    | Info : result.hal consumes   : " << (std::chrono::high_resolution_clock::now() - maf_point) << "\n"; //输出总耗费时间
		std::cout << "                    | Info : result.hal mem usage  : " << getPeakRSS() << " B" << std::endl;//输出内存耗费
		//deleteDirectory(arguments::out_file_name + "/NoN");
		//deleteDirectory(arguments::out_file_name + "/maf");
		deleteFile(arguments::out_file_name + "/tmp.out");
		deleteFile(arguments::out_file_name + "/result.maf");
		cout_cur_time();
	}
	std::cout << "Info : Current pid   : " << getpid() << std::endl;
	std::cout << "                    | Info : Time consumes : " << (std::chrono::high_resolution_clock::now() - start_point) << "\n"; //输出总耗费时间
	std::cout << "                    | Info : Memory usage  : " << getPeakRSS() << " B" << std::endl;//输出内存耗费
	std::cout << "                    | Info : Finished\n";
	std::cout << "http://lab.malab.cn/soft/halign/\n";

	return 0;
}
