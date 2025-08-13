#include "mum_main.hpp"


std::map<std::thread::id, Kband*> threadMAP;
//delta形式的同源区间转为h4的insert形式
void get_inserts(std::vector<utils::Insertion>& inserts_1, std::vector<utils::Insertion>& inserts_2, std::vector <long>& numbers, size_t start1, size_t start2)
{
	size_t index = 0;
	size_t num1 = 0, num2 = 0;

	for (size_t i = 0; i < numbers.size(); ++i)
	{
		long num = numbers[i];
		if (num > 0) {
			index += num;
			if (!inserts_2.empty() && inserts_2.back().index == (start2 + index - num2 - 1)) {
				inserts_2.back().number += 1;
			}
			else {
				inserts_2.push_back({ start2 + index - num2 - 1, 1 });
			}
			num2 += 1;
		}
		else if (num < 0)
		{
			index += std::abs(num);
			if (!inserts_1.empty() && inserts_1.back().index == (start1 + index - num1 - 1)) {
				inserts_1.back().number += 1;
			}
			else {
				inserts_1.push_back({ start1 + index - num1 - 1, 1 });
			}
			num1 += 1;
		}
	}
}

Mum_struct_insert get_inserts(Mum_struct& Struct)
{
	Mum_struct_insert Struct_sv;
	Struct_sv.file1 = Struct.file1;
	Struct_sv.file2 = Struct.file2;
	Struct_sv.start1 = Struct.start1;
	Struct_sv.start2 = Struct.start2;
	Struct_sv.end1 = Struct.end1;
	Struct_sv.end2 = Struct.end2;
	Struct_sv.tag = Struct.tag;
	std::vector<utils::Insertion>& inserts_1 = Struct_sv.insertA;
	std::vector<utils::Insertion>& inserts_2 = Struct_sv.insertB;
	std::vector <long>& numbers = Struct.string;
	size_t start1 = Struct.start1;
	size_t start2 = Struct.start2;

	size_t index = 0;
	size_t num1 = 0, num2 = 0;

	for (size_t i = 0; i < numbers.size(); ++i)
	{
		long num = numbers[i];
		if (num > 0) {
			index += num;
			if (!inserts_2.empty() && inserts_2.back().index == (start2 + index - num2 - 1)) {
				inserts_2.back().number += 1;
			}
			else {
				inserts_2.push_back({ start2 + index - num2 - 1, 1 });
			}
			num2 += 1;
		}
		else if (num < 0)
		{
			index += std::abs(num);
			if (!inserts_1.empty() && inserts_1.back().index == (start1 + index - num1 - 1)) {
				inserts_1.back().number += 1;
			}
			else {
				inserts_1.push_back({ start1 + index - num1 - 1, 1 });
			}
			num1 += 1;
		}
	}
	return Struct_sv;
}


//序列逆补
void nibu(std::string& str)
{
	std::reverse(str.begin(), str.end());

	std::transform(str.begin(), str.end(), str.begin(), [](char c) {
		switch (c) {
		case 'A':
		case 'a':
			return 'T';
		case 'C':
		case 'c':
			return 'G';
		case 'T':
		case 't':
			return 'A';
		case 'G':
		case 'g':
			return 'C';
		default:
			return c;
		}
		});
	return;
}

//delta之间插入对齐，SV专属，输出
void align_delta_sv(std::string& aligned_reference, std::string& aligned_query, std::vector<long>& numbers) {
	size_t index = 0;
	for (const auto& num : numbers) {
		if (num > 0) { // 缺失
			index += num;
			aligned_query.insert(index - 1, "-");
		}
		else if (num < 0) { // 插入
			index += std::abs(num);
			aligned_reference.insert(index - 1, "-");
		}
	}
}

