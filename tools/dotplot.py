import sys, os
import matplotlib.pyplot as plt
import datetime
# <将比对后的.fasta和.maf格式 作图>

# <要求>：
#1 seq_name不能带空格
#2 .maf不可省略中心序列
#3 .fasta 有可选参数[指定为中心序列的名字][-number 可容忍的gap数量，默认为0]

# <用法>：
#1  path.fast [-0]                      (默认0号为中心序列，与其他序列分别做图)
#2  path.fast [center_seq_name] [-0]    (指定中心序列，与其他序列分别做图)
#3  path.fast seq1_name seq2_name [-0]  (该两条作图)
#4  path.maf center_seq_name            (指定中心序列，与其他序列分别做图)
#5  path.maf seq1_name seq2_name        (该两条作图)

out_path = ""
# 读入指定两条fasta  -------ok
def read_fasta_2(filename, seq1_name, seq2_name):
    tag1 = False
    tag1_ok = False
    tag2 = False
    tag2_ok = False
    str1 = ""
    str2 = ""
    with open(filename, 'r') as f:
        for line in f:
            if line.startswith('>'):
                if tag1:
                    tag1 = False
                    tag1_ok = True
                if tag2:
                    tag2 = False
                    tag2_ok = True
                if (not tag1) and (line[1:].strip() == seq1_name):
                    tag1 = True
                if (not tag2) and (line[1:].strip() == seq2_name):
                    tag2 = True
                continue
            if line.startswith('\n'):
                continue
            if tag1:
                str1 += line[:-1].upper()
            if tag2:
                str2 += line[:-1].upper()
    if tag1:
        tag1_ok = True
    if tag2:
        tag2_ok = True
    if not (tag1_ok and tag2_ok):
        print("error: No two sequences were found. Please check the sequence ID")
        exit(-1)
    if len(str1) != len(str2):
        print("error: The two sequences are of different lengths")
        exit(-1)
    return str1,str2

# 读入全部fasta  -------ok
def read_fasta_all(filename):
    with open(filename, 'r') as f:
        temp = ""
        strs = []
        names = []
        for line in f:
            if line.startswith('>'):
                names.append(line[1:].strip())
                strs.append(temp)
                temp = ""
                continue
            if line.startswith('\n'):
                continue
            temp += line[:-1].upper()
        strs.append(temp)
    if len(strs)<3:
        print("error: Read less than two sequences")
        exit(-1)
    for i in range(2, len(strs)):
        if(len(strs[i])!=len(strs[1])):
            print("error: The sequence lengths are not exactly equal")
            exit(-1)
    return strs[1:],names

# 读入两条maf
def read_maf_2(filename, seq1_name, seq2_name):
    tmp = []
    blocks = []  # 0[a_len,b_len], [a_start, a_l, a1/0, b_start, b_l, b1/0]
    alen = blen = 0
    with open(filename, 'r') as f:
        for line in f:
            if line.startswith('s'):
                tmpi = line[1:].split()
                if tmpi[0] == seq1_name:
                    tmp.insert(0,[int(tmpi[1]), int(tmpi[2]), 1 if tmpi[3] == '+' else 0])
                    alen = int(tmpi[4])
                if tmpi[0] == seq2_name:
                    tmp.append([int(tmpi[1]), int(tmpi[2]), 1 if tmpi[3] == '+' else 0])
                    blen = int(tmpi[4])
            if line.startswith('\n'):
                continue
            if line.startswith('a'):
                if len(tmp) == 2:
                    blocks.append(tmp[0]+tmp[1])
                tmp = []
        if len(tmp) == 2:  # last block
            blocks.append(tmp[0]+tmp[1])
        blocks.insert(0, [alen, blen])
    return blocks

