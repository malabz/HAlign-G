# import pandas as pd
# import matplotlib.pyplot as plt
# import numpy as np
#
# filename = 'A-msa-no1.xlsx'
# h4 = []
# par = []
# cac = []
# sheets = pd.read_excel(filename, sheet_name=None)
# for sheet_name, sheet_data in list(sheets.items())[:]:
#     h4.append(sheet_data['H4_maf_score'].fillna(0).values[1:])
#     par.append(sheet_data['Parsnp8_score'].fillna(0).values[1:])
#     cac.append(sheet_data['cactus80_score'].fillna(0).values[1:])
#
# y1 = cac
# y2 = par
# y3 = h4
#
# x1 = np.arange(1, 4 * 24, 4)
# x2 = x1 + 1
# x3 = x1 + 2
#
# plt.figure(figsize=(12, 6))
#
# # 设置箱线图宽度
# box_width = 0.75
#
# box1 = plt.boxplot(y1, positions=x1, patch_artist=True, showmeans=False,
#                    boxprops={"facecolor": "#377EB8CC", "edgecolor": "k", "linewidth": 0.5},
#                    medianprops={"color": "k", "linewidth": 0.5},
#                    meanprops={"marker": "+", "markerfacecolor": "k", "markeredgecolor": "k", "markersize": 0.5},
#                    showfliers=False, widths=box_width)
#
# box2 = plt.boxplot(y2, positions=x2, patch_artist=True, showmeans=False,
#                    boxprops={"facecolor": "#4DAF4ACC", "edgecolor": "k", "linewidth": 0.5},
#                    medianprops={"color": "k", "linewidth": 0.5},
#                    meanprops={"marker": "+", "markerfacecolor": "k", "markeredgecolor": "k", "markersize": 0.5},
#                    showfliers=False, widths=box_width)
#
# box3 = plt.boxplot(y3, positions=x3, patch_artist=True, showmeans=False,
#                    boxprops={"facecolor": "#E41A1CCC", "edgecolor": "k", "linewidth": 0.5},
#                    medianprops={"color": "k", "linewidth": 0.5},
#                    meanprops={"marker": "+", "markerfacecolor": "k", "markeredgecolor": "k", "markersize": 0.5},
#                    showfliers=False, widths=box_width)
#
#
# XX = np.arange(1, 23)
# XX = np.append(XX, "X")
# XX = np.append(XX, "Y")
# plt.xticks(np.arange(0, 24 * 4, 4), XX, ha='center', fontsize=11)
# plt.ylim(-0.05, 1)
# plt.ylabel('score%', fontsize=11)
# plt.grid(axis='y', ls='--', alpha=0.3)
# plt.grid(axis='x', linestyle='--', alpha=0.8)
#
# # 添加散点图
# colors = ['#377EB8', '#4DAF4A', '#E41A1C']  # 设置散点图的颜色
# markers = ['o', 'o', 'o']  # 设置散点图的标记样式
# sizes = [9, 9, 9]  # 设置散点图的大小
# edgecolors = ['black', 'black', 'black']  # 设置描边的颜色
#
# for i in range(24):
#     plt.scatter(np.full((len(y1[i]),), x1[i]), y1[i], color=colors[0], marker=markers[0],
#                 s=sizes[0], edgecolor=edgecolors[0], linewidth=0.3)
#     plt.scatter(np.full((len(y2[i]),), x2[i]), y2[i], color=colors[1], marker=markers[1],
#                 s=sizes[1], edgecolor=edgecolors[1], linewidth=0.3)
#     plt.scatter(np.full((len(y3[i]),), x3[i]), y3[i], color=colors[2], marker=markers[2],
#                 s=sizes[2], edgecolor=edgecolors[2], linewidth=0.3)
# # 绘制图例
# plt.legend(handles=[box1['boxes'][0], box2['boxes'][0], box3['boxes'][0]],
#            labels=['Cactus', 'Parsnp', 'HalignG'])
#
# plt.tight_layout()
# plt.savefig('boxplot1.svg', dpi=3000)
# plt.show()




import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import MultipleLocator