//insert插入对齐，重叠new_SV专属，输出
void align_insert_sv(std::string& aligned_reference, std::string& aligned_query, std::vector <utils::Insertion>& insertA, std::vector <utils::Insertion>& insertB) {
	size_t index = 0;
	// 逆序遍历
	for (auto it = insertA.rbegin(); it != insertA.rend(); ++it) {
		const utils::Insertion& insert = *it;
		aligned_reference.insert(insert.index, insert.number, '-');
	}
	for (auto it = insertB.rbegin(); it != insertB.rend(); ++it) {
		const utils::Insertion& insert = *it;
		aligned_query.insert(insert.index, insert.number, '-');
	}
}

// 找到与 tmpline1 公共前缀最长的字符串的索引
int findLongestPrefixIndex(std::string& tmpline1, std::vector<std::string>& NameA) {
	int maxLength = 0;
	int maxIndex = 0;
	for (int i = 0; i < NameA.size(); ++i) {
		int len = 0;
		int minLen = std::min(tmpline1.length(), NameA[i].length());
		while (len < minLen && tmpline1[len] == NameA[i][len]) {
			len++;
		}

		if (len > maxLength) {
			maxLength = len;
			maxIndex = i;
		}
	}
	return maxIndex;
}
//读取delta文件，获取同源区间+sv，变换索引，得到方向
void read_delta_get_struct(std::string delta_filename, std::vector<std::vector<Mum_struct>>& Final_Structs, std::vector<std::string>& NameA, std::vector<std::string>& NameB)
{
	std::ifstream file; // 外部声明
	{
		std::lock_guard<std::mutex> lock(threadPool0->mutex_fp); // 在申请file-number时加锁
		std::ifstream tempFile(delta_filename); // 内部定义
		//std::cout << "now:"<< delta_filename<<"\n";
		if (!tempFile.is_open()) {
			std::cerr << "Error opening file " << delta_filename << std::endl;
			return;
		}
		std::swap(file, tempFile); // 交换对象
	} // tempFile 超出作用域，在其析构时会关闭文件
	/*std::cout << NameA.size() << " NameA.size()\n";
	for (int i = 0; i < NameA.size(); i++)
		  std::cout << NameA[i] << "\n";
	std::cout << NameB.size() << " NameB.size()\n";
	for (int i = 0; i < NameB.size(); i++)
		std::cout << NameB[i] << "\n";
	std::cout <<"\n";*/

	std::string line, tmpline1, tmpline2;
	std::getline(file, line);//第一行 两个路径
	std::getline(file, line);//第二行 NUCMER
	std::getline(file, line);//第三行 > name1 name2 len1 len2

	size_t length_a, length_b;
	size_t start1, end1, start2, end2;
	size_t f1 = 0, f2 = 0;
	int tag = 1;
	std::string string_line;

	while (true) {
		if (line.empty()) break;
		std::istringstream iss(line.substr(1));
		iss >> tmpline1 >> tmpline2 >> length_a >> length_b;
		f1 = findLongestPrefixIndex(tmpline1, NameA);
		f2 = findLongestPrefixIndex(tmpline2, NameB);
		//std::cout <<">"<< tmpline1 << " " << tmpline2 << " " << f1 << " " << f2 << "\n";
		while (true) {
			std::getline(file, line);
			if (line.empty()) break;
			if (line[0] == '>') break;
			// 解析文件内容并填充Mum_struct
			std::istringstream iss(line);
			iss >> start1 >> end1 >> start2 >> end2;
			if (start2 < end2) tag = 1;
			else { tag = -1; std::swap(start2, end2); }
			start1 = start1 - 1;
			start2 = start2 - 1;
			Mum_struct current_struct = { f1, f2, start1, end1, start2, end2, tag, {} };
			while (true) {
				std::getline(file, string_line);
				if (string_line == "0")
					break;
				current_struct.string.push_back(std::stol(string_line));
			}
			Final_Structs[current_struct.file1].push_back(current_struct);
		}
	}
	//std::cout << "sort start\n";
	for (auto& Mss : Final_Structs)
	{
		std::sort(Mss.begin(), Mss.end(), [](const Mum_struct& a, const Mum_struct& b) {
			return a.start1 < b.start1;
			});
	}
	/*
	int II = 0;
	for (auto& Mss : Final_Structs)
	{
		std::cout << (II++) <<"  "<< Mss.size() << "  ????\n";
		for (auto& structs : Mss)
		{
			std::cout << "Mum_struct { file1: " << structs.file1
				<< ", file2: " << structs.file2
				<< ", start1: " << structs.start1
				<< ", end1: " << structs.end1
				<< ", start2: " << structs.start2
				<< ", end2: " << structs.end2
				<< ", tag: " << structs.tag
				<< ", string: " << structs.string.size()<<"\n";
		}
	}*/
}

