import random
from collections import namedtuple
import numpy as np
import sys

#全局tag列表
flags = []
original_sequence = ''
ACGT = {'A': '1', 'C': '2', 'G': '3', 'T': '4'}
#sv类型字典
sv_type = {
    1: 'snp_insert',
    2: 'snp_delete',
    3: 'snp_tihuan',

    4: 'big_insert',
    5: 'big_delete',
    6: 'duplication',
    7: 'translocation',
    8: 'inversion'}

SV = namedtuple('sv', ['A_index', 'B_index', 'len', 'sv_type', 'str_value'])  # 原始位置A，sv后位置B，发生
# delete = namedtuple('delete', ['start', 'len'])  # 将start开始len长的内容替换为-，代表删除
INSERT = namedtuple('insert', ['start', 'len', 'str_value'])# 将start位置后插入长为len的str_value，代表插入
EXCHANGE = namedtuple('exchange', ['start', 'len', 'str_value'])# 将start开始len长的内容替换为str_value，代表替换

def snp_string(input_string, thresh=0.02):
    # 判断输入字符串是大写串还是小写串
    if input_string[0:1].isupper():
        # 将字符串转换为列表
        char_list = list(input_string)

        # 生成替换位置的索引（使用正态分布）
        num_chars = len(char_list)
        mean = num_chars // 2  # 平均值设为字符串长度的一半
        std = max(1, round(num_chars * 0.2))  # 标准差设为字符串长度的 0.2 倍
        replace_indices = np.random.normal(mean, std, size=max(1, round(num_chars * thresh))).astype(int)

        # 限制替换索引在有效范围内
        replace_indices = [index for index in replace_indices if 0 <= index < num_chars]

        # 替换选定位置上的字符
        for index in replace_indices:
            replacement = random.choice([char for char in ["A", "C", "G", "T"] if char != char_list[index]])
            char_list[index] = replacement
        # 将列表转换回字符串
        processed_string = ''.join(char_list)
        return processed_string
    elif input_string[0:1].islower():
        # 将字符串转换为列表
        char_list = list(input_string)

        # 生成替换位置的索引（使用正态分布）
        num_chars = len(char_list)
        mean = num_chars // 2  # 平均值设为字符串长度的一半
        std = max(1, round(num_chars * 0.2))  # 标准差设为字符串长度的 0.2 倍
        replace_indices = np.random.normal(mean, std, size=max(1, round(num_chars * thresh))).astype(int)

        # 限制替换索引在有效范围内
        replace_indices = [index for index in replace_indices if 0 <= index < num_chars]

        # 替换选定位置上的字符
        for index in replace_indices:
            replacement = random.choice([char for char in ["a", "c", "g", "t"] if char != char_list[index]])
            char_list[index] = replacement
        # 将列表转换回字符串
        processed_string = ''.join(char_list)
        return processed_string
    else:
        print( "输入字符串既不是大写串也不是小写串")
        exit(1)


#小删除 为 0
#小插入 为 1A 2C 3G 4T
#snp为
# 函数：生成除当前外，随机碱基 snp
def generate_random_ACGT(char):
    bases = ['A', 'C', 'G', 'T'].remove(char)
    return random.choice(bases)

def generate_random_1234(char):
    bases = ['1', '2', '3', '4'].remove(ACGT[char])
    return random.choice(bases)

# 函数：生成一条长length的随机序列
def generate_random_str_acgt(length):
    bases = ['a', 'c', 'g', 't']
    random_str = ''
    for _ in range(length):
        random_str += random.choice(bases)
    return random_str

# 函数：生成一条长length的随机序列
def generate_random_str1234(length):
    bases = ['1', '2', '3', '4']
    random_str = ''
    for _ in range(length):
        random_str += random.choice(bases)
    return random_str

#随机取一个位点
def random_1_in_flags():
    global flags  # 使用全局变量 flags
    non_zero_flags = [x for x in flags if x != 0]
    if non_zero_flags:
        values = random.choice(non_zero_flags)
        flags[values] = 0
        return values
    else:
        return -1

