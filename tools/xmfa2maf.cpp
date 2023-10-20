#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <iomanip>
#include <chrono>

#if defined(_WIN32)
#include <io.h> 
#include <direct.h>
#include <windows.h>
#include <psapi.h>
#include <process.h>
#elif defined(__unix__) || defined(__unix) || defined(unix)
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/resource.h>
#include <pthread.h>

#endif
#undef max
#undef min

using namespace std;

struct seqi
{
	size_t id;
	size_t start;
	size_t len;
	float score;
	string strand;
	string seq;
};

struct block
{
	float score;
	size_t num;
	vector<seqi> seq;
};

vector<string> names;
vector<size_t> lengths;
size_t NUM=0;
int NSCORE = 0;
int d = 400, e = 30; //首个gap和后续gap
int HOXD70[6][6] = 
{ {},
	{0,91,-114,-31,-123,NSCORE},
	{0,-114,100,-125,-31,NSCORE},
	{0,-31,-125,100,-114,NSCORE},
	{0,-123,-31,-114,91,NSCORE},
	{0,NSCORE,NSCORE,NSCORE,NSCORE,NSCORE} 
};
int cs[8] = { 0,91,100,100,91,0,0,0 };
bool* gap_tag;

unsigned char* _get_map() //统一DNA/RNA 及大小写
{
	static unsigned char map[std::numeric_limits<unsigned char>::max()];
	memset(map, 10, sizeof(map));

	// map['-'] = GAP; // we could not process sequences with '-'
	map['c'] = map['C'] = 2;
	map['g'] = map['G'] = 3;
	map['a'] = map['A'] = 1;
	map['t'] = map['T'] = map['u'] = map['U'] = 4;
	map['N'] = map['R'] = map['Y'] = map['M'] = map['K'] = map['S'] = map['W'] = map['H'] = map['B'] = map['V'] = map['D'] = 5;
	map['n'] = map['r'] = map['y'] = map['m'] = map['k'] = map['s'] = map['w'] = map['h'] = map['b'] = map['v'] = map['d'] = 5;
	map['-'] = 7;
	return map;
}

static const unsigned char* _map = _get_map();

unsigned char M(unsigned char ch)  //预处理，char->int
{
	return _map[ch];
}

