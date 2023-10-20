import matplotlib.pyplot as plt
import pandas as pd

# 读取文件并获取第六列数据
data = pd.read_csv('C:/Users/13508/Desktop/Halign-G/实验/实验1-18人cac-par/Halign-G/100w-score.txt', sep='\t')
column_data = data['Match/noN_Len'].replace('--', float(1)).astype(float)
mean_val = column_data.mean()
print("平均值为：", mean_val)
# 绘制直方图
plt.hist(column_data, bins=10000)  # 设置bins的数量适当调整以获得更好的效果
plt.xlabel('M-score')
plt.ylabel('Frequency')
plt.title('Data Distribution')
# plt.ylim([0, 10000]) # 设置y轴范围
plt.xlim([0.98, 1]) # 设置x轴范围

# 保存图像
plt.savefig('C:/Users/13508/Desktop/Halign-G/实验/实验1-18人cac-par/Halign-G/histogram.svg')

# 显示图像
plt.show()
