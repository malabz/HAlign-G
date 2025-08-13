#include "Utils.hpp"
#include "Pseudo.hpp"
#include "Arguments.hpp"

#include <stdio.h>
#include <regex>
#include <iostream>
#include <limits>
#include <iomanip>
#include <algorithm>
#include <list>
#include <fstream>
#include <unordered_map>//头文件
#include <filesystem>
#if defined(_WIN32)
#include <io.h> 
#include <direct.h>
#elif defined(__unix__) || defined(__unix) || defined(unix)
#include <sys/stat.h>
#include <unistd.h>
#endif

int my_mk_dir(std::string output_dir)
{
#if defined(_WIN32)
    if (0 != access(output_dir.c_str(), 0))
    {
        return mkdir(output_dir.c_str());
    }
    else
        return 0;
#elif defined(__unix__) || defined(__unix) || defined(unix)
    if (access(output_dir.c_str(), F_OK) == -1) //如果文件夹不存在
    {
        return mkdir(output_dir.c_str(), S_IRWXU | S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
    }
    else
        return 0;
#else
    return -1;
#endif
    return -1;
}

size_t getPeakRSS()
{
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return (size_t)info.PeakWorkingSetSize;

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
    /* AIX and Solaris ------------------------------------------ */
    struct psinfo psinfo;
    int fd = -1;
    if ((fd = open("/proc/self/psinfo", O_RDONLY)) == -1)
        return (size_t)0L;        /* Can't open? */
    if (read(fd, &psinfo, sizeof(psinfo)) != sizeof(psinfo))
    {
        close(fd);
        return (size_t)0L;        /* Can't read? */
    }
    close(fd);
    return (size_t)(psinfo.pr_rssize * 1024L);
#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
    /* BSD, Linux, and OSX -------------------------------------- */
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
#if defined(__APPLE__) && defined(__MACH__)
    return (size_t)rusage.ru_maxrss;
#else
    return (size_t)(rusage.ru_maxrss * 1024L);
#endif
#else
    /* Unknown OS ----------------------------------------------- */
    return (size_t)0L;            /* Unsupported. */
#endif
}

#if defined(_WIN32)
#include <filesystem>
void getFiles_win(std::string path, std::vector<std::string>& files)
{
    //文件句柄
    intptr_t hFile = 0;
    //文件信息
    struct _finddata_t fileinfo;
    std::string p;
    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
    {
        do
        {
            //如果是目录,迭代之
            //如果不是,加入列表
            if ((fileinfo.attrib & _A_SUBDIR))
            {
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
                    getFiles_win(p.assign(path).append("\\").append(fileinfo.name), files);
            }
            else
            {
                files.push_back(p.assign(path).append(fileinfo.name));
            }
        } while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
}
#elif defined(__unix__) || defined(__unix) || defined(unix)
void getFiles_linux(std::string path, std::vector<std::string>& filenames)
{
    DIR* pDir = NULL;
    struct dirent* ptr = NULL;
    if (!(pDir = opendir(path.c_str()))) {
        std::cout << "Folder doesn't Exist!" << std::endl;
        return;
    }
    while ((ptr = readdir(pDir)) != 0) {
        if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0) {
            filenames.push_back(path + ptr->d_name);
        }
    }
    closedir(pDir);
}
#endif

//删除文件
bool deleteFile(std::string path) {
#ifdef _WIN32
    std::wstring widePath(path.begin(), path.end());
    LPCWSTR filePath = widePath.c_str();
    if (DeleteFile(filePath)) {
        return true;
    }
    else {
        std::cout << "Error deleting file: " << path << std::endl;
        return false;
    }
#else
    const char* filePath = path.c_str();
    if (remove(filePath) == 0) {
        return true;
    }
    else {
        std::cout << "Error deleting file: " << path << std::endl;
        return false;
    }
#endif
}
//删除文件夹
bool deleteDirectory(std::string path) {
#ifdef _WIN32
    try
    {
        std::filesystem::remove_all(path);
        return true;
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "Error deleting directory: " << e.what() << '\n';
        return false;
    }
#else
    const char* filePath = path.c_str();
    char command[1024];
    sprintf(command, "rm -rf %s", filePath);

    if (system(command) == 0) {
        return true;
    }
    else {
        std::cout << "Error deleting directory: " << path << std::endl;
        return false;
    }
#endif
}
//移动文件
void renameAndMoveFile(const std::string& filePath) {
#ifdef _WIN32
    // 获取文件名和目标路径
    std::filesystem::path oldPath(filePath);
    std::filesystem::path newPath = oldPath.parent_path().parent_path() / "SV.maf";

    // 检查文件是否存在
    if (!std::filesystem::exists(oldPath)) {
        std::cerr << "File does not exist." << std::endl;
        return;
    }

    // 重命名文件
    std::filesystem::rename(oldPath, newPath);
#else
    // 获取文件名和目标路径
    std::string oldPath = filePath;
    size_t lastSlash = oldPath.find_last_of('/');
    std::string newPath = oldPath.substr(0, lastSlash - 3) + "SV.maf";

    // 检查文件是否存在
    if (access(oldPath.c_str(), F_OK) == -1) {
        std::cerr << "File does not exist." << std::endl;
        return;
    }

    // 重命名文件
    if (rename(oldPath.c_str(), newPath.c_str()) != 0) {
        std::cerr << "Failed to rename and move file." << std::endl;
        return;
    }
#endif
}


//移动文件
void renameFile(const std::string& oldPath, const std::string& newPath) {
#ifdef _WIN32
    // 检查文件是否存在
    if (!std::filesystem::exists(oldPath)) {
        std::cerr << "File does not exist." << std::endl;
        return;
    }

    // 重命名文件
    std::filesystem::rename(oldPath, newPath);
#else
    // 检查文件是否存在
    if (access(oldPath.c_str(), F_OK) == -1) {
        std::cerr << "File does not exist." << std::endl;
        return;
    }

    // 重命名文件
    if (rename(oldPath.c_str(), newPath.c_str()) != 0) {
        std::cerr << "Failed to rename and move file." << std::endl;
        return;
    }
#endif
}


int NSCORE = 0;
int HOXD70[6][6] = { {},{0,91,-114,-31,-123,NSCORE},{0,-114,100,-125,-31,NSCORE},{0,-31,-125,100,-114,NSCORE},
    {0,-123,-31,-114,91,NSCORE},{0,NSCORE,NSCORE,NSCORE,NSCORE,NSCORE} };//评分矩阵
int cs[8] = { 0,91,100,100,91,0,0,0 };
int d = 400, e = 30; //首个gap和后续gap
int stop_g = 5;
char chars[8] = { 'A','C','G','T','N','N','-','-' };

//std::map<unsigned char, unsigned char> dick{
//    {'A', '\1'}, {'C', '\2'}, {'G', '\3'}, {'T', '\4'}, {'N', '\5'}, {'-', '\7'}
//};
char t100[100];
char t10000[10000];
void cout_cur_time()
{
    auto now = std::chrono::system_clock::now();
    //通过不同精度获取相差的毫秒数
    uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
        - std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
    time_t tt = std::chrono::system_clock::to_time_t(now);
    auto time_tm = localtime(&tt);
    char strTime[25] = { 0 };
    sprintf(strTime, "%d-%02d-%02d %02d:%02d:%02d | ", time_tm->tm_year + 1900,
        time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
        time_tm->tm_min, time_tm->tm_sec);
    std::cout << strTime;
}

inline UCHAR utils::get_char(utils::seq_NCBI2NA& Seq, int i)
{
    return NCBI2NA_UNPACK_BASE(Seq.seq[i / 4], (3 - i % 4));
};

void utils::push_char(utils::seq_NCBI2NA& Seq, UCHAR c)
{
    switch (Seq.length % 4)
    {
    case 0:Seq.seq[Seq.length / 4] |= ((c & 3) << 6); break;
    case 1:Seq.seq[Seq.length / 4] |= ((c & 3) << 4); break;
    case 2:Seq.seq[Seq.length / 4] |= ((c & 3) << 2); break;
    case 3:Seq.seq[Seq.length / 4] |= (c & 3); break;
    default:
        break;
    }
    Seq.length++;
}

UCHAR* utils::BLAST_PackDNA(std::string buffer)
{
    int length = buffer.size();
    int new_length = length / 4 + 1;
    int index, new_index, shift;
    UCHAR* new_buffer = new UCHAR[new_length];
    for (index = 0, new_index = 0; new_index < new_length - 1; ++new_index, index += 4)
    {
        new_buffer[new_index] =
            ((buffer[index] & 3) << 6) |
            ((buffer[index + 1] & 3) << 4) |
            ((buffer[index + 2] & 3) << 2) |
            (buffer[index + 3] & 3);
    }
    /* Handle the last byte of the compressed sequence.
      Last 2 bits of the last byte tell the number of valid
      packed sequence bases in it. */
    new_buffer[new_index] = length % 4;
    for (; index < length; index++) {
        switch (index % 4) {
        case 0: shift = 6; break;
        case 1: shift = 4; break;
        case 2: shift = 2; break;
        default: abort();     /* should never happen */
        }
        new_buffer[new_index] |= ((buffer[index] & 3) << shift);
    }
    return new_buffer;
}

UCHAR* utils::BLAST_PackDNA(std::vector<unsigned char>& buffer)
{
    int length = buffer.size();
    int new_length = length / 4 + 1;
    int index, new_index, shift;
    UCHAR* new_buffer = new UCHAR[new_length];
    for (index = 0, new_index = 0; new_index < new_length - 1; ++new_index, index += 4)
    {
        new_buffer[new_index] =
            ((buffer[index] & 3) << 6) |
            ((buffer[index + 1] & 3) << 4) |
            ((buffer[index + 2] & 3) << 2) |
            (buffer[index + 3] & 3);
    }
    /* Handle the last byte of the compressed sequence.
      Last 2 bits of the last byte tell the number of valid
      packed sequence bases in it. */
    new_buffer[new_index] = length % 4;
    for (; index < length; index++) {
        switch (index % 4) {
        case 0: shift = 6; break;
        case 1: shift = 4; break;
        case 2: shift = 2; break;
        default: abort();     /* should never happen */
        }
        new_buffer[new_index] |= ((buffer[index] & 3) << shift);
    }
    return new_buffer;
}

UCHAR* utils::BLAST_PackDNA(std::string buffer, UCHAR* new_buffer)
{
    int length = buffer.size();
    int new_length = length / 4 + 1;
    int index, new_index, shift;
    //UCHAR* new_buffer = new UCHAR[new_length];
    for (index = 0, new_index = 0; new_index < new_length - 1; ++new_index, index += 4)
    {
        new_buffer[new_index] =
            ((buffer[index] & 3) << 6) |
            ((buffer[index + 1] & 3) << 4) |
            ((buffer[index + 2] & 3) << 2) |
            (buffer[index + 3] & 3);
    }
    /* Handle the last byte of the compressed sequence.
      Last 2 bits of the last byte tell the number of valid
      packed sequence bases in it. */
    new_buffer[new_index] = length % 4;
    for (; index < length; index++) {
        switch (index % 4) {
        case 0: shift = 6; break;
        case 1: shift = 4; break;
        case 2: shift = 2; break;
        default: abort();     /* should never happen */
        }
        new_buffer[new_index] |= ((buffer[index] & 3) << shift);
    }
    return new_buffer;
}

std::vector<unsigned char> utils::bin_to_vector(utils::seq_NCBI2NA& Seq, int start, int end)
{
    std::vector<unsigned char> ans(end - start);
    int i = start;
    int j = 0;
    for (; i < end; i++, j++)
        ans[j] = NCBI2NA_UNPACK_BASE(Seq.seq[i / 4], (3 - i % 4));
    return std::move(ans);
}

std::vector<unsigned char> utils::maf_bin_to_vector(utils::seq_NCBI2NA& Seq)
{
    std::vector<unsigned char> ans(Seq.length);
    for (int i = 0; i < Seq.length; i++)
        ans[i] = NCBI2NA_UNPACK_BASE(Seq.seq[i / 4], (3 - i % 4)) + 1;
    return std::move(ans);
}

UCHAR* utils::copy_DNA(UCHAR* buffer, int start, int end)
{
    int length = end - start;
    if (length == 0)
        return new UCHAR[1];
    int new_length = (end - 1) / 4 - start / 4 + 1;
    int index, new_index;
    UCHAR* new_buffer = new UCHAR[new_length];
    for (index = start / 4, new_index = 0; new_index < new_length; ++new_index, ++index)
        new_buffer[new_index] = buffer[index];
    switch (start % 4)
    {
    case 0:break;
    case 1:
        for (new_index = 0; new_index < new_length - 1; ++new_index)
            new_buffer[new_index] = new_buffer[new_index] << 2 | ((new_buffer[new_index + 1] >> 6) & 3);
        new_buffer[new_index] = new_buffer[new_index] << 2;
        break;
    case 2:
        for (new_index = 0; new_index < new_length - 1; ++new_index)
            new_buffer[new_index] = new_buffer[new_index] << 4 | ((new_buffer[new_index + 1] >> 4) & 15);
        new_buffer[new_index] = new_buffer[new_index] << 4;
        break;
    case 3:
        for (new_index = 0; new_index < new_length - 1; ++new_index)
            new_buffer[new_index] = new_buffer[new_index] << 6 | ((new_buffer[new_index + 1] >> 2) & 63);
        new_buffer[new_index] = new_buffer[new_index] << 6;
        break;
    default:
        break;
    }
    return new_buffer;
}

//先按正串索引取正串，再逆补。注意传入时需要把逆串索引转为正串索引
UCHAR* utils::copy_DNA_ni(UCHAR* buffer, int start, int end)
{
    int length = end - start;
    if (length == 0)
        return new UCHAR[1];
    int new_length = (end - 1) / 4 - start / 4 + 1;
    int index, new_index;
    UCHAR* new_buffer = new UCHAR[new_length];
    for (index = (end - 1) / 4, new_index = 0; new_index < new_length; ++new_index, --index)
        new_buffer[new_index] = ~(((buffer[index] & 0x03) << 6)
                                + ((buffer[index] & 0x0C) << 2)
                                + ((buffer[index] & 0x30) >> 2)
                                + ((buffer[index] & 0xC0) >> 6));
    switch (end % 4)
    {
    case 0:break;
    case 1:
        for (new_index = 0; new_index < new_length - 1; ++new_index)
            new_buffer[new_index] = new_buffer[new_index] << 6 | ((new_buffer[new_index + 1] >> 2) & 63);
        new_buffer[new_index] = new_buffer[new_index] << 6;
        break;
    case 2:
        for (new_index = 0; new_index < new_length - 1; ++new_index)
            new_buffer[new_index] = new_buffer[new_index] << 4 | ((new_buffer[new_index + 1] >> 4) & 15);
        new_buffer[new_index] = new_buffer[new_index] << 4;
        break;
    case 3:
        for (new_index = 0; new_index < new_length - 1; ++new_index)
            new_buffer[new_index] = new_buffer[new_index] << 2 | ((new_buffer[new_index + 1] >> 6) & 3);
        new_buffer[new_index] = new_buffer[new_index] << 2;
        break;
    default:
        break;
    }
    return new_buffer;
}


UCHAR* utils::copy_DNA(UCHAR* buffer, UCHAR* new_buffer, int start, int end)
{
    //std::cout << start << " " << end << "   1\n";
    int length = end - start;
    int new_length = (end - 1) / 4 - start / 4 + 1;
    int index, new_index;
    //UCHAR* new_buffer = new UCHAR[new_length];
    for (index = start / 4, new_index = 0; new_index < new_length; ++new_index, ++index)
        new_buffer[new_index] = buffer[index];

    switch (start % 4)
    {
    case 0:break;
    case 1:
        for (new_index = 0; new_index < new_length - 1; ++new_index)
            new_buffer[new_index] = new_buffer[new_index] << 2 | ((new_buffer[new_index + 1] >> 6) & 3);
        new_buffer[new_index] = new_buffer[new_index] << 2;
        break;
    case 2:
        for (new_index = 0; new_index < new_length - 1; ++new_index)
            new_buffer[new_index] = new_buffer[new_index] << 4 | ((new_buffer[new_index + 1] >> 4) & 15);
        new_buffer[new_index] = new_buffer[new_index] << 4;
        break;
    case 3:
        for (new_index = 0; new_index < new_length - 1; ++new_index)
            new_buffer[new_index] = new_buffer[new_index] << 6 | ((new_buffer[new_index + 1] >> 2) & 63);
        new_buffer[new_index] = new_buffer[new_index] << 6;
        break;
    default:
        break;
    }
    //std::cout << start << " " << end << "   1\n";
    return new_buffer;
}


void utils::DNA_cout(utils::seq_NCBI2NA& Seq)
{
    for (int i = 0; i < Seq.length; i++)
        std::cout << chars[NCBI2NA_UNPACK_BASE(Seq.seq[i / 4], (3 - i % 4))];
    std::cout << "\n";
}


std::string utils::remove_white_spaces(const std::string& str) //去除空格
{
    static const std::regex white_spaces("\\s+");
    return std::regex_replace(str, white_spaces, "");
}

unsigned char* _get_map() //统一DNA/RNA 及大小写
{
    using namespace nucleic_acid_pseudo;

    static unsigned char map[std::numeric_limits<unsigned char>::max()];
    memset(map, N, sizeof(map));

    // map['-'] = GAP; // we could not process sequences with '-'
    map['c'] = map['C'] = C;
    map['g'] = map['G'] = G;
    map['a'] = map['A'] = A;
    map['t'] = map['T'] = map['u'] = map['U'] = T;
    map['N'] = map['R'] = map['Y'] = map['M'] = map['K'] = map['S'] = map['W'] = map['H'] = map['B'] = map['V'] = map['D'] = N;
    map['n'] = map['r'] = map['y'] = map['m'] = map['k'] = map['s'] = map['w'] = map['h'] = map['b'] = map['v'] = map['d'] = N;
    return map;
}

static const unsigned char* _map = _get_map();


void utils::to_pseudo(std::string& str)//预处理，char->int
{
    for (int i = 0; i < str.size(); i++)
        str[i] = to_pseudo(str[i]);
}

void utils::to_pseudo(std::string& str, std::vector<unsigned char>& seq)//预处理，char->int
{
    seq.resize(str.size());
    for (int i = 0; i < str.size(); i++)
        seq[i] = to_pseudo(str[i]);
}

std::string utils::from_pseudo(const std::vector<unsigned char>& pseu)//后处理，int->char
{
    static constexpr char map[nucleic_acid_pseudo::NUMBER]{ '-', 'c', 'g', 'a', 't', 'n' };

    std::string str;
    str.reserve(pseu.size());

    for (auto i : pseu) str.push_back(map[i]);
    return str;
}


unsigned char utils::to_pseudo(unsigned char ch)  //预处理，char->int
{
    return _map[ch];
}

void utils::read_to_pseudo(std::istream& is, std::ofstream& nf, std::vector<std::string>& name, std::vector<size_t>& Length, std::vector<bool>& TU) //读入数据 
{
    std::ofstream nof(arguments::out_file_name + "/NoN/" + std::to_string(arguments::File_i++), std::ios::binary | std::ios::out); //判断输出noN路径合法否
    if (!nof)
    {
        std::cout << "cannot write file " << arguments::out_file_name + "/NoN/" + std::to_string(arguments::File_i) << '\n';
        exit(1);
    }


    std::string each_line;
    std::string each_sequence;
    std::unordered_map<char, bool> TU_map{ {'U', false},{'T', true} };
    std::unordered_map<char, char> replace_map{
        {'W', 'N'}, {'S', 'N'}, {'Y', 'N'}, {'K', 'N'},
        {'M', 'N'}, {'B', 'N'}, {'D', 'N'}, {'H', 'N'},
        {'V', 'N'}, {'R', 'N'} }; //, {'-', 'N'}
    size_t index = 0, num = 0, ai = 0;
    bool tag;
    bool tagTU;
    bool tagTU_OK;
    char base;
    std::unordered_map<char, char>::iterator iter;
    for (bool flag = false; std::getline(is, each_line); ) {
        if (each_line.size() == 0 || (each_line.size() == 1 && (int)each_line[0] == 13)) //跳过空行
            continue;

        if (each_line[0] == '>') {
            each_line.erase(0, 1);
            if ((int)(*each_line.rbegin()) == 13)//回车 '\r'
                each_line.pop_back();
            each_line.erase(std::remove_if(each_line.begin(), each_line.end(), [](char c) {
                return (c == ' ' || c == '\t');
                }), each_line.end());
            name.push_back(each_line);
            if (flag) {
                nof << ">" << name[name.size() - 2] << "\n";
                tagTU_OK = true;//默认还没得到 T or U
                tagTU = true;//默认DNA
                index = 0;
                num = 0;
                tag = true;
                for (int j = 0; j < each_sequence.size(); j++) {
                    base = std::toupper(each_sequence[j]);
                    iter = replace_map.find(base);
                    if (iter != replace_map.end()) {
                        base = iter->second;
                    }
                    if (base == 'N') {
                        if (tag) //首个N
                            tag = false;
                        num++;
                    }
                    else if (base == '-');
                    else {
                        if (tagTU_OK)
                        {
                            auto TUiter = TU_map.find(base);
                            if (TUiter != TU_map.end()) {
                                tagTU = TUiter->second;
                                tagTU_OK = false;//得到了，以后不用算了
                            }
                        }
                        nof << base;
                        if (num != 0) {
                            nf << index << " " << num << " ";
                            tag = true;
                            num = 0;
                        }
                        index++;
                    }
                }
                nof << "\n";
                nof.close();
                nof.open(arguments::out_file_name + "/NoN/" + std::to_string(arguments::File_i++), std::ios::binary | std::ios::out);
                Length.push_back(index);
                TU.push_back(tagTU);
                if (num != 0)
                    nf << index << " " << num << " ";
                nf << "0 0\n";
                each_sequence.clear();
            }
            else {
                // 这部分是为了处理第一个序列之前的输出（如果有的话）
                flag = true;
            }
        }
        else if (flag) {
            each_sequence += each_line;
            if ((int)(*each_line.rbegin()) == 13)
                each_sequence.pop_back();
        }
    }

    // 处理最后一个序列
    nof << ">" << name[name.size() - 1] << "\n";
    tagTU_OK = true;//默认还没得到 T or U
    tagTU = true;//默认DNA
    index = 0;
    num = 0;
    tag = true;
    for (int j = 0; j < each_sequence.size(); j++) {
        base = std::toupper(each_sequence[j]);
        iter = replace_map.find(base);
        if (iter != replace_map.end()) {
            base = iter->second;
        }
        if (base == 'N') {
            if (tag) //首个N
                tag = false;
            num++;
        }
        else if (base == '-');
        else {
            if (tagTU_OK)
            {
                auto TUiter = TU_map.find(base);
                if (TUiter != TU_map.end()) {
                    tagTU = TUiter->second;
                    tagTU_OK = false;//得到了，以后不用算了
                }
            }
            nof << base;
            if (num != 0) {
                nf << index << " " << num << " ";
                tag = true;
                num = 0;
            }
            index++;
        }
    }
    nof << "\n";
    nof.close();
    Length.push_back(index);
    TU.push_back(tagTU);
    if (num != 0)
        nf << index << " " << num << " ";
    nf << "0 0\n";
    nof.close();
    each_sequence.clear();
}

//mummer 不处理N
void utils::read_to_pseudo(std::istream& is, std::vector<std::string>& name, std::vector<size_t>& Length, std::vector<size_t>& non_Length, std::vector<bool>& TU) //读入数据 
{
    std::ofstream nof(arguments::out_file_name + "/NoN/" + std::to_string(arguments::File_i++), std::ios::binary | std::ios::out); 
    std::string each_line;
    std::string each_sequence;
    std::unordered_map<char, bool> TU_map{ {'U', false}, { 'T', true } };
    std::unordered_map<char, size_t> Num_map{ {'A', 0}, { 'C', 0 }, { 'G', 0 }, { 'T', 0 }, { 'U', 0 } };
    bool tagTU;
    bool tagTU_OK;
    char base;

    size_t all_len = 0;
    std::unordered_map<char, char>::iterator iter;
    for (bool flag = false; std::getline(is, each_line); ) {
        if (each_line.size() == 0 || (each_line.size() == 1 && (int)each_line[0] == 13)) //跳过空行
            continue;

        if (each_line[0] == '>') {
            each_line.erase(0, 1);
            if ((int)(*each_line.rbegin()) == 13)//回车 '\r'
                each_line.pop_back();
            each_line.erase(std::remove_if(each_line.begin(), each_line.end(), [](char c) {
                return (c == ' ' || c == '\t');
                }), each_line.end());
            name.push_back(each_line);
            if (flag) {
                nof << ">0\n";
                tagTU_OK = true;//默认还没得到 T or U
                tagTU = true;//默认DNA

                for (int j = 0; j < each_sequence.size(); j++) {
                    if (std::isalpha(each_sequence[j]))
                    {
                        base = std::toupper(each_sequence[j]);
                        if (tagTU_OK)
                        {
                            auto TUiter = TU_map.find(base);
                            if (TUiter != TU_map.end()) {
                                tagTU = TUiter->second;
                                tagTU_OK = false;//得到了，以后不用算了
                            }
                        }
                        Num_map[base]++;
                        all_len++;
                        nof << base;
                    }
                }
                nof << "\n";
                nof.close();
                nof.open(arguments::out_file_name + "/NoN/" + std::to_string(arguments::File_i++), std::ios::binary | std::ios::out);
                non_Length.push_back(Num_map['A']+ Num_map['C']+ Num_map['G']+ Num_map['T']+ Num_map['U']);
                Length.push_back(all_len);
                all_len = Num_map['A'] = Num_map['C'] = Num_map['G'] = Num_map['T'] = Num_map['U'] = 0;
                TU.push_back(tagTU);
                each_sequence.clear();
            }
            else {
                // 这部分是为了处理第一个序列之前的输出（如果有的话）
                flag = true;
            }
        }
        else if (flag) {
            each_sequence += each_line;
            if ((int)(*each_line.rbegin()) == 13)
                each_sequence.pop_back();
        }
    }

    // 处理最后一个序列
    nof << ">0\n";
    tagTU_OK = true;//默认还没得到 T or U
    tagTU = true;//默认DNA

    for (int j = 0; j < each_sequence.size(); j++) {
        if (std::isalpha(each_sequence[j]))
        {
            base = std::toupper(each_sequence[j]);
            if (tagTU_OK)
            {
                auto TUiter = TU_map.find(base);
                if (TUiter != TU_map.end()) {
                    tagTU = TUiter->second;
                    tagTU_OK = false;//得到了，以后不用算了
                }
            }
            Num_map[base]++;
            all_len++;
            nof << base;
        }
    }
    nof << "\n";
    nof.close();
    non_Length.push_back(Num_map['A'] + Num_map['C'] + Num_map['G'] + Num_map['T'] + Num_map['U']);
    Length.push_back(all_len);
    TU.push_back(tagTU);
    each_sequence.clear();
}

inline std::string extractFirstWord(std::string& each_line) {
    std::string line = each_line;

    // Step 1: Remove the first character
    if (!line.empty()) {
        line.erase(0, 1);
    }

    // Step 2: Remove leading whitespace
    line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](char c) {
        return !std::isspace(c);
        }));

    // Step 3: Find the end of the first word (up to whitespace)
    auto end_it = std::find_if(line.begin(), line.end(), [](char c) {
        return std::isspace(c);
        });

    // Step 4: Extract the first word
    std::string first_word(line.begin(), end_it);

    // Step 5: Ensure first_word does not exceed 254 characters
    if (first_word.length() > 254) {
        first_word.resize(254);
    }

    return first_word;
}