template<typename Representation, typename Period>
std::ostream& operator<<(std::ostream& os, std::chrono::duration<Representation, Period> duration) //时间消耗
{
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << " ms";
	return os;
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


void Read_xmfa_head(std::istream& in_stream)
{
	std::string cur_line;
	std::string token;
	if (!getline(in_stream, cur_line))
		return;

	while (getline(in_stream, cur_line))
	{
		if (cur_line.find("Count") != std::string::npos)
		{
			std::stringstream ss1(cur_line);
			std::getline(ss1, token, ' ');
			ss1 >> NUM;
			break;
		}
	}
	if (NUM == 0)
		return;

	size_t num = NUM,i;
	names.resize(NUM + 1);
	lengths.resize(NUM + 1);

	while (getline(in_stream, cur_line))
	{
		if (cur_line[0] != '#')
		{
			in_stream.seekg(0 - (int)cur_line.size() - 1, ios::cur);
			break;
		}
		else if (cur_line.find("Index") != std::string::npos)
		{
			std::stringstream ss2(cur_line);
			std::getline(ss2, token, ' ');
			ss2 >> i;

			getline(in_stream, cur_line);
			getline(in_stream, cur_line);
			cur_line.erase(std::remove_if(cur_line.begin(), cur_line.end(), [](char c) {
				return (c == ' ' || c == '\t');
				}), cur_line.end());
			std::stringstream ss3(cur_line);
			std::getline(ss3, token, '>');
			ss3 >> names[i];

			getline(in_stream, cur_line);
			std::stringstream line_str(cur_line);
			std::getline(line_str, cur_line, ' ');
			std::getline(line_str, cur_line, 'b');
			// take off leading whitespace
			std::stringstream parse_str(cur_line);
			parse_str >> lengths[i];	// the sequence number

			num--;
		}
	}
	if (num != 0)
		return;
}

bool Read_next_block(std::istream& in_stream, block& Block)
{
	Block.num = 0;
	Block.score = 0;
	vector<seqi>().swap(Block.seq);

	std::string cur_line;
	std::string tmp_seq;
	size_t stop;
	if (!getline(in_stream, cur_line))
		return false;

	while (true)
	{
		if (cur_line[0] == '>')
		{
			Block.seq.push_back(seqi());
			Block.num++;

			std::stringstream line_str(cur_line);
			std::getline(line_str, cur_line, '>');
			std::getline(line_str, cur_line, ':');
			std::stringstream parse_str(cur_line);
			parse_str >> Block.seq.back().id;


			std::getline(line_str, cur_line, '-');
			parse_str.clear();
			parse_str.str(cur_line);
			parse_str >> Block.seq.back().start;
			line_str >> stop;
			line_str >> Block.seq.back().strand;
			Block.seq.back().len = stop - Block.seq.back().start + 1;
			tmp_seq = "";
			while (getline(in_stream, cur_line))
			{
				if (cur_line[0] == '=')
				{
					Block.seq.back().seq.swap(tmp_seq);
					return true;
				}
				else if (cur_line[0] == '>')
				{

					Block.seq.back().seq.swap(tmp_seq);
					break;
				}
				tmp_seq += cur_line;
			}
			//if (!getline(in_stream, cur_line))
				//return true;
		}
		else
			break;
	}
	if (Block.num > 0)
	{
		if(Block.seq.back().seq.size()==0 && tmp_seq.size()!=0)
			Block.seq.back().seq.swap(tmp_seq);
		return true;
	}
	else
		return false;
}

void get_score(block& Block)
{
	int len0 = Block.seq[0].seq.size();
	Block.seq[0].score = 0; 
	gap_tag[0] = true;
	for (int i = 1; i < Block.num; i++)
	{
		Block.seq[i].score = 0;
		gap_tag[i] = true;
		if (Block.seq[i].seq.size() != len0)
		{
			std::cerr << "Sequence lengths are not equal!\n";
			exit(1);
		}
	}
	float score100 = 0;
	
	for (int j = 0; j < len0; j++)
	{
		score100 += cs[M(Block.seq[0].seq[j])];
		for (int i = 0; i < Block.num; i++)
		{
			if ((M(Block.seq[i].seq[j]) == '\7') || (M(Block.seq[0].seq[j]) == '\7'))
			{
				if (gap_tag[i]) { Block.seq[i].score -= d; gap_tag[i] = false; }
				else  Block.seq[i].score -= e;
			}
			else { Block.seq[i].score += HOXD70[M(Block.seq[0].seq[j])][M(Block.seq[i].seq[j])]; gap_tag[i] = true; }
		}
	}
	if (Block.num > 1)
	{
		score100 = 0;
		for (int i = 0; i < Block.num; i++)
			score100 += Block.seq[i].score;
		score100 = 1.0*score100 / Block.num;
	}
	Block.score = score100;

}

int main(int argc, char* argv[]) {
	const auto align_start = std::chrono::high_resolution_clock::now(); //记录开始时间
	string in_file_name;// = "C:/Users/周通/Desktop/parsnp.xmfa";
	string out_file_name;// = "C:/Users/周通/Desktop/parsnp.maf";

	int zt = 0;
	if (argc == 2)
	{
		if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-H") == 0 || strcmp(argv[1], "-help") == 0)
		{
			std::cout << "Usage: softname -i xxx.xmfa -o xxx.maf\n";
			exit(0);
		}
	}
	else if (argc == 5)
	{
		for (zt = 1; zt < argc; zt += 2)
		{
			if (strcmp(argv[zt], "-i") == 0)
				in_file_name = argv[zt + 1];
			else if (strcmp(argv[zt], "-o") == 0)
				out_file_name = argv[zt + 1];
			else
				break;
		}
	}
	else
	{
		std::cout << "Parameter quantity error!\n";
		exit(-1);
	}
	if (zt != argc)
	{
		std::cout << "Parameter error!\n";
		exit(-1);
	}

	std::ifstream ifs(in_file_name, std::ios::binary | std::ios::in); //判断输入路径合法否
	if (!ifs)
	{
		std::cout << "cannot access file " << in_file_name << '\n';
		exit(0);
	}
	Read_xmfa_head(ifs);
	
	gap_tag = new bool[NUM + 1];
	//cout << NUM << "\n";
	size_t len_num=0;
	size_t name_num = 0;
	for (int i = 1; i <= NUM; i++)
	{
		if (lengths[i] > len_num)
			len_num = lengths[i];
		if (names[i].size() > name_num)
			name_num = names[i].size();
		//cout << i << " " << names[i] << " " << lengths[i] << "\n";
	}
	len_num = to_string(len_num).size();
	name_num++;

	std::ofstream os(out_file_name, std::ios::binary | std::ios::out); //判断输出路径合法否
	if (!os)
	{
		std::cout << "cannot write file " << out_file_name << '\n';
		exit(0);
	}os << "##maf version=1 scoring=lastz.v1.04.00\n";

	block Block;
	int i;
	while (Read_next_block(ifs, Block))
	{
		get_score(Block);
		
		os << "a score=" << Block.score << "\n";
		for (int j = 0; j < Block.num; j++)
		{
			i = Block.seq[j].id;
			os << "s " << std::setw(name_num) << std::left << names[i] << std::setw(len_num) 
			   << std::right << Block.seq[j].start << " " << std::setw(len_num) << std::right << Block.seq[j].len 
			   <<" " << Block.seq[j].strand <<" " << std::setw(9) << std::right << lengths[i] << " " << Block.seq[j].seq << "\n";
		}
		os << "\n";
	}
	ifs.close();
	os.close();
	std::cout << "Time consumes: " << (std::chrono::high_resolution_clock::now() - align_start) << '\n'; //输出比对耗费时间
	std::cout << "Memory  usage: " << getPeakRSS() << " B\n";//输出内存耗费
	return 0;
}