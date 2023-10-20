import sys
import os
import re
import csv
import pandas as pd
#

"""
统计maf的  score  和   覆盖度
"""

# infile = 'C:/Users/周通/Desktop/parsnp.xmfa'
infile = ""
pre = 0
if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("请提供输入文件路径作为参数")
    else:
        infile = sys.argv[1]
        ansname = sys.argv[2]
        ref = sys.argv[3]
        print("filename:", infile)
        print("ansname:", ansname)
        print("ref:", ref)
    Name = []
    Length = []
    match_list = []
    with open(infile, "r") as file:
        line = file.readline()
        # line = file.readline()
        pre = 0
        while (True):
            line = file.readline()
            if not line:
                break
            if line == '\n':
                continue
            if line.startswith('a'):
                line = file.readline()
                match = re.match(r's\s+(\S+)\s+(\d+)\s+(\d+)\s+([+-])\s+(\d+)\s+(\S+)', line)
                sequence_name1 = match.group(1)
                start1 = int(match.group(2))
                len1 = int(match.group(3))
                orientation1 = match.group(4)
                length1 = int(match.group(5))
                seq1 = match.group(6).lower()
                if sequence_name1 not in Name:
                    Name.append(sequence_name1)
                    Length.append(length1)
                    match_list.append([0] * length1)
                # if sequence_name1 == ref:
                #     for j in range(start1,start1+len1):
                #         DNA_list[0][j] += 1
                #         match_list[0][j] = 1
            else:
                print(line)
                exit(0)

            while(1):
                line = file.readline()
                if not line.startswith('s'):
                    break
                match = re.match(r's\s+(\S+)\s+(\d+)\s+(\d+)\s+([+-])\s+(\d+)\s+(\S+)', line)
                sequence_name2 = match.group(1)
                start2 = int(match.group(2))
                len2 = int(match.group(3))
                orientation2 = match.group(4)
                length2 = int(match.group(5))
                seq2 = match.group(6).lower()

                if sequence_name2 not in Name:
                    Name.append(sequence_name2)
                    Length.append(length2)
                    match_list.append([0] * length2)

                index = Name.index(sequence_name2)
                if index==0:
                    continue
                if orientation2 == '-':
                    # print(start2, start2 + len2,end=' | ')
                    start2 = Length[index] - start2 - len2

                # print(start2, start2 + len2)
                # if start2 < pre:
                #     print("*******")
                # pre = start2 + len2
                # print(seq1)
                # print(seq2)

                # print()
                #tongji
                if(len(seq1) == len(seq2)):
                    k=0
                    if orientation2 =='-':
                        j = start2 + len2 -1
                        while(k < len(seq2)):
                            if (seq2[k] == '-'):
                                k += 1
                                continue
                            if(seq1[k]==seq2[k] and seq2[k] != 'N' and seq2[k] != 'n' and  j>=0 and  j<Length[index]):
                                match_list[index][j] += 1
                            k+=1
                            j-=1

                    else:
                        j = start2
                        while(k < len(seq2)):
                            if (seq2[k] == '-'):
                                k += 1
                                continue
                            if(seq1[k]==seq2[k] and seq2[k] != 'N' and seq2[k] != 'n' and  j>=0 and  j<Length[index]):
                                match_list[index][j] += 1
                            k += 1
                            j += 1

    maf = open(ansname, 'w')
    maf.write("\n")
    for i in range(len(Length)):
        Mnum = [0] * 11
        Msum = 0
        for j in range(Length[i]):

            if (match_list[i][j] >= 10):
                Mnum[10] += 1
            else:
                Mnum[match_list[i][j]] += 1


        # maf.write(str(0) + "\t: " + str(Mnum[0]) + "\n")
        for j in range(1, 10):
            # maf.write(str(j) + "\t: " + str(Mnum[j]) + "\n")
            Msum += Mnum[j]
        # maf.write("match >10\t: " + str(Mnum[10]) + " \n\n")

        maf.write("**** " + str(i + 1) + " " + " " + str(Msum) + " " + str(Length[i]) + " " + Name[i] + "\n")
        # maf.write("\n")
    maf.close()