# 读入全部maf
def read_maf_all(filename, seq_center):
    tmp = []
    blocks = {} # 0[a_len,b_len], [a_start, a_l, 1/0, b_start, b_l, 1/0]
    tagi = -1
    with open(filename, 'r') as f:
        for line in f:
            if line.startswith('s'):
                tmp.append(line[1:].split())
                if tmp[-1][0] == seq_center:
                    tagi = len(tmp) - 1
                    # print(tagi,len(tmp))
            if line.startswith('\n'):
                continue
            if tagi >= 0 and line.startswith('a'):
                for i in range(len(tmp)):
                    if i == tagi:
                        continue
                    if tmp[i][0] in blocks.keys():
                        blocks[tmp[i][0]].append([int(tmp[tagi][1]), int(tmp[tagi][2]), 1, int(tmp[i][1]), int(tmp[i][2]), 1 if tmp[i][3] == '+' else 0])
                    else:
                        blocks[tmp[i][0]] = [[int(tmp[tagi][4]), int(tmp[i][4])]]
                        blocks[tmp[i][0]].append(
                            [int(tmp[tagi][1]), int(tmp[tagi][2]), 1, int(tmp[i][1]), int(tmp[i][2]),
                             1 if tmp[i][3] == '+' else 0])
                tagi = -1
                tmp = []
        if tagi >= 0: # last block
            for i in range(len(tmp)):
                if i == tagi:
                    continue
                if tmp[i][0] in blocks.keys():
                    blocks[tmp[i][0]].append([int(tmp[tagi][1]), int(tmp[tagi][2]), 1, int(tmp[i][1]), int(tmp[i][2]),
                                              1 if tmp[i][3] == '+' else 0])
                else:
                    blocks[tmp[i][0]] = [[int(tmp[tagi][4]), int(tmp[i][4])]]
                    blocks[tmp[i][0]].append([int(tmp[tagi][1]), int(tmp[tagi][2]), 1, int(tmp[i][1]), int(tmp[i][2]),
                                              1 if tmp[i][3] == '+' else 0])
    return blocks

# 为fasta画图
def plot_fasta(str1, str2, seq1_name, seq2_name, gap_num=3):
    i1 = i2 = i = ei1 = ei2 = 0
    block = []  # s1 s2 e1 e2 [s,e]
    while i < len(str1):
        while i < len(str1) and (str1[i] == '-' or str2[i] == '-'):
            if str1[i] != "-":
                i1 += 1
            if str2[i] != "-":
                i2 += 1
            i += 1
        si1 = i1
        si2 = i2
        while i < len(str1) and (str1[i] != '-' and str2[i] != '-'):
            i1 += 1
            i2 += 1
            i += 1
        if si1 - ei1 < gap_num and si2 - ei2 < gap_num:
            try:
                block[-1][2] = i1 - 1
                block[-1][3] = i2 - 1
            except:
                block.append([si1, si2, i1 - 1, i2 - 1])
        elif i1 - si1 > 0 and i2 - si2 > 0:
            block.append([si1, si2, i1 - 1, i2 - 1])
        ei1 = i1
        ei2 = i2

    plt.figure()
    font = {'family': 'serif',
            'color': 'darkred',
            'weight': 'normal',
            'size': "small",
            }
    plt.xlabel(seq1_name, fontdict=font)
    plt.ylabel(seq2_name, fontdict=font)
    ax = plt.gca()
    ax.set_xlim(left=0, right=ei1)
    ax.set_ylim(bottom=ei2, top=0)  # 此处将原点设置为左上角
    plt.xticks([0, ei1])
    plt.yticks([0, ei2], rotation=90, verticalalignment='center')
    ax.xaxis.tick_top()
    ax.xaxis.set_label_position('top')
    ax.xaxis.set_ticks_position('top')
    ax.yaxis.set_label_position('left')
    ax.yaxis.set_ticks_position('left')
    ax.spines['bottom'].set_linewidth('0.5')
    for i in block:
        plt.plot([i[0], i[2]], [i[1], i[3]], '-r', linewidth=0.1)
    # plt.plot([0, ei1], [0, ei2], 'dodgerblue','.', linewidth=1)
    ax.ticklabel_format(style='plain')
    # plt.show()

    plt.savefig(out_path+seq2_name+".svg", bbox_inches='tight', format="svg")