void processFastaFile(int index,  const std::string& filename, std::vector<size_t>& sequence_lengths, std::vector<std::string>& Name_All) {
    std::ofstream nof;
    std::string name="";
    std::string each_line;
    std::string each_sequence;
    size_t Length = 0;
    // Create directory with index name
    std::string dir_name = arguments::out_file_name + "/NoN/" + std::to_string(index) + "/";
    if (my_mk_dir(dir_name) != 0)
    {
        std::cout << dir_name << "; Folder creation error\n";
        exit(-1);
    }

    // Open input fasta file
    std::ifstream is(filename.c_str(), std::ios::binary | std::ios::in);

    size_t FI = 0;
    for (bool flag = false; std::getline(is, each_line); ) {
        if (each_line.size() == 0 || (each_line.size() == 1 && (int)each_line[0] == 13)) //跳过空行
            continue;

        if (each_line[0] == '>') {
            each_line = extractFirstWord(each_line);
            
            if (flag) {
                Name_All.push_back(name);
                sequence_lengths.push_back(each_sequence.size());
                nof << each_sequence;
                nof.close();
                each_sequence.clear();
            }
            else {
                // 这部分是为了处理第一个序列之前的输出（如果有的话）
                flag = true;
            }
            name = each_line;
            nof.open(dir_name + std::to_string(FI), std::ios::binary | std::ios::out);
            FI++;
        }
        else if (flag) {
            each_sequence += each_line;
            if ((int)(*each_line.rbegin()) == 13)
                each_sequence.pop_back();
        }
    }

    // 处理最后一个序列
    Name_All.push_back(name);
    sequence_lengths.push_back(each_sequence.size());
    nof << each_sequence;
    nof.close();
    each_sequence.clear();
}



void utils::get_BIN_seq_from_noN(int id, std::string& cur_line)
{
    std::ifstream is(arguments::out_file_name + "/NoN/" + std::to_string(id), std::ios::binary | std::ios::in); //判断输入路径合法否
    if (!is)
    {
        std::cout << "cannot write file " << arguments::out_file_name + "/NoN/" + std::to_string(id) << '\n';
        exit(1);
    }
    is >> cur_line;
    is >> cur_line;
    is.close();
    to_pseudo(cur_line);

}

void utils::read_BYTE_seq(std::fstream& io, std::vector<unsigned char>& seq)
{
    std::string cur_line;
    io.seekg(0, std::ios::beg);
    io >> cur_line;
    io >> cur_line;
    to_pseudo(cur_line, seq);
    cur_line.clear();
}
void utils::write_BYTE_seq(std::fstream& io, std::vector<unsigned char>& seq, int seek, bool TU)
{
    char chars[8] = { 'A','C','G','T','N','N','-','-' };
    if (!TU)
        chars[3] = 'U';
    io.seekp(seek, std::ios::beg);
    for (int i = 0; i < seq.size(); i++)
        io << chars[seq[i]];
}

void insert_others(utils::Insertion2 That, utils::Insertion2 This, std::vector<std::vector<utils::Insertion2>>& more_insertions,
    int k, int sequence_number, int* ii)
{
    for (int m = 0; m < sequence_number; m++)
    {
        if (m == k)
        {
            while ((ii[m] < more_insertions[m].size()) && (more_insertions[m][ii[m]].index < That.index)) ii[m]++;
            if (ii[m] == more_insertions[m].size())
                more_insertions[m].push_back(That);
            else if (more_insertions[m][ii[m]].index == That.index)
            {
                more_insertions[m][ii[m]].n_num += That.n_num;
                more_insertions[m][ii[m]].gap_num += That.gap_num;
            }
            else
                more_insertions[m].insert(more_insertions[m].begin() + (ii[m]++), That);
        }
        else
        {
            while ((ii[m] < more_insertions[m].size()) && (more_insertions[m][ii[m]].index < This.index)) ii[m]++;
            if (ii[m] == more_insertions[m].size())
                more_insertions[m].push_back(This);
            else if (more_insertions[m][ii[m]].index == This.index)
            {
                more_insertions[m][ii[m]].n_num += This.n_num;
                more_insertions[m][ii[m]].gap_num += This.gap_num;
            }
            else
                more_insertions[m].insert(more_insertions[m].begin() + (ii[m]++), This);
        }
    }
}

size_t utils::mum_insert_and_write_fasta(std::ostream& os, std::vector<std::vector<Insertion>>& insertions, std::vector<std::string>& name, std::vector<bool>& sign, std::vector<size_t>& Length, bool mg_tag)
{
    std::vector<size_t> ALength = Length;
    const size_t sequence_number = insertions.size();
    int all_length = 0, ti = 0, k = 0;
    std::string tmp_vector, cur_seq;

    const auto align_start1 = std::chrono::high_resolution_clock::now(); //记录插入起始时间
    
    for (int i = 0; i < sequence_number; i++)
    {
        std::ifstream is(arguments::out_file_name + "/NoN/" + std::to_string(i), std::ios::binary | std::ios::in); //判断输入路径合法否
        if (!is)
        {
            std::cout << "cannot access file " << arguments::out_file_name + "/NoN/" + std::to_string(i) << '\n';
            exit(1);
        }
        is >> cur_seq;
        is >> cur_seq;
        is.close();
        if (i == 0)
        {
            all_length = Length[i];
            //std::cout << more << " more\n";
            for (int j = 0; j < insertions[i].size(); j++)
                all_length += insertions[i][j].number;
            tmp_vector.resize(all_length);
        }

        ti = 0;
        k = 0;
        for (int j = 0; j < insertions[i].size(); j++)
        {
            std::copy(cur_seq.begin() + k, cur_seq.begin() + insertions[i][j].index, tmp_vector.begin() + ti);
            ti += insertions[i][j].index - k;

            std::fill(tmp_vector.begin() + ti, tmp_vector.begin() + ti + insertions[i][j].number, '-');
            ti += insertions[i][j].number;

            k = insertions[i][j].index;
        }
        std::copy(cur_seq.begin() + k, cur_seq.end(), tmp_vector.begin() + ti);
        /*for (int j = 0; j < insertions[i].size(); j++)
        {
            while (k < insertions[i][j].index)
                tmp_vector[ti++] = cur_seq[k++];
            for (int p = 0; p < insertions[i][j].number; p++)
                tmp_vector[ti++] = '-';
        }
        while (k < cur_seq.size())tmp_vector[ti++] = cur_seq[k++];
        */


        os << "> " << name[i];
        if (sign[i]) os << "(+)\n";
        else os << "(-)\n";
        if (mg_tag) //maf输出两个文件
        {
            std::ofstream osi(arguments::out_file_name + "/fasta/" + std::to_string(i) + ".fasta", std::ios::binary | std::ios::out);
            /*for (k = 0; k < tmp_vector.size(); k++)
            {
                os << tmp_vector[k];
                osi << tmp_vector[k];
            }*/
            std::copy(tmp_vector.begin(), tmp_vector.end(), std::ostream_iterator<char>(os));
            std::copy(tmp_vector.begin(), tmp_vector.end(), std::ostream_iterator<char>(osi));
            osi.close();
        }
        else//fasta只输出一个
        {
            //for (k = 0; k < tmp_vector.size(); k++) os << tmp_vector[k];
            std::copy(tmp_vector.begin(), tmp_vector.end(), std::ostream_iterator<char>(os));
        }
        os << "\n";
    }

    std::string().swap(tmp_vector);
    std::string().swap(cur_seq);
    const auto align_start2 = std::chrono::high_resolution_clock::now(); //记录插入结束时间
    return all_length;
}


//不输出,按内存原串插入 内部使用两条序列的PSA
unsigned int* utils::insert_fasta01(std::array<std::vector<utils::Insertion>, 2>& insertions, size_t L0)
//写回比对结果
{
    const auto align_start1 = std::chrono::high_resolution_clock::now();
    int Ai = 0, Bi = 0, All_len = L0, i = 0, j = 0;
    //std::cout << L0 << " " << All_len << "  All_len\n";
    for (i = 0; i < insertions[0].size(); i++)
        All_len += insertions[0][i].number;
    //std::cout << L0 << " " << All_len << "  All_len\n";
    unsigned char* Seq0 = new unsigned char[All_len];
    memset(Seq0, 1, All_len);
    unsigned char* Seq1 = new unsigned char[All_len];
    memset(Seq1, 1, All_len);
    Ai = 0, i = 0, j = 0;
    while (j < insertions[0].size())
    {
        if (Ai == insertions[0][j].index)
        {
            memset(Seq0 + i, 0, insertions[0][j].number);
            i += insertions[0][j].number;
            j++;
        }
        else if (Ai < insertions[0][j].index)
        {
            Ai++;
            i++;
        }
    }

    Bi = 0, i = 0, j = 0;
    while (j < insertions[1].size())
    {
        if (Bi == insertions[1][j].index)
        {
            memset(Seq1 + i, 0, insertions[1][j].number);
            i += insertions[1][j].number;
            j++;
        }
        else if (Bi < insertions[1][j].index)
        {
            Bi++;
            i++;
        }
    }
    unsigned int* Fasta_Center = new unsigned int[L0];
    Ai = 0; Bi = 0;
    int za = 0, zb = 0;
    for (i = 0; i < All_len; i++)
    {
        if (Seq1[i] == 1)
            Bi++;
        if (Seq0[i] == 1)
            Fasta_Center[Ai++] = Bi;
    }
    /*for (i = 0; i < All_len; i++)
        std::cout << (int)Seq0[i];
    std::cout << "\n";
    for (i = 0; i < All_len; i++)
        std::cout << (int)Seq1[i];
    std::cout << "\n";*/
    /*for (size_t i = 0; i < L0; i++) {
       std::cout << Fasta_Center[i] << " ";
    }*/
    delete[]Seq0;
    delete[]Seq1;
    const auto align_start2 = std::chrono::high_resolution_clock::now(); //记录插入结束时间
    //std::cout << align_start2 - align_start1 << "  insert_fasta01\n";
    return Fasta_Center;
}


//比对两条序列得分
int* utils::Compare_two(std::string& s1, std::string& s2, int d, int e, int m, int mis)//, int d = 3, int e = 1, int m = 1, int mis = -2
{
    int N = 0;
    int length = s1.size();
    int* score_two = new int[2];  //score_two[0]得分,score_two[1]长度
    score_two[1] = length;
    if (length != s2.size())//长度不同，抛出异常
    {
        std::cerr << length << " != " << s2.size() << " !the length of s1 and s2 is wrong!\n";
        exit(1);
    }
    score_two[0] = 0.0;// score_two[0]得分
    bool gap1 = false;   //false代表首个gap
    for (int i = 0; i < length; i++)
    {
        if ((s1[i] != '-') && (s2[i] != '-'))//都不是空格
        {
            if (gap1) gap1 = false; //下一次为首个空格
            if ((s1[i] == 'N') || (s2[i] == 'N')) score_two[0] += N; //匹配N
            else if (s1[i] == s2[i]) score_two[0] += m; //匹配
            else score_two[0] += mis;   // 不匹配
        }
        else if ((s1[i] != '-') || (s2[i] != '-'))
        {
            if (gap1 == false)//首个空格罚分
            {
                score_two[0] -= d; gap1 = true;//下次非首个空格
            }
            else score_two[0] -= e;//非首个空格罚分
        }
    }

    return score_two;   //返回比对得分
}

int utils::get_next_maf(std::ifstream& mfs, struct maf_2_block& mafi, int d, int e, int m, int mis)
{
    int score_two = 0;
    size_t len1, len2;
    std::string s, sign, tmp, name, len;
    std::string A, B;
    std::getline(mfs, name);//score那一行
    //std::cout << name << "   \n";
    if (name.size() == 0)
        return score_two;

    mfs >> s >> name >> mafi.start1 >> mafi.end1 >> sign >> len1 >> A;
    //std::cout << s <<" "<<  name << " " << mafi.start1 << " " << mafi.end1 << " " << sign << " " << len1 << " \n\n" ;
    mfs >> s >> name >> mafi.start2 >> mafi.end2 >> sign >> len2 >> B;
    //std::cout << s << " " << name << " " << mafi.start2 << " " << mafi.end2 << " " << sign << " " << len2 << " \n\n" ;
    std::getline(mfs, name);//换行
    std::getline(mfs, name);//空行
    //std::cout << name << "   \n";
    mafi.end1 += mafi.start1 - 1;//闭区间
    //mafi.sign = false;
    mafi.start2 = len2 - mafi.start2 - mafi.end2;
    mafi.end2 += mafi.start2 - 1;
    /*if (sign[0] == '-')
    {
        mafi.sign = false;
        mafi.start2 = len2 - mafi.start2 - mafi.end2;
        mafi.end2 += mafi.start2 - 1;
    }
    else
    {
        mafi.sign = true;
        mafi.end2 += mafi.start2 - 1;
    }*/
    //std::cout << " " << mafi.start1 << " " << mafi.start2 << " " << mafi.end1 << " " << mafi.end2 << "   \n";
    int N = 0;
    int length = A.size();
    if (length != B.size())//长度不同，抛出异常
    {
        std::cerr << length << " != " << B.size() << " !the length of A and B is wrong!\n";
        exit(1);
    }

    bool gap1 = false;   //false代表首个gap
    for (int i = 0; i < length; i++)
    {
        if ((A[i] != '-') && (B[i] != '-'))//都不是空格
        {
            if (gap1) gap1 = false; //下一次为首个空格
            if ((A[i] == 'N') || (B[i] == 'N')) score_two += N; //匹配N
            else if (A[i] == B[i]) score_two += m; //匹配
            else score_two += mis;   // 不匹配
        }
        else if ((A[i] != '-') || (B[i] != '-'))
        {
            if (gap1 == false)//首个空格罚分
            {
                score_two -= d; gap1 = true;//下次非首个空格
            }
            else score_two -= e;//非首个空格罚分
        }
    }

    return score_two;   //返回比对得分

}

int utils::get_next_sv_maf(std::ifstream& mfs, struct maf_sv_block& mafi, std::string sign_i, int d, int e, int m, int mis)
{
    int score_two = 0;
    mafi.score = score_two;
    size_t len1, len2;
    std::string s, sign, tmp, name, len;
    std::string A, B;
    std::getline(mfs, name);//score那一行
    //std::cout << name << "   \n";
    if (name.size() == 0)
        return score_two;

    mfs >> s >> name >> mafi.start1 >> mafi.end1 >> sign >> len1 >> A;
    //std::cout << s <<" "<<  name << " " << mafi.start1 << " " << mafi.end1 << " " << sign << " " << len1 << " \n\n" ;
    mfs >> s >> name >> mafi.start2 >> mafi.end2 >> sign >> len2 >> B;

    mafi.end1 += mafi.start1 - 1;//闭区间

    if (sign_i == sign)
    {
        mafi.end2 += mafi.start2 - 1;
    }
    else
    {
        mafi.start2 = len2 - mafi.start2 - mafi.end2;
        mafi.end2 += mafi.start2 - 1;
    }

    //std::cout << s << " " << name << " " << mafi.start2 << " " << mafi.end2 << " " << sign << " " << len2 << " \n\n" ;
    std::getline(mfs, name);//换行
    std::getline(mfs, name);//空行

    //std::cout << " " << mafi.start1 << " " << mafi.start2 << " " << mafi.end1 << " " << mafi.end2 << "   \n";
    int N = 0;
    int length = A.size();
    if (length != B.size())//长度不同，抛出异常
    {
        std::cerr << length << " != " << B.size() << " !the length of A and B is wrong!\n";
        exit(1);
    }

    bool gap1 = false;   //false代表首个gap
    for (int i = 0; i < length; i++)
    {
        if ((A[i] != '-') && (B[i] != '-'))//都不是空格
        {
            if (gap1) gap1 = false; //下一次为首个空格
            if ((A[i] == 'N') || (B[i] == 'N')) score_two += N; //匹配N
            else if (A[i] == B[i]) score_two += m; //匹配
            else score_two += mis;   // 不匹配
        }
        else if ((A[i] != '-') || (B[i] != '-'))
        {
            if (gap1 == false)//首个空格罚分
            {
                score_two -= d; gap1 = true;//下次非首个空格
            }
            else score_two -= e;//非首个空格罚分
        }
    }
    mafi.score = score_two;
    return score_two;   //返回比对得分

}

int* utils::maf_Compare_two(std::ifstream& mfs, std::ifstream& mfsv, std::string& s1, std::string& s2, bool signi, int d, int e, int m, int mis)//, int d = 3, int e = 1, int m = 1, int mis = -2
{
    int N = 0;
    int length = s1.size();
    int ai = 0, bi = 0, scorei;

    int* score_two = new int[2];  //score_two[0]得分,score_two[1]长度
    score_two[1] = length;
    if (length != s2.size())//长度不同，抛出异常
    {
        std::cerr << length << " != " << s2.size() << " !the length of s1 and s2 is wrong!\n";
        exit(1);
    }
    score_two[0] = 0.0;// score_two[0]得分
    bool gap1 = false;   //false代表首个gap
    struct maf_2_block mafi;
    std::vector<std::tuple<int, int>> ni_maf;
    bool tag = true;
    int i = 0;
    while (i < length)
    {
        if (tag)
        {
            scorei = get_next_maf(mfs, mafi);

            if (scorei == 0)
            {
                tag = false;
                mafi.start1 = mafi.start2 = mafi.end1 = mafi.end2 = length;
            }
            else
            {
                score_two[0] += scorei;
                ni_maf.push_back(std::make_tuple(mafi.start2, mafi.end2));
            }

            //std::cout << scorei << " " << mafi.start1 << " " << mafi.end1 << " " << mafi.start2 << " " << mafi.end2 << "   \n";
        }
        bool gap1 = false;
        while ((i < length) && (bi < mafi.start2))//(ai < mafi.start1) && 
        {
            if ((s1[i] != '-') && (s2[i] != '-'))//都不是空格
            {
                if (gap1) gap1 = false; //下一次为首个空格
                if ((s1[i] == 'N') || (s2[i] == 'N')) score_two[0] += N; //匹配N
                else if (s1[i] == s2[i]) score_two[0] += m; //匹配
                else score_two[0] += mis;   // 不匹配
                ai++; bi++;
            }
            else if ((s1[i] != '-') || (s2[i] != '-'))
            {
                if (s1[i] != '-')ai++;
                if (s2[i] != '-')bi++;
                if (gap1 == false)//首个空格罚分
                {
                    score_two[0] -= d; gap1 = true;//下次非首个空格
                }
                else score_two[0] -= e;//非首个空格罚分
            }
            i++;
        }
        //std::cout << i <<" " <<ai << " " << mafi.start1 << " " << bi << " " <<mafi.start2 << "   ?\n";
        while ((i < length) && ((bi <= mafi.end2)))//(ai <= mafi.end1) || 
        {
            if (s1[i] != '-')ai++;
            if (s2[i] != '-')bi++;
            i++;
        }
        //std::cout << i << " " << ai << " " << mafi.end1 << " " << bi << " " << mafi.end2 << "   ??\n";
    }


    struct maf_sv_block mafsv;
    std::map<int, maf_sv_block> mp;
    std::string sigstr;
    if (signi)
        sigstr = "+";
    else
        sigstr = "-";

    scorei = get_next_sv_maf(mfsv, mafsv, sigstr);
    while (scorei)
    {
        if (mp.find(mafsv.start2) == mp.end())
            mp[mafsv.start2] = mafsv;
        else if (scorei > mp[mafsv.start2].score)
            mp[mafsv.start2] = mafsv;
        scorei = get_next_sv_maf(mfsv, mafsv, sigstr);
    }

    std::vector<std::pair<int, maf_sv_block>> vec_pair;
    for (auto& p : mp)
        vec_pair.emplace_back(p.first, p.second);
    //std::cout <<" *****6s\n";
    std::sort(vec_pair.begin(), vec_pair.end(), [](const std::pair<int, maf_sv_block>& a, const std::pair<int, maf_sv_block>& b) -> bool {
        return a.first < b.first;
        });
    //std::cout << " *****6e\n";
    std::map<int, maf_sv_block>().swap(mp);
    std::vector<std::tuple<int, int, double>> result;
    //std::cout <<" *****4s\n";
    for (auto i = vec_pair.begin(); i < vec_pair.end(); ++i) {
        if (i < vec_pair.begin()) {
            i = vec_pair.begin();
        }
        auto j = i + 1;
        if (j == vec_pair.end()) {
            // 如果已经到达最后一个区间，则直接将其加入结果向量
            result.push_back(std::make_tuple(i->second.start2, i->second.end2, i->second.score));
            break;
        }

        int start1 = i->second.start2, end1 = i->second.end2;
        int start2 = j->second.start2, end2 = j->second.end2;
        double score1 = i->second.score, score2 = j->second.score;

        if (start2 <= end1) {  // 如果两个区间重叠
            double ratio1 = score1 / (end1 - start1 + 1);
            double ratio2 = score2 / (end2 - start2 + 1);
            if (ratio1 >= ratio2) {
                // 保留第一个区间，收缩第二个区间
                if ((end1 + 1) >= end2)
                    vec_pair.erase(j);
                else
                {
                    j->second.score = ratio2 * (end2 - end1);
                    j->second.start2 = end1 + 1;
                }
                result.push_back(std::make_tuple(i->second.start2, i->second.end2, i->second.score));
            }
            else {
                if ((start2 - 1) <= start1)
                    vec_pair.erase(i--);
                else
                {
                    i->second.score = ratio1 * (start2 - start1);
                    i->second.end2 = start2 - 1;
                    result.push_back(std::make_tuple(i->second.start2, i->second.end2, i->second.score));
                }
            }
        }
        else {
            // 如果两个区间不重叠，则直接将第一个区间加入结果向量
            result.push_back(std::make_tuple(i->second.start2, i->second.end2, i->second.score));
        }
    }
    std::vector<std::pair<int, maf_sv_block>>().swap(vec_pair);
    //std::cout <<  " *****4e\n";
    // 用 j_idx 记录当前处理到 ni_maf 中的第几个区间
    int j_idx = 0;
    //std::cout << " *****5s\n";
    for (auto i = result.begin(); i < result.end();) {

        if (j_idx >= ni_maf.size() || std::get<0>(ni_maf[j_idx]) > std::get<1>(*i)) {
            // ni_maf 中的所有区间都处理完了，或者当前区间不与 ni_maf 中的任何区间重叠
            ++i;
        }
        else {
            // 当前区间与 ni_maf 中的某个区间重叠，计算得分缩减比例
            auto start = std::get<0>(*i), end = std::get<1>(*i);
            auto score = std::get<2>(*i);
            double ratio = score / (end - start + 1);
            while (j_idx < ni_maf.size() && std::get<1>(ni_maf[j_idx]) < start) {
                // 跳过 ni_maf 中在当前区间左边的区间
                ++j_idx;
            }
            while (j_idx < ni_maf.size() && i < result.end() && i >= result.begin() && std::get<0>(ni_maf[j_idx]) <= end) {
                // 计算要缩减的长度和得分
                if (std::get<0>(ni_maf[j_idx]) <= start && std::get<1>(ni_maf[j_idx]) >= end)
                {
                    i = result.erase(i);
                }
                else if (std::get<0>(ni_maf[j_idx]) <= start && std::get<1>(ni_maf[j_idx]) < end)
                {
                    std::get<0>(*i) = std::get<1>(ni_maf[j_idx]) + 1;
                    
                    if ((std::get<1>(*i) - std::get<0>(*i)) > 10)
                    {
                        std::get<2>(*i) = ratio * (std::get<1>(*i) - std::get<0>(*i) + 1);
                        i++;
                    }
                    else
                        i = result.erase(i);
                }
                else if (std::get<0>(ni_maf[j_idx]) > start && std::get<1>(ni_maf[j_idx]) >= end)
                {
                    std::get<1>(*i) = std::get<0>(ni_maf[j_idx]) - 1;
                    
                    if ((std::get<1>(*i) - std::get<0>(*i)) > 10)
                    {
                        std::get<2>(*i) = ratio * (std::get<1>(*i) - std::get<0>(*i) + 1);
                        i++;
                    }
                    else
                        i = result.erase(i);
                }
                else if (std::get<0>(ni_maf[j_idx]) > start && std::get<1>(ni_maf[j_idx]) < end)
                {
                    std::tuple<int, int, double> tmp(*i);
                    std::get<1>(*i) = std::get<0>(ni_maf[j_idx]) - 1;
                    if ((std::get<1>(*i) - std::get<0>(*i)) > 10)
                    { 
                        std::get<2>(*i) = ratio * (std::get<1>(*i) - std::get<0>(*i) + 1);
                        i++;
                    }
                    else
                        i = result.erase(i);

                    std::get<0>(tmp) = std::get<1>(ni_maf[j_idx]) + 1;
                    if ((std::get<1>(tmp) - std::get<0>(tmp)) > 10)
                    {
                        std::get<2>(tmp) = ratio * (std::get<1>(tmp) - std::get<0>(tmp) + 1);
                        i = result.insert(i, tmp);
                        i++;
                    }
                }
               
                ++j_idx;
            }
        }
    }
    //std::cout << " *****5e\n";
    ai = bi = i = 0;
    int preb = 0;
    for (auto& p : result)
    {
        while ((i < length) && (bi < std::get<0>(p)))
        {
            if (s1[i] != '-')ai++;
            if (s2[i] != '-')bi++;
            i++;
        }
        scorei = 0;
        preb = bi;
        while ((i < length) && (bi <= std::get<1>(p)))
        {
            if ((s1[i] != '-') && (s2[i] != '-'))//都不是空格
            {
                if (gap1) gap1 = false; //下一次为首个空格
                if ((s1[i] == 'N') || (s2[i] == 'N'))scorei += N; //匹配N
                else if (s1[i] == s2[i]) scorei += m; //匹配
                else scorei += mis;   // 不匹配
                ai++; bi++;
            }
            else if ((s1[i] != '-') || (s2[i] != '-'))
            {
                if (s1[i] != '-')ai++;
                if (s2[i] != '-')bi++;
                if (gap1 == false)//首个空格罚分
                {
                    scorei -= d; gap1 = true;//下次非首个空格
                }
                else scorei -= e;//非首个空格罚分
            }
            i++;
        }

        if ((bi - preb - 1) == (std::get<1>(p) - std::get<0>(p)) && std::get<2>(p) > scorei)
        {
            score_two[0] = score_two[0] - scorei + std::get<2>(p);
        }
    }
    return score_two;   //返回比对得分
}


