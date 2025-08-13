import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.patches as patches

softname = ["Halign-G/Fasta", "Halign-G/Maf", "Mugsy", "Kalign", "MLAGAN", "PMauve", "Parsnp", "Cactus", "Halign-3", "FAME", "Fsa", "Pasta", "FMAlign"]
colors = ['#FF8080', '#FF0000', '#FF00FC', '#00F6EF', '#FF7300','#FFFA00', '#00FF22', '#0029FF','#00EFA6']
tmcolors = ['#FF8080CC', '#FF0000CC', '#FF00FCCC', '#00F6EFCC', '#FF7300CC','#FFFA00CC', '#00FF22CC', '#0029FFCC','#00EFA6CC']
#
# h4m = []
# mugsy = []
# kalign = []
# mlagan = []
# pmauve = []
# parsnp = []
# cactus = []
# h3 = []
filename = '0score.xlsx'
sheets = pd.read_excel(filename, sheet_name=None)
sheet = 0
for sheet_name, sheet_data in list(sheets.items())[:]:
    ids = []
    alltime = [0,0,0,0,0,0,0,0,0]
    allmem =  [0,0,0,0,0,0,0,0,0]
    alldata = [np.empty(0), np.empty(0), np.empty(0), np.empty(0), np.empty(0), np.empty(0), np.empty(0), np.empty(0), np.empty(0)]
    for i in range(9):
        try:
            value = sheet_data[softname[i]].fillna(0).values
            alldata[i] = value[2:]
            alltime[i] = value[0]
            allmem[i] = value[1]
        except:
            pass
    for i in range(9):
        if alldata[i].size != 0:
            ids.append(i)
    new_list = [x for x in alltime if x != 0]
    alltime = new_list
    new_list = [x for x in allmem if x != 0]
    allmem = new_list
    new_list = [x for x in alldata if x.size != 0]
    alldata = new_list

    print(ids)
    # print(alltime)
    # print(allmem)
    # print(len(alldata))
    # print(alldata)
    plt.figure(figsize=(6, 10))
    sof_num = len(ids)
    # 设置箱线图宽度
    box_width = 0.75
    x = np.arange(1, len(ids), len(ids))
    XX = []
    for i in range(sof_num):
        plt.boxplot(alldata[i], positions=x+i, patch_artist=True, showmeans=False,
                    boxprops={"facecolor": tmcolors[ids[i]], "edgecolor": "k", "linewidth": 0.5},
                    medianprops={"color": "k", "linewidth": 0.5},
                    meanprops={"marker": "+", "markerfacecolor": "k", "markeredgecolor": "k", "markersize": 0.5},
                    showfliers=False, widths=box_width)
        XX.append(softname[ids[i]])




    plt.xticks(np.arange(1, sof_num+1, 1), XX, ha='center', fontsize=11)
    plt.xticks(rotation=15)
    if sheet == 0:
        plt.ylim(0.99, 1)
    elif sheet == 1:
        plt.ylim(0.5, 1)
    elif sheet == 2:
        plt.ylim(0.7, 1) #0.7
    elif sheet == 3:
        plt.ylim(0.4, 1)
    elif sheet == 4:
        plt.ylim(0.7, 1)
    plt.ylabel('score%', fontsize=11)
    plt.grid(axis='y', ls='--', alpha=0.3)
    plt.grid(axis='x', linestyle='--', alpha=0.8)

    # 添加散点图
    markers = ['o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o']  # 设置散点图的标记样式
    sizes = [9, 9, 9,9,9,9,9,9,9,9]  # 设置散点图的大小
    edgecolors = ['black', 'black', 'black','black','black','black','black','black','black']  # 设置描边的颜色

    for i in range(sof_num):
        # print(np.full((len(alldata[i]),), (x+i)[0]))
        # print(len(alldata[i]))
        # print(alldata[i])
        plt.scatter(np.full((len(alldata[i]),), (x+i)[0]), alldata[i], color=colors[ids[i]], marker=markers[ids[i]],
                    s=sizes[ids[i]], edgecolor=edgecolors[ids[i]], linewidth=0.3)
    # 绘制图例
    if sheet==0:
        handles = [patches.Rectangle((0, 0), 1, 1, linewidth=0.5, edgecolor='black', facecolor=color) for color in colors]
        plt.legend(handles=handles[:9], labels=softname[:9])

    plt.tight_layout()
    if sheet == 0:
        plt.savefig('s1.svg', dpi=3000)
    elif sheet == 1:
        plt.savefig('s2.svg', dpi=3000)
    elif sheet == 2:
        plt.savefig('s3.svg', dpi=3000)
    elif sheet == 3:
        plt.savefig('s4.svg', dpi=3000)
    elif sheet == 4:
        plt.savefig('s5.svg', dpi=3000)

    plt.show()
    sheet+=1