# 处理0 or 1参数 n-1张图 fasta格式
def plot_all_fasta(filename , gap_num=0, center_name=""):
    strs ,name = read_fasta_all(filename)
    if center_name=="":
        centeri = 0
    elif center_name in name:
        centeri = name.index(center_name)
    else:
        print("error: No central sequence of the same name was found")
        exit(-1)
    for i in range(len(name)):
        if i == centeri:
            continue
        else:
            plot_fasta(strs[centeri], strs[i], name[centeri], name[i], gap_num)

# 处理 2参数 1张图 fasta格式
def plot_2_fasta(filename, seq1_name, seq2_name,gap_num=0):
    str1,str2 = read_fasta_2(filename, seq1_name, seq2_name)
    # print(str1)
    # print(str2)
    plot_fasta(str1, str2, seq1_name, seq2_name,gap_num)

def plot_maf(block, seq1_name, seq2_name, a_len, b_len):
    plt.figure()  # [a_start, a_l, a1/0, b_start, b_l, b1/0]
    font = {'family': 'serif',
            'color': 'darkred',
            'weight': 'normal',
            'size': "small",
            }
    plt.xlabel(seq1_name, fontdict=font)
    plt.ylabel(seq2_name, fontdict=font)
    ax = plt.gca()
    ax.set_xlim(left=0, right=a_len)
    ax.set_ylim(bottom=b_len, top=0)  # 此处将原点设置为左上角
    plt.xticks([0, a_len])
    plt.yticks([0, b_len], rotation=90, verticalalignment='center')
    ax.xaxis.tick_top()
    ax.xaxis.set_label_position('top')
    ax.xaxis.set_ticks_position('top')
    ax.yaxis.set_label_position('left')
    ax.yaxis.set_ticks_position('left')
    ax.spines['bottom'].set_linewidth('0.5')

    # for i in block:
    #     if (i[2] == i[5]):
    #         plt.plot([i[0], (i[0] + i[1])], [i[3], (i[3] + i[4])], '-r', linewidth=0.2)
    #     else:
    #         plt.plot([i[0], (i[0] + i[1])], [(i[3] + i[4]), i[3]], 'dodgerblue', linewidth=0.3)

    num1 = 0
    num0 = 0
    for i in block:
        if (i[2] == i[5]):
            num1 += i[4]
        else:
            num0 += i[4]
    if num1 > num0:
        block1 = []
        for i in block:
            if (i[2] == i[5]):
                plt.plot([i[0], (i[0] + i[1])], [i[3], (i[3] + i[4])], '-r', linewidth=0.2)
            else:
                block1.append(i)
        for i in block1:
            plt.plot([i[0], (i[0] + i[1])], [(i[3] + i[4]), i[3]], 'dodgerblue', linewidth=0.3)
    else:
        block1 = []
        for i in block:
            if (i[2] == i[5]):
                block1.append(i)
            else:
                plt.plot([i[0], (i[0] + i[1])], [b_len-i[3], b_len-i[3]-i[4]], 'dodgerblue', linewidth=0.2)
        for i in block1:
            plt.plot([i[0], (i[0] + i[1])], [b_len - i[3] - i[4], b_len - i[3]], '-r', linewidth=0.3)
    # plt.plot([0, a_len], [0, b_len], 'dodgerblue','.', linewidth=1)
    ax.ticklabel_format(style='plain')
    # plt.show()
    print(out_path+seq2_name+".svg")
    plt.savefig(out_path+seq2_name+".svg", bbox_inches='tight', format="svg")