//自定义输出依赖函数
typedef struct
{
    int index;
    int value;
}sort_st;
inline bool compare(sort_st a, sort_st b)
{
    return a.value < b.value; //升序排列，如果改为return a.value<b.value，则为降序
}
std::vector<int> index_100(std::vector<int>& nscore, int score100)
{
    int j;
    float sum = 0;
    std::vector<int> ans;
    //std::cout << "? " << score100 << " " << nscore[0] << " " << nscore[1] << " \n";
    for (j = 0; j < nscore.size(); j++)
        if (nscore[j] >= score100)
            break;
    if (j == nscore.size()) return ans;

    std::vector <sort_st> sort_array(nscore.size());
    for (int i = 0; i < nscore.size(); ++i) {
        sort_array[i].index = i;
        sort_array[i].value = nscore[i];
        sum += nscore[i];
    }
    sort(sort_array.begin(), sort_array.end(), compare);
    while (sort_array.size() > 0 && sort_array[0].value <= 0)//负分先删掉
    {
        sum -= sort_array[0].value;
        sort_array.erase(sort_array.begin());
    }
    while (sort_array.size()!=0)//删除低分，直到达到分数阈值
    {
        if (sum / sort_array.size() >= score100)
            break;
        else
        {
            sum -= sort_array[0].value;
            sort_array.erase(sort_array.begin());
        }
    }
    
    for (int i = 0; i < sort_array.size(); ++i) ans.push_back(sort_array[i].index);
    sort(ans.begin(), ans.end());
    return ans;

}
//按用户自定义输出maf

//处理逆补串maf -------|
int sc[8][8] = { {},
    {0,1,-2,-2,-2,0,0,-2},
    {0,-2,1,-2,-2,0,0,-2},
    {0,-2,-2,1,-2,0,0,-2},
    {0,-2,-2,-2,1,0,0,-2},
    {0,0,0,0,0,0,0,-2},
    {0,0,0,0,0,0,0,-2},
    {0,-2,-2,-2,-2,-2,0,-2} };//评分矩阵
//获得最大得分子序列
bool get_max_begin_end(std::vector<unsigned char>& seq1, std::vector<unsigned char>& seq2, int& begin1, int& begin2, int& clen1, int& clen2, int& ss, int& ee)
{
    if (seq1.size() != seq2.size())
    {
        std::cout << "error:The length of the different\n";
        exit(-1);
    }
    int n = seq1.size();
    int f = -1, s, mi = -10;
    for (int i = 0; i < n; i++)//寻找的一个正数
    {
        if (sc[seq1[i]][seq2[i]] > 0)
        {
            f = i;
            break;
        }
        else if (sc[seq1[i]][seq2[i]] > mi)
        {
            mi = sc[seq1[i]][seq2[i]];
            s = i;
        }
    }
    if (f == -1)//如果没有正数，那肯定最大的就是0了
    {
        return false;
    }
    int add = sc[seq1[f]][seq2[f]], sum = sc[seq1[f]][seq2[f]], l = f, r = f, sl = f, sr = f;//把第一个看成当前最大子序列，a也从第一个开始
    for (f++; f < n; f++)
    {
        add += sc[seq1[f]][seq2[f]];//添加元素
        if (add > sum)//更新最大子序列，连同始末位置一起更新
        {
            sum = add;
            l = sl;
            r = f;
        }
        if (add < 0)//成为了累赘，舍弃变为零
        {
            add = 0;
            sl = f + 1;//把始位置也更新掉
        }
    }
    sl = 0;
    for (f = 0; f < l; f++)
    {
        if (seq1[f] != '\7')
            sl++;
    }
    sr = 0;
    for (f = seq2.size() - 1; f > r; f--)
    {
        if (seq2[f] != '\7')
            sr++;
    }

    begin1 += sl;
    begin2 += sr;
    clen1 = 0; clen2 = 0;
    for (f = l; f <= r; f++)
    {
        if (seq1[f] != '\7')
            clen1++;
        if (seq2[f] != '\7')
            clen2++;
    }
    ss = l, ee = r;
    return true;
}
//插入并输出
utils::PSA_ni_block* utils::insertion_gap_out(std::ostream& os, int seqi, std::vector<std::vector<unsigned char>>& sequences, std::vector<std::string>& name,
    std::vector<bool>& sign, utils::m_block& more_block, int nsum1, int nsum2, std::vector<Insertion>& N_insertion1, std::vector<Insertion>& N_insertion2, PSA_ni_block* PSA_head, int thresh1)

{
    std::vector<std::vector<Insertion2>> all_insertions;
    std::vector<Insertion2> i_all_insertions;
    std::vector<unsigned char> tmp_vector;
    int i = 0, j = 0, pre = 0, mi, g_num, k;
    int* ii = new int[2];
    PSA_ni_block* psa_tmp = NULL;
    std::vector<std::vector<Insertion2>> more_insertions(2);
    int a_start = more_block.start1;
    int b_start = more_block.start2;
    int a_len = more_block.end1 - more_block.start1;
    int b_len = more_block.end2 - more_block.start2;

    std::vector<unsigned char> A(a_len);
    std::vector<unsigned char> B(b_len);
    i = 0; j = more_block.start1;
    while (j < more_block.end1) A[i++] = sequences[0][j++];
    i = 0; j = more_block.end2 - 1;
    while (j > more_block.start2) B[i++] = 5 - sequences[seqi][j--];
    B[i] = 5 - sequences[seqi][j];


    for (i = 0; i < N_insertion1.size(); i++)
        N_insertion1[i].index = N_insertion1[i].index - a_start;
    for (i = 0; i < N_insertion2.size(); i++)
        N_insertion2[i].index = more_block.end2 - N_insertion2[i].index;


    std::reverse(N_insertion2.begin(), N_insertion2.end()); //逆置
    //00000000000000
    i = 0; j = 0; g_num = 0, ii[0] = 0; ii[1] = 0; k = 0;
    std::vector<Insertion2>().swap(i_all_insertions);//清空 i_all_insertions
    while (i < more_block.gap1.size() && j < N_insertion1.size())
    {
        if (std::get<0>(more_block.gap1[i]) == N_insertion1[j].index)//相等变number
        {
            g_num += std::get<1>(more_block.gap1[i]);
            if (N_insertion1[j].number > std::get<1>(more_block.gap1[i]))
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap1[i]), (size_t)std::get<1>(more_block.gap1[i]), 0 }));  //gap 个 N
                insert_others(Insertion2({ (size_t)std::get<0>(more_block.gap1[i]) + g_num ,N_insertion1[j].number - std::get<1>(more_block.gap1[i]), 0 }), utils::Insertion2({ (size_t)std::get<0>(more_block.gap1[i]) + g_num ,0, N_insertion1[j].number - std::get<1>(more_block.gap1[i]) }), more_insertions, k, 2, ii);
            }
            else
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap1[i]) ,N_insertion1[j].number, (size_t)std::get<1>(more_block.gap1[i]) - N_insertion1[j].number }));
            }
            i++;
            j++;
        }
        else if (std::get<0>(more_block.gap1[i]) > N_insertion1[j].index) //不等插新
        {
            insert_others(Insertion2({ N_insertion1[j].index + g_num ,N_insertion1[j].number, 0 }), utils::Insertion2({ N_insertion1[j].index + g_num ,0, N_insertion1[j].number }), more_insertions, k, 2, ii);
            j++;
        }
        else
        {
            g_num += std::get<1>(more_block.gap1[i]);
            i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap1[i]), 0, (size_t)std::get<1>(more_block.gap1[i]) }));
            i++;
        }
    }
    while (j < N_insertion1.size()) //后续多余的N，继续插入
    {
        insert_others(Insertion2({ N_insertion1[j].index + g_num ,N_insertion1[j].number, 0 }), utils::Insertion2({ N_insertion1[j].index + g_num ,0, N_insertion1[j].number }), more_insertions, k, 2, ii);
        j++;
    }
    while (i < more_block.gap1.size()) //后续多余的gap，继续插入
    {
        g_num += std::get<1>(more_block.gap1[i]);
        i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap1[i]), 0, (size_t)std::get<1>(more_block.gap1[i]) }));
        i++;
    }
    all_insertions.push_back(i_all_insertions);
    //11111111111111
    i = 0; j = 0; g_num = 0, ii[0] = 0; ii[1] = 0; k = 1;
    std::vector<Insertion2>().swap(i_all_insertions);//清空 i_all_insertions
    while (i < more_block.gap2.size() && j < N_insertion2.size())
    {
        if (std::get<0>(more_block.gap2[i]) == N_insertion2[j].index)//相等变number
        {
            g_num += std::get<1>(more_block.gap2[i]);
            if (N_insertion2[j].number > std::get<1>(more_block.gap2[i]))
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap2[i]), (size_t)std::get<1>(more_block.gap2[i]), 0 }));  //gap 个 N
                insert_others(Insertion2({ (size_t)std::get<0>(more_block.gap2[i]) + g_num ,N_insertion2[j].number - std::get<1>(more_block.gap2[i]), 0 }), utils::Insertion2({ (size_t)std::get<0>(more_block.gap2[i]) + g_num ,0, N_insertion2[j].number - std::get<1>(more_block.gap2[i]) }), more_insertions, k, 2, ii);
            }
            else
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap2[i]) ,N_insertion2[j].number, (size_t)std::get<1>(more_block.gap2[i]) - N_insertion2[j].number }));
            }
            i++;
            j++;
        }
        else if (std::get<0>(more_block.gap2[i]) > N_insertion2[j].index) //不等插新
        {
            insert_others(Insertion2({ N_insertion2[j].index + g_num ,N_insertion2[j].number, 0 }), utils::Insertion2({ N_insertion2[j].index + g_num ,0, N_insertion2[j].number }), more_insertions, k, 2, ii);
            j++;
        }
        else
        {
            g_num += std::get<1>(more_block.gap2[i]);
            i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap2[i]), 0, (size_t)std::get<1>(more_block.gap2[i]) }));
            i++;
        }
    }
    while (j < N_insertion2.size()) //后续多余的N，继续插入
    {
        insert_others(Insertion2({ N_insertion2[j].index + g_num ,N_insertion2[j].number, 0 }), utils::Insertion2({ N_insertion2[j].index + g_num ,0, N_insertion2[j].number }), more_insertions, k, 2, ii);
        j++;
    }
    while (i < more_block.gap2.size()) //后续多余的gap，继续插入
    {
        g_num += std::get<1>(more_block.gap2[i]);
        i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap2[i]), 0, (size_t)std::get<1>(more_block.gap2[i]) }));
        i++;
    }
    all_insertions.push_back(i_all_insertions);

    //消除  一列多个N导致de多个插空
    std::vector<Insertion> multi;
    while (i < more_insertions[0].size() && j < more_insertions[1].size())
    {
        if (more_insertions[0][i].index == more_insertions[1][j].index)
        {
            if (more_insertions[0][i].gap_num == 0 || more_insertions[1][j].gap_num == 0);
            else if (more_insertions[0][i].gap_num > more_insertions[1][j].gap_num)
                multi.push_back(Insertion({ more_insertions[0][i].index ,more_insertions[1][j].gap_num }));
            else multi.push_back(Insertion({ more_insertions[0][i].index ,more_insertions[0][i].gap_num }));
            i++; j++;
        }
        else if (more_insertions[0][i].index > more_insertions[1][j].index) j++;
        else i++;
    }
    //插入到原串
    int more = 0, all_size = 0, ti = 0;
    k = 0;
    //00000000000000
    i = 0;
    more = A.size();
    for (j = 0; j < all_insertions[i].size(); j++)
        more += (all_insertions[i][j].n_num + all_insertions[i][j].gap_num);
    all_size = more;
    mi = 0;
    for (j = 0; j < more_insertions[i].size(); j++)
    {
        all_size += more_insertions[i][j].n_num;
        if (mi < multi.size() && more_insertions[i][j].index == multi[mi].index)
            all_size -= multi[mi++].number;
        all_size += more_insertions[i][j].gap_num;
    }
    tmp_vector.resize(more);
    ti = 0;
    k = 0;
    for (j = 0; j < all_insertions[i].size(); j++)
    {
        while (k < all_insertions[i][j].index)
            tmp_vector[ti++] = A[k++];
        for (int p = 0; p < all_insertions[i][j].n_num; p++)
            tmp_vector[ti++] = '\5';
        for (int p = 0; p < all_insertions[i][j].gap_num; p++)
            tmp_vector[ti++] = '\7';
    }while (k < A.size())tmp_vector[ti++] = A[k++];
    A.resize(all_size);

    mi = 0;
    k = 0;
    ti = 0;
    for (j = 0; j < more_insertions[i].size(); j++)
    {

        while (ti < more_insertions[i][j].index) A[k++] = tmp_vector[ti++];
        for (int p = 0; p < more_insertions[i][j].n_num; p++) A[k++] = '\5';
        if (mi < multi.size() && more_insertions[i][j].index == multi[mi].index)
        {
            for (int p = 0; p < (more_insertions[i][j].gap_num - multi[mi].number); p++)
                A[k++] = '\7';
            mi++;
        }
        else
            for (int p = 0; p < more_insertions[i][j].gap_num; p++)  A[k++] = '\7';
    }
    while (ti < more) A[k++] = tmp_vector[ti++];

    //111111111111111111
    i = 1;
    ti = 0;
    k = 0;
    for (j = 0; j < all_insertions[i].size(); j++)
    {
        while (k < all_insertions[i][j].index)
            tmp_vector[ti++] = B[k++];
        for (int p = 0; p < all_insertions[i][j].n_num; p++)
            tmp_vector[ti++] = '\5';
        for (int p = 0; p < all_insertions[i][j].gap_num; p++)
            tmp_vector[ti++] = '\7';
    }while (k < B.size())tmp_vector[ti++] = B[k++];
    B.resize(all_size);
    mi = 0;
    k = 0;
    ti = 0;
    for (j = 0; j < more_insertions[i].size(); j++)
    {
        while (ti < more_insertions[i][j].index)
            B[k++] = tmp_vector[ti++];
        for (int p = 0; p < more_insertions[i][j].n_num; p++)
            B[k++] = '\5';
        if (mi < multi.size() && more_insertions[i][j].index == multi[mi].index)
        {
            for (int p = 0; p < (more_insertions[i][j].gap_num - multi[mi].number); p++)
                B[k++] = '\7';
            mi++;
        }
        else
            for (int p = 0; p < more_insertions[i][j].gap_num; p++)
                B[k++] = '\7';
    }while (ti < more) B[k++] = tmp_vector[ti++];
    std::vector<unsigned char>().swap(tmp_vector);
    std::vector<std::vector<Insertion2>>().swap(all_insertions);
    std::vector<Insertion2>().swap(i_all_insertions);
    std::vector<std::vector<Insertion2>>().swap(more_insertions);
    std::vector<Insertion>().swap(multi);
    delete[] ii;
    a_start += nsum1;
    b_start += nsum2;
    //os << "start\n\n";
    if (get_max_begin_end(A, B, a_start, b_start, a_len, b_len, i, j) && ((j - i) >= thresh1))
    {
        //原输出
        /*
        int name_len = name[0].size() > name[seqi].size() ? name[0].size() : name[seqi].size();
        os << "a score=0 "<< "\n";
        os << "s " << std::setw(name_len + 1) << std::left << name[0] << std::setw(9) << std::right << a_start << " " << std::setw(9) << std::right << a_len << " + " << std::setw(9) << std::right << final_sequences[0] << " ";
        for (k = i; k <= j; k++) os << chars[A[k]];
        os <<"\n";
        if (sign[seqi])
            os << "s " << std::setw(name_len + 1) << std::left << name[seqi] << std::setw(9) << std::right << b_start << " " << std::setw(9) << std::right << b_len << " - " << std::setw(9) << std::right << final_sequences[seqi] << " ";
        else
            os << "s " << std::setw(name_len + 1) << std::left << name[seqi] << std::setw(9) << std::right << b_start << " " << std::setw(9) << std::right << b_len << " + " << std::setw(9) << std::right << final_sequences[seqi] << " ";

        for (k = i; k <= j; k++) os << chars[B[k]];

        os << "\n\n";
        */
        psa_tmp = new PSA_ni_block();
        psa_tmp->start[0] = a_start;
        psa_tmp->start[1] = b_start;
        psa_tmp->length[0] = a_len;
        psa_tmp->length[1] = b_len;
        psa_tmp->end[0] = a_start + a_len;
        psa_tmp->end[1] = b_start + b_len;
        psa_tmp->a_seq.assign(A.begin() + i, A.begin() + j + 1);
        psa_tmp->b_seq.assign(B.begin() + i, B.begin() + j + 1);
        psa_tmp->sign = sign[seqi] ^ 1;
        psa_tmp->next = NULL;
        PSA_head->next = psa_tmp;
        PSA_head = psa_tmp;
    }
    std::vector<unsigned char>().swap(A);
    std::vector<unsigned char>().swap(B);
    return PSA_head;
}
//处理逆补串maf
utils::PSA_ni_block* utils::insertion_gap_more(std::ostream& os, std::vector<std::vector<unsigned char>>& sequences,
    const std::vector<std::vector<Insertion>>& N_insertions, std::vector<std::string>& name,
    std::vector<bool>& sign, std::vector<utils::more_block>& more_gap, int thresh1) //写回比对结果
{
    int i, j, k, jm, jn, j0, nsum1, nsum2;
    const size_t sequence_number = sequences.size();
    int* len_sequences = new int[sequence_number];
    int* final_sequences = new int[sequence_number];
    std::vector<Insertion> gapn1, gapn2;
    for (k = 0; k < sequence_number; k++)  final_sequences[k] = len_sequences[k] = sequences[k].size();
    for (i = 0; i < sequence_number; i++)
        for (j = 0; j < N_insertions[i].size(); j++)
            final_sequences[i] += N_insertions[i][j].number;
    PSA_ni_block* PSA_head = new PSA_ni_block();
    PSA_head->next = NULL;
    PSA_ni_block* tmp = PSA_head;

    for (i = 1; i < sequence_number; i++)
    {
        jm = jn = j0 = nsum1 = nsum2 = 0;
        while (jm < more_gap[i].size())
        {
            std::vector<Insertion>().swap(gapn1);
            std::vector<Insertion>().swap(gapn2);
            while (j0 < N_insertions[0].size() && jm < more_gap[i].size() && N_insertions[0][j0].index <= more_gap[i][jm].start1)nsum1 += N_insertions[0][j0++].number;
            while (jn < N_insertions[i].size() && jm < more_gap[i].size() && N_insertions[i][jn].index <= more_gap[i][jm].start2)nsum2 += N_insertions[i][jn++].number;
            while (j0 < N_insertions[0].size() && jm < more_gap[i].size() && N_insertions[0][j0].index < more_gap[i][jm].end1)gapn1.push_back(N_insertions[0][j0++]);
            while (jn < N_insertions[i].size() && jm < more_gap[i].size() && N_insertions[i][jn].index < more_gap[i][jm].end2)gapn2.push_back(N_insertions[i][jn++]);
            tmp = insertion_gap_out(os, i, sequences, name, sign, more_gap[i][jm], nsum1, nsum2, gapn1, gapn2, tmp, thresh1);
            jm++;
        }
        utils::more_block().swap(more_gap[i]);
    }
    return PSA_head;
}

bool AB_exist(unsigned int* Fasta_Center, unsigned char* array_A, unsigned char* array_B, size_t start_A, size_t end_A, size_t start_B, size_t end_B)
{
    int intersection_start = std::max((size_t)Fasta_Center[start_A], start_B);
    int intersection_end = std::min((size_t)Fasta_Center[end_A], end_B);
    if ((static_cast<double>(intersection_end - intersection_start) / (end_B - start_B)) >= 0.9)   //应该在sv输出前跟fasta文件比对一下，A区间相同，且B区间也相同，则删除这个sv
        return false;

    size_t num_B = 0;
    for (size_t i = start_B; i < end_B; i++)
        if (array_B[i] != 0)
            num_B++;

    //std::cout <<end_B <<" "<< start_B<<" " << num_B << " nAB ";
    if (num_B > (end_B - start_B) * 0.05)
        return false;
    for (size_t i = start_B; i < end_B; i++)
        if (array_B[i] == 0)
            array_B[i] = 1;

    return true;
}
bool AB_exist(unsigned char* array_A, unsigned char* array_B, size_t start_A, size_t end_A, size_t start_B, size_t end_B)
{
    size_t num_B = 0;
    for (size_t i = start_B; i < end_B; i++)
        if (array_B[i] != 0)
            num_B++;

    //std::cout <<end_B <<" "<< start_B<<" " << num_B << " nAB ";
    if (num_B > (end_B - start_B) * 0.05)
        return false;
    for (size_t i = start_B; i < end_B; i++)
        if (array_B[i] == 0)
            array_B[i] = 1;

    return true;
}
bool AB_exist_ni(unsigned char* array_A, unsigned char* array_B, size_t start_A, size_t end_A, size_t start_B, size_t end_B)
{
    //return true;
    size_t num_A = 0, num_B = 0;
    for (size_t i = start_A; i < end_A; i++)
        if (array_A[i] == 1)
            num_A++;

    for (size_t i = start_B; i < end_B; i++)
        if (array_B[i] == 1)
            num_B++;
    //std::cout << num_A << " " << (end_A - start_A) * 0.05 << " " << num_B << " " << (end_B - start_B) * 0.05 << " nAB ";
    if ((num_B > (end_B - start_B) * 0.05) && (num_A > (end_A - start_A) * 0.05))
    {
        //std::cout << " false\n";
        return false;
    }
    //std::cout << " true\n";
    if (num_B >= num_A)
    {
        for (size_t i = start_A; i < end_A; i++)
            if (array_A[i] != 1)
                array_A[i] = 1;
    }
    else
    {
        for (size_t i = start_B; i < end_B; i++)
            if (array_B[i] != 1)
                array_B[i] = 1;
    }
    return true;
}