#随机取一段区间
def random_x_in_flags(x):
    global flags  # 使用全局变量 flags

    sub_lists = []  # 存储每个以 0 分割的子列表
    temp_list = []  # 临时存储子列表
    tag = True
    for num in flags:
        if num == 0:
            if tag:
                if len(temp_list) >= x:
                    sub_lists.extend(temp_list[:-x+1]) # 添加满足长度要求的子列表（去掉最后 x 个元素）
                temp_list = []
                tag = False
        else:
            temp_list.append(num)
            tag = True

    if len(temp_list) >= x:
        sub_lists.extend(temp_list[:-x + 1])  # 添加满足长度要求的子列表（去掉最后 x 个元素）

    if len(sub_lists) == 0:
        return -1  # 返回 -1 表示无法找到满足要求的区间
    else:
        start_index = random.choice(sub_lists)  # 随机选择一个子列表
        for i in range(start_index,start_index+x):
            flags[i] = 0
        return start_index  # 返回起点值

# 1函数：生成小插入
def _1_snp_insert(length):
    index = random_1_in_flags()
    if (index == -1):
        return []
    str = generate_random_str1234(length)
    svi = SV(A_index=index, B_index=index, sv_type=1, len=length, str_value=str)
    inserti = INSERT(start=index, len=length, str_value=str)
    return [svi, inserti]

# 2函数：生成小删除
def _2_snp_delete(length):
    index = random_x_in_flags(length)
    if (index == -1):
        return []
    str = '<'*length
    svi = SV(A_index=index, B_index=index, sv_type=2, len=length, str_value=str)
    exchangei = EXCHANGE(start=index, len=length, str_value=str)
    return [svi, exchangei]

# 3函数：生成小snp替换
def _3_snp_tihuan(length):
    index = random_x_in_flags(length)
    if (index == -1):
        return []
    str = ''
    for i in range(index,index+length):
        str += generate_random_1234(original_sequence[index])
    svi = SV(A_index=index, B_index=index, sv_type=3, len=length, str_value=str)
    exchangei = EXCHANGE(start=index, len=length, str_value=str)
    return [svi, exchangei]

# 4函数：生成大插入sv
def _4_big_insert(length):
    index = random_1_in_flags()
    if (index == -1):
        return []
    str = generate_random_str_acgt(length)
    svi = SV(A_index=index, B_index=index, sv_type=4, len=length, str_value=str)
    inserti = INSERT(start=index, len=length, str_value=str)
    return [svi, inserti]

# 5函数：生成大删除sv
def _5_big_delete(length):
    index = random_x_in_flags(length)
    if (index == -1):
        return []
    str = '>' * length
    svi = SV(A_index=index, B_index=index, sv_type=5, len=length, str_value=str)
    exchangei = EXCHANGE(start=index, len=length, str_value=str)
    return [svi, exchangei]

# 6函数：生成串联重复sv
def _6_duplication(length, times):
    index = random_x_in_flags(length)
    if (index == -1):
        return []
    str = snp_string(original_sequence[index:index+length].lower())
    svi = SV(A_index=index, B_index=index+length, sv_type=6, len=length*times, str_value=str)
    inserti = INSERT(start=index+length, len=length*times, str_value=str*times)
    return [svi, inserti]

# 61函数：生成散布(易位)重复sv
def _61_tran_duplication(length):
    index_start = random_x_in_flags(length)
    index_end = random_1_in_flags()
    if (index_start == -1 or index_end == -1):
        return []
    str_start = snp_string(original_sequence[index_start:index_start + length].lower())
    svi = SV(A_index=index_start, B_index=index_end, sv_type=61, len=length, str_value=str_start)
    inserti = INSERT(start=index_end, len=length, str_value=str_start)
    return [svi, inserti]

# 62函数：生成倒位重复sv
def _62_inver_duplication(length):
    index_start = random_x_in_flags(length)
    index_end = random_1_in_flags()
    if (index_start == -1 or index_end == -1):
        return []
    str_start = snp_string(original_sequence[index_start:index_start + length][::-1].replace('A', 't').replace('T', 'a').replace(
        'C', 'g').replace('G', 'c').lower())
    svi = SV(A_index=index_start, B_index=index_end, sv_type=62, len=length, str_value=str_start)
    inserti = INSERT(start=index_end, len=length, str_value=str_start)
    return [svi, inserti]

