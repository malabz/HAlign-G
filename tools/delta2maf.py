import sys

def read_fasta_file(file_path):
    sequences = []
    name = []
    current_seq = ""

    with open(file_path, 'r') as file:
        for line in file:
            line = line.strip()
            if line.startswith('>'):
                name.append(line[1:])
                sequences.append(current_seq)
                current_seq = ""
            else:
                current_seq += line
        if(current_seq!=""):
            sequences.append(current_seq)
    return sequences[1:], name

def nibu(str):
    # 反向字符串并替换字符
    reversed_str = str[::-1]
    reversed_str = reversed_str.replace('A', 't')
    reversed_str = reversed_str.replace('C', 'g')
    reversed_str = reversed_str.replace('T', 'a')
    reversed_str = reversed_str.replace('G', 'c')
    # 大小写转换
    reversed_str = reversed_str.upper()
    return reversed_str

def align_strings(aligned_reference, aligned_query, numbers):
    # 初始化对齐后的结果字符串
    index = 0

    for num in numbers:
        # 判断是否为缺失
        if num > 0:
            # 缺失
            index += num
            aligned_query = aligned_query[:index-1] + "-" + aligned_query[index-1:]
        elif num < 0:
            # 插入
            index += abs(num)
            aligned_reference = aligned_reference[:index - 1] + "-" + aligned_reference[index - 1:]

    return aligned_reference, aligned_query

def main(filename,ans_file):
    intervals = []
    maf = open(ans_file, 'w')
    maf.write(" ##maf version=1 scoring=lastz.v1.04.00\n")


    with open(filename, 'r') as file:
        line = file.readline()
        # 读取第一行的fasta文件路径，获取两条序列进来
        fasta_path1, fasta_path2 = line.strip().split()
        seq, name = read_fasta_file(fasta_path1)
        seqA = seq[0].upper()
        nameA = name[0].replace(" ", "").replace("\t", "")
        print("A ", len(seqA), nameA)
        seq, name = read_fasta_file(fasta_path2)
        seqB = seq[0].upper()
        nameB = name[0].replace(" ", "").replace("\t", "")
        print("B ", len(seqB), nameB)

        line = file.readline()# 跳过第二行
        line = file.readline()# 第3行
        values = line.strip().split()[-2:]
        lengthA = int(values[0])
        lengthB = int(values[1])
        print("lengthA ", lengthA)
        print("lengthB ", lengthB)
        max_name_width = max(len(nameA), len(nameB)) + 1


        DNA_list1 = [0] * lengthA
        match_list1 = [0] * lengthA

        DNA_list2 = [0] * lengthB
        match_list2 = [0] * lengthB

        # 获取后续多组区间和插空信息
        while 1:
            line = file.readline()
            if len(line) <= 1:  # 0结尾  1空行
                break
            # 每个区间第一行获取前4个值
            a_start, a_end, b_start, b_end = map(int, line.split()[:4])

            tag = 1
            Aq = ""
            Bq = ""
            if (b_start < b_end) :
                a_start = a_start - 1
                b_start = b_start - 1
                tag = "+"
                Aq = seqA[a_start:a_end]
                Bq = seqB[b_start:b_end]
                a_len = a_end - a_start
                b_len = b_end - b_start
                Bs = b_start
            else:
                a_start = a_start - 1
                tag = b_start
                b_start = b_end
                b_end = tag
                b_start = b_start - 1
                tag = "-"
                a_len = a_end - a_start
                b_len = b_end - b_start
                Aq = seqA[a_start:a_end]
                Bq = nibu(seqB[b_start:b_end])
                Bs = b_start
                b_start = lengthB - b_end
            interval_info = []
            while 1:
                line = file.readline().strip()
                if len(line) == 1 and int(line) == 0:
                    Aq, Bq = align_strings(Aq, Bq, interval_info)
                    #score start
                    for j in range(a_start, a_start + a_len):
                        DNA_list1[j] += 1
                        match_list1[j] = 1
                    for j in range(Bs, Bs + b_len):
                        DNA_list2[j] += 1

                    if(tag=='+'):
                        k = Bs
                        for i in range(len(Aq)):
                            if (Aq[i] == Bq[i]):
                                match_list2[k] = 1
                                k += 1
                            elif (Bq[i] != '-'):
                                k += 1
                    else:
                        k = b_end - 1
                        for i in range(len(Aq)):
                            if (Aq[i] == Bq[i]):
                                match_list2[k] = 1
                                k -= 1
                            elif (Bq[i] != '-'):
                                k -= 1

                    # score end
                    maf.write("a score = 0\n")
                    # 格式化输出
                    output = "s {:<{}} {:>11} {:>11} {:>2} {:>11} {}\n".format(nameA, str(max_name_width), str(a_start), str(a_len), "+", str(lengthA), Aq)
                    maf.write(output)
                    output = "s {:<{}} {:>11} {:>11} {:>2} {:>11} {}\n\n".format(nameB, str(max_name_width), str(b_start), str(b_len), tag, str(lengthB), Bq)
                    maf.write(output)

                    interval_info = []
                    break
                # 获取当前组的插空信息
                interval_info.append(int(line.strip()))
    maf.close()  # 关闭文件

    Dnum = [0] * 11
    Mnum = 0
    Dsum = 0
    for j in range(lengthA):
        if (DNA_list1[j] >= 10):
            Dnum[10] += 1
        else:
            Dnum[DNA_list1[j]] += 1
        Mnum += match_list1[j]
    print(0, "\t: ", Dnum[0])
    for j in range(1, 10):
        print(j, "\t: ", Dnum[j])
        Dsum += Dnum[j]
    print(">10\t: ", Dnum[10])
    print("****1 ", Dsum, Mnum, lengthA, nameA,"\n\n")

    Dnum = [0] * 11
    Mnum = 0
    Dsum = 0
    for j in range(lengthB):
        if (DNA_list2[j] >= 10):
            Dnum[10] += 1
        else:
            Dnum[DNA_list2[j]] += 1
        Mnum += match_list2[j]
    print(0, "\t: ", Dnum[0])
    for j in range(1, 10):
        print(j, "\t: ", Dnum[j])
        Dsum += Dnum[j]
    print(">10\t: ", Dnum[10])
    print("****2 ", Dsum, Mnum, lengthB, nameB, "\n\n")

if __name__ == "__main__":
    # 获取命令行参数
    filename = sys.argv[1]
    ansname = sys.argv[2]

    print("filename:", filename)
    print("ansname:", ansname)
    # 示例用法
    # filename = 'C:/Users/13508/Desktop/mum_chr22-psa2/1.txt'
    # ansname = 'C:/Users/13508/Desktop/mum_chr22-psa2/2.txt'
    main(filename, ansname)