//插入并输出
void utils::insertion_gap_out_new(std::ostream& os, int seqi, std::vector<std::vector<unsigned char>>& sequences, std::vector<std::string>& name,
    std::vector<bool>& sign, std::vector<bool>& TU, utils::m_block& more_block, int* final_sequences, int nsum1, int nsum2, std::vector<Insertion>& N_insertion1, std::vector<Insertion>& N_insertion2, int thresh1)
{
    std::vector<std::vector<Insertion2>> all_insertions;
    std::vector<Insertion2> i_all_insertions;
    std::vector<unsigned char> tmp_vector;
    int i = 0, j = 0, pre = 0, mi, g_num, k;
    int* ii = new int[2];
    std::vector<std::vector<Insertion2>> more_insertions(2);
    int a_start = more_block.start1;
    int b_start = more_block.start2;
    int a_len = more_block.end1 - more_block.start1;
    int b_len = more_block.end2 - more_block.start2;

    std::vector<unsigned char> A(a_len);
    std::vector<unsigned char> B(b_len);
    i = 0; j = more_block.start1;
    while (j < more_block.end1) A[i++] = sequences[0][j++];
    i = 0; j = more_block.end2 - 1;
    while (j > more_block.start2) B[i++] = 5 - sequences[seqi][j--];
    B[i] = 5 - sequences[seqi][j];


    for (i = 0; i < N_insertion1.size(); i++)
    {
        N_insertion1[i].index = N_insertion1[i].index - a_start;
        a_len += N_insertion1[i].number;
    }
    for (i = 0; i < N_insertion2.size(); i++)
    {
        N_insertion2[i].index = more_block.end2 - N_insertion2[i].index;
        b_len += N_insertion2[i].number;
    }


    std::reverse(N_insertion2.begin(), N_insertion2.end()); //逆置
    //00000000000000
    i = 0; j = 0; g_num = 0, ii[0] = 0; ii[1] = 0; k = 0;
    std::vector<Insertion2>().swap(i_all_insertions);//清空 i_all_insertions
    while (i < more_block.gap1.size() && j < N_insertion1.size())
    {
        if (std::get<0>(more_block.gap1[i]) == N_insertion1[j].index)//相等变number
        {
            g_num += std::get<1>(more_block.gap1[i]);
            if (N_insertion1[j].number > std::get<1>(more_block.gap1[i]))
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap1[i]), (size_t)std::get<1>(more_block.gap1[i]), 0 }));  //gap 个 N
                insert_others(Insertion2({ (size_t)std::get<0>(more_block.gap1[i]) + g_num ,N_insertion1[j].number - std::get<1>(more_block.gap1[i]), 0 }), utils::Insertion2({ (size_t)std::get<0>(more_block.gap1[i]) + g_num ,0, N_insertion1[j].number - std::get<1>(more_block.gap1[i]) }), more_insertions, k, 2, ii);
            }
            else
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap1[i]) ,N_insertion1[j].number, (size_t)std::get<1>(more_block.gap1[i]) - N_insertion1[j].number }));
            }
            i++;
            j++;
        }
        else if (std::get<0>(more_block.gap1[i]) > N_insertion1[j].index) //不等插新
        {
            insert_others(Insertion2({ N_insertion1[j].index + g_num ,N_insertion1[j].number, 0 }), utils::Insertion2({ N_insertion1[j].index + g_num ,0, N_insertion1[j].number }), more_insertions, k, 2, ii);
            j++;
        }
        else
        {
            g_num += std::get<1>(more_block.gap1[i]);
            i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap1[i]), 0, (size_t)std::get<1>(more_block.gap1[i]) }));
            i++;
        }
    }
    while (j < N_insertion1.size()) //后续多余的N，继续插入
    {
        insert_others(Insertion2({ N_insertion1[j].index + g_num ,N_insertion1[j].number, 0 }), utils::Insertion2({ N_insertion1[j].index + g_num ,0, N_insertion1[j].number }), more_insertions, k, 2, ii);
        j++;
    }
    while (i < more_block.gap1.size()) //后续多余的gap，继续插入
    {
        g_num += std::get<1>(more_block.gap1[i]);
        i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap1[i]), 0, (size_t)std::get<1>(more_block.gap1[i]) }));
        i++;
    }
    all_insertions.push_back(i_all_insertions);
    //11111111111111
    i = 0; j = 0; g_num = 0, ii[0] = 0; ii[1] = 0; k = 1;
    std::vector<Insertion2>().swap(i_all_insertions);//清空 i_all_insertions
    while (i < more_block.gap2.size() && j < N_insertion2.size())
    {
        if (std::get<0>(more_block.gap2[i]) == N_insertion2[j].index)//相等变number
        {
            g_num += std::get<1>(more_block.gap2[i]);
            if (N_insertion2[j].number > std::get<1>(more_block.gap2[i]))
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap2[i]), (size_t)std::get<1>(more_block.gap2[i]), 0 }));  //gap 个 N
                insert_others(Insertion2({ (size_t)std::get<0>(more_block.gap2[i]) + g_num ,N_insertion2[j].number - std::get<1>(more_block.gap2[i]), 0 }), utils::Insertion2({ (size_t)std::get<0>(more_block.gap2[i]) + g_num ,0, N_insertion2[j].number - std::get<1>(more_block.gap2[i]) }), more_insertions, k, 2, ii);
            }
            else
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap2[i]) ,N_insertion2[j].number, (size_t)std::get<1>(more_block.gap2[i]) - N_insertion2[j].number }));
            }
            i++;
            j++;
        }
        else if (std::get<0>(more_block.gap2[i]) > N_insertion2[j].index) //不等插新
        {
            insert_others(Insertion2({ N_insertion2[j].index + g_num ,N_insertion2[j].number, 0 }), utils::Insertion2({ N_insertion2[j].index + g_num ,0, N_insertion2[j].number }), more_insertions, k, 2, ii);
            j++;
        }
        else
        {
            g_num += std::get<1>(more_block.gap2[i]);
            i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap2[i]), 0, (size_t)std::get<1>(more_block.gap2[i]) }));
            i++;
        }
    }
    while (j < N_insertion2.size()) //后续多余的N，继续插入
    {
        insert_others(Insertion2({ N_insertion2[j].index + g_num ,N_insertion2[j].number, 0 }), utils::Insertion2({ N_insertion2[j].index + g_num ,0, N_insertion2[j].number }), more_insertions, k, 2, ii);
        j++;
    }
    while (i < more_block.gap2.size()) //后续多余的gap，继续插入
    {
        g_num += std::get<1>(more_block.gap2[i]);
        i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap2[i]), 0, (size_t)std::get<1>(more_block.gap2[i]) }));
        i++;
    }
    all_insertions.push_back(i_all_insertions);

    //消除  一列多个N导致de多个插空
    std::vector<Insertion> multi;
    while (i < more_insertions[0].size() && j < more_insertions[1].size())
    {
        if (more_insertions[0][i].index == more_insertions[1][j].index)
        {
            if (more_insertions[0][i].gap_num == 0 || more_insertions[1][j].gap_num == 0);
            else if (more_insertions[0][i].gap_num > more_insertions[1][j].gap_num)
                multi.push_back(Insertion({ more_insertions[0][i].index ,more_insertions[1][j].gap_num }));
            else multi.push_back(Insertion({ more_insertions[0][i].index ,more_insertions[0][i].gap_num }));
            i++; j++;
        }
        else if (more_insertions[0][i].index > more_insertions[1][j].index) j++;
        else i++;
    }
    //插入到原串
    int more = 0, all_size = 0, ti = 0;
    k = 0;
    //00000000000000
    i = 0;
    more = A.size();
    for (j = 0; j < all_insertions[i].size(); j++)
        more += (all_insertions[i][j].n_num + all_insertions[i][j].gap_num);
    all_size = more;
    mi = 0;
    for (j = 0; j < more_insertions[i].size(); j++)
    {
        all_size += more_insertions[i][j].n_num;
        if (mi < multi.size() && more_insertions[i][j].index == multi[mi].index)
            all_size -= multi[mi++].number;
        all_size += more_insertions[i][j].gap_num;
    }
    tmp_vector.resize(more);
    ti = 0;
    k = 0;
    for (j = 0; j < all_insertions[i].size(); j++)
    {
        while (k < all_insertions[i][j].index)
            tmp_vector[ti++] = A[k++];
        for (int p = 0; p < all_insertions[i][j].n_num; p++)
            tmp_vector[ti++] = '\5';
        for (int p = 0; p < all_insertions[i][j].gap_num; p++)
            tmp_vector[ti++] = '\7';
    }while (k < A.size())tmp_vector[ti++] = A[k++];
    A.resize(all_size);

    mi = 0;
    k = 0;
    ti = 0;
    for (j = 0; j < more_insertions[i].size(); j++)
    {

        while (ti < more_insertions[i][j].index) A[k++] = tmp_vector[ti++];
        for (int p = 0; p < more_insertions[i][j].n_num; p++) A[k++] = '\5';
        if (mi < multi.size() && more_insertions[i][j].index == multi[mi].index)
        {
            for (int p = 0; p < (more_insertions[i][j].gap_num - multi[mi].number); p++)
                A[k++] = '\7';
            mi++;
        }
        else
            for (int p = 0; p < more_insertions[i][j].gap_num; p++)  A[k++] = '\7';
    }
    while (ti < more) A[k++] = tmp_vector[ti++];

    //111111111111111111
    i = 1;
    ti = 0;
    k = 0;
    for (j = 0; j < all_insertions[i].size(); j++)
    {
        while (k < all_insertions[i][j].index)
            tmp_vector[ti++] = B[k++];
        for (int p = 0; p < all_insertions[i][j].n_num; p++)
            tmp_vector[ti++] = '\5';
        for (int p = 0; p < all_insertions[i][j].gap_num; p++)
            tmp_vector[ti++] = '\7';
    }while (k < B.size())tmp_vector[ti++] = B[k++];
    B.resize(all_size);
    mi = 0;
    k = 0;
    ti = 0;
    for (j = 0; j < more_insertions[i].size(); j++)
    {
        while (ti < more_insertions[i][j].index)
            B[k++] = tmp_vector[ti++];
        for (int p = 0; p < more_insertions[i][j].n_num; p++)
            B[k++] = '\5';
        if (mi < multi.size() && more_insertions[i][j].index == multi[mi].index)
        {
            for (int p = 0; p < (more_insertions[i][j].gap_num - multi[mi].number); p++)
                B[k++] = '\7';
            mi++;
        }
        else
            for (int p = 0; p < more_insertions[i][j].gap_num; p++)
                B[k++] = '\7';
    }while (ti < more) B[k++] = tmp_vector[ti++];
    std::vector<unsigned char>().swap(tmp_vector);
    std::vector<std::vector<Insertion2>>().swap(all_insertions);
    std::vector<Insertion2>().swap(i_all_insertions);
    std::vector<std::vector<Insertion2>>().swap(more_insertions);
    std::vector<Insertion>().swap(multi);
    delete[] ii;
    a_start += nsum1;
    b_start += nsum2;
    i = 0; j = A.size() - 1;
    //os << "start\n\n";

    char charA[8] = { 'N', 'A','C','G','T','N','N','-' };
    char charB[8] = { 'N', 'A','C','G','T','N','N','-' };
    if (!TU[0])
        charA[4] = 'U';
    if (!TU[1])
        charB[4] = 'U';

    //if ((more_block.tag && a_len>=arguments::sv_thresh_len && b_len >= arguments::sv_thresh_len) || (get_max_begin_end(A, B, a_start, b_start, a_len, b_len, i, j) && ((j - i) >= arguments::sv_thresh_len)))//thresh1
        //if (more_block.tag)
    //if ((more_block.tag) || (get_max_begin_end(A, B, a_start, b_start, a_len, b_len, i, j) && ((j - i) >= arguments::sv_thresh_len)))//thresh1
    if (more_block.tag)
    {
        //os << j <<" " << i << " " << thresh1 << "\n";
        //原输出
        int name_len = name[0].size() > name[seqi].size() ? name[0].size() : name[seqi].size();
        os << "a score=0 " << "\n";
        os << "s " << std::setw(name_len + 1) << std::left << name[0] << std::setw(9) << std::right << a_start << " " << std::setw(9) << std::right << a_len << " + " << std::setw(9) << std::right << final_sequences[0] << " ";
        //std::cout << "s " << std::setw(name_len + 1) << std::left << name[0] << std::setw(9) << std::right << a_start << " " << std::setw(9) << std::right << a_len << " + " << std::setw(9) << std::right << final_sequences[0] << " \n";
        for (k = i; k <= j; k++) os << charA[A[k]];
        //ts << a_start << " " << a_start + a_len << " ";
        os << "\n";
        if (!sign[seqi])
            os << "s " << std::setw(name_len + 1) << std::left << name[seqi] << std::setw(9) << std::right << std::right << final_sequences[seqi] - b_start - b_len << " " << std::setw(9) << std::right << b_len << " + " << std::setw(9) << std::right << final_sequences[seqi] << " ";
        else
            os << "s " << std::setw(name_len + 1) << std::left << name[seqi] << std::setw(9) << std::right << std::right << final_sequences[seqi] - b_start - b_len << " " << std::setw(9) << std::right << b_len << " - " << std::setw(9) << std::right << final_sequences[seqi] << " ";

        /*if (sign[seqi])
            os << "s " << std::setw(name_len + 1) << std::left << name[seqi] << std::setw(9) << std::right << std::right << final_sequences[seqi] - b_start - b_len << " " << std::setw(9) << std::right << b_len << " - " << std::setw(9) << std::right << final_sequences[seqi] << " ";
        else
            os << "s " << std::setw(name_len + 1) << std::left << name[seqi] << std::setw(9) << std::right << b_start << " " << std::setw(9) << std::right << b_len << " + " << std::setw(9) << std::right << final_sequences[seqi] << " ";
        */
        for (k = i; k <= j; k++) os << charB[B[k]];
        //ts << b_start << " " << b_start + b_len << " ";
        os << "\n\n";
    }
    std::vector<unsigned char>().swap(A);
    std::vector<unsigned char>().swap(B);
}

//*********************插入并输出SV***************************
void utils::insertion_gap_out_new_sv(std::ostream& os, std::vector<unsigned char>& A, std::vector<unsigned char>& B, std::vector<std::string>& name,
    std::vector<bool>& sign, std::vector<bool>& TU, utils::m_block& more_block, int* final_sequences, int nsum1, int nsum2, std::vector<Insertion>& N_insertion1, std::vector<Insertion>& N_insertion2)
{
    std::vector<std::vector<Insertion2>> all_insertions;
    std::vector<Insertion2> i_all_insertions;
    std::vector<unsigned char> tmp_vector;
    int i = 0, j = 0, pre = 0, mi, g_num, k;
    int* ii = new int[2];
    std::vector<std::vector<Insertion2>> more_insertions(2);
    int a_start = more_block.start1;
    int b_start = more_block.start2;
    int a_len = more_block.end1 - more_block.start1;
    int b_len = more_block.end2 - more_block.start2;


    for (i = 0; i < N_insertion1.size(); i++)
    {
        N_insertion1[i].index = N_insertion1[i].index - a_start;
        a_len += N_insertion1[i].number;
    }
    for (i = 0; i < N_insertion2.size(); i++)
    {
        //N_insertion2[i].index = more_block.end2 - N_insertion2[i].index;
        N_insertion2[i].index = N_insertion2[i].index - b_start;
        b_len += N_insertion2[i].number;
    }


    //std::reverse(N_insertion2.begin(), N_insertion2.end()); //逆置
    //00000000000000
    i = 0; j = 0; g_num = 0, ii[0] = 0; ii[1] = 0; k = 0;
    std::vector<Insertion2>().swap(i_all_insertions);//清空 i_all_insertions
    while (i < more_block.gap1.size() && j < N_insertion1.size())
    {
        if (std::get<0>(more_block.gap1[i]) == N_insertion1[j].index)//相等变number
        {
            g_num += std::get<1>(more_block.gap1[i]);
            if (N_insertion1[j].number > std::get<1>(more_block.gap1[i]))
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap1[i]), (size_t)std::get<1>(more_block.gap1[i]), 0 }));  //gap 个 N
                insert_others(Insertion2({ (size_t)std::get<0>(more_block.gap1[i]) + g_num ,N_insertion1[j].number - std::get<1>(more_block.gap1[i]), 0 }), utils::Insertion2({ (size_t)std::get<0>(more_block.gap1[i]) + g_num ,0, N_insertion1[j].number - std::get<1>(more_block.gap1[i]) }), more_insertions, k, 2, ii);
            }
            else
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap1[i]) ,N_insertion1[j].number, (size_t)std::get<1>(more_block.gap1[i]) - N_insertion1[j].number }));
            }
            i++;
            j++;
        }
        else if (std::get<0>(more_block.gap1[i]) > N_insertion1[j].index) //不等插新
        {
            insert_others(Insertion2({ N_insertion1[j].index + g_num ,N_insertion1[j].number, 0 }), utils::Insertion2({ N_insertion1[j].index + g_num ,0, N_insertion1[j].number }), more_insertions, k, 2, ii);
            j++;
        }
        else
        {
            g_num += std::get<1>(more_block.gap1[i]);
            i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap1[i]), 0, (size_t)std::get<1>(more_block.gap1[i]) }));
            i++;
        }
    }
    while (j < N_insertion1.size()) //后续多余的N，继续插入
    {
        insert_others(Insertion2({ N_insertion1[j].index + g_num ,N_insertion1[j].number, 0 }), utils::Insertion2({ N_insertion1[j].index + g_num ,0, N_insertion1[j].number }), more_insertions, k, 2, ii);
        j++;
    }
    while (i < more_block.gap1.size()) //后续多余的gap，继续插入
    {
        g_num += std::get<1>(more_block.gap1[i]);
        i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap1[i]), 0, (size_t)std::get<1>(more_block.gap1[i]) }));
        i++;
    }
    all_insertions.push_back(i_all_insertions);
    //11111111111111
    i = 0; j = 0; g_num = 0, ii[0] = 0; ii[1] = 0; k = 1;
    std::vector<Insertion2>().swap(i_all_insertions);//清空 i_all_insertions
    while (i < more_block.gap2.size() && j < N_insertion2.size())
    {
        if (std::get<0>(more_block.gap2[i]) == N_insertion2[j].index)//相等变number
        {
            g_num += std::get<1>(more_block.gap2[i]);
            if (N_insertion2[j].number > std::get<1>(more_block.gap2[i]))
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap2[i]), (size_t)std::get<1>(more_block.gap2[i]), 0 }));  //gap 个 N
                insert_others(Insertion2({ (size_t)std::get<0>(more_block.gap2[i]) + g_num ,N_insertion2[j].number - std::get<1>(more_block.gap2[i]), 0 }), utils::Insertion2({ (size_t)std::get<0>(more_block.gap2[i]) + g_num ,0, N_insertion2[j].number - std::get<1>(more_block.gap2[i]) }), more_insertions, k, 2, ii);
            }
            else
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap2[i]) ,N_insertion2[j].number, (size_t)std::get<1>(more_block.gap2[i]) - N_insertion2[j].number }));
            }
            i++;
            j++;
        }
        else if (std::get<0>(more_block.gap2[i]) > N_insertion2[j].index) //不等插新
        {
            insert_others(Insertion2({ N_insertion2[j].index + g_num ,N_insertion2[j].number, 0 }), utils::Insertion2({ N_insertion2[j].index + g_num ,0, N_insertion2[j].number }), more_insertions, k, 2, ii);
            j++;
        }
        else
        {
            g_num += std::get<1>(more_block.gap2[i]);
            i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap2[i]), 0, (size_t)std::get<1>(more_block.gap2[i]) }));
            i++;
        }
    }
    while (j < N_insertion2.size()) //后续多余的N，继续插入
    {
        insert_others(Insertion2({ N_insertion2[j].index + g_num ,N_insertion2[j].number, 0 }), utils::Insertion2({ N_insertion2[j].index + g_num ,0, N_insertion2[j].number }), more_insertions, k, 2, ii);
        j++;
    }
    while (i < more_block.gap2.size()) //后续多余的gap，继续插入
    {
        g_num += std::get<1>(more_block.gap2[i]);
        i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_block.gap2[i]), 0, (size_t)std::get<1>(more_block.gap2[i]) }));
        i++;
    }
    all_insertions.push_back(i_all_insertions);

    //消除  一列多个N导致de多个插空
    std::vector<Insertion> multi;
    while (i < more_insertions[0].size() && j < more_insertions[1].size())
    {
        if (more_insertions[0][i].index == more_insertions[1][j].index)
        {
            if (more_insertions[0][i].gap_num == 0 || more_insertions[1][j].gap_num == 0);
            else if (more_insertions[0][i].gap_num > more_insertions[1][j].gap_num)
                multi.push_back(Insertion({ more_insertions[0][i].index ,more_insertions[1][j].gap_num }));
            else multi.push_back(Insertion({ more_insertions[0][i].index ,more_insertions[0][i].gap_num }));
            i++; j++;
        }
        else if (more_insertions[0][i].index > more_insertions[1][j].index) j++;
        else i++;
    }
    //插入到原串
    int more = 0, all_size = 0, ti = 0;
    k = 0;
    //00000000000000
    i = 0;
    more = A.size();
    for (j = 0; j < all_insertions[i].size(); j++)
        more += (all_insertions[i][j].n_num + all_insertions[i][j].gap_num);
    all_size = more;
    mi = 0;
    for (j = 0; j < more_insertions[i].size(); j++)
    {
        all_size += more_insertions[i][j].n_num;
        if (mi < multi.size() && more_insertions[i][j].index == multi[mi].index)
            all_size -= multi[mi++].number;
        all_size += more_insertions[i][j].gap_num;
    }
    tmp_vector.resize(more);
    ti = 0;
    k = 0;
    for (j = 0; j < all_insertions[i].size(); j++)
    {
        while (k < all_insertions[i][j].index)
            tmp_vector[ti++] = A[k++];
        for (int p = 0; p < all_insertions[i][j].n_num; p++)
            tmp_vector[ti++] = '\5';
        for (int p = 0; p < all_insertions[i][j].gap_num; p++)
            tmp_vector[ti++] = '\7';
    }while (k < A.size())tmp_vector[ti++] = A[k++];
    A.resize(all_size);

    mi = 0;
    k = 0;
    ti = 0;
    for (j = 0; j < more_insertions[i].size(); j++)
    {

        while (ti < more_insertions[i][j].index) A[k++] = tmp_vector[ti++];
        for (int p = 0; p < more_insertions[i][j].n_num; p++) A[k++] = '\5';
        if (mi < multi.size() && more_insertions[i][j].index == multi[mi].index)
        {
            for (int p = 0; p < (more_insertions[i][j].gap_num - multi[mi].number); p++)
                A[k++] = '\7';
            mi++;
        }
        else
            for (int p = 0; p < more_insertions[i][j].gap_num; p++)  A[k++] = '\7';
    }
    while (ti < more) A[k++] = tmp_vector[ti++];

    //111111111111111111
    i = 1;
    ti = 0;
    k = 0;
    for (j = 0; j < all_insertions[i].size(); j++)
    {
        while (k < all_insertions[i][j].index)
            tmp_vector[ti++] = B[k++];
        for (int p = 0; p < all_insertions[i][j].n_num; p++)
            tmp_vector[ti++] = '\5';
        for (int p = 0; p < all_insertions[i][j].gap_num; p++)
            tmp_vector[ti++] = '\7';
    }while (k < B.size())tmp_vector[ti++] = B[k++];
    B.resize(all_size);
    mi = 0;
    k = 0;
    ti = 0;
    for (j = 0; j < more_insertions[i].size(); j++)
    {
        while (ti < more_insertions[i][j].index)
            B[k++] = tmp_vector[ti++];
        for (int p = 0; p < more_insertions[i][j].n_num; p++)
            B[k++] = '\5';
        if (mi < multi.size() && more_insertions[i][j].index == multi[mi].index)
        {
            for (int p = 0; p < (more_insertions[i][j].gap_num - multi[mi].number); p++)
                B[k++] = '\7';
            mi++;
        }
        else
            for (int p = 0; p < more_insertions[i][j].gap_num; p++)
                B[k++] = '\7';
    }while (ti < more) B[k++] = tmp_vector[ti++];
    std::vector<unsigned char>().swap(tmp_vector);
    std::vector<std::vector<Insertion2>>().swap(all_insertions);
    std::vector<Insertion2>().swap(i_all_insertions);
    std::vector<std::vector<Insertion2>>().swap(more_insertions);
    std::vector<Insertion>().swap(multi);
    delete[] ii;
    a_start += nsum1;
    b_start += nsum2;
    i = 0; j = A.size() - 1;
    
    
    //score
    int maf_score = 0, common_num = 0;
    bool tag_first = true;
    for (k = i; k <= j; k++)
    {
        if (A[k] != '\7' && A[k] != '\5' && A[k] == B[k])
            common_num++;

        if ((A[k] == '\7') || (B[k] == '\7'))
        {
            if (tag_first) { maf_score -= d; tag_first = false; }
            else  maf_score -= e;
        }
        else { maf_score += HOXD70[A[k]][B[k]]; tag_first = true; }
    }

    //os << "start\n\n";
    char charA[8] = { 'N', 'A','C','G','T','N','N','-' };
    char charB[8] = { 'N', 'A','C','G','T','N','N','-' };
    if (!TU[0])
        charA[4] = 'U';
    if (!TU[1])
        charB[4] = 'U';

    //if (more_block.tag && a_len>= arguments::sv_thresh_len && b_len>= arguments::sv_thresh_len)
    if (more_block.tag)
    {
        //os << j <<" " << i << " " << thresh1 << "\n";
        //原输出
        int name_len = name[0].size() > name[1].size() ? name[0].size() : name[1].size();
        //os << "a score= "<< maf_score<<" "<< common_num <<" "<< (float)(1.0*common_num/A.size()) << " \n";
        os << "a score= " << maf_score << " \n";
       // std::cout << "a score= " << maf_score << " " << common_num << " " << (float)(1.0 * common_num / A.size()) << " \n";
        os << "s " << std::setw(name_len + 1) << std::left << name[0] << std::setw(9) << std::right << a_start << " " << std::setw(9) << std::right << a_len << " + " << std::setw(9) << std::right << final_sequences[0] << " ";
        for (k = i; k <= j; k++) os << charA[A[k]];
        //ts << a_start << " " << a_start + a_len << " ";
        os << "\n";
        if (sign[1])
            os << "s " << std::setw(name_len + 1) << std::left << name[1] << std::setw(9) << std::right << std::right << b_start << " " << std::setw(9) << std::right << b_len << " + " << std::setw(9) << std::right << final_sequences[1] << " ";
        else
            os << "s " << std::setw(name_len + 1) << std::left << name[1] << std::setw(9) << std::right << std::right << b_start << " " << std::setw(9) << std::right << b_len << " - " << std::setw(9) << std::right << final_sequences[1] << " ";
        for (k = i; k <= j; k++) os << charB[B[k]];
        //ts << b_start << " " << b_start + b_len << " ";
        os << "\n\n";
    }
    std::vector<unsigned char>().swap(A);
    std::vector<unsigned char>().swap(B);
}