# 7函数：生成易位sv
def _7_translocation(length):
    index_start = random_x_in_flags(length)
    index_end = random_1_in_flags()
    if (index_start == -1 or index_end == -1):
        return []
    str_start = snp_string(original_sequence[index_start:index_start + length].lower())
    svi = SV(A_index=index_start, B_index=index_end, sv_type=7, len=length, str_value=str_start)
    exchangei = EXCHANGE(start=index_start, len=length, str_value='>'*length)
    inserti = INSERT(start=index_end, len=length, str_value=str_start)
    return [svi, exchangei, inserti]

# 8函数：生成倒位sv
def _8_inversion(length):
    index_start = random_x_in_flags(length)
    if (index_start == -1):
        return []
    str_start = snp_string(original_sequence[index_start:index_start + length][::-1].replace('A', 't').replace('T', 'a').replace('C', 'g').replace('G', 'c').upper())
    svi = SV(A_index=index_start, B_index=index_start, sv_type=8, len=length, str_value=str_start)
    exchangei = EXCHANGE(start=index_start, len=length, str_value=str_start)
    return [svi, exchangei]

# 9函数：生成倒位易位sv
def _9_tran_inversion(length):
    index_start = random_x_in_flags(length)
    index_end = random_1_in_flags()
    if (index_start == -1 or index_end == -1):
        return []
    str_start = snp_string(original_sequence[index_start:index_start + length][::-1].replace('A', 't').replace('T', 'a').replace('C', 'g').replace('G', 'c').lower())
    svi = SV(A_index=index_start, B_index=index_end, sv_type=9, len=length, str_value=str_start)
    exchangei = EXCHANGE(start=index_start, len=length, str_value='>' * length)
    inserti = INSERT(start=index_end, len=length, str_value=str_start)
    return [svi, exchangei, inserti]


def compare_start(element):
    return element.start

def compare_Aindex(element):
    return element.A_index

def compare_Bindex(element):
    return element.B_index

