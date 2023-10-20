import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.patches as patches
import matplotlib.cm as cm

softname = ["Halign-G/Maf","Halign-G+Cactus", "Parsnp+Cactus"]
colors = [ '#FF0000','#0029FF', '#00FF22']
tmcolors = ['#FF0000CC','#0029FFCC', '#00FF22CC']
dark_colors = []
for color in colors:
    r, g, b = int(color[1:3], 16), int(color[3:5], 16), int(color[5:7], 16)
    dark_r = max(r - 25, 0)
    dark_g = max(g - 25, 0)
    dark_b = max(b - 25, 0)
    alpha = 'BB'  # 添加alpha值（十六进制）
    dark_color = f'#{dark_r:02X}{dark_g:02X}{dark_b:02X}{alpha}'
    dark_colors.append(dark_color)
tmcolors = dark_colors

filename = '4score.xlsx'
sheets = pd.read_excel(filename, sheet_name=None)
sheet = 0
for sheet_name, sheet_data in list(sheets.items())[:]:
    ids = []
    alldata = [np.empty(0), np.empty(0), np.empty(0), np.empty(0), np.empty(0), np.empty(0), np.empty(0), np.empty(0), np.empty(0)]
    for i in range(9):
        try:
            value = sheet_data[softname[i]].fillna(0).values
            alldata[i] = value[:]
        except:
            pass
    for i in range(9):
        if alldata[i].size != 0:
            ids.append(i)
    new_list = [x for x in alldata if x.size != 0]
    alldata = new_list

    print(ids)

    print(alldata)
    plt.figure(figsize=(6, 6))
    sof_num = len(ids)
    # 设置箱线图宽度
    box_width = 0.8
    x = np.arange(1, len(ids), len(ids))
    XX = []
    for i in range(sof_num):
        plt.boxplot(alldata[i], positions=x+i, patch_artist=True, showmeans=False,
                    boxprops={"facecolor": tmcolors[ids[i]], "edgecolor": "k", "linewidth": 1.5},
                    medianprops={"color": "k", "linewidth": 1.5},
                    meanprops={"marker": "+", "markerfacecolor": "k", "markeredgecolor": "k", "markersize": 1.5},
                    showfliers=False, widths=box_width)
        XX.append(softname[ids[i]])

    plt.xticks(np.arange(1, sof_num+1, 1), XX, ha='center', fontsize=11)
    plt.xticks(rotation=15)
    if sheet == 0:
        plt.ylim(0.7, 1)
    elif sheet == 1:
        plt.ylim(0.7, 1) #0.7
    elif sheet == 2:
        plt.ylim(0.7, 1)
    plt.ylabel('score%', fontsize=11)
    plt.grid(axis='y', ls='--', alpha=0.3)
    plt.grid(axis='x', linestyle='--', alpha=0.8)

    # 添加散点图
    markers = ['o', 'o', 'o']  # 设置散点图的标记样式
    sizes = [80, 80, 80]  # 设置散点图的大小
    edgecolors = ['black', 'black', 'black']  # 设置描边的颜色

    for i in range(sof_num):
        # print(np.full((len(alldata[i]),), (x+i)[0]))
        # print(len(alldata[i]))
        # print(alldata[i])
        plt.scatter(np.full((len(alldata[i]),), (x+i)[0]), alldata[i], color=colors[ids[i]], marker=markers[ids[i]],
                    s=sizes[ids[i]], edgecolor=edgecolors[ids[i]], linewidth=0.6)
    # 绘制图例
    if sheet==0:
        handles = [patches.Rectangle((0, 0), 1, 1, linewidth=0.5, edgecolor='black', facecolor=color) for color in colors]
        plt.legend(handles=handles[:9], labels=softname[:9])

    plt.tight_layout()
    if sheet == 0:
        plt.savefig('ss1.svg', dpi=3000)
    elif sheet == 1:
        plt.savefig('ss2.svg', dpi=3000)
    elif sheet == 2:
        plt.savefig('ss3.svg', dpi=3000)

    plt.show()
    sheet+=1