/////************mummer 整合gap和N，得到最后的gap-insert
void utils::insertion_gap_out_new_mum(std::vector<unsigned char>& A, std::vector<unsigned char>& B, std::tuple<std::vector<std::tuple<int, int>>, std::vector<std::tuple<int, int>>>& tmp_insert, std::vector<Insertion>& N_insertion1, std::vector<Insertion>& N_insertion2, int nsum1, int nsum2)
{
    std::vector<std::tuple<int, int>>& more_insert_1 = std::get<0>(tmp_insert);
    std::vector<std::tuple<int, int>>& more_insert_2 = std::get<1>(tmp_insert);
    std::vector<std::vector<Insertion2>> all_insertions;
    std::vector<Insertion2> i_all_insertions;
    std::vector<unsigned char> tmp_vector;
    int i = 0, j = 0, pre = 0, mi, g_num, k;
    int* ii = new int[2];
    std::vector<std::vector<Insertion2>> more_insertions(2);


    //std::reverse(N_insertion2.begin(), N_insertion2.end()); //逆置
    //00000000000000
    i = 0; j = 0; g_num = 0, ii[0] = 0; ii[1] = 0; k = 0;
    std::vector<Insertion2>().swap(i_all_insertions);//清空 i_all_insertions
    while (i < more_insert_1.size() && j < N_insertion1.size())
    {
        if (std::get<0>(more_insert_1[i]) == N_insertion1[j].index)//相等变number
        {
            g_num += std::get<1>(more_insert_1[i]);
            if (N_insertion1[j].number > std::get<1>(more_insert_1[i]))
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_insert_1[i]), (size_t)std::get<1>(more_insert_1[i]), 0 }));  //gap 个 N
                insert_others(Insertion2({ (size_t)std::get<0>(more_insert_1[i]) + g_num ,N_insertion1[j].number - std::get<1>(more_insert_1[i]), 0 }), utils::Insertion2({ (size_t)std::get<0>(more_insert_1[i]) + g_num ,0, N_insertion1[j].number - std::get<1>(more_insert_1[i]) }), more_insertions, k, 2, ii);
            }
            else
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_insert_1[i]) ,N_insertion1[j].number, (size_t)std::get<1>(more_insert_1[i]) - N_insertion1[j].number }));
            }
            i++;
            j++;
        }
        else if (std::get<0>(more_insert_1[i]) > N_insertion1[j].index) //不等插新
        {
            insert_others(Insertion2({ N_insertion1[j].index + g_num ,N_insertion1[j].number, 0 }), utils::Insertion2({ N_insertion1[j].index + g_num ,0, N_insertion1[j].number }), more_insertions, k, 2, ii);
            j++;
        }
        else
        {
            g_num += std::get<1>(more_insert_1[i]);
            i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_insert_1[i]), 0, (size_t)std::get<1>(more_insert_1[i]) }));
            i++;
        }
    }
    while (j < N_insertion1.size()) //后续多余的N，继续插入
    {
        insert_others(Insertion2({ N_insertion1[j].index + g_num ,N_insertion1[j].number, 0 }), utils::Insertion2({ N_insertion1[j].index + g_num ,0, N_insertion1[j].number }), more_insertions, k, 2, ii);
        j++;
    }
    while (i < more_insert_1.size()) //后续多余的gap，继续插入
    {
        g_num += std::get<1>(more_insert_1[i]);
        i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_insert_1[i]), 0, (size_t)std::get<1>(more_insert_1[i]) }));
        i++;
    }
    all_insertions.push_back(i_all_insertions);
    //11111111111111
    i = 0; j = 0; g_num = 0, ii[0] = 0; ii[1] = 0; k = 1;
    std::vector<Insertion2>().swap(i_all_insertions);//清空 i_all_insertions
    while (i < more_insert_2.size() && j < N_insertion2.size())
    {
        if (std::get<0>(more_insert_2[i]) == N_insertion2[j].index)//相等变number
        {
            g_num += std::get<1>(more_insert_2[i]);
            if (N_insertion2[j].number > std::get<1>(more_insert_2[i]))
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_insert_2[i]), (size_t)std::get<1>(more_insert_2[i]), 0 }));  //gap 个 N
                insert_others(Insertion2({ (size_t)std::get<0>(more_insert_2[i]) + g_num ,N_insertion2[j].number - std::get<1>(more_insert_2[i]), 0 }), utils::Insertion2({ (size_t)std::get<0>(more_insert_2[i]) + g_num ,0, N_insertion2[j].number - std::get<1>(more_insert_2[i]) }), more_insertions, k, 2, ii);
            }
            else
            {
                i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_insert_2[i]) ,N_insertion2[j].number, (size_t)std::get<1>(more_insert_2[i]) - N_insertion2[j].number }));
            }
            i++;
            j++;
        }
        else if (std::get<0>(more_insert_2[i]) > N_insertion2[j].index) //不等插新
        {
            insert_others(Insertion2({ N_insertion2[j].index + g_num ,N_insertion2[j].number, 0 }), utils::Insertion2({ N_insertion2[j].index + g_num ,0, N_insertion2[j].number }), more_insertions, k, 2, ii);
            j++;
        }
        else
        {
            g_num += std::get<1>(more_insert_2[i]);
            i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_insert_2[i]), 0, (size_t)std::get<1>(more_insert_2[i]) }));
            i++;
        }
    }
    while (j < N_insertion2.size()) //后续多余的N，继续插入
    {
        insert_others(Insertion2({ N_insertion2[j].index + g_num ,N_insertion2[j].number, 0 }), utils::Insertion2({ N_insertion2[j].index + g_num ,0, N_insertion2[j].number }), more_insertions, k, 2, ii);
        j++;
    }
    while (i < more_insert_2.size()) //后续多余的gap，继续插入
    {
        g_num += std::get<1>(more_insert_2[i]);
        i_all_insertions.push_back(Insertion2({ (size_t)std::get<0>(more_insert_2[i]), 0, (size_t)std::get<1>(more_insert_2[i]) }));
        i++;
    }
    all_insertions.push_back(i_all_insertions);

    //消除  一列多个N导致de多个插空
    std::vector<Insertion> multi;
    while (i < more_insertions[0].size() && j < more_insertions[1].size())
    {
        if (more_insertions[0][i].index == more_insertions[1][j].index)
        {
            if (more_insertions[0][i].gap_num == 0 || more_insertions[1][j].gap_num == 0);
            else if (more_insertions[0][i].gap_num > more_insertions[1][j].gap_num)
                multi.push_back(Insertion({ more_insertions[0][i].index ,more_insertions[1][j].gap_num }));
            else multi.push_back(Insertion({ more_insertions[0][i].index ,more_insertions[0][i].gap_num }));
            i++; j++;
        }
        else if (more_insertions[0][i].index > more_insertions[1][j].index) j++;
        else i++;
    }
    //插入到原串
    int more = 0, all_size = 0, ti = 0;
    k = 0;
    //00000000000000
    i = 0;
    more = A.size();
    for (j = 0; j < all_insertions[i].size(); j++)
        more += (all_insertions[i][j].n_num + all_insertions[i][j].gap_num);
    all_size = more;
    mi = 0;
    for (j = 0; j < more_insertions[i].size(); j++)
    {
        all_size += more_insertions[i][j].n_num;
        if (mi < multi.size() && more_insertions[i][j].index == multi[mi].index)
            all_size -= multi[mi++].number;
        all_size += more_insertions[i][j].gap_num;
    }
    tmp_vector.resize(more);
    ti = 0;
    k = 0;
    for (j = 0; j < all_insertions[i].size(); j++)
    {
        while (k < all_insertions[i][j].index)
            tmp_vector[ti++] = A[k++];
        for (int p = 0; p < all_insertions[i][j].n_num; p++)
            tmp_vector[ti++] = '\5';
        for (int p = 0; p < all_insertions[i][j].gap_num; p++)
            tmp_vector[ti++] = '\7';
    }while (k < A.size())tmp_vector[ti++] = A[k++];
    A.resize(all_size);

    mi = 0;
    k = 0;
    ti = 0;
    for (j = 0; j < more_insertions[i].size(); j++)
    {

        while (ti < more_insertions[i][j].index) A[k++] = tmp_vector[ti++];
        for (int p = 0; p < more_insertions[i][j].n_num; p++) A[k++] = '\5';
        if (mi < multi.size() && more_insertions[i][j].index == multi[mi].index)
        {
            for (int p = 0; p < (more_insertions[i][j].gap_num - multi[mi].number); p++)
                A[k++] = '\7';
            mi++;
        }
        else
            for (int p = 0; p < more_insertions[i][j].gap_num; p++)  A[k++] = '\7';
    }
    while (ti < more) A[k++] = tmp_vector[ti++];

    //111111111111111111
    i = 1;
    ti = 0;
    k = 0;
    for (j = 0; j < all_insertions[i].size(); j++)
    {
        while (k < all_insertions[i][j].index)
            tmp_vector[ti++] = B[k++];
        for (int p = 0; p < all_insertions[i][j].n_num; p++)
            tmp_vector[ti++] = '\5';
        for (int p = 0; p < all_insertions[i][j].gap_num; p++)
            tmp_vector[ti++] = '\7';
    }while (k < B.size())tmp_vector[ti++] = B[k++];
    B.resize(all_size);
    mi = 0;
    k = 0;
    ti = 0;
    for (j = 0; j < more_insertions[i].size(); j++)
    {
        while (ti < more_insertions[i][j].index)
            B[k++] = tmp_vector[ti++];
        for (int p = 0; p < more_insertions[i][j].n_num; p++)
            B[k++] = '\5';
        if (mi < multi.size() && more_insertions[i][j].index == multi[mi].index)
        {
            for (int p = 0; p < (more_insertions[i][j].gap_num - multi[mi].number); p++)
                B[k++] = '\7';
            mi++;
        }
        else
            for (int p = 0; p < more_insertions[i][j].gap_num; p++)
                B[k++] = '\7';
    }while (ti < more) B[k++] = tmp_vector[ti++];
    std::vector<unsigned char>().swap(tmp_vector);
    std::vector<std::vector<Insertion2>>().swap(all_insertions);
    std::vector<Insertion2>().swap(i_all_insertions);
    std::vector<std::vector<Insertion2>>().swap(more_insertions);
    std::vector<Insertion>().swap(multi);
    delete[] ii;
    
    std::vector<std::tuple<int, int>>().swap(more_insert_1);
    std::vector<std::tuple<int, int>>().swap(more_insert_2);

    int iindex = 0, nnum = 0;
    for (auto& c : A)
    {
        if (c == '\7')
            nnum++;
        else
        {
            if (nnum != 0)
            {
                more_insert_1.push_back({ nsum1+iindex,nnum });
                nnum = 0;
            }
            iindex++;
        }
    }
    if (nnum != 0)
    {
        more_insert_1.push_back({ nsum1+iindex,nnum });
        nnum = 0;
    }

    iindex = 0, nnum = 0;
    for (auto& c : B)
    {
        if (c == '\7')
            nnum++;
        else
        {
            if (nnum != 0)
            {
                more_insert_2.push_back({ nsum2+iindex,nnum });
                nnum = 0;
            }
            iindex++;
        }
    }
    if (nnum != 0)
    {
        more_insert_2.push_back({ nsum2+iindex,nnum });
        nnum = 0;
    }
}



//处理逆补串maf
void utils::insertion_gap_more_new(std::ostream& os, std::vector<std::vector<unsigned char>>& sequences,
    std::vector<std::vector<Insertion>>& N_insertions, std::vector<std::string>& name,
    std::vector<bool>& sign, std::vector<bool>& TU, std::vector<utils::more_block>& more_gap, int thresh1, unsigned char *array_A, unsigned char *array_B) //写回比对结果
{
    int i, j, k, jm, jn, j0, nsum1, nsum2;
    const size_t sequence_number = sequences.size();
    size_t sequence_len[2];
    sequence_len[0] = sequences[0].size();
    sequence_len[1] = sequences[1].size();
    int* final_sequences = new int[sequence_number];
    std::vector<Insertion> gapn1, gapn2;

    for (i = 0; i < sequence_number; i++)
    {
        final_sequences[i] = sequences[i].size();
        //std::cout << name[i] << " " << sequences[i].size() << " " << final_sequences[i] << "\n";
        for (j = 0; j < N_insertions[i].size(); j++)
            final_sequences[i] += N_insertions[i][j].number;
        //std::cout << name[i] << " " << sequences[i].size() << " " << final_sequences[i] << "\n\n";
    }
    //os << "##maf version=1 scoring=lastz.v1.04.00\n";
    for (i = 1; i < sequence_number; i++)
    {
        jm = jn = j0 = nsum1 = nsum2 = 0;
        while (jm < more_gap[i].size())
        {
            j0 = nsum1 = 0;
            std::vector<Insertion>().swap(gapn1);
            std::vector<Insertion>().swap(gapn2);
            while (j0 < N_insertions[0].size() && jm < more_gap[i].size() && N_insertions[0][j0].index <= more_gap[i][jm].start1)nsum1 += N_insertions[0][j0++].number;
            while (jn < N_insertions[i].size() && jm < more_gap[i].size() && N_insertions[i][jn].index <= more_gap[i][jm].start2)nsum2 += N_insertions[i][jn++].number;
            while (j0 < N_insertions[0].size() && jm < more_gap[i].size() && N_insertions[0][j0].index < more_gap[i][jm].end1)gapn1.push_back(N_insertions[0][j0++]);
            while (jn < N_insertions[i].size() && jm < more_gap[i].size() && N_insertions[i][jn].index < more_gap[i][jm].end2)gapn2.push_back(N_insertions[i][jn++]);
            if (!AB_exist_ni(array_A, array_B, more_gap[i][jm].start1, more_gap[i][jm].end1, sequence_len[1] - more_gap[i][jm].end2, sequence_len[1] - more_gap[i][jm].start2))
            {
                jm++;
                continue;
            }
            insertion_gap_out_new(os, i, sequences, name, sign,TU, more_gap[i][jm], final_sequences, nsum1, nsum2, gapn1, gapn2, thresh1);
            jm++;
        }
        utils::more_block().swap(more_gap[i]);
    }
    delete[]final_sequences;
    std::vector<std::vector<unsigned char>>().swap(sequences);
    std::vector<std::vector<Insertion>>().swap(N_insertions);
    std::vector<std::string>().swap(name);
    std::vector<bool>().swap(sign);
    std::vector<utils::more_block>().swap(more_gap);
}

void Stream::main_maf(int num, std::vector<std::string>& name, std::vector<size_t>& Length, std::vector<bool>& sign, utils::MAF_info& MAFinfo)
{
     //std::vector<std::ifstream> strs(name.size());
    std::vector<std::unique_ptr<Sequence>> strs(name.size());
    //std::cout << "*    **\n";
    std::string path = arguments::out_file_name + "/fasta/";

    //打开文件太多了！！！！！！！！！！！！！！！！！！！！！！！！！
    if (num != 0)
    {
        //(strs[0]).open(path + std::to_string(num) + ".fasta", std::ios::binary | std::ios::in);
        strs[0] = std::make_unique<Sequence>(path + std::to_string(num) + ".fasta");
        //(strs[num]).open(path + "0.fasta", std::ios::binary | std::ios::in);
        strs[num] = std::make_unique<Sequence>(path + "0.fasta");
        name[0].swap(name[num]);

        size_t tmp_l = Length[0];
        Length[0] = Length[num];
        Length[num] = tmp_l;

        bool tmp_b = sign[0];
        sign[0] = sign[num];
        sign[num] = tmp_b;

        for (int i = 1; i < name.size(); i++)
        {
            if (i == num)
                continue;
            //(strs[i]).open(path + std::to_string(i) + ".fasta", std::ios::binary | std::ios::in);
            strs[i] = std::make_unique<Sequence>(path + std::to_string(i) + ".fasta");
        }
    }
    else
    {
        for (int i = 0; i < name.size(); i++)
            //(strs[i]).open(path + std::to_string(i) + ".fasta", std::ios::binary | std::ios::in);
            strs[i] = std::make_unique<Sequence>(path + std::to_string(i) + ".fasta");
    }

    //strs[0].seekg(0, std::ios::end);
    //int fasta_len = strs[0].tellg();
    //arguments::fasta_len = fasta_len;
    //strs[0].seekg(0, std::ios::beg);
    arguments::fasta_len = strs[0]->size();
    //std::cout << "*    **\n";
    //const auto align_start1 = std::chrono::high_resolution_clock::now(); //插入前
    Stream::filter_100_maf(strs, name, Length, sign, MAFinfo);
    //const auto align_start2 = std::chrono::high_resolution_clock::now(); //插入后
    //std::cout << align_start2 - align_start1 << "  filter_100_maf\n";
    strs.clear();
}


void Stream::star_hebing(char* str_fa, char* str_maf, std::vector<maf_two>& insert_fa, std::vector<maf_two>& insert_maf)
{
    
    std::vector<maf_two>().swap(insert_fa);
    std::vector<maf_two>().swap(insert_maf);
    std::vector<maf_two> in_fa;
    std::vector<maf_two> in_maf;
    size_t nchar = 0;
    size_t I = 0;
    size_t gap = 0, i1, i2;
    for (char* ptr = str_fa; *ptr != '\0'; ++ptr)
    {
        if ((*ptr) != '-')
        {
            if (gap != 0)
                in_fa.push_back({nchar,gap});
            gap = 0;
            nchar++;
        }
        else
            gap++;
    }
    if (gap != 0)
        in_fa.push_back({ nchar,gap });
    
    nchar = 0;
    gap = 0;
    for (char* ptr = str_maf; *ptr != '\0'; ++ptr)
    {
        if ((*ptr) != '-')
        {
            if (gap != 0)
                in_maf.push_back({ nchar,gap });
            gap = 0;
            nchar++;
        }
        else
            gap++;
    }
    if (gap != 0)
        in_maf.push_back({ nchar,gap });



    i1 = 0;
    i2 = 0;
    while (i1 < in_fa.size() && i2 < in_maf.size())
    {
        if (in_fa[i1].start < in_maf[i2].start)
        {
            insert_maf.push_back({ in_fa[i1].start,in_fa[i1].end });
            i1++;
        }
        else if (in_fa[i1].start > in_maf[i2].start)
        {
            insert_fa.push_back({ in_maf[i2].start,in_maf[i2].end });
            i2++;
        }
        else
        {
            if (in_fa[i1].end > in_maf[i2].end)
                insert_maf.push_back({ in_fa[i1].start, in_fa[i1].end - in_maf[i2].end });
            else if (in_fa[i1].end < in_maf[i2].end)
                insert_fa.push_back({ in_maf[i2].start, in_maf[i2].end - in_fa[i1].end });
            //else == 不变
            i1++;
            i2++;
        }
    }
    while (i1 < in_fa.size())
    {
        insert_maf.push_back({ in_fa[i1].start,in_fa[i1].end });
        i1++;
    }
    while (i2 < in_maf.size())
    {
        insert_fa.push_back({ in_maf[i2].start,in_maf[i2].end });
        i2++;
    }

    i1 = 0;
    I = 0;
    nchar = 0;
    for (char* ptr = str_fa; *ptr != '\0'; ++ptr)
    {
        if (i1 < insert_fa.size() && nchar == insert_fa[i1].start)
        {
            insert_fa[i1].start = I;
            i1++;
        }
        if ((*ptr) != '-')
            nchar++;
        I++;
    }
    if (i1 < insert_fa.size() && nchar == insert_fa[i1].start)
    {
        insert_fa[i1].start = I;
        i1++;
    }

    i2 = 0;
    I = 0;
    nchar = 0;
    for (char* ptr = str_maf; *ptr != '\0'; ++ptr)
    {
        if (i2 < insert_maf.size() && nchar == insert_maf[i2].start)
        {
            insert_maf[i2].start = I;
            i2++;
        }
        if ((*ptr) != '-')
            nchar++;
        I++;
    }
    if (i2 < insert_maf.size() && nchar == insert_maf[i2].start)
    {
        insert_maf[i2].start = I;
        i2++;
    }

    /*
    std::cout << str_fa << " \n";
    std::cout << str_maf << " \n";
    for (const auto& element : in_fa) 
        std::cout << element.start << " " << element.end <<" | ";
    std::cout << "  in_fa\n";
    for (const auto& element : insert_fa)
        std::cout << element.start << " " << element.end <<" | ";
    std::cout << "   insert_fa\n";
    for (const auto& element : in_maf)
        std::cout << element.start << " " << element.end <<" | ";
    std::cout << "  in_maf\n";
    for (const auto& element : insert_maf)
        std::cout << element.start << " " << element.end <<" | ";
    std::cout << "  insert_maf\n";
    */

}
void Stream::sort_hebing(std::string fileName,std::string ref_name)
{
    struct mafFile* maf;
    struct mafAli* cp_list, * tmp_list = NULL, * now, * next, * tail = NULL, * tmp = NULL;
    //init_scores70();
    {
        std::lock_guard<std::mutex> lock(threadPool1->mutex_fp);
        maf = mafReadAll((char*)(fileName.c_str()), 1); 
    }
    cp_list = maf->alignments;
    maf->alignments = NULL;
    mafFileFree(&maf);
    if (cp_list == NULL)
    {
        return;
    }
    FILE* fpw1;
    {
        std::lock_guard<std::mutex> lock(threadPool1->mutex_fp);
        fpw1 = fopen((char*)fileName.c_str(), "w"); 
    }
    fprintf(fpw1, "##maf version=1 scoring=N/A\n");

    

    now = cp_list;
    next = now->next;
    while (next != NULL)
    {
        //std::cout << now->components->start << " " << next->components->start << " ****\n";
        if ((now->components->start == next->components->start) && (now->components->size == next->components->size))
        {
            next = next->next;
            multiz_star_hebing(&now, &(now->next));
            //mafWrite(fpw1, now);
            now->score = 1;
            now->next = next;
        }
        else
        {
            now = next;
            next = now->next;
        }
    }

    while (0 && cp_list!=NULL)
    {
        now = cp_list;
        next = now->next;
        cp_list = NULL;
        tmp = cp_list;
        tmp_list = NULL;
        tail = tmp_list;
        while (next != NULL)
        {
            std::cout << now->components->start << " " << next->components->start << " ****\n";
            if ((now->components->start == next->components->start) && (now->components->size == next->components->size))
            {
                //std::cout << now->components->start << " " << next->components->start << " ||||\n";
                next = next->next;
                multiz_star_hebing(&now, &(now->next));
                now->next = next;
                if (next == NULL)
                {
                    if (cp_list == NULL)
                        cp_list = now;
                    else
                        tmp->next = now;
                    tmp = now;
                    tmp->next = NULL;
                }
            }
            else if (next->components->start <(now->components->start + now->components->size))
            {   //两个区间有重叠，左闭右开，
                //将next连接在tmp_list后面，尾插法。now=next->next,next = now->next.
                //std::cout << now->components->start << " " << next->components->start << " ////\n";
                /*now = next->next;
                if (tmp_list == NULL)
                    tmp_list = next;
                else
                    tail->next = next;
                tail = next;
                tail->next = NULL;
                if (now == NULL)
                    break;
                next = now->next;*/
                if (tmp_list == NULL)
                    tmp_list = now;
                else
                    tail->next = now;
                tail = now;
                tail->next = NULL;
                now = next;
                next = now->next;
            }
            else
            {
                if (cp_list == NULL)
                    cp_list = now;
                else
                    tmp->next = now;
                tmp = now;
                tmp->next = NULL;
                now = next;
                next = now->next;
            }
        }
        /**
        now = cp_list;
        while (now)
        {
            mafWrite(fpw1, now);
            tail = now->next;
            mafAliFree(&now);
            now = tail;
        }
        
        fprintf(fpw1, "\n******cp_list**********\n");
        
        now = tmp_list;
        while (now)
        {
            mafWrite(fpw1, now);
            tail = now->next;
            mafAliFree(&now);
            now = tail;
        }
        fprintf(fpw1, "\n********tmp_list********\n");
        */
        if (tmp_list == NULL)
            break;
        tail = NULL;

        cp_list = two_list_to_one(&cp_list, &tmp_list, fpw1, fpw1);
        std::cout<<"\n****************\n";
        cp_list = mergeSort(cp_list); // 排序
        
        tail = NULL;
        tmp_list = NULL;
    }

    now = cp_list;
    while (now)
    {
        if (strcmp(now->components->src, ref_name.c_str()) == 0)
            mafWrite(fpw1, now);
        tail = now->next;
        mafAliFree(&now);
        now = tail;
    }
    fclose(fpw1);
}

void Stream::sort_hebing2(struct mafAli* cp_list, std::string fileName, std::string ref_name)
{
    struct mafFile* maf;
    struct mafAli* tmp_list = NULL, * now, * next, * tail = NULL, * tmp = NULL;
    init_scores70();

    if (cp_list == NULL)
        return;
    FILE* fpw1 = fopen((char*)fileName.c_str(), "w");
    fprintf(fpw1, "##maf version=1 scoring=N/A\n");

    now = cp_list;
    next = now->next;
    while (next != NULL)
    {
        //std::cout << now->components->start << " " << next->components->start << " ****\n";
        if ((now->components->start == next->components->start) && (now->components->size == next->components->size))
        {
            next = next->next;
            multiz_star_hebing(&now, &(now->next));
            //mafWrite(fpw1, now);
            now->score = 1;
            now->next = next;
        }
        else
        {
            now = next;
            next = now->next;
        }
    }

    while (0 && cp_list != NULL)
    {
        now = cp_list;
        next = now->next;
        cp_list = NULL;
        tmp = cp_list;
        tmp_list = NULL;
        tail = tmp_list;
        while (next != NULL)
        {
            std::cout << now->components->start << " " << next->components->start << " ****\n";
            if ((now->components->start == next->components->start) && (now->components->size == next->components->size))
            {
                //std::cout << now->components->start << " " << next->components->start << " ||||\n";
                next = next->next;
                multiz_star_hebing(&now, &(now->next));
                now->next = next;
                if (next == NULL)
                {
                    if (cp_list == NULL)
                        cp_list = now;
                    else
                        tmp->next = now;
                    tmp = now;
                    tmp->next = NULL;
                }
            }
            else if (next->components->start < (now->components->start + now->components->size))
            {   //两个区间有重叠，左闭右开，
                //将next连接在tmp_list后面，尾插法。now=next->next,next = now->next.
                //std::cout << now->components->start << " " << next->components->start << " ////\n";
                /*now = next->next;
                if (tmp_list == NULL)
                    tmp_list = next;
                else
                    tail->next = next;
                tail = next;
                tail->next = NULL;
                if (now == NULL)
                    break;
                next = now->next;*/
                if (tmp_list == NULL)
                    tmp_list = now;
                else
                    tail->next = now;
                tail = now;
                tail->next = NULL;
                now = next;
                next = now->next;
            }
            else
            {
                if (cp_list == NULL)
                    cp_list = now;
                else
                    tmp->next = now;
                tmp = now;
                tmp->next = NULL;
                now = next;
                next = now->next;
            }
        }
        /**
        now = cp_list;
        while (now)
        {
            mafWrite(fpw1, now);
            tail = now->next;
            mafAliFree(&now);
            now = tail;
        }

        fprintf(fpw1, "\n******cp_list**********\n");

        now = tmp_list;
        while (now)
        {
            mafWrite(fpw1, now);
            tail = now->next;
            mafAliFree(&now);
            now = tail;
        }
        fprintf(fpw1, "\n********tmp_list********\n");
        */
        if (tmp_list == NULL)
            break;
        tail = NULL;

        cp_list = two_list_to_one(&cp_list, &tmp_list, fpw1, fpw1);
        std::cout << "\n****************\n";
        cp_list = mergeSort(cp_list); // 排序

        tail = NULL;
        tmp_list = NULL;
    }

    now = cp_list;
    while (now)
    {
        if (strcmp(now->components->src, ref_name.c_str()) == 0)
            mafWrite(fpw1, now);
        tail = now->next;
        mafAliFree(&now);
        now = tail;
    }
    fclose(fpw1);
}


void reverseString(char* str) {
    if (str == nullptr) {
        return; // 处理空指针情况
    }

    int length = std::strlen(str);
    if (length <= 1) {
        return; // 如果长度小于等于1，无需逆序
    }

    char* start = str;
    char* end = str + length - 1;

    while (start < end) {
        // 交换 start 和 end 的字符
        char temp = *start;
        *start = *end;
        *end = temp;

        // 移动指针
        start++;
        end--;
    }
}