def generate_maf_fasta(input_file, out_maf_file, simulate_file, mutation_prob, mutation_length):
    # 读取原始DNA序列
    global original_sequence
    A_name = ''
    B_name = "simulate"
    with open(input_file, 'r') as file:
        for line in file:
            if not line.startswith('>'):
                original_sequence += line.strip()
            else:
                A_name = line[1:].strip()
    oLen = len(original_sequence)
    global flags
    flags = list(range(oLen))  # 初始化标志数组

    simulated_sequence = ''
    sv_results = []
    mutations = []
    for i in range(len(original_sequence)):
        # 1判断是否发生  小插入
        if random.random() < mutation_prob['snp_insert']:
            svi = _1_snp_insert(mutation_length['snp_insert'])
        # 2判断是否发生  小删除
        elif random.random() < mutation_prob['snp_delete']:
            svi = _2_snp_delete(mutation_length['snp_delete'])
        # 3判断是否发生  小snp
        elif random.random() < mutation_prob['snp_tihuan']:
            svi = _3_snp_tihuan(mutation_length['snp_tihuan'])
        # 4判断是否发生  大插入SV
        elif random.random() < mutation_prob['big_insert']:
            svi = _4_big_insert(mutation_length['big_insert'])
        # 5判断是否发生  大删除SV
        elif random.random() < mutation_prob['big_delete']:
            svi = _5_big_delete(mutation_length['big_delete'])
        # 6判断是否发生  重复SV
        elif random.random() < mutation_prob['duplication']:
            svi = _6_duplication(mutation_length['duplication'], 2)
        # 61判断是否发生 易位重复SV
        elif random.random() < mutation_prob['tran_duplication']:
            svi = _61_tran_duplication(mutation_length['tran_duplication'])
        # 62判断是否发生 倒位重复SV
        elif random.random() < mutation_prob['inver_duplication']:
            svi = _62_inver_duplication(mutation_length['inver_duplication'])
        # 7判断是否发生  易位SV
        elif random.random() < mutation_prob['translocation']:
            svi = _7_translocation(mutation_length['translocation'])
        # 8判断是否发生  倒位SV
        elif random.random() < mutation_prob['inversion']:
            svi = _8_inversion(mutation_length['inversion'])
        # 9判断是否发生  易位倒位SV
        elif random.random() < mutation_prob['tran_inversion']:
            svi = _9_tran_inversion(mutation_length['tran_inversion'])
        else:
            continue
        if (len(svi) > 0):
            sv_results.append(svi[0])
            mutations.extend(svi[1:])

    mutations.sort(key=compare_start, reverse=True)
    sv_results.sort(key=compare_Bindex, reverse=False)
    #过滤小snp
    sv_maf = list(filter(lambda result: result.sv_type > 5, sv_results))

    # 将字符串转换为列表
    modified_sequence = list(original_sequence)
    for mutation in mutations:
        if isinstance(mutation, EXCHANGE):
            modified_sequence[mutation.start:mutation.start+mutation.len] = list(mutation.str_value)
        else:#insert
            modified_sequence[mutation.start:mutation.start] = list(mutation.str_value)
    simulated_sequence = "".join(modified_sequence)  # 将列表转换回字符串

    ai = 0
    bi = 0
    si = 0
    i = 0
    while(si < len(sv_maf)):
        if(ord(simulated_sequence[i])>=60 and ord(simulated_sequence[i])<=90):
            ai+=1
        if(simulated_sequence[i]!='>' and simulated_sequence[i]!='<'):
            bi+=1
        if(ai==sv_maf[si].B_index):
            sv_maf[si] = sv_maf[si]._replace(B_index=bi)
            # sv_maf[si].B_index = bi
            si+=1
        i+=1
    sv_maf.sort(key=compare_Aindex, reverse=False)
    simulated_sequence = simulated_sequence.replace('>', '').replace('<', '').replace('1', 'a').replace('2', 'c').replace('3', 'g').replace('4', 't')

    # 输出fasta文件
    with open(simulate_file, 'w') as file_fasta:
        file_fasta.write("> simulate\n")
        file_fasta.write("%s\n" % simulated_sequence)

    # 输出MAF文件
    with open(out_maf_file, 'w') as file_maf:
        file_maf.write("#\t%s\n" % ("\t".join(["maf_version", "1"])))
        for maf in sv_maf:
            if(maf.sv_type==6):
                Leng = len(maf.str_value)
                for i in range(int(maf.len / Leng)):
                    file_maf.write("a\t%s\n" % ("\t".join(["score = duplication"])))
                    file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                        A_name, maf.A_index, Leng, len(original_sequence),
                        original_sequence[maf.A_index:maf.A_index + Leng]))
                    file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                        B_name, maf.B_index+i*Leng, Leng, len(simulated_sequence),
                        simulated_sequence[maf.B_index+i*Leng:maf.B_index+i*Leng+ len(maf.str_value)]))
                    file_maf.write("s %-20s %10d %10d + %10d %s\n\n" % (
                        B_name, maf.B_index+i*Leng, Leng, len(simulated_sequence), maf.str_value))
            elif (maf.sv_type == 8):
                file_maf.write("a\t%s\n" % ("\t".join(["score = inversion"])))
                file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                    A_name, maf.A_index, maf.len, len(original_sequence),
                    original_sequence[maf.A_index:maf.A_index + maf.len]))
                file_maf.write("s %-20s %10d %10d - %10d %s\n" % (
                    B_name, (len(simulated_sequence) - maf.B_index - maf.len), maf.len, len(simulated_sequence),
                    simulated_sequence[maf.B_index:maf.B_index + maf.len]))
                file_maf.write("s %-20s %10d %10d + %10d %s\n\n" % (
                    B_name, maf.B_index, maf.len, len(simulated_sequence),
                    maf.str_value))
            elif (maf.sv_type == 7):
                file_maf.write("a\t%s\n" % ("\t".join(["score = translocation"])))
                file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                    A_name, maf.A_index, maf.len, len(original_sequence),
                    original_sequence[maf.A_index:maf.A_index + maf.len]))
                file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                    B_name, maf.B_index, maf.len, len(simulated_sequence),
                    simulated_sequence[maf.B_index:maf.B_index + maf.len]))
                file_maf.write("s %-20s %10d %10d + %10d %s\n\n" % (
                    B_name, maf.B_index, maf.len, len(simulated_sequence), maf.str_value))
            elif (maf.sv_type == 9):
                file_maf.write("a\t%s\n" % ("\t".join(["score = tran_inversion"])))
                file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                    A_name, maf.A_index, maf.len, len(original_sequence),
                    original_sequence[maf.A_index:maf.A_index + maf.len]))
                file_maf.write("s %-20s %10d %10d - %10d %s\n" % (
                    B_name, (len(simulated_sequence) - maf.B_index - maf.len), maf.len, len(simulated_sequence),
                    simulated_sequence[maf.B_index:maf.B_index + maf.len]))
                file_maf.write("s %-20s %10d %10d + %10d %s\n\n" % (
                    B_name, maf.B_index, maf.len, len(simulated_sequence),
                    maf.str_value))
            elif (maf.sv_type == 61):
                file_maf.write("a\t%s\n" % ("\t".join(["score = tran_duplication"])))
                file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                    A_name, maf.A_index, maf.len, len(original_sequence),
                    original_sequence[maf.A_index:maf.A_index + maf.len]))
                file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                    B_name, maf.B_index, maf.len, len(simulated_sequence),
                    simulated_sequence[maf.B_index:maf.B_index + maf.len]))
                file_maf.write("s %-20s %10d %10d + %10d %s\n\n" % (
                    B_name, maf.B_index, maf.len, len(simulated_sequence), maf.str_value))
            elif (maf.sv_type == 62):
                file_maf.write("a\t%s\n" % ("\t".join(["score = inver_duplication"])))
                file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                    A_name, maf.A_index, maf.len, len(original_sequence),
                    original_sequence[maf.A_index:maf.A_index + maf.len]))
                file_maf.write("s %-20s %10d %10d - %10d %s\n" % (
                    B_name, (len(simulated_sequence) - maf.B_index - maf.len), maf.len, len(simulated_sequence),
                    simulated_sequence[maf.B_index:maf.B_index + maf.len]))
                file_maf.write("s %-20s %10d %10d + %10d %s\n\n" % (
                    B_name, maf.B_index, maf.len, len(simulated_sequence),
                    maf.str_value))