#
# import pandas as pd
# import matplotlib.pyplot as plt
# import numpy as np
# import matplotlib.patches as patches
#
# softname = ["Halign-G/Fasta", "Halign-G/Maf", "Mugsy", "Kalign", "MLAGAN", "PMauve", "Parsnp", "Cactus", "Halign-3", "FAME", "Fsa", "Pasta", "FMAlign"]
# colors = ['#FF8080', '#FF0000', '#FF00FC', '#00F6EF', '#FF7300','#FFFA00', '#00FF22', '#0029FF','#00EFA6']
# tmcolors = ['#FF8080CC', '#FF0000CC', '#FF00FCCC', '#00F6EFCC', '#FF7300CC','#FFFA00CC', '#00FF22CC', '#0029FFCC','#00EFA6CC']
# # h4f = []
# # h4m = []
# # mugsy = []
# # kalign = []
# # mlagan = []
# # pmauve = []
# # parsnp = []
# # cactus = []
# # h3 = []
# filename = '0score.xlsx'
# sheets = pd.read_excel(filename, sheet_name=None)
# sheet = 0
# for sheet_name, sheet_data in list(sheets.items())[:]:
#     ids = []
#     alltime = [0,0,0,0,0,0,0,0,0]
#     allmem =  [0,0,0,0,0,0,0,0,0]
#     software_labels = []
#     software_c = []
#     for i in range(9):
#         try:
#             value = sheet_data[softname[i]].fillna(0).values
#             alltime[i] = value[0]
#             allmem[i] = value[1]
#         except:
#             pass
#     for i in range(9):
#         if alltime[i] != 0:
#             ids.append(i)
#             software_labels.append(softname[i])
#             software_c.append(colors[i])
#     new_list = [x for x in alltime if x != 0]
#     alltime = new_list
#     new_list = [x for x in allmem if x != 0]
#     allmem = new_list
#
#
#     print(ids)
#
#     plt.figure(figsize=(6, 8))
#     sof_num = len(ids)
#     # 设置箱线图宽度
#     box_width = 0.75
#     # x = np.arange(1, len(ids), len(ids))
#     #
#     # plt.plot(software_labels, alltime, linestyle='-', marker='o', markersize=2, linewidth=0, alpha=1, color='black')
#     # plt.scatter(software_labels, alltime, s=allmem, c=software_c, alpha=0.5)
#     # plt.scatter(software_labels, alltime, s=allmem, c=software_c, alpha=0.5, edgecolors='black', linewidths=0.5)
#     # for i, label in enumerate(software_labels):
#     #     plt.text(label, alltime[i], f'{alltime[i]:.2f} S\n{allmem[i]:.2f} MB', ha='left', va='bottom',rotation=45)
#
#     # 计算刻度位置和间距
#     x_ticks = [i for i in range(1, 2 * len(ids) + 1, 2)]
#     x_spacing = (x_ticks[-1] - x_ticks[0]) / (len(x_ticks) - 1)
#     # 绘制图形
#     plt.plot(x_ticks, alltime, linestyle='-', marker='o', markersize=2, linewidth=0, alpha=1, color='black')
#     plt.scatter(x_ticks, alltime, s=allmem, c=software_c, alpha=0.5)
#     plt.scatter(x_ticks, alltime, s=allmem, c=software_c, alpha=0.5, edgecolors='black', linewidths=0.5)
#     # plt.scatter(x_ticks, alltime, s=allmem, c='blue', alpha=0.5)
#     # plt.scatter(x_ticks, alltime, s=allmem, c='blue', alpha=0.5, edgecolors='black', linewidths=0.5)
#
#     # 设置刻度位置和标签
#     plt.xticks(x_ticks, [f'{label:.2f}' for label in x_ticks])
#
#     # 调整x轴范围，保证第一个刻度和最后一个刻度到轴开始和轴末尾的距离相同
#     plt.xlim(x_ticks[0] - x_spacing / 2, x_ticks[-1] + x_spacing / 2)
#     # 添加文本标注
#     for i in range(len(ids)):
#         plt.text(x_ticks[i], alltime[i], f'{alltime[i]:.2f} S\n{allmem[i]:.2f} MB', ha='left', va='bottom', rotation=45)
#
#     plt.xlabel('Software')
#     plt.ylabel('Time (s)')
#     plt.title('Time and Memory Consumption for Different Software')
#     plt.xticks(rotation=15)
#     plt.grid(axis='y', ls='--', alpha=0.3)
#     plt.grid(axis='x', linestyle='--', alpha=0.3)
#     # 添加气泡大小的图例
#     # sizes = [10 * mem for mem in allmem]  # 调整气泡大小的比例
#     # plt.scatter([], [], s=sizes[0], c='blue', alpha=0.5, label='Memory (MB)')
#     # plt.legend(scatterpoints=1, frameon=False, labelspacing=1, title='Memory (MB)')
#     sheet+=1
#     plt.savefig(f'l{sheet}.svg', dpi=3000)
#     plt.show()
#     # exit(0)