void Stream::sort_hebing_plus(std::string fileName, std::string ref_name)
{
    struct mafFile* maf;
    struct mafAli* cp_list, * tmp_list = NULL, * now, * next, * tmp = NULL;
    std::set<size_t> INS_maf;
   
    size_t j, I,list_len=0, nchar;
    {
        std::lock_guard<std::mutex> lock(threadPool0->mutex_fp); // 在申请file-number时加锁
        maf = mafOpen((char*)(fileName.c_str()), 1);
    }
    
    struct mafAli* a, * last, * tail = NULL;
    struct mafComp* tmp_c, *c;
    //1读入
    for (last = NULL; (a = my_mafNext(maf)) != NULL; last = a)
    {
        //交换AB
        tmp_c = a->components;
        a->components = a->components->next;
        a->components->next = tmp_c;
        tmp_c->next = NULL;

        if (a->components->strand == '-')
        {
            tmp_c = a->components;
            tmp_c->start = tmp_c->srcSize - (tmp_c->start + tmp_c->size);
            reverseString(tmp_c->text);

            reverseString(a->components->next->text);
        }
        INS_maf.insert(a->components->start);
        INS_maf.insert(a->components->start + a->components->size);
        if (last == NULL)
            maf->alignments = a;
        else
            last->next = a;
        list_len++;
    }
    
    if (maf->alignments == NULL)
    {
        mafFileFree(&maf);
        return;
    }
    
    //2排序
    maf->alignments = mergeSort(maf->alignments); // 排序
    //3遍历列表，分割

    j = 0;
    I = 0;
    auto it = INS_maf.begin();
    std::vector <std::set<size_t>> maf_fenge(list_len);
    if (!INS_maf.empty())
    {
        now = maf->alignments;
        while (now)
        {
            j = now->components->start;
            while ((it != INS_maf.begin()) && (j < *it))
                it--;
            while ((it != INS_maf.end()) && (j > *it))
                ++it;
            //j == *it;
            while (it != INS_maf.end() && (*it <= (now->components->start + now->components->size)))
            {
                maf_fenge[I].insert(*it);
                ++it;
            }
            I++;
            now = now->next;
        }
    }

    std::set<size_t> new_fenge;
    std::vector<size_t> vec_new_fenge;
    char* newString;
    bool tag_n;
    now = maf->alignments;
    size_t maf_i = 0;
    while (now != NULL)
    {
        if (maf_fenge[maf_i].size() > 2)
        {
            I = 0;
            j = now->components->start;
            auto itc = maf_fenge[maf_i].begin();
            std::set<size_t>().swap(new_fenge);
            new_fenge.insert(0);
            for (char* ptr = now->components->text; *ptr != '\0'; ++ptr) {
                if ((itc != maf_fenge[maf_i].end()) && (j == *itc))
                {
                    new_fenge.insert(I);
                    itc++;
                }
                if ((*ptr) != '-')
                    j++;
                I++;
            }
            new_fenge.insert(I);
            std::vector<size_t>().swap(vec_new_fenge);
            vec_new_fenge.reserve(new_fenge.size()); // 提前分配足够的空间
            std::copy(new_fenge.begin(), new_fenge.end(), std::back_inserter(vec_new_fenge));//set 转  vector
            
            
            for (int e = 0; e < vec_new_fenge.size() - 1; e++)
            {
                if (e == 0)
                {
                    //std::cout << now->components->start << "   ns\n";
                    tmp_list = copyMafAliNode(now);
                    tmp = tmp_list;
                    tmp->next = NULL;
                }
                else
                {
                    //std::cout << now->components->start << "   ns\n";
                    tmp->next = copyMafAliNode(now);
                    tmp = tmp->next;
                    tmp->next = NULL;
                }

                if(1)
                {
                    c = now->components;
                    newString = tmp->components->text;
                    strncpy(newString, c->text + vec_new_fenge[e], vec_new_fenge[e + 1] - vec_new_fenge[e]);
                    newString[vec_new_fenge[e + 1] - vec_new_fenge[e]] = '\0';
                    nchar = 0;
                    for (char* ptr = newString; *ptr != '\0'; ++ptr)
                        if ((*ptr) != '-')
                            nchar++;
                    tmp->components->start = c->start;
                    tmp->components->size = nchar;

                    c->start += nchar;

                    c = now->components->next;
                    newString = tmp->components->next->text;
                    strncpy(newString, c->text + vec_new_fenge[e], vec_new_fenge[e + 1] - vec_new_fenge[e]);
                    newString[vec_new_fenge[e + 1] - vec_new_fenge[e]] = '\0';
                    nchar = 0;
                    for (char* ptr = newString; *ptr != '\0'; ++ptr)
                        if ((*ptr) != '-')
                            nchar++;
                    tmp->components->next->start = c->start;
                    tmp->components->next->size = nchar;
                    
                    c->start += nchar;
                }
            }
            now->score = -1;//删除  mafAliFree(&now);
            
            tmp->next = now->next;
            now->next = tmp_list;
            now = tmp->next; //now = now->next;
            tmp_list = NULL;
            tmp = NULL;
        }
        else
            now = now->next;
        maf_i++;
    }
    //删除原来节点
    now = maf->alignments;
    
    tmp = now->next;
    while (tmp != NULL)
    {
        if (tmp->score == -1 || tmp->components->size == 0 || tmp->components->next->size == 0)
        {
            now->next = tmp->next;
            mafAliFree(&tmp);
            tmp = now->next;
        }
        else
        {
            now = now->next;
            tmp = now->next;
        }
    }
    
    //删除相同节点
    maf->alignments = mergeSort(maf->alignments); // 排序
    now = maf->alignments;
    tmp = now->next;
    while (tmp != NULL)
    {
        if ((now->components->start == tmp->components->start) && (now->components->size == tmp->components->size))
        {
            now->next = tmp->next;
            mafAliFree(&tmp);
            tmp = now->next;
        }
        else
        {
            now = now->next;
            tmp = now->next;
        }
    }

    //转换坐标 AB
    now = maf->alignments;
    while (now != NULL)
    {
        if (now->components->strand == '-')
        {
            tmp_c = now->components;
            tmp_c->start = tmp_c->srcSize - (tmp_c->start + tmp_c->size);
            reverseString(tmp_c->text);

            reverseString(now->components->next->text);
        }
        //交换AB
        tmp_c = now->components;
        now->components = now->components->next;
        now->components->next = tmp_c;
        tmp_c->next = NULL;

        now = now->next;
    }

    maf->alignments = mergeSort(maf->alignments); // 排序
    FILE* fpw1 = NULL;
    {
        std::lock_guard<std::mutex> lock(threadPool0->mutex_fp); // 在申请file-number时加锁
        fpw1 = fopen((char*)fileName.c_str(), "w");
    }
    fprintf(fpw1, "##maf version=1 scoring=N/A\n");

    now = maf->alignments;
    while (now)
    {
        if (strcmp(now->components->src, ref_name.c_str()) == 0)
        mafWrite(fpw1, now);
        tail = now->next;
        mafAliFree(&now);
        now = tail;
    }
    maf->alignments = NULL;
    mafFileFree(&maf);
    fclose(fpw1);

}

inline bool compareByEnd(const Stream::maf_two& a, const Stream::maf_two& b) {
    return a.end < b.end;
}


size_t maf_i_score(char* ref, char* newString)
{
    if (ref == NULL || newString == NULL)
        return 0;
    size_t score = 0;
    for (int i = 0; i < strlen(newString) - 1; i++)
    {
        if (newString[i] != 'N' && newString[i] != '-' && ref[i] == newString[i])
            score++;
    }
    return score;
}

void Stream::cut_maf(struct mafAli* cp_list, std::string file_name, std::string ref_name)
{
    std::vector<struct maf_three> INS;
    std::set<size_t> INS_maf;

    size_t j, I;
    struct mafAli* now = cp_list;
    struct mafComp* c;
    size_t maf_i = 0;
    while (now != NULL)
    {
        //中心序列
        c = now->components;
        INS.push_back({ maf_i , size_t(c->start),size_t(c->start + c->size) });
        INS_maf.insert(c->start);
        INS_maf.insert(c->start + c->size);
        now = now->next;
        maf_i++;
    }
    j = 0;
    I = 0;

    auto it = INS_maf.begin();
    std::vector <std::set<size_t>> maf_fenge(INS.size());
    if (!INS_maf.empty())
        for (const auto& element : INS) {
            j = element.start;
            while ((it != INS_maf.begin()) && (j < *it))
                it--;
            while ((it != INS_maf.end()) && (j > *it))
                ++it;
            //j == *it;
            while (it != INS_maf.end() && (*it <= element.end))
            {
                maf_fenge[element.imaf].insert(*it);
                ++it;
            }
            if (it == INS_maf.end())
                it = std::prev(INS_maf.end());
        }
    std::set<size_t>().swap(INS_maf);
    FILE* fpw1;
    {
        std::lock_guard<std::mutex> lock(threadPool1->mutex_fp);
        fpw1 = fopen(file_name.c_str(), "w");
    }
    
    fprintf(fpw1, "##maf version=1 scoring=N/A\n");
    maf_i = 0;
    size_t nchar;
    bool tag_n;
    std::set<size_t> new_fenge;
    std::vector<size_t> vec_new_fenge;
    char* newString;
    now = cp_list;
    while (now != NULL)
    {
        if (maf_fenge[maf_i].size() > 2)
        {
            j = now->components->start;
            I = 0;
            auto itc = maf_fenge[maf_i].begin();
            std::set<size_t>().swap(new_fenge);
            new_fenge.insert(0);
            for (char* ptr = now->components->text; *ptr != '\0'; ++ptr) {
                if ((itc != maf_fenge[maf_i].end()) && (j == *itc))
                {
                    new_fenge.insert(I);
                    itc++;
                }
                if ((*ptr) != '-')
                    j++;
                I++;
            }
            new_fenge.insert(I);
            std::vector<size_t>().swap(vec_new_fenge);
            vec_new_fenge.reserve(new_fenge.size()); // 提前分配足够的空间
            std::copy(new_fenge.begin(), new_fenge.end(), std::back_inserter(vec_new_fenge));//set 转  vector

            for (int e = 0; e < vec_new_fenge.size() - 1; e++)
            {
                tag_n = true;
                newString = new char[vec_new_fenge[e + 1] - vec_new_fenge[e] + 1];
                for (c = now->components; c != NULL; c = c->next)
                {
                    strncpy(newString, c->text + vec_new_fenge[e], vec_new_fenge[e + 1] - vec_new_fenge[e]);
                    newString[vec_new_fenge[e + 1] - vec_new_fenge[e]] = '\0';
                    nchar = 0;
                    for (char* ptr = newString; *ptr != '\0'; ++ptr)
                        if ((*ptr) != '-')
                            nchar++;
                    if (strcmp(c->src, ref_name.c_str()) == 0)
                    {
                        if (nchar != 0)
                            fprintf(fpw1, "a score=0\n");
                        else
                            tag_n = false;
                    }

                    if (tag_n && nchar != 0)
                    {
                        fprintf(fpw1, "s %s %d %lu %c %d %s\n",
                            c->src, c->start, nchar, c->strand, c->srcSize, newString);
                    }
                    c->start += nchar;
                }
                delete[]newString;
                fprintf(fpw1, "\n");
            }
        }
        else
        {
            mafWrite(fpw1, now);
        }
        cp_list = now;
        now = now->next;
        mafAliFree(&cp_list);
        maf_i++;
    }
    fclose(fpw1);
    return;
}

void Stream::cut_maf1000(std::string infile_name, FILE* fpw1, size_t max_lenght)
{
    struct mafFile* filefp;
    {
        std::lock_guard<std::mutex> lock(threadPool1->mutex_fp);
        filefp = mafOpen(const_cast<char*>(infile_name.c_str()), 1);
    }
    struct mafAli * pre;
    struct mafAli * now;
    struct mafComp* cp;
    struct mafComp* ori;
    char * tmp;
    size_t left_len = 0;
    int count = 0;
    char* ptr;
    while ((pre = my_mafNext(filefp)) != NULL)
    {
        if (strlen(pre->components->text) <= max_lenght)
        {
            mafWrite(fpw1, pre);
            mafAliFree(&pre);
        }
        else
        {
            while (strlen(pre->components->text) > max_lenght)
            {
                now = copyMafAliNode(pre);
                cp = now->components;
                ori = pre->components;
                for (; cp != NULL; cp = cp->next, ori = ori->next)
                {
                    left_len = strlen(ori->text) - max_lenght + 1;
                    tmp = (char*)malloc(left_len * sizeof(char));
                    cp->text[max_lenght] = '\0';
                    strncpy(tmp, ori->text+ max_lenght, left_len);
                    tmp[left_len - 1] = '\0';
                    free(ori->text);
                    ori->text = tmp;
                    
                    count = 0;
                    ptr = cp->text;
                    while (*ptr != '\0') 
                    {
                        if (*ptr != '-') 
                            count++;
                        ptr++;
                    }
                    cp->size = count;
                    ori->size -= count;
                    ori->start += count;
                }
                now->textSize = max_lenght;
                pre->textSize -= max_lenght;

                mafWrite(fpw1, now);
                mafAliFree(&now);
            }
            if (strlen(pre->components->text) > 0)
            {
                mafWrite(fpw1, pre);
                mafAliFree(&pre);
            }
        }
    }
    mafFileFree(&filefp);

    return;
}

void Stream::get_strand(std::vector<std::vector<char>>& strand, std::vector<std::string>& file_name, std::vector<std::vector<std::string>>& Name_All, std::string infile_name)
{
    std::vector<std::vector<long long>> strand_num(file_name.size());
    for (int i = 0; i < Name_All.size(); i++)
        for (int j = 0; j < Name_All[i].size(); j++)
            strand_num[i].push_back(0);

    struct mafFile* filefp = mafOpen(const_cast<char*>(infile_name.c_str()), 1);
    struct mafAli* pre;
    struct mafComp* cp;
    size_t left_len = 0;
    int count = 0;
    std::string str_ptr, str1,str2;
    int index1, index2;
    while ((pre = my_mafNext(filefp)) != NULL)
    {
        for (cp = pre->components->next; cp != NULL; cp = cp->next) {
            str_ptr = cp->src;
            std::stringstream ss(str_ptr);
            std::getline(ss, str1, '.'); // 从 ss 中按 '.' 分割，并将第一部分存入 str1
            std::getline(ss, str2, '.'); // 将剩余部分存入 str2
            auto it = std::find(file_name.begin(), file_name.end(), str1);
            if (it != file_name.end()) {
                // 计算找到的下标
                index1 = std::distance(file_name.begin(), it);
                auto it2 = std::find(Name_All[index1].begin(), Name_All[index1].end(), str2);
                if (it2 != Name_All[index1].end()) {
                    index2 = std::distance(Name_All[index1].begin(), it2);
                    if(cp->strand=='+')
                        strand_num[index1][index2] += cp->size;
                    else
                        strand_num[index1][index2] -= cp->size;
                }
                else {
                    std::cout << "'" << str2 << "' not found in Name_All" << std::endl;
                    exit(-1);
                }
            }
            else {
                std::cout << "'" << str1 << "' not found in file_name" << std::endl;
                exit(-1);
            }
        }
        mafAliFree(&pre);
    }
    mafFileFree(&filefp);

    for (int i = 1; i < Name_All.size(); i++)
        for (int j = 0; j < Name_All[i].size(); j++)
            if (strand_num[i][j] < 0)
                strand[i][j] = '-';

    return;
}

bool TupleCompare::operator()(const std::tuple<int, int, int>& lhs, const std::tuple<int, int, int>& rhs) const {
    if (std::get<0>(lhs) != std::get<0>(rhs))
        return std::get<0>(lhs) < std::get<0>(rhs);
    if (std::get<1>(lhs) != std::get<1>(rhs))
        return std::get<1>(lhs) < std::get<1>(rhs);
    return std::get<2>(lhs) < std::get<2>(rhs);
}

struct mafComp* Stream::sortAndReconnect(std::map<std::tuple<int, int, int>, struct mafComp*, TupleCompare>& mapping) {
    if (mapping.empty()) {
        return nullptr;
    }

    // 寻找排序后的链表头节点
    struct mafComp* head = nullptr;
    struct mafComp* tail = nullptr;

    for (auto& pair : mapping) {
        if (!head) {
            head = pair.second;
            tail = pair.second;
        }
        else {
            tail->next = pair.second;
            tail = pair.second;
        }
    }

    // 断开最后一个节点的连接
    if (tail) {
        tail->next = nullptr;
    }

    return head;
}

void Stream::sort_maf1000(std::vector<std::vector<char>>& strand, std::vector<std::string>& file_name, std::vector<std::vector<std::string>>& Name_All, std::string infile_name, FILE* fpw1, size_t max_lenght)
{
    struct mafFile* filefp = mafOpen(const_cast<char*>(infile_name.c_str()), 1);
    struct mafAli* pre;
    struct mafAli* now;
    struct mafComp* cp;
    struct mafComp* ori;
    char* tmp;
    size_t left_len = 0;
    int count = 0;
    char* ptr;
    std::string str_ptr, str1, str2;
    int index1, index2,index3;
    while ((pre = my_mafNext(filefp)) != NULL)
    {
        std::map<std::tuple<int, int, int>, struct mafComp*, TupleCompare> mapping;

        for (cp = pre->components; cp != NULL; cp = cp->next) {
            str_ptr = cp->src;
            std::stringstream ss(str_ptr);
            std::getline(ss, str1, '.'); // 从 ss 中按 '.' 分割，并将第一部分存入 str1
            std::getline(ss, str2, '.'); // 将剩余部分存入 str2
            auto it = std::find(file_name.begin(), file_name.end(), str1);
            if (it != file_name.end()) {
                // 计算找到的下标
                index1 = std::distance(file_name.begin(), it);
                auto it2 = std::find(Name_All[index1].begin(), Name_All[index1].end(), str2);
                if (it2 != Name_All[index1].end()) {
                    index2 = std::distance(Name_All[index1].begin(), it2);
                    if (cp->strand == strand[index1][index2])
                        index3 = cp->start;
                    else
                        index3 = cp->srcSize - cp->size - cp->start;
                    mapping[std::make_tuple(index1, index2, index3)] = cp;
                }
                else {
                    std::cout << "'" << str2 << "' not found in Name_All" << std::endl;
                    exit(-1);
                }
            }
            else {
                std::cout << "'" << str1 << "' not found in file_name" << std::endl;
                exit(-1);
            }
        }

        pre->components = sortAndReconnect(mapping);

        if (strlen(pre->components->text) <= max_lenght)
        {
            mafWrite(fpw1, pre);
            mafAliFree(&pre);
        }
        else
        {

            while (strlen(pre->components->text) > max_lenght)
            {
                now = copyMafAliNode(pre);
                cp = now->components;
                ori = pre->components;
                for (; cp != NULL; cp = cp->next, ori = ori->next)
                {
                    left_len = strlen(ori->text) - 1000 + 1;
                    tmp = (char*)malloc(left_len * sizeof(char));
                    cp->text[1000] = '\0';
                    strncpy(tmp, ori->text + 1000, left_len);
                    tmp[left_len - 1] = '\0';
                    free(ori->text);
                    ori->text = tmp;

                    count = 0;
                    ptr = cp->text;
                    while (*ptr != '\0')
                    {
                        if (*ptr != '-')
                            count++;
                        ptr++;
                    }
                    cp->size = count;
                    ori->size -= count;
                    ori->start += count;
                }
                now->textSize = 1000;
                pre->textSize -= 1000;

                mafWrite(fpw1, now);
                mafAliFree(&now);
            }
            if (strlen(pre->components->text) > 0)
            {
                mafWrite(fpw1, pre);
                mafAliFree(&pre);
            }
        }
    }
    mafFileFree(&filefp);
    return;
}