//求关键路径，将其tag为1，其余易位sv为0，倒位为-1
void max_weight_increasing_subsequence(std::vector<Mum_struct>& struct_list) {
	// 找到tag=1的struct中最长的上升子序列
	std::vector<size_t> start;
	std::vector<size_t> length;
	for (const auto& s : struct_list) {
		if (s.tag == 1) {
			start.push_back(s.start1);
			length.push_back(s.end1 - s.start1);
		}
	}

	size_t n = start.size();

	if (n == 0) {
		return;
	}

	// dp[i] 表示以 start[i] 结尾的最大权上升子序列的权重和
	std::vector<size_t> dp = length;

	// 记录每个位置的前一个位置，用于构建结果
	std::vector<int> prev(n, -1);

	for (size_t i = 1; i < n; ++i) {
		for (size_t j = 0; j < i; ++j) {
			if (start[i] > start[j] && dp[i] < dp[j] + length[i]) {
				dp[i] = dp[j] + length[i];
				prev[i] = static_cast<int>(j);
			}
		}
	}

	// 找到最大权上升子序列的结束位置
	auto max_index = std::max_element(dp.begin(), dp.end()) - dp.begin();

	// 构建最大权上升子序列的索引列表
	std::vector<size_t> result;
	while (max_index != -1) {
		result.insert(result.begin(), start[max_index]);
		max_index = prev[max_index];
	}

	size_t k = 0;
	for (auto& s : struct_list) {
		if (k < result.size()) {
			if (s.tag == -1) {
				continue;
			}
			if (s.start1 == result[k]) {
				++k;
			}
			else {
				s.tag = 0;
			}
		}
		else {
			if (s.tag != -1)
				s.tag = 0;
		}
	}
	return;
}
void BinaryMUMSave(int I, std::vector<std::string>& files, int inThreads, bool FAorMA)
{
	// 构造参数字符串
	std::string arg0;
	if (FAorMA) //true maf q
		arg0 = "mynucq";
	else       //false fasta 1
		arg0 = "mynuc1";
	std::string arg1 = files[0];
	std::string arg2 = files[I];
	std::string arg3 = arguments::out_file_name + "/maf/" + std::to_string(I);
	std::string arg5 = "--save";
	std::string arg6 = arguments::out_file_name + "/save/save";
	// 创建字符指针数组
	std::vector<char*> args;
	args.push_back(const_cast<char*>(arg0.c_str()));
	args.push_back(const_cast<char*>(arg1.c_str()));
	args.push_back(const_cast<char*>(arg2.c_str()));
	args.push_back(const_cast<char*>(arg3.c_str()));
	std::string arg4 = std::to_string(inThreads);
	args.push_back(const_cast<char*>(arg4.c_str()));
	args.push_back(const_cast<char*>(arg5.c_str()));
	args.push_back(const_cast<char*>(arg6.c_str()));
	args.push_back(nullptr);  // 最后一个元素必须是空指针

	// 创建字符指针数组的副本
	char** argv = new char* [args.size()];
	for (size_t i = 0; i < args.size(); ++i) {
		argv[i] = args[i];
	}

	pid_t pid;

	{
		std::lock_guard<std::mutex> lock(threadPool0->mutex_pid);
		pid = fork();
	}

	if (pid < 0) {
		std::cerr << "Error: fork failed." << std::endl;
		return;
	}
	else if (pid == 0) {
		int exitval = 0;
		// 子进程执行函数2的代码
		if (FAorMA) //true maf q
			exitval = execvp("mynucq", argv);
		else       //false fasta 1
			exitval = execvp("mynuc1", argv);

		delete[]argv;
		exit(0);
	}
	else {
		// 父进程继续执行其他操作
		int status;
		waitpid(pid, &status, 0);  // 等待子进程结束
	}
	return;

	return;
}
void BinaryMUMLoad(int I, std::vector<std::string>& files, int inThreads, bool FAorMA)
{
	// 构造参数字符串
	std::string arg0;
	if (FAorMA) //true maf q
		arg0 = "mynucq";
	else       //false fasta 1
		arg0 = "mynuc1";
	std::string arg1 = files[0];
	std::string arg2 = files[I];
	std::string arg3 = arguments::out_file_name + "/maf/" + std::to_string(I);
	std::string arg5 = "--load";
	std::string arg6 = arguments::out_file_name + "/save/save";
	// 创建字符指针数组
	std::vector<char*> args;
	args.push_back(const_cast<char*>(arg0.c_str()));
	args.push_back(const_cast<char*>(arg1.c_str()));
	args.push_back(const_cast<char*>(arg2.c_str()));
	args.push_back(const_cast<char*>(arg3.c_str()));
	std::string arg4 = std::to_string(inThreads);
	args.push_back(const_cast<char*>(arg4.c_str()));
	args.push_back(const_cast<char*>(arg5.c_str()));
	args.push_back(const_cast<char*>(arg6.c_str()));
	args.push_back(nullptr);  // 最后一个元素必须是空指针

	// 创建字符指针数组的副本
	char** argv = new char* [args.size()];
	for (size_t i = 0; i < args.size(); ++i) {
		argv[i] = args[i];
	}

	pid_t pid;

	{
		std::lock_guard<std::mutex> lock(threadPool0->mutex_pid);
		pid = fork();
	}

	if (pid < 0) {
		std::cerr << "Error: fork failed." << std::endl;
		return;
	}
	else if (pid == 0) {
		int exitval = 0;
		// 子进程执行函数2的代码
		if (FAorMA) //true maf q
			exitval = execvp("mynucq", argv);
		else       //false fasta 1
			exitval = execvp("mynuc1", argv);

		delete[]argv;
		exit(0);
	}
	else {
		// 父进程继续执行其他操作
		int status;
		waitpid(pid, &status, 0);  // 等待子进程结束
	}
	return;
}

