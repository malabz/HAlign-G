#pragma once
extern "C"
{
#include "multiz/multiz.h"
}

#include <iostream>
#include <vector>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <string>
#include <fstream>

using namespace std;

bool isStringInNames(const char* str, const std::vector<char*>& Names) {
	for (const char* name : Names) {
		if (strcmp(name, str) == 0) {
			return true;
		}
	}
	return false;
}

int findIndex(const char* str, const std::vector<char*>& Names) {
	for (int i = 0; i < Names.size(); i++) {
		if (strcmp(Names[i], str) == 0) {
			return i;
		}
	}
	return -1;  // 返回-1表示未找到
}

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		std::cout << "Usage: ./program filename tree_ans_file" << std::endl;
		return 1;
	}

	std::string filename = argv[1];
	std::string ans_name = argv[2];
	
	//std::string filename = "C:/Users/13508/Desktop/Halign-G/实验/实验2-5小数据/0MGA/0maf/Ec-h4.maf";
	//std::string ans_name = "C:/Users/13508/Desktop/对比进化/Ec/Ec-h4.tree";

	size_t seq_num=0, max_Len=0;
	int si,start;
	int z;
	char* ref_str;
	std::vector<char *> Names;
	std::vector<size_t> Lengths;
	std::vector<char*> Sequences;
	char* ref_name;
	struct mafComp* c, * prev = NULL;
	struct mafFile* maf;
	struct mafAli* now,*tmp;
	init_scores70();

	maf = mafReadAll((char*)filename.c_str(), 0);
	now = maf->alignments;
	char prefix[4] = { 'A', 'n','c','\0' };
	while (now)
	{
		c = now->components;
		prev = NULL;
		while (c != NULL) {
			if (strncmp(c->src, prefix, 3) == 0) {
				// 删除节点 c
				if (prev != NULL) {
					prev->next = c->next;
					free(c);
					c = prev->next;
				}
				else {
					now->components = c->next;
					free(c);
					c = now->components;
				}
			}
			else {
				prev = c;
				c = c->next;
			}
		}
		
		now = now->next;
	}

	now = maf->alignments;
	while (now)
	{
		for (c = now->components; c != NULL;) {
			if (!isStringInNames(c->src, Names))
			{
				ref_str = new char[strlen(c->src) + 100];
				strcpy(ref_str, c->src);
				Names.push_back(ref_str);
				
				Lengths.push_back(c->srcSize);
				if (max_Len < c->srcSize)
					max_Len = c->srcSize;
			}
			c = c->next;
		}
		now = now->next;
	}

	seq_num = Names.size();
	std::cout << seq_num<< "  seq_num\n";
	int** Score_matri = new int* [seq_num];
	for (int i = 0; i < seq_num; i++) {
		Score_matri[i] = new int[seq_num];
		memset(Score_matri[i], 0, sizeof(int) * seq_num);
	}
	double** Score_matri_double = new double* [seq_num];
	for (int i = 0; i < seq_num; i++) {
		Score_matri_double[i] = new double[seq_num];
		memset(Score_matri_double[i], 0, sizeof(double) * seq_num);
	}

	for (int i = 0; i < seq_num; i++)
		Sequences.push_back(new char[Lengths[i] + 100]);
	for (int r = 0; r < seq_num; r++)
	{
		for (int i = 0; i < seq_num; i++)
			std::fill(Sequences[i], Sequences[i] + Lengths[i] + 100, 0);
		ref_name = Names[r];

		now = maf->alignments;
		while (now)
		{
			tmp = trans_ref(ref_name, now);
			if (tmp != NULL)
			{
				for (c = tmp->components->next; c != NULL; c = c->next) {
					si = findIndex(c->src, Names);
					if (si == r)
						continue;
					if (c->strand == '+')
					{
						start = c->start;
						z = 0;
						for (int j = 0; j < strlen(tmp->components->text); j++)
						{
							if (c->text[j] == tmp->components->text[j] && c->text[j] != 'N' && c->text[j] != 'n' && c->text[j] != '-')
								Sequences[si][start + z] = 1;
							if (c->text[j] != '-')
								z++;
						}
					}
					else
					{
						start = c->srcSize - c->start-1;
						z = 0;
						for (int j = 0; j < strlen(tmp->components->text); j++)
						{
							if (c->text[j] == tmp->components->text[j] && c->text[j] != 'N' && c->text[j] != 'n' && c->text[j] != '-')
								Sequences[si][start - z] = 1;
							if (c->text[j] != '-')
								z++;
						}
					}
				}
			}
			now = now->next;
		}
		for (int i = 0; i < seq_num; i++)
		{
			if (i == r)
				continue;
			for (int k = 0; k < Lengths[i]; k++)
				Score_matri[r][i] += Sequences[i][k];
		}
	}
	double d_tmp;
	for (int i = 0; i < seq_num; i++) {
		for (int j = i+1; j < seq_num; j++) {
			d_tmp = 1.0*(Score_matri[i][j] + Score_matri[j][i])/(Lengths[i] + Lengths[j]);
			Score_matri_double[i][j] = Score_matri_double[j][i] = d_tmp;
		}
	}
	for (int i = 0; i < seq_num; i++) {
		for (int j = 0; j < seq_num; j++) {
			std::cout << Score_matri[i][j] << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;

	for (int i = 0; i < seq_num; i++) {
		//Score_matri_double[i][i] = 1.0;
		for (int j = 0; j < seq_num; j++) {
			std::cout << Score_matri_double[i][j] << " ";
		}
		std::cout << std::endl;
	}
	
	std::ofstream outputFile(ans_name);
	// 检查文件是否成功打开
	if (!outputFile.is_open()) {
		std::cout << "无法打开输出文件！" << std::endl;
		return 1;
	}
	// 将输出写入文件
	/*for (int i = 0; i < seq_num; i++) {
		for (int j = 0; j < seq_num; j++) {
			outputFile << Score_matri[i][j] << " ";
		}
		outputFile << std::endl;
	}
	outputFile << std::endl;*/
	outputFile << seq_num << "\n";
	for (int i = 0; i < seq_num; i++) 
		outputFile << Names[i] << "\n";

	for (int i = 0; i < seq_num; i++) {
		for (int j = 0; j < seq_num; j++) {
			outputFile << Score_matri_double[i][j] << " ";
		}
		outputFile << std::endl;
	}
	// 关闭文件
	outputFile.close();
	

	

	return 0;
}