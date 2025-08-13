from Bio import Phylo
from Bio.Phylo.TreeConstruction import DistanceMatrix, DistanceTreeConstructor
from io import StringIO
import matplotlib.pyplot as plt
import os
import sys

def sort_files_by_names(direname, names):
    # 存储文件名和序列名的列表
    files = []

    # 遍历文件夹中的每个文件
    for filename in os.listdir(direname):
        if filename.startswith('.'):  # 跳过隐藏文件
            continue
        filepath = os.path.join(direname, filename)
        with open(filepath, 'r') as file:
            line = file.readline().strip()
            if line.startswith('>'):
                seqname = line[1:].strip()
                files.append((filename, seqname))

    # 根据 Names 列表的顺序对文件进行排序
    files.sort(key=lambda x: names.index(x[1]) if x[1] in names else float('inf'))

    # 提取排序后的文件名到 Files 列表
    Files = [file[0] for file in files]

    return Files

def data_tree_dir(data_file, tree_file,direname):
    # 读取输入文件
    with open(data_file, "r") as f:
        n_seq = int(f.readline())
        seq_ids = []
        for i in range(n_seq):
            line = f.readline().strip()
            seq_id = line.split("\t")[0]
            seq_ids.append(seq_id)
        matrix = []
        for i in range(n_seq):
            line = f.readline().strip()
            row = [float(x) for x in line.split()]
            matrix.append(row)
    file_ids = sort_files_by_names(direname,seq_ids)
    new_matrix = []
    for i in range(len(matrix)):
        new_matrix.append(matrix[i][:i+1])
    # 构建距离矩阵对象
    dist_matrix = DistanceMatrix(file_ids, new_matrix)

    # 使用UPGMA算法构建进化树
    constructor = DistanceTreeConstructor()
    tree = constructor.upgma(dist_matrix)

    # 将中间节点的 name 属性设为空字符串
    for clade in tree.get_nonterminals():
        clade.name = ''

    # 将标识符前面的序列名去掉，只保留标识符
    for clade in tree.get_terminals():
        clade.name = clade.name.split(':')[0]

    new_newick_str = tree.format('newick').replace('\n', '').replace(' ', '')
    Phylo.write(tree, tree_file, "newick")

    print(new_newick_str)

def data_tree(data_file, tree_file):
    # 读取输入文件
    with open(data_file, "r") as f:
        n_seq = int(f.readline())
        seq_ids = []
        for i in range(n_seq):
            line = f.readline().strip()
            seq_id = line.split("\t")[0]
            seq_ids.append(seq_id)
        matrix = []
        for i in range(n_seq):
            line = f.readline().strip()
            row = [float(x) for x in line.split()]
            matrix.append(row)
    new_matrix = []
    for i in range(len(matrix)):
        new_matrix.append(matrix[i][:i+1])
    # 构建距离矩阵对象
    dist_matrix = DistanceMatrix(seq_ids, new_matrix)

    # 使用UPGMA算法构建进化树
    constructor = DistanceTreeConstructor()
    tree = constructor.upgma(dist_matrix)

    # 将中间节点的 name 属性设为空字符串
    for clade in tree.get_nonterminals():
        clade.name = ''

    # 将标识符前面的序列名去掉，只保留标识符
    for clade in tree.get_terminals():
        clade.name = clade.name.split(':')[0]

    new_newick_str = tree.format('newick').replace('\n', '').replace(' ', '')
    Phylo.write(tree, tree_file, "newick")

    print(new_newick_str)


def read_draw(tree_file, pic_file):
    tree = Phylo.read(tree_file,  "newick")
    # 绘制树
    Phylo.draw(tree, do_show=False)

    # 保存为图片文件
    plt.savefig(pic_file)
#
# data_file = "C:/Users/13508/Desktop/对比进化/Ec/Ec-h4.tree"
# tree_file = "C:/Users/13508/Desktop/对比进化/Ec/Ec-h4.newick"
# pic_file = "C:/Users/13508/Desktop/对比进化/Ec/Ec-h4.svg"
# direname = "C:/Users/13508/Desktop/Halign-G/实验/实验2-5小数据/Escherichia-coil"
#
# data_tree_dir(data_file,tree_file,direname)
# read_draw(tree_file,pic_file)
def main():
    if len(sys.argv) < 5:
        print("Usage: python main.py <data_file> <tree_file> <pic_file> <direname>")
        return

    data_file = sys.argv[1]
    tree_file = sys.argv[2]
    pic_file = sys.argv[3]
    direname = sys.argv[4]

    data_tree_dir(data_file, tree_file, direname)
    read_draw(tree_file, pic_file)

if __name__ == '__main__':
    main()
# data_file = "C:/Users/13508/Desktop/Nm-Par.tree"
# tree_file = "C:/Users/13508/Desktop/Nm-Par.newick"
# pic_file = "C:/Users/13508/Desktop/Nm-Par.svg"
# direname = "C:/Users/13508/Desktop/Halign-G/实验/实验2-5小数据/Neisseria-meningitidis"
#
# data_tree_dir(data_file,tree_file,direname)
# read_draw(tree_file,pic_file)

# data_file = "C:/Users/13508/Desktop/Nm-h4.tree"
# tree_file = "C:/Users/13508/Desktop/Nm-h4.newick"
# pic_file = "C:/Users/13508/Desktop/Nm-h4.svg"
# data_tree_dir(data_file,tree_file,direname)
# read_draw(tree_file,pic_file)
#
# data_file = "C:/Users/13508/Desktop/Nm-cac.tree"
# tree_file = "C:/Users/13508/Desktop/Nm-cac.newick"
# pic_file = "C:/Users/13508/Desktop/Nm-cac.svg"
# data_tree(data_file,tree_file)
# read_draw(tree_file,pic_file)
#
# tree_file = "C:/Users/13508/Desktop/parsnp.tree"
# pic_file = "C:/Users/13508/Desktop/parsnp.svg"
#
# read_draw(tree_file,pic_file)