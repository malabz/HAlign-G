#include "mum_main.hpp"
#define DEBUG_PATH 0

#include <filesystem>
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

//序列逆补
void nibu(std::string &str, bool TU)
{
    std::reverse(str.begin(), str.end());
    if (TU)
    {
        std::transform(str.begin(), str.end(), str.begin(), [](char c) {
            switch (c) {
            case 'A':
                return 'T';
            case 'C':
                return 'G';
            case 'T':
                return 'A';
            case 'G':
                return 'C';
            default:
                return c;
            }
            });
    }
    else
    {
        std::transform(str.begin(), str.end(), str.begin(), [](char c) {
            switch (c) {
            case 'A':
                return 'U';
            case 'C':
                return 'G';
            case 'U':
                return 'A';
            case 'G':
                return 'C';
            default:
                return c;
            }
            });

    }
    return ;
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


//读取delta文件，获取同源区间+sv，变换索引，得到方向
bool read_delta_get_struct(std::vector<Mum_struct>& Structs, std::string delta_filename, std::string seq_file, size_t lengthA, size_t lengthB, bool TU) {
    std::ifstream file; // 外部声明

    {
        std::lock_guard<std::mutex> lock(threadPool0->mutex_fp); // 在申请file-number时加锁
        std::ifstream tempFile(delta_filename); // 内部定义
	//std::cout << "now:"<< delta_filename<<"\n";
        if (!tempFile.is_open()) {
            std::cerr << "Error opening file " <<delta_filename<< std::endl;
            return false;
        }
        std::swap(file, tempFile); // 交换对象
    } // tempFile 超出作用域，在其析构时会关闭文件
    
    
    std::string line, tmpline1,tmpline2;
    std::getline(file, line);//第一行 两个路径
    std::getline(file, line);//第二行 NUCMER
    std::getline(file, line);//第三行 > name1 name2 len1 len2

    
    size_t length_a, length_b;
    std::istringstream iss(line);
    iss >> tmpline1 >> tmpline2 >> length_a >> length_b;
    tmpline1 = "";
    tmpline2 = "";
    int NUM1 = 0;
    int NUM0 = 0;

    while (true) {
        std::getline(file, line);
        if (line.empty()) break;
        // 解析文件内容并填充Mum_struct
        std::istringstream iss(line);
        size_t start1, end1, start2, end2;
        iss >> start1 >> end1 >> start2 >> end2;
        if (start2 < end2) {
            NUM1 += (end2 - start2);
        }
        else {
            NUM0 += (start2 - end2);
        }
        Mum_struct current_struct = { start1, end1, start2, end2, 1, {} };
        while (true) {
            std::string string_line;
            std::getline(file, string_line);
            if (string_line == "0") 
                break;
            current_struct.string.push_back(std::stol(string_line));
        }
        Structs.push_back(current_struct);
    }

    
    // 判断方向，转换索引
    bool fangxiang = true;
    if (NUM1 >= NUM0) {
        fangxiang = true; // 正向
        for (auto& struct_item : Structs) {
            struct_item.start1 -= 1;
            if (struct_item.start2 > struct_item.end2) {
                struct_item.tag = -1; // 反转结构变异
                std::swap(struct_item.start2, struct_item.end2);
            }
            struct_item.start2 -= 1;
        }
    }
    else {
        fangxiang = false; // 反向
//# 将原序列反向!!!!!!!!!!!!!   strat       
        std::fstream nibufile;
        {
            std::lock_guard<std::mutex> lock(threadPool0->mutex_fp); 
            std::fstream tmp_nibu(seq_file, std::ios::in | std::ios::out);
            std::swap(nibufile, tmp_nibu); // 交换对象
        }
        
        if (nibufile.is_open())
        {
            std::getline(nibufile, tmpline1);
            std::getline(nibufile, tmpline2);
            nibu(tmpline2, TU);
            nibufile.seekp(0, std::ios::beg);
            nibufile << tmpline1 << '\n' << tmpline2 << "\n";
            tmpline1 = "";
            tmpline2 = "";
        }
        else 
            std::cerr << "无法打开文件" << std::endl;
//# 将原序列反向!!!!!!!!!!!!!!   end
        for (auto& struct_item : Structs) {
            struct_item.start1 -= 1;
            if (struct_item.start2 > struct_item.end2) {
                std::swap(struct_item.start2, struct_item.end2);
            }
            else {
                struct_item.tag = -1; // 反转结构变异
            }
            struct_item.start2 -= 1;
            size_t tmp = lengthB - struct_item.start2;
            struct_item.start2 = lengthB - struct_item.end2;
            struct_item.end2 = tmp;
        }
    }

    std::sort(Structs.begin(), Structs.end(), [](const Mum_struct& a, const Mum_struct& b) {
        return a.start2 < b.start2;
        });

    return fangxiang;
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

void cut_and_get_inserts(std::vector<Mum_struct_insert>& new_SV, std::vector<utils::Insertion>& Ainsert, std::vector<utils::Insertion>& Binsert,
    size_t& new_end1, size_t& new_end2, std::vector<long>& numbers, size_t start1, size_t start2,
    size_t end1, size_t end2, size_t next_start1, size_t next_start2)
{
    Mum_struct_insert new_sv;
    new_sv.end1 = end1;
    new_sv.end2 = end2;
    new_sv.tag = 0;

    // 初始化对齐后的结果字符串
    size_t index = 0;
    size_t num1 = 0, num2 = 0;
    std::vector<utils::Insertion>().swap(Ainsert);
    std::vector<utils::Insertion>().swap(Binsert);

    new_end1 = end1;
    new_end2 = end2;

    for (long num : numbers) {
        if (num > 0) {
            index += num;
            if (!Binsert.empty() && Binsert.back().index == (start2 + index - num2 - 1)) {
                Binsert.back().number += 1;
            }
            else {
                Binsert.push_back({ start2 + index - num2 - 1, 1 });
            }
            num2 += 1;
        }
        else if (num < 0) {
            index += std::abs(num);
            if (!Ainsert.empty() && Ainsert.back().index == (start1 + index - num1 - 1)) {
                Ainsert.back().number += 1;
            }
            else {
                Ainsert.push_back({ start1 + index - num1 - 1, 1 });
            }
            num1 += 1;
        }
    }

    std::vector<utils::Insertion> copyVectorA(Ainsert);
    std::vector<utils::Insertion> copyVectorB(Binsert);

    if (end1 <= next_start1 && end2 <= next_start2) {
        return;
    }
    size_t len1 = 0;
    while (new_end1 > next_start1) {
        if (Ainsert.empty()) {
            new_end1 -= 1;
        }
        else if (Ainsert.back().index < new_end1) {
            new_end1 -= 1;
        }
        else if (Ainsert.back().index == new_end1) {
            Ainsert.back().number -= 1;
            if (Ainsert.back().number == 0) {
                Ainsert.pop_back();
            }
        }
        len1 += 1;
    }


    size_t len2 = 0;
    while (new_end2 > next_start2) {
        if (Binsert.empty()) {
            new_end2 -= 1;
        }
        else if (Binsert.back().index < new_end2) {
            new_end2 -= 1;
        }
        else if (Binsert.back().index == new_end2) {
            Binsert.back().number -= 1;
            if (Binsert.back().number == 0) {
                Binsert.pop_back();
            }
        }
        len2 += 1;
    }

    while (len1 > len2) {
        if (Binsert.empty()) {
            new_end2 -= 1;
        }
        else if (Binsert.back().index < new_end2) {
            new_end2 -= 1;
        }
        else if (Binsert.back().index == new_end2) {
            Binsert.back().number -= 1;
            if (Binsert.back().number == 0) {
                Binsert.pop_back();
            }
        }
        len2 += 1;
    }

    while (len1 < len2) {
        if (Ainsert.empty()) {
            new_end1 -= 1;
        }
        else if (Ainsert.back().index < new_end1) {
            new_end1 -= 1;
        }
        else if (Ainsert.back().index == new_end1) {
            Ainsert.back().number -= 1;
            if (Ainsert.back().number == 0) {
                Ainsert.pop_back();
            }
        }
        len1 += 1;
    }

    new_sv.start1 = new_end1;
    new_sv.start2 = new_end2;
    if ((new_sv.end1 - new_sv.start1 > 10) && (new_sv.end2 - new_sv.start2 > 10));
    else
        return;


    // 同步逆序遍历copyVectorA和Ainsert
    std::reverse_iterator<std::vector<utils::Insertion>::iterator> itA = copyVectorA.rbegin(); // 逆序遍历copyVectorA的起始迭代器
    std::reverse_iterator<std::vector<utils::Insertion>::iterator> itB = Ainsert.rbegin(); // 逆序遍历Ainsert的起始迭代器

    while (itA != copyVectorA.rend()) {
        if (itB != Ainsert.rend())
        {
            if ((*itB).index < (*itA).index)
            {
                new_sv.insertA.insert(new_sv.insertA.begin(), { (*itA).index - new_sv.start1, (*itA).number });
                ++itA;
            }
            else if ((*itB).index == (*itA).index)
            {
                if ((*itB).number == (*itA).number)
                    break;
                else
                {
                    new_sv.insertA.insert(new_sv.insertA.begin(), { (*itA).index - new_sv.start1, (*itA).number - (*itB).number });
                    break;
                }
            }
        }
        else
        {
            new_sv.insertA.insert(new_sv.insertA.begin(), { (*itA).index - new_sv.start1, (*itA).number });
            ++itA;
        }
    }

    itA = copyVectorB.rbegin(); // 逆序遍历copyVectorA的起始迭代器
    itB = Binsert.rbegin(); // 逆序遍历Ainsert的起始迭代器

    while (itA != copyVectorB.rend()) {
        if (itB != Binsert.rend())
        {
            if ((*itB).index < (*itA).index)
            {
                new_sv.insertB.insert(new_sv.insertB.begin(), { (*itA).index - new_sv.start2, (*itA).number });
                ++itA;
            }
            else if ((*itB).index == (*itA).index)
            {
                if ((*itB).number == (*itA).number)
                    break;
                else
                {
                    new_sv.insertB.insert(new_sv.insertB.begin(), { (*itA).index - new_sv.start2, (*itA).number - (*itB).number });
                    break;
                }
            }
        }
        else
        {
            new_sv.insertB.insert(new_sv.insertB.begin(), { (*itA).index - new_sv.start2, (*itA).number });
            ++itA;
        }
    }

    new_SV.push_back(new_sv);

    /*num1 = 0;
    for (auto& ins : new_sv.insertA)
        num1 += ins.number;
    num2 = 0;
    for (auto& ins : new_sv.insertB)
        num2 += ins.number;
    if ((num1 + new_sv.end1 - new_sv.start1) != (num2 + new_sv.end2 - new_sv.start2))
    {

        std::cout << new_sv.insertA.size() << "\t" << new_sv.insertB.size() << "\n";
        std::cout << num1 + new_sv.end1 - new_sv.start1 << "\t" << num2 + new_sv.end2 - new_sv.start2 << "\n\n";
    }*/
    /*
    int numA = 0, numB = 0;
    for (auto& insert : Ainsert)
        numA += insert.number;
    for (auto& insert : Binsert)
        numB += insert.number;
    if((new_end1 - start1 + numA) != (new_end2 - start2 + numB))
        std::cout << new_end1 - start1 + numA << " " << new_end2 - start2 + numB << " ?\n";
    */
}

void BinaryMUMSave(int I, bool FAorMA, size_t inThreads, int filter_level)
{
	// 构造参数字符串
	std::string arg0;

    namespace fs = std::filesystem;
    arg0 = "mynuc";
    std::string dir_to_create = arguments::out_file_name + "/save";
        try {
        if (!fs::exists(dir_to_create)) {
            if (fs::create_directories(dir_to_create)) {
                std::cout << "Directory created: " << dir_to_create << std::endl;
            } else {
                std::cout << "Failed to create directory: " << dir_to_create << std::endl;
            }
        } else {
            std::cout << "Directory already exists: " << dir_to_create << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Error creating directory: " << e.what() << std::endl;
    }

    std::string arg1 = arguments::out_file_name + "/NoN/0";
    std::string arg2 = arguments::out_file_name + "/NoN/" + std::to_string(I);
    std::string arg3 = arguments::out_file_name + "/maf/" + std::to_string(I);
	std::string arg5 = "--save";
	std::string arg6 = arguments::out_file_name + "/save/save";
	std::string arg7 = std::to_string(filter_level);
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
	args.push_back(const_cast<char*>(arg7.c_str()));
	args.push_back(nullptr);  // 最后一个元素必须是空指针

	// 创建字符指针数组的副本
	char** argv = new char* [args.size()];
	for (size_t i = 0; i < args.size(); ++i) {
		argv[i] = args[i];
	}
#if DEBUG_PATH
    std::cout << "Command: ";
    for (size_t i = 0; i < args.size() - 1; ++i) {  // 忽略最后的空指针
        std::cout << argv[i] << " ";
    }
    std::cout << std::endl;
#endif
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
		exitval = execvp(arg0.c_str(), argv);

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

void BinaryMUMLoad(int I, bool FAorMA, size_t inThreads, int filter_level)
{
	// 构造参数字符串
	std::string arg0;



    arg0 = "mynuc";

    std::string arg1 = arguments::out_file_name + "/NoN/0";
    std::string arg2 = arguments::out_file_name + "/NoN/" + std::to_string(I);
    std::string arg3 = arguments::out_file_name + "/maf/" + std::to_string(I);
	std::string arg5 = "--load";
	std::string arg6 = arguments::out_file_name + "/save/save";
	std::string arg7 = std::to_string(filter_level);
	// 创建字符指针数组
	std::vector<char*> args;
	args.push_back(const_cast<char*>(arg0.c_str()));
	args.push_back(const_cast<char*>(arg1.c_str()));
	args.push_back(const_cast<char*>(arg2.c_str()));
	args.push_back(const_cast<char*>(arg3.c_str()));
	std::string arg4 = std::to_string(inThreads);  // signle thread
	args.push_back(const_cast<char*>(arg4.c_str()));
	args.push_back(const_cast<char*>(arg5.c_str()));
	args.push_back(const_cast<char*>(arg6.c_str()));
	args.push_back(const_cast<char*>(arg7.c_str()));
	args.push_back(nullptr);  // 最后一个元素必须是空指针

	// 创建字符指针数组的副本
	char** argv = new char* [args.size()];
	for (size_t i = 0; i < args.size(); ++i) {
		argv[i] = args[i];
	}
#if DEBUG_PATH
    std::cout << "Command: ";
    for (size_t i = 0; i < args.size() - 1; ++i) {  // 忽略最后的空指针
        std::cout << argv[i] << " ";
    }
    std::cout << std::endl;
#endif
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
		exitval = execvp(arg0.c_str(), argv);

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

void BinaryMUM(int I, bool FAorMA)
{
    // 构造参数字符串
    std::string arg0;
    if(FAorMA) //true maf q
        arg0 = "mynucq";
    else       //false fasta 1
        arg0 = "mynuc1";
    std::string arg1 = arguments::out_file_name + "/NoN/0";
    std::string arg2 = arguments::out_file_name + "/NoN/" + std::to_string(I);
    std::string arg3 = arguments::out_file_name + "/maf/" + std::to_string(I);

    // 创建字符指针数组
    std::vector<char*> args;
    args.push_back(const_cast<char*>(arg0.c_str()));
    args.push_back(const_cast<char*>(arg1.c_str()));
    args.push_back(const_cast<char*>(arg2.c_str()));
    args.push_back(const_cast<char*>(arg3.c_str()));
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
        delete []argv;
        exit(0);
    }
    else {
        // 父进程继续执行其他操作
        int status;
        waitpid(pid, &status, 0);  // 等待子进程结束
    }
    return;
}


void mum_main_psa(int I, const std::unique_ptr<Sequence>& seqA, std::vector<std::array<std::vector<utils::Insertion>, 2>>& all_pairwise_gaps, std::vector<std::string>& name, std::vector<size_t>& Length, std::vector<bool>& TU, std::vector<bool>& sign, bool FAorMA, size_t threshold2, size_t inThreads, int filter_level)
{
    std::string nameA = name[0];
    std::string nameB = name[I];
    size_t LengthA = Length[0];
    size_t LengthB = Length[I];
    bool TUi = TU[I];

    sign[I] = true;
    size_t max_name_width = std::max(nameA.length(), nameB.length()) + 1;
    std::string delta_file = arguments::out_file_name + "/maf/" + std::to_string(I) + ".delta";
    std::string seq_file = arguments::out_file_name + "/NoN/" + std::to_string(I);
    std::string sv_file = arguments::out_file_name + "/maf/" + std::to_string(I) + ".maf";
    std::vector<Mum_struct> Structs;
    std::vector<Mum_struct> main_Structs;
    std::vector<Mum_struct> sv_Structs;

    //##### 调用二进制


    if (I == 1)
		BinaryMUMSave(I, FAorMA, inThreads, filter_level);
	else
		BinaryMUMLoad(I, FAorMA, inThreads, filter_level);

    // BinaryMUM(I, FAorMA);

    //##### 读取delta文件，获取同源区间+sv，变换索引，得到方向
    sign[I] = read_delta_get_struct(Structs, delta_file, seq_file, LengthA, LengthB, TU[I]);

    size_t max_weight = 0;
    for (auto& Struct : Structs)
        max_weight += Struct.end2 - Struct.start2;
    //std::cout << "max_weight_all " << max_weight<<"\n";


    //##### ref最大权上升子序列（关键路径）
    max_weight_increasing_subsequence(Structs);
    /*
    std::cout << "Structs:" << Structs.size() << "\n";
    for (int i = 0; i < Structs.size(); i++)
        std::cout << Structs[i].start1 << "\t" << Structs[i].end1 << "\t" << Structs[i].start2 << "\t" << Structs[i].end2 << "\t" << Structs[i].tag << "\n";
    */

    //打开seqB
    auto seqB = std::make_unique<Sequence>(seq_file);
    std::array<std::vector<utils::Insertion>, 2>& pairwise_gaps = all_pairwise_gaps[I];
    std::vector<utils::Insertion>& A_insert_all = pairwise_gaps[0]; //A的全部插空信息（不包括精细比对的非同源区间）
    std::vector<utils::Insertion>& B_insert_all = pairwise_gaps[1]; //B的全部插空信息（不包括精细比对的非同源区间）
    std::vector<utils::Insertion> Ainsert; //A的全部插空信息（不包括精细比对的非同源区间）
    std::vector<utils::Insertion> Binsert; //B的全部插空信息（不包括精细比对的非同源区间）
    quadras diff_area; // 精细比对的非同源区间
    quadras fasta_area;// 所有非同源区间

    //for (auto& Struct : Structs)
        //std::cout << Struct.start1 << "\t" << Struct.end1 << "\t" << Struct.start2 << "\t" << Struct.end2 << "\t" << Struct.tag << "\n";

    //##### 分开关键路径和sv，记录关键路径1-1是否有sv
    int pre_struct = 1;
    for (auto& struct_item : Structs)
        if (struct_item.tag == 1) { // 关键路径
            if (pre_struct == 0) {
                struct_item.tag = 0;
            }
            pre_struct = 1;
            main_Structs.push_back(struct_item);
        }
        else { // 结构变异
            sv_Structs.push_back(struct_item);
            pre_struct = 0;
        }


    /*
    if (FAorMA)
    {
        std::sort(main_Structs.begin(), main_Structs.end(), [](const Mum_struct& a, const Mum_struct& b) {
            return a.start1 < b.start1;
            });
        std::ofstream maf; // 外部声明
        {
            std::lock_guard<std::mutex> lock(threadPool0->mutex_fp); // 在申请file-number时加锁
            std::ofstream tempMaf(arguments::out_file_name + "/maf/" + std::to_string(I) + "_main.maf"); // 内部定义
            std::swap(maf, tempMaf); // 交换对象
        } // tempMaf 超出作用域，在其析构时会关闭文件
        maf << "##maf version=1 scoring=lastz.v1.04.00" << std::endl;
        for (auto& structi : main_Structs)
        {
            size_t a_start = structi.start1;
            size_t a_end = structi.end1;
            size_t b_start = structi.start2;
            size_t b_end = structi.end2;
            size_t a_len = a_end - a_start;
            size_t b_len = b_end - b_start;
            std::string Aq = seqA->substr(a_start, a_end);
            std::string Bq;
            if (structi.tag == -1) {
                Bq = seqB->substr(b_start, b_end);
                nibu(Bq, TU[I]);
                b_start = LengthB - b_end;
            }
            else
                Bq = seqB->substr(b_start, b_end);

            align_delta_sv(Aq, Bq, structi.string);

            std::string fx;
            if ((sign[I] && structi.tag != -1) || (!sign[I] && structi.tag == -1))
                fx = "+";
            else
                fx = "-";

            maf << "a score = 0" << std::endl;
            // 格式化输出
            maf << "s " << std::left << std::setw(max_name_width) << nameA << std::right << std::setw(12) << a_start << std::setw(12) << a_len
                << std::setw(2) << "+" << std::setw(12) << LengthA << " " << Aq << std::endl;
            maf << "s " << std::left << std::setw(max_name_width) << nameB << std::right << std::setw(12) << b_start << std::setw(12) << b_len
                << std::setw(2) << fx << std::setw(12) << LengthB << " " << Bq << std::endl << std::endl;
        }
        maf.close();  // 关闭文件
        //##### 0 或者 - 1的 结构变异按maf写出。end
    }

    exit(5);
    */

    std::vector<Mum_struct_insert> new_SV;
    size_t new_end1 = 0, new_end2 = 0;
    if (main_Structs.size() > 0) {
        if (0 == main_Structs[0].start1 && 0 == main_Structs[0].start2) {
            // do nothing
        }
        else if (0 == main_Structs[0].start1) {
            A_insert_all.push_back({ 0, main_Structs[0].start2 });
        }
        else if (0 == main_Structs[0].start2) {
            B_insert_all.push_back({ 0, main_Structs[0].start1 });
        }
        else {
            if (main_Structs[0].start1 == Structs[0].start1) {
                //if (FAorMA) diff_area.push_back({ 0, (int)main_Structs[0].start1, 0, (int)main_Structs[0].start2, 1 });
                //else 
                fasta_area.push_back({ 0, (int)main_Structs[0].start1, 0, (int)main_Structs[0].start2, 1 });
            }
            else {
                /*if (FAorMA)
                {
                    if (main_Structs[0].start1 < main_Structs[0].start2) {
                        A_insert_all.push_back({ main_Structs[0].start1, main_Structs[0].start2 - main_Structs[0].start1 });
                    }
                    else if (main_Structs[0].start1 > main_Structs[0].start2) {
                        B_insert_all.push_back({ main_Structs[0].start2, main_Structs[0].start1 - main_Structs[0].start2 });
                    }
                }else*/
                fasta_area.push_back({ 0, (int)main_Structs[0].start1, 0, (int)main_Structs[0].start2, 0 });
            }
        }
    }
    else {
        //if (FAorMA)diff_area.push_back({ 0, (int)LengthA, 0, (int)LengthB, 1 });
        //else 
        fasta_area.push_back({ 0, (int)LengthA, 0, (int)LengthB, 1 });

    }
    size_t cut_length = 0;
    for (int i = 1; i <= main_Structs.size(); i++) {
        auto pre_struct = main_Structs[i - 1];
        if (i == main_Structs.size()) {
            cut_and_get_inserts(new_SV, Ainsert, Binsert, new_end1, new_end2, pre_struct.string, pre_struct.start1,
                pre_struct.start2,
                pre_struct.end1, pre_struct.end2, LengthA,
                LengthB);
            cut_length += new_end2 - pre_struct.start2;
            /*if (pre_struct.start1 > new_end1 || pre_struct.start2 > new_end2)
                std::cout << new_end1 << "!:\t" << pre_struct.start1 << "\t" << pre_struct.end1 << "\t" << LengthA << "\n"
                << new_end2 << "!:\t" << pre_struct.start2 << "\t" << pre_struct.end2 << "\t" << LengthB << "\n\n";*/
            A_insert_all.insert(A_insert_all.end(), Ainsert.begin(), Ainsert.end());
            B_insert_all.insert(B_insert_all.end(), Binsert.begin(), Binsert.end());
            if (new_end1 == LengthA && new_end2 == LengthB) {
                // do nothing
            }
            else if (new_end1 == LengthA) {
                A_insert_all.push_back({ LengthA, LengthB - new_end2 });
            }
            else if (new_end2 == LengthB) {
                B_insert_all.push_back({ LengthB, LengthA - new_end1 });
            }
            else {
                if (main_Structs.back().start1 == Structs.back().start1) {
                    //if (FAorMA) diff_area.push_back({ (int)new_end1, (int)LengthA, (int)new_end2, (int)LengthB, 1 });
                    //else 
                    fasta_area.push_back({ (int)new_end1, (int)LengthA, (int)new_end2, (int)LengthB, 1 });
                }
                else {
                    /*if (FAorMA)
                    {
                        if ((LengthA - new_end1) < (LengthB - new_end2)) {
                            A_insert_all.push_back({ LengthA, (LengthB - new_end2 - LengthA + new_end1) });
                        }
                        else if ((LengthA - new_end1) > (LengthB - new_end2)) {
                            B_insert_all.push_back({ LengthB, (LengthA - new_end1 - LengthB + new_end2) });
                        }
                    }else*/
                    fasta_area.push_back({ (int)new_end1, (int)LengthA, (int)new_end2, (int)LengthB, 0 });
                }
            }
        }
        else {
            auto struct_ = main_Structs[i];
            cut_and_get_inserts(new_SV, Ainsert, Binsert, new_end1, new_end2, pre_struct.string, pre_struct.start1, pre_struct.start2,
                pre_struct.end1, pre_struct.end2, struct_.start1, struct_.start2);
            cut_length += new_end2 - pre_struct.start2;
            /*if (pre_struct.start1 > new_end1 || pre_struct.start2 > new_end2)
                std::cout << new_end1 << ":\t" << pre_struct.start1 << "\t" << pre_struct.end1 << "\t" << struct_.start1 << "\n"
                << new_end2 << ":\t" << pre_struct.start2 << "\t" << pre_struct.end2 << "\t" << struct_.start2 << "\n\n";*/
            A_insert_all.insert(A_insert_all.end(), Ainsert.begin(), Ainsert.end());
            B_insert_all.insert(B_insert_all.end(), Binsert.begin(), Binsert.end());
            if (new_end1 == struct_.start1 && new_end2 == struct_.start2) {
                // do nothing
            }
            else if (new_end1 == struct_.start1) {
                A_insert_all.push_back({ (size_t)new_end1, struct_.start2 - new_end2 });
            }
            else if (new_end2 == struct_.start2) {
                B_insert_all.push_back({ (size_t)new_end2, struct_.start1 - new_end1 });
            }
            else {
                if (struct_.tag == 1) {
                    //if (FAorMA) diff_area.push_back({ (int)new_end1, (int)struct_.start1, (int)new_end2, (int)struct_.start2, 1 });
                    //else 
                    fasta_area.push_back({ (int)new_end1, (int)struct_.start1, (int)new_end2, (int)struct_.start2, 1 });
                }
                else {
                    /*if (FAorMA)
                    {
                        if ((struct_.start1 - new_end1) < (struct_.start2 - new_end2)) {
                            A_insert_all.push_back({ struct_.start1, (struct_.start2 - new_end2 - struct_.start1 + new_end1) });
                        }
                        else if ((struct_.start1 - new_end1) > (struct_.start2 - new_end2)) {
                            B_insert_all.push_back({ struct_.start2, (struct_.start1 - new_end1 - struct_.start2 + new_end2) });
                        }
                    }else */
                    fasta_area.push_back({ (int)new_end1, (int)struct_.start1, (int)new_end2, (int)struct_.start2, 0 });
                }
            }
        }
    }
    //std::cout << "cut_length " << cut_length << "\n";

    if (FAorMA)//maf=1//##### 0 或者 - 1的 结构变异按maf写出。start
    {
        std::sort(sv_Structs.begin(), sv_Structs.end(), [](const Mum_struct& a, const Mum_struct& b) {
            return a.start1 < b.start1;
            });
        std::ofstream maf; // 外部声明
        {
            std::lock_guard<std::mutex> lock(threadPool0->mutex_fp); // 在申请file-number时加锁
            std::ofstream tempMaf(sv_file); // 内部定义
            std::swap(maf, tempMaf); // 交换对象
        } // tempMaf 超出作用域，在其析构时会关闭文件
        maf << "##maf version=1 scoring=lastz.v1.04.00" << std::endl;
        for (auto& structi : sv_Structs)
        {
            size_t a_start = structi.start1;
            size_t a_end = structi.end1;
            size_t b_start = structi.start2;
            size_t b_end = structi.end2;
            size_t a_len = a_end - a_start;
            size_t b_len = b_end - b_start;
            std::string Aq = seqA->substr(a_start, a_end);
            std::string Bq;
            if (structi.tag == -1) {
                Bq = seqB->substr(b_start, b_end);
                nibu(Bq, TU[I]);
                b_start = LengthB - b_end;
            }
            else
                Bq = seqB->substr(b_start, b_end);

            align_delta_sv(Aq, Bq, structi.string);

            std::string fx;
            if ((sign[I] && structi.tag != -1) || (!sign[I] && structi.tag == -1))
                fx = "+";
            else
                fx = "-";

            maf << "a score = 0" << std::endl;
            // 格式化输出
            maf << "s " << std::left << std::setw(max_name_width) << nameA << std::right << std::setw(12) << a_start << std::setw(12) << a_len
                << std::setw(2) << "+" << std::setw(12) << LengthA << " " << Aq << std::endl;
            maf << "s " << std::left << std::setw(max_name_width) << nameB << std::right << std::setw(12) << b_start << std::setw(12) << b_len
                << std::setw(2) << fx << std::setw(12) << LengthB << " " << Bq << std::endl << std::endl;
        }
        for (auto& structi : new_SV)
        {
            size_t a_start = structi.start1;
            size_t a_end = structi.end1;
            size_t b_start = structi.start2;
            size_t b_end = structi.end2;
            size_t a_len = a_end - a_start;
            size_t b_len = b_end - b_start;
            std::string Aq = seqA->substr(a_start, a_end);
            std::string Bq;
            if (structi.tag == -1) {
                Bq = seqB->substr(b_start, b_end);
                nibu(Bq, TU[I]);
                b_start = LengthB - b_end;
            }
            else
                Bq = seqB->substr(b_start, b_end);

            align_insert_sv(Aq, Bq, structi.insertA, structi.insertB);

            std::string fx;
            if ((sign[I] && structi.tag != -1) || (!sign[I] && structi.tag == -1))
                fx = "+";
            else
                fx = "-";

            maf << "a score = 0" << std::endl;
            // 格式化输出
            maf << "s " << std::left << std::setw(max_name_width) << nameA << std::right << std::setw(12) << a_start << std::setw(12) << a_len
                << std::setw(2) << "+" << std::setw(12) << LengthA << " " << Aq << std::endl;
            maf << "s " << std::left << std::setw(max_name_width) << nameB << std::right << std::setw(12) << b_start << std::setw(12) << b_len
                << std::setw(2) << fx << std::setw(12) << LengthB << " " << Bq << std::endl << std::endl;
        }
        maf.close();  // 关闭文件
        //##### 0 或者 - 1的 结构变异按maf写出。end
    }

    //##### 精细比对非同源区间
    //##### 返回插空信息
    std::vector<utils::Insertion>().swap(Ainsert); //A的插空信息-精细比对的非同源区间
    std::vector<utils::Insertion>().swap(Binsert); //B的插空信息-精细比对的非同源区间
    quadras& diffs = fasta_area;
    Kband* kband = threadMAP[std::this_thread::get_id()];
    for (auto& arr : diffs)
    {
        auto [lhs_gaps, rhs_gaps] = mum_main_Align(kband, seqA, arr[0], arr[1], seqB, arr[2], arr[3], threshold2); //非同源区域，动态规划比对分治到比thresh小，然后k-band
        for (int ii = 0; ii < lhs_gaps.size(); ii++)
        {
            pairwise_gaps[0].push_back(utils::Insertion({ (size_t)std::get<0>(lhs_gaps[ii]),(size_t)std::get<1>(lhs_gaps[ii]) }));
        }
        for (int ii = 0; ii < rhs_gaps.size(); ii++)
        {
            pairwise_gaps[1].push_back(utils::Insertion({ (size_t)std::get<0>(rhs_gaps[ii]),(size_t)std::get<1>(rhs_gaps[ii]) }));
        }
        /*
       int numA = 0, numB = 0;
       for (auto& insert : lhs_gaps)
           numA += std::get<1>(insert);
       for (auto& insert : rhs_gaps)
           numB += std::get<1>(insert);
       if((arr[1] - arr[0] + numA) != (arr[3] - arr[2] + numB))
           std::cout << arr[0] <<"\t"<<arr[1] << "\t" << arr[2] << "\t" << arr[3] << "\t" <<  numA << "\t" << numB << " ?\n";
       */
        insert().swap(lhs_gaps);
        insert().swap(rhs_gaps);
    }

    // 按照 index 进行排序
    std::sort(pairwise_gaps[0].begin(), pairwise_gaps[0].end(), [](const utils::Insertion& a, const utils::Insertion& b) {
        return a.index < b.index;
        });
    // 按照 index 进行排序
    std::sort(pairwise_gaps[1].begin(), pairwise_gaps[1].end(), [](const utils::Insertion& a, const utils::Insertion& b) {
        return a.index < b.index;
        });
    size_t i = 0;
    while (i < pairwise_gaps[0].size())
    {
        if (pairwise_gaps[0][i].number == 0)
            pairwise_gaps[0].erase(pairwise_gaps[0].begin() + i);
        else if ((i < pairwise_gaps[0].size() - 1) && (pairwise_gaps[0][i].index == pairwise_gaps[0][i + 1].index))
        {
            pairwise_gaps[0][i].number += pairwise_gaps[0][i + 1].number;
            pairwise_gaps[0].erase(pairwise_gaps[0].begin() + i + 1);
        }
        else
            i++;
    }i = 0;
    while (i < pairwise_gaps[1].size())
    {
        if (pairwise_gaps[1][i].number == 0)
            pairwise_gaps[1].erase(pairwise_gaps[1].begin() + i);
        else if ((i < pairwise_gaps[1].size() - 1) && (pairwise_gaps[1][i].index == pairwise_gaps[1][i + 1].index))
        {
            pairwise_gaps[1][i].number += pairwise_gaps[1][i + 1].number;
            pairwise_gaps[1].erase(pairwise_gaps[1].begin() + i + 1);
        }
        else
            i++;
    }
    
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
std::vector<std::vector<utils::Insertion>> mum_main(std::vector<std::string>& name, std::vector<size_t>& Length, std::vector<size_t>& non_Length, std::vector<bool>& TU, std::vector<bool>& sign, bool FAorMA, size_t threshold2, size_t inThreads, int filter_level) {
    /////////////////////////////////index -3 !
    for (int i = 0; i < threadPool0->Thread_num; i++)
        threadMAP[threadPool0->workers[i].get_id()] = new Kband();
    std::vector<std::array<std::vector<utils::Insertion>, 2>> all_pairwise_gaps(name.size());
    auto seq_ref = std::make_unique<Sequence>(arguments::out_file_name + "/NoN/0");
    
    threadPool0->execute(&mum_main_psa, 1, std::cref(seq_ref), std::ref(all_pairwise_gaps), std::ref(name), std::ref(Length), std::ref(TU), std::ref(sign), FAorMA, threshold2, inThreads, filter_level);
    threadPool0->waitFinished();
    for (int I = 2; I < name.size(); I++)
    {
        threadPool0->execute(&mum_main_psa, I, std::cref(seq_ref), std::ref(all_pairwise_gaps), std::ref(name), std::ref(Length), std::ref(TU), std::ref(sign), FAorMA, threshold2, inThreads, filter_level);
        //sign[I] = mum_main_psa(I, seq_ref, name[0], name[I], Length[0], Length[I], TU[I]);//return sign[i]
    }
    threadPool0->waitFinished();
    for (int i = 0; i < threadPool0->Thread_num; i++)
        delete threadMAP[threadPool0->workers[i].get_id()];
    return std::move(mum_merge_results(all_pairwise_gaps));
}