def generate_maf_fasta_times(input_file, out_maf_file, simulate_file, mutation_times, mutation_length):
    # 读取原始DNA序列
    global original_sequence
    A_name = ''
    B_name = "simulate"
    with open(input_file, 'r') as file:
        for line in file:
            if not line.startswith('>'):
                original_sequence += line.strip()
            else:
                A_name = line[1:].strip()
    oLen = len(original_sequence)
    global flags
    flags = list(range(oLen))  # 初始化标志数组

    simulated_sequence = ''
    sv_results = []
    mutations = []
    for key, value in mutation_times.items():
        for _ in range(value):
            # 1判断是否发生 小插入
            if key == 'snp_insert':
                svi = _1_snp_insert(mutation_length[key])
            # 2判断是否发生 小删除
            elif key == 'snp_delete':
                svi = _2_snp_delete(mutation_length[key])
            # 3判断是否发生 小snp
            elif key == 'snp_tihuan':
                svi = _3_snp_tihuan(mutation_length[key])
            # 4判断是否发生 大插入SV
            elif key == 'big_insert':
                svi = _4_big_insert(mutation_length[key])
            # 5判断是否发生 大删除SV
            elif key == 'big_delete':
                svi = _5_big_delete(mutation_length[key])
            # 6判断是否发生 重复SV
            elif key == 'duplication':
                svi = _6_duplication(mutation_length[key], 2)
            # 61判断是否发生 易位重复SV
            elif key == 'tran_duplication':
                svi = _61_tran_duplication(mutation_length[key])
            # 62判断是否发生 倒位重复SV
            elif key == 'inver_duplication':
                svi = _62_inver_duplication(mutation_length[key])
            # 7判断是否发生 易位SV
            elif key == 'translocation':
                svi = _7_translocation(mutation_length[key])
            # 8判断是否发生 倒位SV
            elif key == 'inversion':
                svi = _8_inversion(mutation_length[key])
            # 9判断是否发生 易位倒位SV
            elif key == 'tran_inversion':
                svi = _9_tran_inversion(mutation_length[key])
            else:
                continue
            if len(svi) > 0:
                sv_results.append(svi[0])
                mutations.extend(svi[1:])
    mutations.sort(key=compare_start, reverse=True)
    sv_results.sort(key=compare_Bindex, reverse=False)
    #过滤小snp
    sv_maf = list(filter(lambda result: result.sv_type > 5, sv_results))

    # 将字符串转换为列表
    modified_sequence = list(original_sequence)
    for mutation in mutations:
        if isinstance(mutation, EXCHANGE):
            modified_sequence[mutation.start:mutation.start+mutation.len] = list(mutation.str_value)
        else:#insert
            modified_sequence[mutation.start:mutation.start] = list(mutation.str_value)
    simulated_sequence = "".join(modified_sequence)  # 将列表转换回字符串

    ai = 0
    bi = 0
    si = 0
    i = 0
    while(si < len(sv_maf)):
        if(ord(simulated_sequence[i])>=60 and ord(simulated_sequence[i])<=90):
            ai+=1
        if(simulated_sequence[i]!='>' and simulated_sequence[i]!='<'):
            bi+=1
        if(ai==sv_maf[si].B_index):
            sv_maf[si] = sv_maf[si]._replace(B_index=bi)
            # sv_maf[si].B_index = bi
            si+=1
        i+=1
    sv_maf.sort(key=compare_Aindex, reverse=False)
    simulated_sequence = simulated_sequence.replace('>', '').replace('<', '').replace('1', 'a').replace('2', 'c').replace('3', 'g').replace('4', 't')

    # 输出fasta文件
    with open(simulate_file, 'w') as file_fasta:
        file_fasta.write("> simulate\n")
        file_fasta.write("%s\n" % simulated_sequence)

    # 输出MAF文件
    with open(out_maf_file, 'w') as file_maf:
        file_maf.write("#\t%s\n" % ("\t".join(["maf_version", "1"])))
        for maf in sv_maf:
            if(maf.sv_type==6):
                Leng = len(maf.str_value)
                for i in range(int(maf.len / Leng)):
                    file_maf.write("a\t%s\n" % ("\t".join(["score = duplication"])))
                    file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                        A_name, maf.A_index, Leng, len(original_sequence),
                        original_sequence[maf.A_index:maf.A_index + Leng]))
                    file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                        B_name, maf.B_index+i*Leng, Leng, len(simulated_sequence),
                        simulated_sequence[maf.B_index+i*Leng:maf.B_index+i*Leng+ len(maf.str_value)]))
                    # file_maf.write("s %-20s %10d %10d + %10d %s\n\n" % (
                    #     B_name, maf.B_index+i*Leng, Leng, len(simulated_sequence), maf.str_value))
            elif (maf.sv_type == 8):
                file_maf.write("a\t%s\n" % ("\t".join(["score = inversion"])))
                file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                    A_name, maf.A_index, maf.len, len(original_sequence),
                    original_sequence[maf.A_index:maf.A_index + maf.len]))
                file_maf.write("s %-20s %10d %10d - %10d %s\n" % (
                    B_name, (len(simulated_sequence) - maf.B_index - maf.len), maf.len, len(simulated_sequence),
                    simulated_sequence[maf.B_index:maf.B_index + maf.len]))
                # file_maf.write("s %-20s %10d %10d + %10d %s\n\n" % (
                #     B_name, maf.B_index, maf.len, len(simulated_sequence),
                #     maf.str_value))
            elif (maf.sv_type == 7):
                file_maf.write("a\t%s\n" % ("\t".join(["score = translocation"])))
                file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                    A_name, maf.A_index, maf.len, len(original_sequence),
                    original_sequence[maf.A_index:maf.A_index + maf.len]))
                file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                    B_name, maf.B_index, maf.len, len(simulated_sequence),
                    simulated_sequence[maf.B_index:maf.B_index + maf.len]))
                # file_maf.write("s %-20s %10d %10d + %10d %s\n\n" % (
                #     B_name, maf.B_index, maf.len, len(simulated_sequence), maf.str_value))
            elif (maf.sv_type == 9):
                file_maf.write("a\t%s\n" % ("\t".join(["score = tran_inversion"])))
                file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                    A_name, maf.A_index, maf.len, len(original_sequence),
                    original_sequence[maf.A_index:maf.A_index + maf.len]))
                file_maf.write("s %-20s %10d %10d - %10d %s\n" % (
                    B_name, (len(simulated_sequence) - maf.B_index - maf.len), maf.len, len(simulated_sequence),
                    simulated_sequence[maf.B_index:maf.B_index + maf.len]))
                # file_maf.write("s %-20s %10d %10d + %10d %s\n\n" % (
                #     B_name, maf.B_index, maf.len, len(simulated_sequence),
                #     maf.str_value))
            elif (maf.sv_type == 61):
                file_maf.write("a\t%s\n" % ("\t".join(["score = tran_duplication"])))
                file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                    A_name, maf.A_index, maf.len, len(original_sequence),
                    original_sequence[maf.A_index:maf.A_index + maf.len]))
                file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                    B_name, maf.B_index, maf.len, len(simulated_sequence),
                    simulated_sequence[maf.B_index:maf.B_index + maf.len]))
                # file_maf.write("s %-20s %10d %10d + %10d %s\n\n" % (
                #     B_name, maf.B_index, maf.len, len(simulated_sequence), maf.str_value))
            elif (maf.sv_type == 62):
                file_maf.write("a\t%s\n" % ("\t".join(["score = inver_duplication"])))
                file_maf.write("s %-20s %10d %10d + %10d %s\n" % (
                    A_name, maf.A_index, maf.len, len(original_sequence),
                    original_sequence[maf.A_index:maf.A_index + maf.len]))
                file_maf.write("s %-20s %10d %10d - %10d %s\n" % (
                    B_name, (len(simulated_sequence) - maf.B_index - maf.len), maf.len, len(simulated_sequence),
                    simulated_sequence[maf.B_index:maf.B_index + maf.len]))
                # file_maf.write("s %-20s %10d %10d + %10d %s\n\n" % (
                #     B_name, maf.B_index, maf.len, len(simulated_sequence),
                #     maf.str_value))