void BinaryMUM(int I, std::vector<std::string>& files, int inThreads, bool FAorMA)
{
	// 构造参数字符串
	std::string arg0;
	if (FAorMA) //true maf q
		arg0 = "mynucq";
	else       //false fasta 1
		arg0 = "mynuc1";
	std::string arg1 = files[0];
	std::string arg2 = files[I];
	std::string arg3 = arguments::out_file_name + "/maf/" + std::to_string(I);

	// 创建字符指针数组
	std::vector<char*> args;
	args.push_back(const_cast<char*>(arg0.c_str()));
	args.push_back(const_cast<char*>(arg1.c_str()));
	args.push_back(const_cast<char*>(arg2.c_str()));
	args.push_back(const_cast<char*>(arg3.c_str()));
	std::string arg4 = std::to_string(inThreads);
	args.push_back(const_cast<char*>(arg4.c_str()));
	args.push_back(nullptr);  // 最后一个元素必须是空指针

	// 创建字符指针数组的副本
	char** argv = new char* [args.size()];
	for (size_t i = 0; i < args.size(); ++i) {
		argv[i] = args[i];
	}

	pid_t pid;

	{
		std::lock_guard<std::mutex> lock(threadPool0->mutex_pid);
		pid = fork();
	}

	if (pid < 0) {
		std::cerr << "Error: fork failed." << std::endl;
		return;
	}
	else if (pid == 0) {
		// 子进程执行函数2的代码
		if (FAorMA) //true maf q
			execvp("mynucq", argv);
		else       //false fasta 1
			execvp("mynuc1", argv);
		delete[]argv;
		exit(0);
	}
	else {
		// 父进程继续执行其他操作
		int status;
		waitpid(pid, &status, 0);  // 等待子进程结束
	}
	return;
}