def plot_maf_zf(block, seq1_name, seq2_name, a_len, b_len,zf):
    plt.figure()  # [a_start, a_l, a1/0, b_start, b_l, b1/0]
    font = {'family': 'serif',
            'color': 'darkred',
            'weight': 'normal',
            'size': "small",
            }
    plt.xlabel(seq1_name, fontdict=font)
    plt.ylabel(seq2_name, fontdict=font)
    ax = plt.gca()
    ax.set_xlim(left=0, right=a_len)
    ax.set_ylim(bottom=b_len, top=0)  # 此处将原点设置为左上角
    plt.xticks([0, a_len])
    plt.yticks([0, b_len], rotation=90, verticalalignment='center')
    ax.xaxis.tick_top()
    ax.xaxis.set_label_position('top')
    ax.xaxis.set_ticks_position('top')
    ax.yaxis.set_label_position('left')
    ax.yaxis.set_ticks_position('left')
    ax.spines['bottom'].set_linewidth('0.5')

    # for i in block:
    #     if (i[2] == i[5]):
    #         plt.plot([i[0], (i[0] + i[1])], [i[3], (i[3] + i[4])], '-r', linewidth=0.2)
    #     else:
    #         plt.plot([i[0], (i[0] + i[1])], [(i[3] + i[4]), i[3]], 'dodgerblue', linewidth=0.3)

    if zf==1:
        block1 = []
        for i in block:
            if (i[2] == i[5]):
                plt.plot([i[0], (i[0] + i[1])], [i[3], (i[3] + i[4])], '-r', linewidth=0.2)
            else:
                block1.append(i)
        for i in block1:
            plt.plot([i[0], (i[0] + i[1])], [(i[3] + i[4]), i[3]], 'dodgerblue', linewidth=0.3)
    else:
        block1 = []
        for i in block:
            if (i[2] == i[5]):
                block1.append(i)
            else:
                plt.plot([i[0], (i[0] + i[1])], [b_len-i[3], b_len-i[3]-i[4]], 'dodgerblue', linewidth=0.2)
        for i in block1:
            plt.plot([i[0], (i[0] + i[1])], [b_len - i[3] - i[4], b_len - i[3]], '-r', linewidth=0.3)
    # plt.plot([0, a_len], [0, b_len], 'dodgerblue','.', linewidth=1)
    ax.ticklabel_format(style='plain')
    # plt.show()
    print(out_path+seq2_name+".svg")
    plt.savefig(out_path+seq2_name+".svg", bbox_inches='tight', format="svg")

# 处理 1参数 n-1张图 maf格式
def plot_all_maf(filename, center_name):
    blocks = read_maf_all(filename, center_name)
    for k in blocks.keys():
        # print("name: ",k)
        # print("len: ", blocks[k][0][0], blocks[k][0][1])
        try:
            plot_maf(blocks[k][1:], center_name, k, blocks[k][0][0], blocks[k][0][1])
        except:
            pass
# 处理 2参数 一张图 maf格式
def plot_2_maf(filename, seq1_name, seq2_name):
    block = read_maf_2(filename, seq1_name, seq2_name)
    # print(len(block))
    # for i in block:
    #     print(i)
    plot_maf(block[1:], seq1_name, seq2_name, block[0][0], block[0][1])

# 处理 2参数 一张图 maf格式
def plot_2_maf_zf(filename, seq1_name, seq2_name,zf):
    block = read_maf_2(filename, seq1_name, seq2_name)
    # print(len(block))
    # for i in block:
    #     print(i)
    if(zf=='+'):
        plot_maf_zf(block[1:], seq1_name, seq2_name, block[0][0], block[0][1], 1)
    else:
        plot_maf_zf(block[1:], seq1_name, seq2_name, block[0][0], block[0][1], 0)

# 主函数，判断参数
if __name__ == "__main__":
    starttime = datetime.datetime.now()
    a = sys.argv

    if len(os.path.split(a[1])[0])==0:
        out_path = "."
    else:
        out_path = os.path.split(a[1])[0].replace("\\","/")
    out_path += "/"
    # print(out_path)
    if a[1][-3:] == "maf":
        if len(a)==3:
            plot_all_maf(a[1], a[2])
        elif len(a)==4:
            plot_2_maf(a[1], a[2], a[3])
        elif len(a)==5:
            plot_2_maf_zf(a[1], a[2], a[3], a[4])
        else:
            print("Input command error!")
            exit(-1)
    elif a[1][-5:] == "fasta":
        if a[-1][0]=="-":
            gap_num = int(a[-1][1:])
            a.pop()
        else:
            gap_num = 0

        if len(a) == 2:
            plot_all_fasta(a[1],gap_num)
        elif len(a) == 3:
            plot_all_fasta(a[1], gap_num, a[2])
        elif len(a) == 4:
            plot_2_fasta(a[1], a[2], a[3], gap_num)
        else:
            print("Input command error!")
            exit(-1)


    endtime = datetime.datetime.now()
    print("time: ",(endtime - starttime))