filename = 'A-msa-no1.xlsx'
h4_t = []
h4_m = []
par_t = []
par_m = []
cac_t = []
cac_m = []
#time 0-20
#mem 0-26
sheets = pd.read_excel(filename, sheet_name=None)
for sheet_name, sheet_data in list(sheets.items())[:]:
    h4_t.append(sheet_data['H4_maf8_time'].fillna(0).values[1])
    h4_m.append(sheet_data['H4_maf8_mem'].fillna(0).values[1])
    par_t.append(sheet_data['Parsnp8_time'].fillna(0).values[1])
    par_m.append(sheet_data['Parsnp8_mem'].fillna(0).values[1])
    cac_t.append(sheet_data['cactus80_time'].fillna(0).values[1])
    cac_m.append(sheet_data['cactus80_mem'].fillna(0).values[1])

print(max(h4_t),min([x for x in h4_t if x != 0]))
print(max(h4_m),min([x for x in h4_m if x != 0]))
print(max(par_t),min([x for x in par_t if x != 0]))
print(max(par_m),min([x for x in par_m if x != 0]))
print(max(cac_t),min([x for x in cac_t if x != 0]))
print(max(cac_m),min([x for x in cac_m if x != 0]))

y1 = cac_t
y2 = par_t
y3 = h4_t

m1 = cac_m
m2 = par_m
m3 = h4_m


x1 = np.arange(0, 4 * 24, 4)
x2 = x1 + 1
x3 = x1 + 2

plt.figure(figsize=(12, 2.5))



XX = np.arange(1, 23)
XX = np.append(XX, "X")
XX = np.append(XX, "Y")
plt.xticks(np.arange(0, 24 * 4, 4), XX, ha='center', fontsize=11)

# 设置y轴刻度为整数
y_major_locator = MultipleLocator(5)
plt.gca().yaxis.set_major_locator(y_major_locator)
plt.ylim(0, 20)
plt.ylabel('Time/H', fontsize=11)
plt.grid(axis='y', ls='--', alpha=0.3)
plt.grid(axis='x', linestyle='--', alpha=0.8)
# 将网格置于底层
plt.gca().set_axisbelow(True)
# 逆时针旋转横纵坐标的刻度标签
plt.xticks(rotation=90)
plt.yticks(rotation=90)


# 添加散点图
colors = ['#377EB8', '#4DAF4A', '#E41A1C']  # 设置散点图的颜色
markers = ['o', 'o', 'o']  # 设置散点图的标记样式
sizes = [9, 9, 9]  # 设置散点图的大小
edgecolors = ['black', 'black', 'black']  # 设置描边的颜色

# 确保 sizes 是一个二维数组
sizes = np.array([np.array(m1), np.array(m2), np.array(m3)]) * 50

# 绘制散点图
for i in range(24):
    plt.scatter(x1[i], y1[i], color=colors[0], marker=markers[0], s=sizes[0][i],
                edgecolor=edgecolors[0], linewidth=0.3, alpha=0.7)
    plt.scatter(x1[i], y1[i], color='black', marker=markers[0], s=3,edgecolor='black', linewidth=1)

    plt.scatter(x1[i], y2[i], color=colors[1], marker=markers[1], s=sizes[1][i],
                edgecolor=edgecolors[1], linewidth=0.3, alpha=0.7)
    plt.scatter(x1[i], y2[i], color='black', marker=markers[0], s=3,edgecolor='black', linewidth=1)

    plt.scatter(x1[i], y3[i], color=colors[2], marker=markers[2], s=sizes[2][i],
                edgecolor=edgecolors[2], linewidth=0.3, alpha=0.7)
    plt.scatter(x1[i], y3[i], color='black', marker=markers[0], s=3, edgecolor='black', linewidth=1)

# 添加气泡图的大小图例
legend_labels = ['15G', '10G',' 5G' ]
legend_sizes = [15*50, 10*50,5*50 ]  # 指定图例中气泡的大小

# 创建图例散点图
legend_scatter = []
legend_scatter.append(plt.scatter([], [], c='black', marker='o', s=legend_sizes[0],
                             edgecolor='black', linewidth=0.3))
legend_scatter.append(plt.scatter([], [], c='black', marker='o', s=legend_sizes[1],
                             edgecolor='black', linewidth=0.3))
legend_scatter.append(plt.scatter([], [], c='black', marker='o', s=legend_sizes[2],
                             edgecolor='black', linewidth=0.3))

plt.legend(legend_scatter, legend_labels, title="Memory",loc='upper center',scatterpoints=1, labelspacing=2, handlelength=2)


plt.tight_layout()
plt.savefig('line1.svg', dpi=3000)
plt.show()