std::string getFileName(const std::string& filePath) {
	// 找到最后一个斜杠的位置
	size_t slashPos = filePath.find_last_of("/");
	std::string fileName;
	if (slashPos != std::string::npos)
		fileName = filePath.substr(slashPos + 1);
	else
		fileName = filePath;
	// 找到最后一个点号的位置
	size_t dotPos = fileName.find_last_of(".");
	if (dotPos != std::string::npos)
		fileName = fileName.substr(0, dotPos);

	// 删除文件名中的点号和空格
	fileName.erase(std::remove(fileName.begin(), fileName.end(), '.'), fileName.end());
	fileName.erase(std::remove(fileName.begin(), fileName.end(), ' '), fileName.end());

	return fileName;
}

void mum_main_psa(int I, std::vector<std::string>& files, std::vector<std::vector<size_t>>& Length_All, std::vector<std::vector<std::string>>& Name_All, int inThreads, bool tag_load)
{
	std::string file_name_A = getFileName(files[0]);
	std::string file_name_B = getFileName(files[I]);
	std::vector<std::string>& NameA = Name_All[0];
	std::vector<std::string>& NameB = Name_All[I];
	std::vector<size_t>& LengthA = Length_All[0];
	std::vector<size_t>& LengthB = Length_All[I];

	size_t max_length_A = 0;
	for (const auto& name : NameA)
		max_length_A = std::max(max_length_A, name.length());
	size_t max_length_B = 0;
	for (const auto& name : NameB)
		max_length_B = std::max(max_length_B, name.length());
	size_t max_name_width = std::max((max_length_A + file_name_A.size()), (max_length_B + file_name_B.size())) + 2;

	std::string delta_file = arguments::out_file_name + "/maf/" + std::to_string(I) + ".delta";
	if (my_mk_dir(arguments::out_file_name + "/maf/" + std::to_string(I)) != 0)
	{
		std::cout << "Folder creation error\n";
		exit(-1);
	}
	std::string sv_file = arguments::out_file_name + "/maf/" + std::to_string(I) + "/";
	std::vector<std::vector<Mum_struct>> Final_Structs(NameA.size());

	//##### 调用二进制
	//std::cout << "Start BinaryMUM\n";
	if ((!tag_load) && (I == 1))
		BinaryMUMSave(I, files, inThreads);
	else
		BinaryMUMLoad(I, files, inThreads);
	//std::cout << "End BinaryMUM\n";
	//##### 读取delta文件，获取同源区间+sv，变换索引，得到方向
	read_delta_get_struct(delta_file, Final_Structs, NameA, NameB);
	//std::cout << "End read_delta_get_struct\n";

	/*
	std::vector<std::unique_ptr<Sequence>> StrA(NameA.size());
	std::vector<std::unique_ptr<Sequence>> StrB(NameB.size());
	{
		std::lock_guard<std::mutex> lock(threadPool0->mutex_fp);
		for (int i = 0; i < NameA.size(); i++)
			StrA[i] = std::make_unique<Sequence>(arguments::out_file_name + "/NoN/0/"+ std::to_string(i));
		for (int i = 0; i < NameB.size(); i++)
			StrB[i] = std::make_unique<Sequence>(arguments::out_file_name + "/NoN/" + std::to_string(I) + "/" + std::to_string(i));
	}*/
	std::string NaA, NaB;
	std::unordered_map<int, std::unique_ptr<Sequence>> mapA;
	std::unordered_map<int, std::unique_ptr<Sequence>> mapB;
	int x;
	std::list<int> sequence_queueA;
	std::string pathA = arguments::out_file_name + "/NoN/0/";
	std::list<int> sequence_queueB;
	std::string pathB = arguments::out_file_name + "/NoN/" + std::to_string(I) + "/";
	x = NameA.size() > 10 ? 10 : NameA.size();
	for (int i = 0; i < x; i++)
	{
		std::lock_guard<std::mutex> lock(threadPool0->mutex_fp);
		sequence_queueA.push_front(i);
		mapA[i] = std::make_unique<Sequence>(pathA + std::to_string(i));
	}
	x = NameB.size() > 10 ? 10 : NameB.size();
	for (int i = 0; i < x; i++)
	{
		std::lock_guard<std::mutex> lock(threadPool0->mutex_fp);
		sequence_queueB.push_front(i);
		mapB[i] = std::make_unique<Sequence>(pathB + std::to_string(i));
	}

	//std::cout << "End StrA\n";
	//std::cout << "Ai\tBi\ta_start\ta_end\tb_start\tb_end\ta_len\tb_len\tstructi.tag\n";
	for (int Fi = 0; Fi < NameA.size(); Fi++)
	{
		auto& Structs = Final_Structs[Fi];
		std::ofstream maf; // 外部声明
		{
			std::lock_guard<std::mutex> lock(threadPool0->mutex_fp); // 在申请file-number时加锁
			std::ofstream tempMaf(sv_file + std::to_string(Fi) + ".maf"); // 内部定义
			std::swap(maf, tempMaf); // 交换对象
		}
		maf << "##maf version=1 scoring=lastz.v1.04.00" << std::endl;
		for (auto& structi : Structs)
		{
			size_t Ai = structi.file1;
			size_t Bi = structi.file2;
			NaA = NameA[Ai];
			NaA.erase(std::remove(NaA.begin(), NaA.end(), '.'), NaA.end());
			NaA.erase(std::remove(NaA.begin(), NaA.end(), ' '), NaA.end());
			NaB = NameB[Bi];
			NaB.erase(std::remove(NaB.begin(), NaB.end(), '.'), NaB.end());
			NaB.erase(std::remove(NaB.begin(), NaB.end(), ' '), NaB.end());
			size_t a_start = structi.start1;
			size_t a_end = structi.end1;
			size_t b_start = structi.start2;
			size_t b_end = structi.end2;
			size_t a_len = a_end - a_start;
			size_t b_len = b_end - b_start;
			//std::cout << Ai << "\t" << Bi << "\t" << a_start << "\t" << a_end << "\t" << b_start << "\t" << b_end << "\t" << a_len << "\t" << b_len << "\t" << structi.tag << "\n";

			auto it = mapA.find(Ai);
			if (it != mapA.end()) {
				// Move the accessed element to the front of the queue
				for (auto qit = sequence_queueA.begin(); qit != sequence_queueA.end(); ++qit) {
					if (*qit == Ai) {
						sequence_queueA.splice(sequence_queueA.begin(), sequence_queueA, qit);
						break;
					}
				}
			}
			else {
				// Create a new Sequence and add it to the front of the queue
				{
					std::lock_guard<std::mutex> lock(threadPool0->mutex_fp);
					mapA[Ai] = std::make_unique<Sequence>(pathA + std::to_string(Ai));
				}
				sequence_queueA.push_front(Ai);

				// Check and possibly remove the last element if queue exceeds max_size
				if (sequence_queueA.size() > 10) {
					int last_index = sequence_queueA.back();
					sequence_queueA.pop_back();
					mapA.erase(last_index); // Automatically destroys the unique_ptr
				}
			}

			it = mapB.find(Bi);
			if (it != mapB.end()) {
				// Move the accessed element to the front of the queue
				for (auto qit = sequence_queueB.begin(); qit != sequence_queueB.end(); ++qit) {
					if (*qit == Bi) {
						sequence_queueB.splice(sequence_queueB.begin(), sequence_queueB, qit);
						break;
					}
				}
			}
			else {
				// Create a new Sequence and add it to the front of the queue
				{
					std::lock_guard<std::mutex> lock(threadPool0->mutex_fp);
					mapB[Bi] = std::make_unique<Sequence>(pathB + std::to_string(Bi));
				}
				sequence_queueB.push_front(Bi);

				// Check and possibly remove the last element if queue exceeds max_size
				if (sequence_queueB.size() > 10) {
					int last_index = sequence_queueB.back();
					sequence_queueB.pop_back();
					mapB.erase(last_index); // Automatically destroys the unique_ptr
				}
			}


			std::string Aq = mapA[Ai]->sub_str(a_start, a_end);

			std::string Bq;
			std::string fx;
			if (structi.tag == -1) {
				Bq = mapB[Bi]->sub_str(b_start, b_end);
				nibu(Bq);
				b_start = LengthB[Bi] - b_end;
				fx = "-";
			}
			else
			{
				fx = "+";
				Bq = mapB[Bi]->sub_str(b_start, b_end);
			}

			align_delta_sv(Aq, Bq, structi.string);

			maf << "a score = 0" << std::endl;
			// 格式化输出
			maf << "s " << std::left << std::setw(max_name_width) << (file_name_A + "." + NaA) << std::right << std::setw(12) << a_start << std::setw(12) << a_len
				<< std::setw(2) << "+" << std::setw(12) << LengthA[Ai] << " " << Aq << std::endl;
			maf << "s " << std::left << std::setw(max_name_width) << (file_name_B + "." + NaB) << std::right << std::setw(12) << b_start << std::setw(12) << b_len
				<< std::setw(2) << fx << std::setw(12) << LengthB[Bi] << " " << Bq << std::endl << std::endl;
		}
		maf.close();

	}
	//std::cout << "End NameA\n";
	return;
}