# import pandas as pd
# import matplotlib.pyplot as plt
# import numpy as np
# from matplotlib.ticker import MultipleLocator
#
# filename = 'A-msa-no1.xlsx'
# h4_t = []
# h4_m = []
# par_t = []
# par_m = []
# cac_t = []
# cac_m = []
# #time 0-20
# #mem 0-26
# sheets = pd.read_excel(filename, sheet_name=None)
# for sheet_name, sheet_data in list(sheets.items())[:]:
#     h4_t.append(sheet_data['H4_maf8_time'].fillna(0).values[1])
#     h4_m.append(sheet_data['H4_maf8_mem'].fillna(0).values[1])
#     par_t.append(sheet_data['Parsnp8_time'].fillna(0).values[1])
#     par_m.append(sheet_data['Parsnp8_mem'].fillna(0).values[1])
#     cac_t.append(sheet_data['cactus80_time'].fillna(0).values[1])
#     cac_m.append(sheet_data['cactus80_mem'].fillna(0).values[1])
#
# print(max(h4_t),min([x for x in h4_t if x != 0]))
# print(max(h4_m),min([x for x in h4_m if x != 0]))
# print(max(par_t),min([x for x in par_t if x != 0]))
# print(max(par_m),min([x for x in par_m if x != 0]))
# print(max(cac_t),min([x for x in cac_t if x != 0]))
# print(max(cac_m),min([x for x in cac_m if x != 0]))
#
# y1 = cac_t
# y2 = par_t
# y3 = h4_t
#
# m1 = cac_m
# m2 = par_m
# m3 = h4_m
#
#
# x1 = np.arange(0, 4 * 24, 4)
# x2 = x1 + 1
# x3 = x1 + 2
#
# plt.figure(figsize=(12, 2.5))
#
#
#
# XX = np.arange(1, 23)
# XX = np.append(XX, "X")
# XX = np.append(XX, "Y")
# plt.xticks(np.arange(0, 24 * 4, 4), XX, ha='center', fontsize=11)
#
# # 设置y轴刻度为整数
# y_major_locator = MultipleLocator(5)
# plt.gca().yaxis.set_major_locator(y_major_locator)
# plt.ylim(0, 20)
# plt.ylabel('Time/H', fontsize=11)
# plt.grid(axis='y', ls='--', alpha=0.3)
# plt.grid(axis='x', linestyle='--', alpha=0.8)
# # 将网格置于底层
# plt.gca().set_axisbelow(True)
# # 逆时针旋转横纵坐标的刻度标签
# plt.xticks(rotation=90)
# plt.yticks(rotation=90)
#
#
# # 添加散点图
# colors = ['#377EB8', '#4DAF4A', '#E41A1C']  # 设置散点图的颜色
# markers = ['o', 'o', 'o']  # 设置散点图的标记样式
# sizes = [9, 9, 9]  # 设置散点图的大小
# edgecolors = ['black', 'black', 'black']  # 设置描边的颜色
#
# # 确保 sizes 是一个二维数组
# sizes = np.array([np.array(m1), np.array(m2), np.array(m3)]) * 50
#
# # 绘制散点图
# for i in range(24):
#     plt.scatter(x1[i], y1[i], color=colors[0], marker=markers[0], s=sizes[0][i],
#                 edgecolor=edgecolors[0], linewidth=0.3, alpha=0.7)
#     plt.scatter(x1[i], y1[i], color='black', marker=markers[0], s=3,edgecolor='black', linewidth=1)
#
#     plt.scatter(x1[i], y2[i], color=colors[1], marker=markers[1], s=sizes[1][i],
#                 edgecolor=edgecolors[1], linewidth=0.3, alpha=0.7)
#     plt.scatter(x1[i], y2[i], color='black', marker=markers[0], s=3,edgecolor='black', linewidth=1)
#
#     plt.scatter(x1[i], y3[i], color=colors[2], marker=markers[2], s=sizes[2][i],
#                 edgecolor=edgecolors[2], linewidth=0.3, alpha=0.7)
#     plt.scatter(x1[i], y3[i], color='black', marker=markers[0], s=3, edgecolor='black', linewidth=1)
#
# # 添加气泡图的大小图例
# legend_labels = ['15G', '10G',' 5G' ]
# legend_sizes = [15*50, 10*50,5*50 ]  # 指定图例中气泡的大小
#
# # 创建图例散点图
# legend_scatter = []
# legend_scatter.append(plt.scatter([], [], c='black', marker='o', s=legend_sizes[0],
#                              edgecolor='black', linewidth=0.3))
# legend_scatter.append(plt.scatter([], [], c='black', marker='o', s=legend_sizes[1],
#                              edgecolor='black', linewidth=0.3))
# legend_scatter.append(plt.scatter([], [], c='black', marker='o', s=legend_sizes[2],
#                              edgecolor='black', linewidth=0.3))
#
# plt.legend(legend_scatter, legend_labels, title="Memory",loc='upper center',scatterpoints=1, labelspacing=2, handlelength=2)
#
#
# plt.tight_layout()
# plt.savefig('line1.svg', dpi=3000)
# plt.show()