void Stream::main_maf2(int num, std::vector<std::string>& name, std::vector<size_t>& Length, std::vector<bool>& sign, utils::MAF_info& MAFinfo)
{
    if (num != 0)
    {
        name[0].swap(name[num]);

        size_t tmp_l = Length[0];
        Length[0] = Length[num];
        Length[num] = tmp_l;

        bool tmp_b = sign[0];
        sign[0] = sign[num];
        sign[num] = tmp_b;
    }
    int srcChars = 0, startChars = 0, sizeChars = 0, srcSizeChars = 0;
    for (int i = 0; i < name.size(); i++) {
        srcChars = MAX(srcChars, (size_t)name[i].size());
        startChars = MAX(startChars, std::to_string(Length[i]).size());
    }sizeChars = srcSizeChars = startChars;
    //std::cout << "第一遍读maf，获取位点\n";
    //**************************************第一遍读maf，获取位点**************************************
    std::vector<struct maf_three> INS;

    char file_name[1000];
    struct mafComp* c;
    size_t j, I;
    size_t maf_i = 0;
    strcpy(file_name, (arguments::out_file_name + "/maf/small.maf").c_str());
    //将small.maf全部读入，排序，按节点逐个合并，写出_maf, 改回原名
    sort_hebing(arguments::out_file_name + "/maf/small.maf", name[0]);
    //std::cout << "1    \n";
    struct mafFile* sv_maf_file = mafOpen(file_name, 1);
    struct mafAli* now = my_mafNext(sv_maf_file);
    std::set<size_t> INS_maf;
    while (now != NULL)
    {
        //中心序列
        c = now->components;
        INS.push_back({ maf_i , size_t(c->start),size_t(c->start + c->size) });
        INS_maf.insert(c->start);
        INS_maf.insert(c->start + c->size);
        mafAliFree(&now);
        now = my_mafNext(sv_maf_file);
        maf_i++;
    }
    //std::cout << "2    \n";
    //maf_fenge
    j = 0;
    I = 0;

    auto it = INS_maf.begin();
    std::vector <std::set<size_t>> maf_fenge(INS.size());
    if (!INS_maf.empty())
        for (const auto& element : INS) {
            j = element.start;
            while ((it != INS_maf.begin()) && (j < *it))
                it--;
            while ((it != INS_maf.end()) && (j > *it))
                ++it;
            //j == *it;
            while (it != INS_maf.end() && (*it <= element.end))
            {
                maf_fenge[element.imaf].insert(*it);
                ++it;
            }
            if (it == INS_maf.end())
                it = std::prev(INS_maf.end());
        }
    //std::cout << "3    \n";
    std::set<size_t>().swap(INS_maf);
    sv_maf_file->alignments = NULL;
    mafFileFree(&sv_maf_file);
    //std::cout << "3    \n";

    //***************************************第二遍读maf，分割*******************
    sv_maf_file = mafOpen(file_name, 1);
    now = my_mafNext(sv_maf_file);
    strcpy(file_name, (arguments::out_file_name + "/maf/small_.maf").c_str());
    FILE* fpw1 = fopen(file_name, "w");
    fprintf(fpw1, "##maf version=1 scoring=N/A\n");
    maf_i = 0;
    size_t nchar;
    size_t ACGTchar;
    bool tag_n;
    std::set<size_t> new_fenge;
    std::vector<size_t> vec_new_fenge;
    char* newString;
    size_t index_c;
    size_t score, tmpI, tmpj;
    while (now != NULL)
    {
        if (maf_fenge[maf_i].size() > 2)
        {
            j = now->components->start;
            I = 0;
            auto itc = maf_fenge[maf_i].begin();
            std::set<size_t>().swap(new_fenge);
            new_fenge.insert(0);
            for (char* ptr = now->components->text; *ptr != '\0'; ++ptr) {
                if ((itc != maf_fenge[maf_i].end()) && (j == *itc))
                {
                    new_fenge.insert(I);
                    itc++;
                }
                if ((*ptr) != '-')
                    j++;
                I++;
            }
            new_fenge.insert(I);
            std::vector<size_t>().swap(vec_new_fenge);
            vec_new_fenge.reserve(new_fenge.size()); // 提前分配足够的空间
            std::copy(new_fenge.begin(), new_fenge.end(), std::back_inserter(vec_new_fenge));//set 转  vector

            for (int e = 0; e < vec_new_fenge.size() - 1; e++)
            {
                tag_n = true;
                newString = new char[vec_new_fenge[e + 1] - vec_new_fenge[e] + 1];
                for (c = now->components; c != NULL; c = c->next)
                {
                    strncpy(newString, c->text + vec_new_fenge[e], vec_new_fenge[e + 1] - vec_new_fenge[e]);
                    newString[vec_new_fenge[e + 1] - vec_new_fenge[e]] = '\0';
                    nchar = 0;
                    for (char* ptr = newString; *ptr != '\0'; ++ptr)
                        if ((*ptr) != '-')
                            nchar++;
                    if (strcmp(c->src, name[0].c_str()) == 0)
                    {
                        if (nchar != 0)
                            fprintf(fpw1, "a score=0\n");
                        else
                            tag_n = false;
                    }

                    if (tag_n && nchar != 0)
                    {
                        fprintf(fpw1, "s %s %d %lu %c %d %s\n",
                            c->src, c->start, nchar, c->strand, c->srcSize, newString);
                    }
                    c->start += nchar;
                }
                delete[]newString;
                fprintf(fpw1, "\n");
            }
        }
        else
        {
            mafWrite(fpw1, now);
        }
        mafAliFree(&now);
        now = my_mafNext(sv_maf_file);
        maf_i++;
    }
    //std::cout << "4    \n";
    sv_maf_file->alignments = NULL;
    mafFileFree(&sv_maf_file);
    fclose(fpw1);

    sort_hebing(file_name, name[0]);
    //*****************************************************到现在，前面中心序列顺序无重叠，非中心序列该合并合并********
   //下一步重新读入，in，out位点-->到全局||||||||maf还要分割！|||||以后分割fasta
    std::vector <std::vector<struct maf_four>> OUTS(name.size());

    struct mafFile* maf;
    struct mafAli* head;

    maf = mafReadAll(file_name, 1);
    head = maf->alignments;
    maf->alignments = NULL;
    mafFileFree(&maf);

    now = head;
    maf_i = 0;
    while (now != NULL)
    {
        INS_maf.insert(now->components->start);
        INS_maf.insert(now->components->start + now->components->size);
        for (c = now->components->next; c != NULL; c = c->next) {
            score = maf_i_score(now->components->text, c->text);
            index_c = std::distance(name.begin(), std::find(name.begin(), name.end(), c->src));
            if ((c->strand == '+') ^ (sign[index_c]))//反向
                OUTS[index_c].push_back({ maf_i, size_t(Length[index_c] - c->start - c->size), size_t(Length[index_c] - c->start), score });
            else //同向
                OUTS[index_c].push_back({ maf_i, size_t(c->start), size_t(c->start + c->size), score });
        }
        now = now->next;
        maf_i++;
    }
    for (int i = 1; i < OUTS.size(); i++)
        std::sort(OUTS[i].begin(), OUTS[i].end(), CompareStart());
    std::vector <std::vector<size_t>> delete_maf(maf_i);
    //***************************************第一遍读fasta，更新fasta*******************

    std::string path = arguments::out_file_name + "/fasta/";
    std::vector<std::unique_ptr<Sequence>> strs(name.size());
    if (num != 0)
    {
        strs[0] = std::make_unique<Sequence>(path + std::to_string(num) + ".fasta");
        strs[num] = std::make_unique<Sequence>(path + "0.fasta");

        for (int i = 1; i < name.size(); i++)
        {
            if (i == num)
                continue;
            strs[i] = std::make_unique<Sequence>(path + std::to_string(i) + ".fasta");
        }
    }
    else
    {
        for (int i = 0; i < name.size(); i++)
            strs[i] = std::make_unique<Sequence>(path + std::to_string(i) + ".fasta");
    }
    arguments::fasta_len = strs[0]->size();

    std::set<size_t> all_fenge;//*********************all-fenge
    size_t pre_end = 0;
    //非中心  //非中心序列 更新fasta
    for (int i = 1; i < name.size(); i++)
    {
        j = 0;
        I = 0;
        pre_end = 0;
        for (auto& element : OUTS[i])
        {
            //std::cout << element.start << " " << element.end << std::endl;
            //if (element.start < pre_end)
                //strs[i]->refresh();
            while ((I < arguments::fasta_len) && (j < element.start))
            {
                if ((*strs[i])[I] != '-')
                    j++;
                I++;
            }
            while ((I < arguments::fasta_len) && (j > element.start))
            {
                if ((*strs[i])[I] != '-')
                    j--;
                I--;
            }

            score = 0;
            tmpI = I;
            tmpj = j;
            while ((I < arguments::fasta_len) && (j < element.end))
            {
                if ((*strs[i])[I] != '-')
                {
                    if (((*strs[i])[I] != 'N') && ((*strs[i])[I] == (*strs[0])[I]))
                        score++;
                    j++;
                }
                I++;
            }




            if (0)//(score > element.score)
            {
                //std::cout << score << " " << element.score << " " << i <<" " << element.start << " " << element.end << "\n";
                delete_maf[element.imaf].push_back(i);
            }
            else
            {
                I = tmpI;
                j = tmpj;
                all_fenge.insert(I);
                while (I < arguments::fasta_len && (*strs[i])[I] == '-')
                    I++;
                all_fenge.insert(I);
                while ((I < arguments::fasta_len) && (j < element.end))
                {
                    if ((*strs[i])[I] != '-')
                    {
                        //strs[i]->replace_character(I, 'N');
                        j++;
                    }
                    I++;
                }
                all_fenge.insert(I);
                pre_end = element.end;
            }
        }
        //strs[i]->refresh();
    }
    std::vector <std::vector<struct maf_four>>().swap(OUTS);
    //中心
    //中心序列 new_fenge
    j = 0;
    I = 0;
    for (const auto& element : INS_maf) {
        while (j < element)
        {
            if ((*strs[0])[I] != '-')
                j++;
            I++;
        }
        all_fenge.insert(I);
        //std::cout << I << " " << element << "\n";
        while (I < arguments::fasta_len && (*strs[0])[I] == '-')
            I++;
        all_fenge.insert(I);//start
        //std::cout << I << " " << element << "\n";
    }

    //删除maf
    strcpy(file_name, (arguments::out_file_name + "/maf/small.maf").c_str());
    fpw1 = fopen(file_name, "w");
    fprintf(fpw1, "##maf version=1 scoring=N/A\n");

    now = head;
    maf_i = 0;
    size_t new_maf_i = 0;
    std::vector<struct maf_three> INS2;
    while (now != NULL)
    {
        nchar = 0;
        for (c = now->components->next; c != NULL; c = c->next) {
            nchar++;
        }
        if (nchar > delete_maf[maf_i].size())
        {
            INS2.push_back({ new_maf_i++ , (size_t)now->components->start,(size_t)(now->components->start + now->components->size) });
            fprintf(fpw1, "a score=0\n");
            for (c = now->components; c != NULL; c = c->next) {
                index_c = std::distance(name.begin(), std::find(name.begin(), name.end(), c->src));
                if (std::find(delete_maf[maf_i].begin(), delete_maf[maf_i].end(), index_c) == delete_maf[maf_i].end())
                    fprintf(fpw1, "s %-*s %*d %*d %c %*d %s\n",
                        srcChars, c->src, startChars, c->start, sizeChars, c->size,
                        c->strand, srcSizeChars, c->srcSize, c->text);
            }
            fprintf(fpw1, "\n");	// blank separator line
        }
        maf_i++;
        head = now;
        now = now->next;
        mafAliFree(&head);
    }
    fclose(fpw1);
    //******************************再分割maf************
    std::set<size_t> all_fenge_j0;
    j = 0;
    I = 0;
    for (const auto& element : all_fenge) {
        while (I < element)
        {
            if ((*strs[0])[I] != '-')
                j++;
            I++;
        }
        all_fenge_j0.insert(j);
    }

    auto it2 = all_fenge_j0.begin();
    std::vector <std::set<size_t>> maf_fenge2(INS2.size());
    if (!all_fenge_j0.empty())
        for (const auto& element : INS2) {
            j = element.start;
            while ((it2 != all_fenge_j0.begin()) && (j < *it2))
                it2--;
            while ((it2 != all_fenge_j0.end()) && (j > *it2))
                ++it2;
            //j == *it2;
            while (it2 != all_fenge_j0.end() && (*it2 <= element.end))
            {
                maf_fenge2[element.imaf].insert(*it2);
                ++it2;
            }
            if (it2 == all_fenge_j0.end())
                it2 = std::prev(all_fenge_j0.end());
        }
    std::set<size_t>().swap(all_fenge_j0);

    maf = mafReadAll(file_name, 1);
    head = maf->alignments;
    maf->alignments = NULL;
    mafFileFree(&maf);
    strcpy(file_name, (arguments::out_file_name + "/maf/small_.maf").c_str());
    fpw1 = fopen(file_name, "w");
    fprintf(fpw1, "##maf version=1 scoring=N/A\n");
    maf_i = 0;
    now = head;
    while (now != NULL)
    {
        if (maf_fenge2[maf_i].size() > 2)
        {
            j = now->components->start;
            I = 0;
            auto itc = maf_fenge2[maf_i].begin();
            std::set<size_t>().swap(new_fenge);
            new_fenge.insert(0);
            for (char* ptr = now->components->text; *ptr != '\0'; ++ptr) {
                if ((itc != maf_fenge2[maf_i].end()) && (j == *itc))
                {
                    new_fenge.insert(I);
                    itc++;
                }
                if ((*ptr) != '-')
                    j++;
                I++;
            }
            new_fenge.insert(I);
            std::vector<size_t>().swap(vec_new_fenge);
            vec_new_fenge.reserve(new_fenge.size()); // 提前分配足够的空间
            std::copy(new_fenge.begin(), new_fenge.end(), std::back_inserter(vec_new_fenge));//set 转  vector
            /*
            std::cout << " | ";
            for (const auto& element : new_fenge) {
                std::cout << element << " ";
            }std::cout << "\n";

            std::cout << " | ";
            for (const auto& element : vec_new_fenge) {
                std::cout << element << " ";
            }std::cout << "\n";
            */

            for (int e = 0; e < vec_new_fenge.size() - 1; e++)
            {
                tag_n = true;
                newString = new char[vec_new_fenge[e + 1] - vec_new_fenge[e] + 1];
                for (c = now->components; c != NULL; c = c->next)
                {
                    strncpy(newString, c->text + vec_new_fenge[e], vec_new_fenge[e + 1] - vec_new_fenge[e]);
                    newString[vec_new_fenge[e + 1] - vec_new_fenge[e]] = '\0';
                    nchar = 0;
                    for (char* ptr = newString; *ptr != '\0'; ++ptr)
                        if ((*ptr) != '-')
                            nchar++;
                    if (strcmp(c->src, name[0].c_str()) == 0)
                    {
                        if (nchar != 0)
                            fprintf(fpw1, "a score=0\n");
                        else
                            tag_n = false;
                    }

                    if (tag_n && nchar != 0)
                    {
                        fprintf(fpw1, "s %s %d %lu %c %d %s\n",
                            c->src, c->start, nchar, c->strand, c->srcSize, newString);
                    }
                    c->start += nchar;
                }
                delete[]newString;
                fprintf(fpw1, "\n");
            }
        }
        else
        {
            mafWrite(fpw1, now);
        }
        maf_i++;
        head = now;
        now = now->next;
        mafAliFree(&head);
    }
    fclose(fpw1);
    //***************************************第二遍读fasta+maf，输出maf*******************
    size_t fi, mi;
    //std::cout << "6    \n";
    std::vector<maf_two> insert_fa;
    std::vector<maf_two> insert_maf;

    sv_maf_file = mafOpen(file_name, 1);
    now = my_mafNext(sv_maf_file);
    strcpy(file_name, (arguments::out_file_name + "/result.maf").c_str());
    fpw1 = fopen(file_name, "w");
    fprintf(fpw1, "##maf version=1 scoring=N/A\n");
    std::vector<size_t> index(name.size(), 0);
    //std::cout << "7    \n";
    I = 0;
    all_fenge.insert(0);
    all_fenge.insert(arguments::fasta_len);
    std::vector<size_t> vec_all_fenge(all_fenge.begin(), all_fenge.end());
    for (int i = 0; i < vec_all_fenge.size() - 1; i++)
    {
        if (now != NULL && index[0] == now->components->start && (*strs[0])[vec_all_fenge[i]] != '-')
        {
            newString = new char[vec_all_fenge[i + 1] - vec_all_fenge[i] + 1];
            strs[0]->get_str(newString, vec_all_fenge[i], vec_all_fenge[i + 1]);
            star_hebing(newString, now->components->text, insert_fa, insert_maf);
            fprintf(fpw1, "a score=0\n");
            //FASTA 0-seq
            fprintf(fpw1, "s %-*s %*lu %*d %c %*lu ",
                srcChars, name[0].c_str(), startChars, index[0], sizeChars, now->components->size,
                sign[0] ? '+' : '-', srcSizeChars, Length[0]);
            fi = 0;
            nchar = 0;
            I = 0;
            for (char* ptr = newString; *ptr != '\0'; ++ptr)
            {
                if (fi < insert_fa.size() && insert_fa[fi].start == I)
                {
                    for (int g = 0; g < insert_fa[fi].end; g++)
                        fprintf(fpw1, "-");
                    fi++;
                }
                if ((*ptr) != '-')
                    nchar++;
                I++;
                fprintf(fpw1, "%c", (*ptr));
            }
            while (fi < insert_fa.size())
            {
                for (int g = 0; g < insert_fa[fi].end; g++)
                    fprintf(fpw1, "-");
                fi++;
            }
            fprintf(fpw1, "\n");
            index[0] += nchar;
            //FASTA 1~n-seq
            for (j = 1; j < name.size(); j++)
            {
                strs[j]->get_str(newString, vec_all_fenge[i], vec_all_fenge[i + 1]);
                nchar = 0;
                ACGTchar = 0;
                for (char* ptr = newString; *ptr != '\0'; ++ptr)
                {
                    if ((*ptr) != '-')
                    {
                        nchar++;
                        if ((*ptr) != 'N')
                            ACGTchar++;
                    }
                }
                if (ACGTchar != 0)
                {
                    fprintf(fpw1, "s %-*s %*lu %*lu %c %*lu ",
                        srcChars, name[j].c_str(), startChars, index[j], sizeChars, nchar,
                        sign[j] ? '+' : '-', srcSizeChars, Length[j]);

                    fi = 0;
                    I = 0;
                    for (char* ptr = newString; *ptr != '\0'; ++ptr)
                    {
                        if (fi < insert_fa.size() && insert_fa[fi].start == I)
                        {
                            for (int g = 0; g < insert_fa[fi].end; g++)
                                fprintf(fpw1, "-");
                            fi++;
                        }
                        //if ((*ptr) != '-')
                            I++;
                        fprintf(fpw1, "%c", (*ptr));
                    }
                    while (fi < insert_fa.size())
                    {
                        for (int g = 0; g < insert_fa[fi].end; g++)
                            fprintf(fpw1, "-");
                        fi++;
                    }
                    fprintf(fpw1, "\n");
                }
                index[j] += nchar;
            }
            delete[] newString;

            //MAF 1~n-seq
            for (c = now->components->next; c != NULL; c = c->next)
            {
                fprintf(fpw1, "s %-*s %*d %*d %c %*d ",
                    srcChars, c->src, startChars, c->start, sizeChars, c->size,
                    c->strand, srcSizeChars, c->srcSize);

                mi = 0;
                I = 0;
                //std::cout << c->text << "\n";
                for (char* ptr = c->text; *ptr != '\0'; ++ptr)
                {
                    if (mi < insert_maf.size() && insert_maf[mi].start == I)
                    {
                        for (int g = 0; g < insert_maf[mi].end; g++)
                            fprintf(fpw1, "-");
                        mi++;
                    }
                    I++;
                    fprintf(fpw1, "%c", (*ptr));
                }
                while (mi < insert_maf.size())
                {
                    for (int g = 0; g < insert_maf[mi].end; g++)
                        fprintf(fpw1, "-");
                    mi++;
                }
                fprintf(fpw1, "\n");
            }
            fprintf(fpw1, "\n");	// blank separator line

            mafAliFree(&now);
            now = my_mafNext(sv_maf_file);
            //std::cout << index[0] << "  over\n";
        }
        else if (now != NULL && index[0] > now->components->start) //没合并上的，单独输出
        {
            my_mafWrite(fpw1, now, 1, 2, 0, 0);
            mafAliFree(&now);
            now = my_mafNext(sv_maf_file);
            i--;
        }
        else
        {
            tag_n = true;
            newString = new char[vec_all_fenge[i + 1] - vec_all_fenge[i] + 1];
            for (j = 0; j < name.size(); j++)
            {
                strs[j]->get_str(newString, vec_all_fenge[i], vec_all_fenge[i + 1]);
                nchar = 0;
                ACGTchar = 0;
                for (char* ptr = newString; *ptr != '\0'; ++ptr)
                {
                    if ((*ptr) != '-')
                    {
                        nchar++;
                        if ((*ptr) != 'N')
                            ACGTchar++;
                    }
                }

                if (ACGTchar != 0)
                {
                    if (tag_n)
                    {
                        tag_n = false;
                        fprintf(fpw1, "a score=0\n");
                    }
                    fprintf(fpw1, "s %-*s %*lu %*lu %c %*lu %s\n",
                        srcChars, name[j].c_str(), startChars, index[j], sizeChars, nchar,
                        sign[j] ? '+' : '-', srcSizeChars, Length[j], newString);
                }
                index[j] += nchar;
            }
            if (!tag_n)
                fprintf(fpw1, "\n");	// blank separator line
            delete[] newString;
        }
    }

    sv_maf_file->alignments = NULL;
    mafFileFree(&sv_maf_file);
    fclose(fpw1);
    strs.clear();
    //deleteFile(arguments::out_file_name + "/maf/small_.maf");

}


//向前延伸
utils::in_block* Stream::qiansu(std::vector<std::unique_ptr<Sequence>>& sequences, utils::in_block* pre, utils::in_block* now, bool* gap_tag, int* gap_numi, int thresh)
{
    int pre_end = 0, ai, stopi = 0, gap_num = 0;
    float scorei100 = 0.0;
    float scorei = 0.0;
    float p, n;
    bool tag, do_tag = false;
    utils::in_block* tmp = now;
    utils::in_block* ttmp = new utils::in_block();
    if (pre != NULL) pre_end = pre->end;    //确定上界
    for (int i = 0; i < now->name.size(); i++)  //初始化gap_tag
    {
        gap_numi[now->name[i]] = 0;
        //if (getStr(sequences[now->name[i]], now->name[i], now->start) != '\7') gap_tag[now->name[i]] = true;
        if ((sequences[now->name[i]])->getStr(now->start) != '\7') gap_tag[now->name[i]] = true;
        else gap_tag[now->name[i]] = false;
    }

    ai = now->start - 1;
    while (ai >= pre_end)
    {
        scorei = 0.0;
        //score100 += cs[(int)sequences[0][ai]];
        for (int i = 0; i < now->name.size(); i++)
        {
            //if ((getStr(sequences[now->name[i]], now->name[i], ai) == '\7') || (getStr(sequences[0], 0, ai) == '\7'))
            if (((sequences[now->name[i]])->getStr(ai) == '\7') || ((sequences[0])->getStr(ai) == '\7'))
            {
                if (gap_tag[now->name[i]]) { scorei -= d; gap_tag[now->name[i]] = false; }
                else  scorei -= e;
            }
            else { scorei += HOXD70[(sequences[0])->getStr(ai)][(sequences[now->name[i]])->getStr(ai)]; gap_tag[i] = true; }
        }
        p = 100.0 / (now->end - now->start + 1);
        n = 1.0 * (now->end - now->start) / (now->end - now->start + 1);
        scorei = scorei / now->name.size() * p + now->score * n;
        //std::cout << scorei << " " << scorei + now->score << " " << (now->score_100 + cs[(int)sequences[0][ai]]) * thresh / 100.0 << " **********\n";
        scorei100 = cs[(sequences[0])->getStr(ai)] * p + now->score_100 * n;
        if (scorei >= (scorei100 * thresh / 100.0))
        {
            for (int i = 0; i < now->name.size(); i++)
                if ((sequences[now->name[i]])->getStr(ai) != '\7') { now->si[i]--; now->length[i]++; }
            now->start--;
            now->score = scorei;
            now->score_100 = scorei100;
        }
        else break;

        tag = true;
        for (int i = 0; i < now->name.size(); i++)
            if ((sequences[now->name[i]])->getStr(ai) == '\7')
            {
                tag = false;
                gap_num++;
                gap_numi[now->name[i]]++;
                if (gap_numi[now->name[i]] >= stop_g)
                    do_tag = true;
            }
        if (tag)
        {
            gap_num = 0;
            stopi = 0;
            for (int i = 0; i < now->name.size(); i++)  //初始化gap_tag
                gap_numi[now->name[i]] = 0;
        }//重置
        else if (stopi == 0)
        {
            tmp = ttmp;
            tmp->score = now->score;
            tmp->score_100 = now->score_100;
            tmp->length = now->length;
            tmp->si = now->si;
            tmp->start = now->start;
            stopi++;
        }
        else stopi++;

        if (do_tag || ((stopi >= stop_g) && (gap_num >= stopi * now->name.size() / 2)))
        {
            now->score = tmp->score;
            now->score_100 = tmp->score_100;
            now->length = tmp->length;
            now->si = tmp->si;
            now->start = tmp->start;
            break;
        }
        ai--;
    }
    if ((pre != NULL) && (ai == pre_end - 1))
    {
        if (now->name.size() == pre->name.size())
        {
            tag = true;
            for (int i = 0; i < now->name.size(); i++)
                if (now->name[i] != pre->name[i])tag = false;
            if (tag) //可以连接
            {
                for (int i = 0; i < now->name.size(); i++)
                    pre->length[i] = now->length[i] + pre->length[i];
                p = 1.0 * (pre->end - pre->start) / (pre->end - pre->start + now->end - now->start);
                n = 1.0 * (now->end - now->start) / (pre->end - pre->start + now->end - now->start);
                pre->score = now->score * n + pre->score * p;
                pre->score_100 = now->score_100 * n + pre->score_100 * p;
                pre->end = now->end;
                pre->next = now->next;
                delete now;
                now = pre;
            }
        }
    }
    delete ttmp;
    return now;
}
//向后延伸
void Stream::housu(int end, std::vector<std::unique_ptr<Sequence>>& sequences, utils::in_block* now, bool* gap_tag, int* gap_numi, int thresh)
{
    int ai, gap_num = 0, stopi = 0;
    utils::in_block* tmp = now;
    utils::in_block* ttmp = new utils::in_block();
    int next_end = end;
    float scorei = 0.0, scorei100 = 0.0, n, p;
    bool tag, do_tag = false;
    if (now->next != NULL)  next_end = now->next->start;    //确定上界
    for (int i = 0; i < now->name.size(); i++)  //初始化gap_tag
    {
        gap_numi[now->name[i]] = 0;
        if ((sequences[now->name[i]])->getStr(now->end - 1) != '\7') gap_tag[now->name[i]] = true;
        else gap_tag[now->name[i]] = false;
    }

    ai = now->end;
    while (ai < end)
    {
        if (ai == next_end)
        {
            tmp = now->next;
            if (now->name.size() == tmp->name.size())
            {
                tag = true;
                for (int i = 0; i < now->name.size(); i++)
                    if (now->name[i] != tmp->name[i])tag = false;
                if (tag) //可以连接
                {
                    for (int i = 0; i < now->name.size(); i++)
                        now->length[i] = now->length[i] + tmp->length[i];
                    p = 1.0 * (tmp->end - tmp->start) / (tmp->end - tmp->start + now->end - now->start);
                    n = 1.0 * (now->end - now->start) / (tmp->end - tmp->start + now->end - now->start);
                    now->score = now->score * n + tmp->score * p;
                    now->score_100 = now->score_100 * n + tmp->score_100 * p;

                    now->end = tmp->end;
                    now->next = tmp->next;
                    delete tmp;

                    for (int i = 0; i < now->name.size(); i++)  //初始化gap_tag
                        if ((sequences[now->name[i]])->getStr(now->end - 1) != '\7') gap_tag[now->name[i]] = true;
                        else gap_tag[now->name[i]] = false;
                    ai = now->end;

                    if (now->next != NULL) next_end = now->next->start;
                    else next_end = end;
                    gap_num = stopi = 0;
                    do_tag = false;
                    for (int i = 0; i < now->name.size(); i++)  //初始化gap_tag
                        gap_numi[now->name[i]] = 0;
                    continue;
                }
                else break;//不能连接
            }
            else break;//不能连接
        }
        scorei = 0.0;
        for (int i = 0; i < now->name.size(); i++)
        {
            if (((sequences[now->name[i]])->getStr(ai) == '\7') || ((sequences[0])->getStr(ai) == '\7'))
            {
                if (gap_tag[now->name[i]]) { scorei -= d; gap_tag[now->name[i]] = false; }
                else  scorei -= e;
            }
            else { scorei += HOXD70[(sequences[0])->getStr(ai)][(sequences[now->name[i]])->getStr(ai)]; gap_tag[i] = true; }
        }
        p = 100.0 / (now->end - now->start + 1);
        n = 1.0 * (now->end - now->start) / (now->end - now->start + 1);
        scorei = scorei / now->name.size() * p + now->score * n;
        //std::cout << scorei << " " << scorei + now->score << " " << (now->score_100 + cs[(int)sequences[0][ai]]) * thresh / 100.0 << " **********\n";
        scorei100 = cs[(sequences[0])->getStr(ai)] * p + now->score_100 * n;
        if (scorei >= (scorei100 * thresh / 100.0))
        {
            for (int i = 0; i < now->name.size(); i++)
                if ((sequences[now->name[i]])->getStr(ai) != '\7')  now->length[i]++;
            now->end++;
            now->score = scorei;
            now->score_100 = scorei100;
        }
        else break;

        tag = true;
        for (int i = 0; i < now->name.size(); i++)
            if ((sequences[now->name[i]])->getStr(ai) == '\7')
            {
                gap_num++;
                gap_numi[now->name[i]]++;
                if (gap_numi[now->name[i]] >= stop_g)
                    do_tag = true;
                tag = false;
            }
        if (tag)
        {
            gap_num = 0;
            stopi = 0;
            for (int i = 0; i < now->name.size(); i++)  //初始化gap_tag
                gap_numi[now->name[i]] = 0;
        }//重置
        else if (stopi == 0)
        {
            tmp = ttmp;
            tmp->score = now->score;
            tmp->score_100 = now->score_100;
            tmp->length = now->length;
            tmp->end = now->end;
            stopi++;
        }
        else stopi++;

        if (do_tag || ((stopi >= stop_g) && (gap_num >= stopi * now->name.size() / 2)))
        {
            now->score = tmp->score;
            now->score_100 = tmp->score_100;
            now->length = tmp->length;
            now->end = tmp->end;
            break;
        }
        ai++;
    }
    delete ttmp;
}

void get_all_100(std::vector<std::unique_ptr<Sequence>>& sequence, std::vector<std::vector<unsigned char>>& seq100)
{

    for (int i = 0; i < sequence.size(); i++)
    {
        sequence[i]->get100(seq100[i]);
    }
}