# 测试示例
def main():
    if len(sys.argv) < 4:
        print("Usage: python script.py <input_file> <out_maf_file> <simulate_file>")
        return

    input_file = sys.argv[1]
    out_maf_file = sys.argv[2]
    simulate_file = sys.argv[3]


    mutation_prob = {
        'snp_insert':    0.0001,          #1
        'snp_delete':    0.0001,          #2
        'snp_tihuan':    0.0001,          #3
        'big_insert':    0.000001,        #4
        'big_delete':    0.000001,        #5
        'duplication':   0.000001,        #6
        'tran_duplication': 0.000001, #61
        'inver_duplication':0.000001, #62
        'translocation': 0.000001,        #7
        'inversion':     0.000001,        #8
        'tran_inversion':0.000001,        #9
    }
    mutation_times = {
        'big_insert': 2,  # 4
        'big_delete': 2,  # 5
        'duplication': 2,  # 6
        'tran_duplication': 2,  # 61
        'inver_duplication': 2,  # 62
        'translocation': 2,  # 7
        'inversion': 2,  # 8
        'tran_inversion': 2,  # 9
        'snp_insert': 400,  # 1
        'snp_delete': 400,  # 2
        'snp_tihuan': 400,  # 3
    }
    mutation_length = {
        'snp_insert':     1,       #1
        'snp_delete':     1,       #2
        'snp_tihuan':     1,       #3
        'big_insert':     50,      #4
        'big_delete':     50,      #5
        'duplication':    3000,    #6
        'tran_duplication': 3000,  # 61
        'inver_duplication': 3000,  # 62
        'translocation':  3000,    #7
        'inversion':      3000,    #8
        'tran_inversion': 3000,    #9
    }

    generate_maf_fasta_times(input_file, out_maf_file, simulate_file, mutation_times, mutation_length)
    print(simulate_file)

    # for i in range(10):
    #     input_file = f'./simu{i}/original.fasta'
    #     out_maf_file = f'{i}simulated.maf'
    #     simulate_file = f'./simu{i}/simulated.fasta'
    #
    #     # generate_maf_fasta(input_file, out_maf_file, simulate_file, mutation_prob, mutation_length)
    #     generate_maf_fasta_times(input_file, out_maf_file, simulate_file, mutation_times, mutation_length)
    #     print(simulate_file)

if __name__ == '__main__':
    main()