std::vector<std::vector<utils::Insertion>> mum_merge_results(const std::vector<std::array<std::vector<utils::Insertion>, 2>>& pairwise_gaps)
{
	std::vector<utils::Insertion> final_centre_gaps;
	for (size_t i = 0; i != pairwise_gaps.size(); ++i)
	{
		const auto& curr_centre_gaps = pairwise_gaps[i][0];
		for (size_t lhs_pointer = 0, rhs_pointer = 0; rhs_pointer != curr_centre_gaps.size(); )
		{
			if (lhs_pointer == final_centre_gaps.size())
			{
				final_centre_gaps.insert(final_centre_gaps.cend(), curr_centre_gaps.cbegin() + rhs_pointer, curr_centre_gaps.cend());
				break;
			}

			if (final_centre_gaps[lhs_pointer].index == curr_centre_gaps[rhs_pointer].index)
			{
				if (final_centre_gaps[lhs_pointer].number < curr_centre_gaps[rhs_pointer].number)
					final_centre_gaps[lhs_pointer].number = curr_centre_gaps[rhs_pointer].number;
				++lhs_pointer;
				++rhs_pointer;
			}
			else if (final_centre_gaps[lhs_pointer].index < curr_centre_gaps[rhs_pointer].index)
			{
				++lhs_pointer;
			}
			else
			{
				final_centre_gaps.insert(final_centre_gaps.cbegin() + lhs_pointer, curr_centre_gaps[rhs_pointer]);
				++lhs_pointer; // because of the insert above
				++rhs_pointer;
			}
		}
	}

	std::vector<std::vector<utils::Insertion>> final_sequence_gaps;
	final_sequence_gaps.reserve(pairwise_gaps.size());
	for (size_t i = 0; i != pairwise_gaps.size(); ++i)
	{
		const auto& curr_centre_gaps = pairwise_gaps[i][0];
		const auto& curr_sequence_gaps = pairwise_gaps[i][1];

		std::vector<utils::Insertion> centre_addition;
		centre_addition.reserve(final_centre_gaps.size());
		utils::Insertion::minus(final_centre_gaps.cbegin(), final_centre_gaps.cend(),
			curr_centre_gaps.cbegin(), curr_centre_gaps.cend(),
			std::back_inserter(centre_addition));

		std::vector<utils::Insertion> sequence_addition;
		for (size_t centre_index = 0, sequence_index = 0, centre_gaps_index = 0, sequence_gaps_index = 0, centre_addition_index = 0;
			centre_addition_index != centre_addition.size(); ++centre_addition_index)
		{
			const auto curr_addition = centre_addition[centre_addition_index]; // current addition pending process

			while (centre_index < curr_addition.index)
			{
				size_t centre_distance = centre_gaps_index < curr_centre_gaps.size() ?
					curr_centre_gaps[centre_gaps_index].index - centre_index : std::numeric_limits<size_t>::max();
				size_t sequence_distance = sequence_gaps_index < curr_sequence_gaps.size() ?
					curr_sequence_gaps[sequence_gaps_index].index - sequence_index : std::numeric_limits<size_t>::max();

				size_t step = std::min({ sequence_distance, centre_distance, curr_addition.index - centre_index }); // assure centre_index <= curr_addtion.index
				centre_index += step;
				sequence_index += step;

				if (centre_gaps_index < curr_centre_gaps.size() && curr_centre_gaps[centre_gaps_index].index == centre_index)
					sequence_index += curr_centre_gaps[centre_gaps_index++].number;

				else if (sequence_gaps_index < curr_sequence_gaps.size() && curr_sequence_gaps[sequence_gaps_index].index == sequence_index)
					centre_index += curr_sequence_gaps[sequence_gaps_index++].number;
			}

			if (sequence_addition.size() && sequence_index == sequence_addition.back().index)
				sequence_addition.back().number += curr_addition.number;
			else
				sequence_addition.push_back(utils::Insertion({ sequence_index, curr_addition.number }));
		}

		std::vector<utils::Insertion> indels_of_current_sequence;
		indels_of_current_sequence.reserve(curr_sequence_gaps.size() + sequence_addition.size());
		utils::Insertion::plus(curr_sequence_gaps.cbegin(), curr_sequence_gaps.cend(),
			sequence_addition.cbegin(), sequence_addition.cend(),
			std::back_inserter(indels_of_current_sequence));
		final_sequence_gaps.push_back(indels_of_current_sequence);

		std::vector<utils::Insertion>().swap(centre_addition);
		std::vector<utils::Insertion>().swap(sequence_addition);
		std::vector<utils::Insertion>().swap(indels_of_current_sequence);

	}
	std::vector<utils::Insertion>().swap(final_centre_gaps);
	return std::move(final_sequence_gaps);
}

//多组nucmer同时运行，主函数，多线程
void mum_main(std::vector<std::string>& files, std::vector<std::vector<size_t>>& Length_All, std::vector<std::vector<std::string>>& Name_All, int inThreads, bool tag_load) {
	if (tag_load)
	{
		for (int I = 1; I < files.size(); I++)
		{
			threadPool0->execute(&mum_main_psa, I, std::ref(files), std::ref(Length_All), std::ref(Name_All), inThreads, tag_load);
		}
	}
	else
	{
		mum_main_psa(1, std::ref(files), std::ref(Length_All), std::ref(Name_All), arguments::seq_num, tag_load);
		for (int I = 2; I < files.size(); I++)
		{
			threadPool0->execute(&mum_main_psa, I, std::ref(files), std::ref(Length_All), std::ref(Name_All), inThreads, tag_load);
		}
	}
	threadPool0->waitFinished();
}