//前后延伸的maf100, 重新读入fasta筛选maf。
void Stream::filter_100_maf(std::vector<std::unique_ptr<Sequence>>& sequence, std::vector<std::string>& name, std::vector<size_t>& Length, std::vector<bool>& sign, utils::MAF_info& MAFinfo)//长度，条数，分数
{
    //std::cout << "filter_100_maf start\n";
    int thresh_extend = MAFinfo.thresh3;//thresh3;
    const size_t sequence_number = name.size();
    int* si = new int[sequence_number];
    int* pre_si = new int[sequence_number];
    int* start = new int[sequence_number];
    bool* gap_tag = new bool[sequence_number];
    bool new_tag = true;
    int* gap_num = new int[sequence_number];//当前block gap数量
    int* pre_gap_num = new int[sequence_number];//前面几个block gap数量
    for (int i = 0; i < sequence_number; i++) { si[i] = gap_num[i] = pre_gap_num[i] = 0; gap_tag[i] = true; }
    int  length = 0, ai = 0, fasta_length, aj;
    float score = 0;
    int* score_two = NULL;
    float ave_score = 0, ave_score_100 = 0;
    int name_len = 0, pre = 0, diff;
    for (int i = 0; i < name.size(); i++)
    {
        //replace(name[i].begin(), name[i].end(), ' ', '-');
        if (name_len < name[i].size())
            name_len = name[i].size();
    }
    //std::cout << "1    **\n";
    fasta_length = arguments::fasta_len;
    const auto align_start1 = std::chrono::high_resolution_clock::now(); //插入前

    const auto align_start2 = std::chrono::high_resolution_clock::now(); //插入后

    std::string zf;
    int i = 0, j, leave = fasta_length % 100, hundred = fasta_length / 100, hi = 0;
    float score100;
    //std::cout << "fasta_length = " << fasta_length << "\n";

    bool tag = false;
    //std::cout << "filter_100_maf 1\n";
    std::vector<float> sum_score;
    std::vector<float> sum_score_100;
    std::vector<int> sum_index;
    std::vector<int> index;
    std::vector<int> nscore(sequence_number);
    bool* out_gap;
    utils::in_block* new_block;
    utils::in_block* all_block = new utils::in_block(); //所有的block 先用结构体存储,头节点。
    all_block->next = NULL;
    utils::in_block* now = all_block;
    std::vector<unsigned char> ci(sequence_number);
    std::vector<std::vector<unsigned char>> seq100(sequence_number);
    for (int i = 0; i < sequence_number; i++)
        seq100[i].resize(100);
    //std::cout << "2    **\n";
    ai = 0;
    for (hi = 0; hi < hundred; hi++)  //处理整百
    {
        //Stream::get100(sequence, seq100);
        get_all_100(sequence, seq100);
        score100 = 0;//后面可以 thresh%*score100
        for (i = 0; i < sequence_number; i++) {
            nscore[i] = 0;
            pre_si[i] = si[i];
            pre_gap_num[i] += gap_num[i];
            gap_num[i] = 0;
        }//每100个分数初始化0
        for (ai = hi * 100, aj = 0; aj < 100; ai++, aj++)
        {
            for (i = 0; i < sequence_number; i++) if (seq100[i][aj] != '\7') si[i]++;
            score100 += cs[seq100[0][aj]];
            for (i = 0; i < sequence_number; i++)
            {
                if ((seq100[i][aj] == '\7') || (seq100[0][aj] == '\7'))
                {
                    if (seq100[i][aj] == '\7') gap_num[i]++;
                    if (gap_tag[i]) { nscore[i] -= d; gap_tag[i] = false; }
                    else  nscore[i] -= e;
                }
                else { nscore[i] += HOXD70[seq100[0][aj]][seq100[i][aj]]; gap_tag[i] = true; }
            }
        }

        std::vector<int>().swap(index);
        index = index_100(nscore, score100 * MAFinfo.thresh3 / 100);
        ave_score_100 = score100;
        if (index.size() != 0)
        {
            ave_score = 0;
            for (i = 0; i < index.size(); i++) ave_score += nscore[index[i]];
            ave_score /= index.size();
            if (sum_score.size() == 0)
            {
                for (i = 0; i < sequence_number; i++) { start[i] = pre_si[i]; pre_gap_num[i] = 0; }
                sum_index.assign(index.begin(), index.end());
                sum_score.push_back(ave_score);
                sum_score_100.push_back(score100);

                continue;
            }
            else if (sum_index.size() == index.size())
            {
                for (i = 0; i < sum_index.size(); i++) if (sum_index[i] != index[i]) break;
                if (i == sum_index.size()) { sum_score.push_back(ave_score); sum_score_100.push_back(score100); continue; }
            }
            //else
        }
        //else

        if (sum_score.size() == 0)//当前为废100，
        {
            for (int i = 0; i < sequence_number; i++) pre_gap_num[i] = gap_num[i] = 0;//把当前100gap数清掉;
            continue;
        }
        else
        {
            score = 0; score100 = 0; length = sum_score.size() * 100;
            for (i = 0; i < sum_score.size(); i++) { score += sum_score[i]; score100 += sum_score_100[i]; }
            score /= sum_score.size();
            score100 /= sum_score_100.size();

            new_block = new utils::in_block();
            new_block->score = score;
            new_block->score_100 = score100;
            for (j = 0; j < sum_index.size(); j++)
            {
                i = sum_index[j];
                new_block->name.push_back(i);
                new_block->si.push_back(start[i]);
                new_block->length.push_back(length - pre_gap_num[i]);
                new_block->start = ai - length - 100;
                new_block->end = ai - 100;
                new_block->next = NULL;
            }
            now->next = new_block;
            now = new_block;

            //写完了
            for (int i = 0; i < sequence_number; i++) pre_gap_num[i] = 0;//写完一组，清空pre gap数
            std::vector<float>().swap(sum_score);
            std::vector<float>().swap(sum_score_100);
            std::vector<int>().swap(sum_index);
            if (index.size() != 0)//从上面筛选下来，连不上，但是分数合格的首100
            {
                for (i = 0; i < sequence_number; i++) start[i] = pre_si[i];
                sum_index.assign(index.begin(), index.end());
                sum_score.push_back(ave_score);
                sum_score_100.push_back(ave_score_100);

                continue;
            }
        }
    }
    //处理后续的leave
    //std::cout << "3    **\n";
    score100 = 0;//-d * leave / 100;
    for (i = 0; i < sequence_number; i++)
    {
        nscore[i] = 0;
        pre_si[i] = si[i];
        pre_gap_num[i] += gap_num[i];
        gap_num[i] = 0;
    }//预处理
    //std::cout << "4    **\n";
    for (; ai < fasta_length; ai++)
    {
        for (i = 0; i < sequence_number; i++)
        {
            //ci[i] = next(sequence[i]);
            ci[i] = sequence[i]->next();
            if (ci[i] != '\7') si[i]++;
        }
        score100 += cs[ci[0]];
        for (i = 0; i < sequence_number; i++)
        {
            if ((ci[i] == '\7') || (ci[0] == '\7'))
            {
                if (ci[i] == '\7') gap_num[i]++;
                if (gap_tag[i]) { nscore[i] -= d; gap_tag[i] = false; }
                else  nscore[i] -= e;
            }
            else { nscore[i] += HOXD70[ci[0]][ci[i]]; gap_tag[i] = true; }
        }
    }
    //std::cout << "5    **\n";
    std::vector<int>().swap(index);
    index = index_100(nscore, score100 * MAFinfo.thresh3 / 100);
    ave_score_100 = score100;
    //std::cout << "6    **\n";
    if (index.size() != 0)
    {
        ave_score = 0;

        for (i = 0; i < index.size(); i++) ave_score += nscore[index[i]]; //std::cout << "???" << index[i] << "\n"; 
        ave_score /= index.size();
        new_tag = false;

        if (sum_index.size() == index.size())
        {
            for (i = 0; i < sum_index.size(); i++) if (sum_index[i] != index[i]) { new_tag = true; break; }

            if (i == sum_index.size()) //qian100 + leave
            {
                sum_score.push_back(ave_score);
                sum_score_100.push_back(score100);
                score = 0; score100 = 0; length = (sum_score.size() - 1) * 100 + leave;
                for (int i = 0; i < sum_score.size(); i++) { score += sum_score[i]; score100 += sum_score_100[i]; }

                score /= sum_score.size();
                score100 /= sum_score_100.size();

                new_block = new utils::in_block();
                new_block->score = score;
                new_block->score_100 = score100;
                for (j = 0; j < sum_index.size(); j++)
                {
                    i = sum_index[j];
                    new_block->name.push_back(i);
                    new_block->si.push_back(start[i]);
                    new_block->length.push_back(length - pre_gap_num[i] - gap_num[i]);
                    new_block->start = ai - length;
                    new_block->end = ai;
                    new_block->next = NULL;
                }
                now->next = new_block;
                now = new_block;
            }
        }

        if ((sum_index.size() != index.size()) || new_tag)//qian100,  ,leave
        {
            if (sum_score.size() > 0)
            {
                score = 0; score100 = 0; length = sum_score.size() * 100;                    //qian100,
                for (i = 0; i < sum_score.size(); i++) { score += sum_score[i]; score100 += sum_score_100[i]; }
                score /= sum_score.size();
                score100 /= sum_score_100.size();

                new_block = new utils::in_block();
                new_block->score = score;
                new_block->score_100 = score100;
                for (j = 0; j < sum_index.size(); j++)
                {
                    i = sum_index[j];
                    new_block->name.push_back(i);
                    new_block->si.push_back(start[i]);
                    new_block->length.push_back(length - pre_gap_num[i]);
                    new_block->start = ai - length - leave;
                    new_block->end = ai - leave;
                    new_block->next = NULL;
                }
                now->next = new_block;
                now = new_block;
            }

            length = leave;                                                //leave
            score = ave_score;

            new_block = new utils::in_block();
            new_block->score = score;
            new_block->score_100 = ave_score_100;
            for (j = 0; j < index.size(); j++)
            {
                i = index[j];
                new_block->name.push_back(i);
                new_block->si.push_back(pre_si[i]);
                new_block->length.push_back(length - gap_num[i]);
                new_block->start = ai - length;
                new_block->end = ai;
                new_block->next = NULL;
            }
            now->next = new_block;
            now = new_block;
        }
        //else
    }
    else if (sum_score.size() > 0) //qian 100
    {
        score = 0; score100 = 0; length = sum_score.size() * 100;
        for (i = 0; i < sum_score.size(); i++) { score += sum_score[i]; score100 += sum_score_100[i]; }
        score /= sum_score.size();
        score100 /= sum_score_100.size();

        new_block = new utils::in_block();
        new_block->score = score;
        new_block->score_100 = score100;
        for (j = 0; j < sum_index.size(); j++)
        {
            i = sum_index[j];
            new_block->name.push_back(i);
            new_block->si.push_back(start[i]);
            new_block->length.push_back(length - pre_gap_num[i]);
            new_block->start = ai - length - leave;
            new_block->end = ai - leave;
            new_block->next = NULL;
        }
        now->next = new_block;
    }
    //std::cout << "7    **\n";
    //前后溯
    if (all_block->next == NULL);
    else
    {
        //首个
        now = all_block->next;

        qiansu(sequence, NULL, now, gap_tag, gap_num, thresh_extend);
        //非首个
        new_block = now;
        now = now->next;
        while (now != NULL)
        {
            now = qiansu(sequence, new_block, now, gap_tag, gap_num, thresh_extend);
            housu(fasta_length, sequence, now, gap_tag, gap_num, thresh_extend);
            new_block = now;
            now = now->next;
        }

    }
    //std::cout << "8    **\n";
    //std::cout << "filter_100_maf 4\n";

    int an, bn, kk;
    utils::in_block* ppre = NULL;
    utils::MSA_ni_block* MSA_head = NULL, * niow = NULL;
    int seqi;
    utils::in_block* ttmp;
    int I;
    std::vector<std::size_t>::iterator it;
    if(0)//是否按中间动态规划的逆补片段裁剪正向？multiz不报错不用
    { 
        for (int k = 1; k < arguments::seq_num; k++)
        {
            //std::cout << arguments::out_file_name + "/maf/" + std::to_string(k) + ".tmp" << " filter_100_maf 5\n";
            MSA_head = read_ni_maf(seqi, name, arguments::out_file_name + "/maf/" + std::to_string(k) + ".tmp");

            //B
            niow = MSA_head->next;
            now = all_block->next;
            ppre = all_block;
            // std::cout << "start a?\n";
            while (now != NULL && niow != NULL)
            {
                it = std::find(now->name.begin(), now->name.end(), seqi);
                if (now->name[0] == 0 and it != now->name.end())
                {
                    I = it - now->name.begin();
                    if (niow->start[1] >= (now->si[I] + now->length[I])) //完全在后面
                    {
                        ppre = now;
                        now = now->next;
                    }
                    else if (niow->end[1] <= now->si[I])  //完全在前面
                    {
                        //ppre = now;
                        //now = now->next;
                        niow = niow->next;
                    }
                    else if (niow->start[1] <= now->si[I] && niow->end[1] >= (now->si[I] + now->length[I])) //完全大于,覆盖
                    {
                        if (now->name.size() == 1)
                        {
                            ppre->next = now->next;
                            delete now;
                            now = ppre->next;
                        }
                        else
                        {
                            now->name.erase(now->name.begin() + I);
                            now->length.erase(now->length.begin() + I);
                            now->si.erase(now->si.begin() + I);
                        }
                    }
                    else if (niow->start[1] >= now->si[I] && niow->end[1] <= (now->si[I] + now->length[I])) //完全小于,包含于:可能分为两个结点
                    {
                        if (niow->start[1] = now->si[I]) //左不要
                            ;
                        else
                        {
                            ttmp = Stream::my_memcpy(now);

                            now->next = ttmp;
                            an = now->si[I] + now->length[I] - niow->start[1];
                            now->length[I] = now->length[I] - an;
                            for (kk = now->end - 1; an > 0; kk--)
                            {
                                //if (Stream::chars(sequence[seqi], kk) != '-') an--;
                                if ((*sequence[seqi])[kk] != '-') an--;
                            }
                            now->end = kk + 1;

                            ttmp->start = now->end;
                            ttmp->length[I] -= now->length[I];
                            ttmp->si[I] += now->length[I];

                            for (i = 0; i < now->name.size(); i++)
                            {
                                if (i == I) continue;
                                for (kk = now->start; kk < now->end; kk++)
                                    //if (Stream::chars(sequence[now->name[i]], kk) != '-')
                                    if ((*sequence[now->name[i]])[kk] != '-')
                                    {
                                        ttmp->length[i]--;
                                        ttmp->si[i]++;
                                    }
                                now->length[i] -= ttmp->length[i];
                            }

                            ppre = now;
                            now = now->next;
                        }

                        if (niow->end[1] = (now->si[I] + now->length[I])) //右不要
                        {
                            if (now->name.size() == 1)
                            {
                                ppre->next = now->next;
                                delete now;
                                now = ppre->next;
                            }
                            else
                            {
                                now->name.erase(now->name.begin() + I);
                                now->length.erase(now->length.begin() + I);
                                now->si.erase(now->si.begin() + I);
                            }
                        }
                        else
                        {
                            ttmp = Stream::my_memcpy(now);

                            an = niow->end[1] - now->si[I];
                            now->length[I] = now->length[I] - an;
                            now->si[I] = niow->end[1];
                            for (kk = now->start; an > 0; kk++)
                            {
                                //if (Stream::chars(sequence[seqi], kk) != '-') an--;
                                if ((*sequence[seqi])[kk] != '-') an--;
                            }
                            now->start = kk;

                            ttmp->end = now->start;
                            //ttmp->length[0] -= now->length[0];

                            for (i = 0; i < now->name.size(); i++)
                            {
                                if (i == I)continue;
                                for (kk = ttmp->start; kk < ttmp->end; kk++)
                                    //if (Stream::chars(sequence[now->name[i]], kk) != '-')
                                    if ((*sequence[now->name[i]])[kk] != '-')
                                    {
                                        now->length[i]--;
                                        now->si[i]++;
                                    }
                                ttmp->length[i] -= now->length[i];
                            }


                            if (ttmp->name.size() == 1)
                            {
                                delete ttmp;
                            }
                            else
                            {
                                ttmp->next = now;
                                ppre->next = ttmp;
                                ttmp->name.erase(ttmp->name.begin() + I);
                                ttmp->length.erase(ttmp->length.begin() + I);
                                ttmp->si.erase(ttmp->si.begin() + I);
                                ppre = ttmp;
                            }


                            //ppre = now;
                            //now = now->next;
                        }
                    }
                    else if (niow->start[1] <= now->si[I] && niow->end[1] < (now->si[I] + now->length[I])) //左端部分重叠：起点，长度都变
                    {
                        ttmp = Stream::my_memcpy(now);

                        an = niow->end[1] - now->si[I];
                        now->length[I] = now->length[I] - an;
                        now->si[I] = niow->end[1];
                        for (kk = now->start; an > 0; kk++)
                        {
                            //if (Stream::chars(sequence[seqi], kk) != '-') an--;
                            if ((*sequence[seqi])[kk] != '-') an--;
                        }
                        now->start = kk;

                        ttmp->end = now->start;
                        //ttmp->length[0] -= now->length[0];

                        for (i = 0; i < now->name.size(); i++)
                        {
                            if (i == I)continue;
                            for (kk = ttmp->start; kk < ttmp->end; kk++)
                                //if (Stream::chars(sequence[now->name[i]], kk) != '-')
                                if ((*sequence[now->name[i]])[kk] != '-')
                                {
                                    now->length[i]--;
                                    now->si[i]++;
                                }
                            ttmp->length[i] -= now->length[i];
                        }

                        if (ttmp->name.size() == 1)
                        {
                            delete ttmp;
                        }
                        else
                        {
                            ttmp->next = now;
                            ppre->next = ttmp;
                            ttmp->name.erase(ttmp->name.begin() + I);
                            ttmp->length.erase(ttmp->length.begin() + I);
                            ttmp->si.erase(ttmp->si.begin() + I);
                            ppre = ttmp;
                        }
                        //ppre = now;
                        //now = now->next;


                    }
                    else if (niow->start[1] > now->si[I] && niow->end[1] >= (now->si[I] + now->length[I])) //右端部分重叠：终点，长度都变
                    {
                        ttmp = Stream::my_memcpy(now);

                        an = now->si[I] + now->length[I] - niow->start[1];
                        now->length[I] = now->length[I] - an;
                        for (kk = now->end - 1; an > 0; kk--)
                        {
                            //if (Stream::chars(sequence[seqi], kk) != '-') an--;
                            if ((*sequence[seqi])[kk] != '-')an--;
                        }
                        now->end = kk + 1;

                        ttmp->start = now->end;
                        ttmp->length[I] -= now->length[I];
                        ttmp->si[I] += now->length[I];

                        for (i = 0; i < now->name.size(); i++)
                        {
                            if (i == I)continue;
                            for (kk = now->start; kk < now->end; kk++)
                                //if (Stream::chars(sequence[now->name[i]], kk) != '-')
                                if ((*sequence[now->name[i]])[kk] != '-')
                                {
                                    ttmp->length[i]--;
                                    ttmp->si[i]++;
                                }
                            now->length[i] -= ttmp->length[i];
                        }

                        if (ttmp->name.size() == 1)
                        {
                            delete ttmp;
                            ppre = now;
                            now = now->next;
                        }
                        else
                        {
                            now->next = ttmp;
                            ttmp->name.erase(ttmp->name.begin() + I);
                            ttmp->length.erase(ttmp->length.begin() + I);
                            ttmp->si.erase(ttmp->si.begin() + I);
                            ppre = ttmp;
                            now = ttmp->next;
                        }
                    }
                }
                else
                {
                    ppre = now;
                    now = now->next;
                }
            }
            //std::cout << "start c?\n";

            //A
            niow = MSA_head->next;
            now = all_block->next;
            ppre = all_block;
            // std::cout << "start a?\n";
            while (now != NULL && niow != NULL)
            {
                it = std::find(now->name.begin(), now->name.end(), seqi);
                if (it != now->name.end() && now->name[0] == 0)
                {
                    if (niow->start[0] >= (now->si[0] + now->length[0])) //完全在后面
                    {
                        ppre = now;
                        now = now->next;
                    }
                    else if (niow->end[0] <= now->si[0])  //完全在前面
                    {
                        //ppre = now;
                        //now = now->next;
                        niow = niow->next;
                    }
                    else if (niow->start[0] <= now->si[0] && niow->end[0] >= (now->si[0] + now->length[0])) //完全大于,覆盖
                    {
                        if (now->name.size() == 1)
                        {
                            ppre->next = now->next;
                            delete now;
                            now = ppre->next;
                        }
                        else
                        {
                            now->name.erase(now->name.begin());
                            now->length.erase(now->length.begin());
                            now->si.erase(now->si.begin());
                        }
                    }
                    else if (niow->start[0] >= now->si[0] && niow->end[0] <= (now->si[0] + now->length[0])) //完全小于,包含于:可能分为两个结点
                    {
                        if (niow->start[0] = now->si[0]) //左不要
                            ;
                        else
                        {
                            //std::cout << niow->start[0] << " " << niow->end[0] << " " << now->si[0] << " " << now->si[0] + now->length[0] << " r1\n";

                            ttmp = Stream::my_memcpy(now);//now左不重，tmp右 

                            now->next = ttmp;
                            an = now->si[0] + now->length[0] - niow->start[0]; //右长
                            now->length[0] = now->length[0] - an;//左长
                            for (kk = now->end - 1; an > 0; kk--) //遍历右，找左终点
                            {
                                //if (Stream::chars(sequence[0], kk) != '-') an--;
                                if ((*sequence[0])[kk] != '-') an--;
                            }
                            now->end = kk + 1;

                            ttmp->start = now->end;
                            ttmp->length[0] -= now->length[0];
                            ttmp->si[0] += now->length[0];

                            for (i = 1; i < now->name.size(); i++)//遍历左，更新右
                            {
                                for (kk = now->start; kk < now->end; kk++)
                                    //if (Stream::chars(sequence[now->name[i]], kk) != '-')
                                    if ((*sequence[now->name[i]])[kk] != '-')
                                    {
                                        ttmp->length[i]--;
                                        ttmp->si[i]++;
                                    }
                                now->length[i] -= ttmp->length[i];
                            }

                            ppre = now;
                            now = now->next;
                        }

                        if (niow->end[0] = (now->si[0] + now->length[0])) //右不要
                        {
                            if (now->name.size() == 1)
                            {
                                ppre->next = now->next;
                                delete now;
                                now = ppre->next;
                            }
                            else
                            {
                                now->name.erase(now->name.begin());
                                now->length.erase(now->length.begin());
                                now->si.erase(now->si.begin());
                            }
                        }
                        else
                        {
                            //std::cout << niow->start[0] << " " << niow->end[0] << " " << now->si[0] << " " << now->si[0] + now->length[0] << " l1\n";
                            ttmp = Stream::my_memcpy(now);//ttmp左中间重叠，now右不重
                            an = niow->end[0] - now->si[0];//ttmp.len
                            now->length[0] = now->length[0] - an;//now is right
                            now->si[0] = niow->end[0];
                            for (kk = now->start; an > 0; kk++)//遍历左，找到左终点，右起点
                            {
                                //if (Stream::chars(sequence[0], kk) != '-') an--;
                                if ((*sequence[0])[kk] != '-')an--;
                            }
                            now->start = kk;

                            ttmp->end = now->start;
                            //ttmp->length[0] -= now->length[0];

                            for (i = 1; i < now->name.size(); i++)//遍历左，更新右
                            {
                                for (kk = ttmp->start; kk < ttmp->end; kk++)
                                    //if (Stream::chars(sequence[now->name[i]], kk) != '-')
                                    if ((*sequence[now->name[i]])[kk] != '-')
                                    {
                                        now->length[i]--;
                                        now->si[i]++;
                                    }
                                ttmp->length[i] -= now->length[i];
                            }

                            if (ttmp->name.size() == 1)
                            {
                                delete ttmp;
                            }
                            else
                            {
                                ttmp->next = now;
                                ppre->next = ttmp;
                                ttmp->name.erase(ttmp->name.begin());
                                ttmp->length.erase(ttmp->length.begin());
                                ttmp->si.erase(ttmp->si.begin());
                                ppre = ttmp;
                            }
                            //ppre = now;
                            //now = now->next;
                        }
                    }
                    else if (niow->start[0] <= now->si[0] && niow->end[0] < (now->si[0] + now->length[0])) //左端部分重叠：起点，长度都变
                    {

                        //qian
                        ttmp = Stream::my_memcpy(now);//tmp左重，now右不重
                        an = niow->end[0] - now->si[0];//左长
                        now->length[0] = now->length[0] - an;//右长
                        now->si[0] = niow->end[0];
                        for (kk = now->start; an > 0; kk++)//遍历左，得右起点
                        {
                            //if (Stream::chars(sequence[0], kk) != '-') an--;
                            if ((*sequence[0])[kk] != '-') an--;
                        }
                        now->start = kk;
                        ttmp->end = now->start;
                        //ttmp->length[0] -= now->length[0];
                        for (i = 1; i < now->name.size(); i++) //遍历左，更新右
                        {
                            for (kk = ttmp->start; kk < ttmp->end; kk++)
                                //if (Stream::chars(sequence[now->name[i]], kk) != '-')
                                if ((*sequence[now->name[i]])[kk] != '-')
                                {
                                    now->length[i]--;
                                    now->si[i]++;
                                }
                            ttmp->length[i] -= now->length[i];
                        }
                        if (ttmp->name.size() == 1)
                        {
                            delete ttmp;
                        }
                        else
                        {
                            ttmp->next = now;
                            ppre->next = ttmp;
                            ttmp->name.erase(ttmp->name.begin());
                            ttmp->length.erase(ttmp->length.begin());
                            ttmp->si.erase(ttmp->si.begin());
                            ppre = ttmp;
                        }
                    }
                    else if (niow->start[0] > now->si[0] && niow->end[0] >= (now->si[0] + now->length[0])) //右端部分重叠：终点，长度都变
                    {
                        //std::cout << niow->start[0] << " " << niow->end[0] << " " << now->si[0] << " " << now->si[0] + now->length[0] << " r\n";
                        ttmp = Stream::my_memcpy(now);//now左边不重，ttmp右边重
                        an = now->si[0] + now->length[0] - niow->start[0];  //右重叠长度
                        now->length[0] = now->length[0] - an;        //左 不重长度
                        for (kk = now->end - 1; an > 0; kk--) //遍历右，找左终点
                        {
                            //if (Stream::chars(sequence[0], kk) != '-') an--;
                            if ((*sequence[0])[kk] != '-') an--;
                        }
                        now->end = kk + 1;

                        ttmp->start = now->end;
                        ttmp->length[0] -= now->length[0];
                        ttmp->si[0] += now->length[0];

                        for (i = 1; i < now->name.size(); i++)//遍历左，更新右
                        {
                            for (kk = now->start; kk < now->end; kk++)
                                //if (Stream::chars(sequence[now->name[i]], kk) != '-')
                                if ((*sequence[now->name[i]])[kk] != '-')
                                {
                                    ttmp->length[i]--;
                                    ttmp->si[i]++;
                                }
                            now->length[i] -= ttmp->length[i];
                        }

                        if (ttmp->name.size() == 1)
                        {
                            delete ttmp;
                            ppre = now;
                            now = now->next;
                        }
                        else
                        {
                            now->next = ttmp;
                            ttmp->name.erase(ttmp->name.begin());
                            ttmp->length.erase(ttmp->length.begin());
                            ttmp->si.erase(ttmp->si.begin());
                            ppre = ttmp;
                            now = ttmp->next;
                        }
                    }
                }
                else
                {
                    ppre = now;
                    now = now->next;
                }
            }
            //std::cout << "start b?\n";

            while (MSA_head)
            {
                niow = MSA_head;
                MSA_head = MSA_head->next;
                delete niow;
            }
        }
    }
    //std::cout << "9    **\n";
    /////////////////////////////////////////////////////
    std::ofstream os(arguments::out_file_name +"/big.maf", std::ios::binary | std::ios::out); //判断输出路径合法否
    if (!os)
    {
        std::cout << "cannot write file " << arguments::out_file_name + "/big.maf" << '\n';
        exit(1);
    }os << "##maf version=1 scoring=lastz.v1.04.00\n#";
    for (i = 0; i < sequence_number; i++) os << i << " ";
    os << "\n";
    //std::cout << "10    **\n";
    now = all_block->next;
    while (now != NULL)
    {
        //写出
        for (j = 0; j < now->length.size(); j++)
            if (now->length[j] == 0)
                break;
        if (j != now->length.size())
        {
            now = now->next;
            continue;
        }

        os << "a score=" << now->score << "\n";
        for (j = 0; j < now->name.size(); j++)
        {
            i = now->name[j];
            if (sign[i])zf = " + "; else zf = " - ";
            os << "s " << std::setw(name_len + 1) << std::left << name[i] << std::setw(9) << std::right << now->si[j] << " " << std::setw(9) << std::right << now->length[j] << zf << std::setw(9) << std::right << Length[i] << " ";
            //Stream::get_put(sequence[i], os, now->start, now->end - now->start);
            sequence[i]->get_put(t10000, os, now->start, now->end - now->start);
            //os << "\n" << now->start << " " << now->end << "\n";
            os << "\n";
        }
        os << "\n";
        //写完
        now = now->next;
    }
    os.close();
    //std::cout << "11    **\n";
    //std::cout << "filter_100_maf end\n";

    while (all_block)
    {
        now = all_block;
        all_block = all_block->next;
        delete now;
    }
    //std::cout << "12    **\n";
    delete[] si;
    delete[] pre_si;
    delete[] start;
    delete[] gap_tag;
    delete[] gap_num;
    delete[] pre_gap_num;
    delete[] score_two;
    std::vector<float>().swap(sum_score);
    std::vector<float>().swap(sum_score_100);
    std::vector<int>().swap(sum_index);
    std::vector<int>().swap(index);
    std::vector<int>().swap(nscore);
    const auto align_start4 = std::chrono::high_resolution_clock::now(); //插入后
    //std::cout << align_start4 - align_start2 << "  choose-output-filter\n";
}

utils::in_block* Stream::my_memcpy(utils::in_block* now)
{
    utils::in_block* ttmp = new utils::in_block();
    ttmp->score = now->score;
    ttmp->score_100 = now->score_100;
    ttmp->start = now->start;
    ttmp->end = now->end;
    ttmp->name = now->name;
    ttmp->length = now->length;
    ttmp->si = now->si;
    ttmp->next = now->next;
    return ttmp;
}

utils::MSA_ni_block* Stream::read_ni_maf(int& seqi, std::vector<std::string>& name, std::string path)
{
    std::ifstream iis(path, std::ios::binary | std::ios::in); //判断N_file_name合法否
    if (!iis)
    {
        std::cout << "cannot access file " << path << '\n';
        exit(1);
    }
    iis >> path;
    //for (int i = 0; i < name.size(); i++)
        //std::cout << name[i] << "\n";
    //std::cout << path << "?\n";
    std::vector<std::string>::iterator it = std::find(name.begin(), name.end(), path);
    seqi = it - name.begin();
    //std::cout << it - name.begin() << "?\n";
    int x;
    utils::MSA_ni_block* headp = new utils::MSA_ni_block();
    utils::MSA_ni_block* head = headp;
    while (true)
    {
        iis >> x;
        if (x == -1)
        {
            head->next = NULL;
            iis.close();
            return headp;
            break;
        }
        head->next = new utils::MSA_ni_block();
        head = head->next;
        head->start[0] = x;
        iis >> head->end[0] >> head->start[1] >> head->end[1];
    }
    iis.close();
    return headp;
}
