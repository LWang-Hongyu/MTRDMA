import matplotlib.pyplot as plt
import numpy as np

# 给定的数据
data = [
    ["2-1", "4-1"],       # 第一行数据（横坐标）
    [63.37, 75.45], # 第二行数据（第一组直方图）
    [33.51, 20.74]  # 第三行数据（第二组直方图）
]

# 提取横坐标和两组数据
x_labels = data[0]
group1 = data[1]
group2 = data[2]

# 设置图形参数
bar_width = 0.35
index = np.arange(len(x_labels))

# 创建图形
plt.figure(figsize=(8, 6), facecolor='white')  # 白色背景
plt.bar(index, group1, width=bar_width, color='#f77635', label='hight weight')
plt.bar(index + bar_width, group2, width=bar_width, color='#55433b', label='low weight')

# 添加标题和标签
# plt.title('实验数据对比', fontsize=14)
# plt.xlabel('横坐标', fontsize=12)
# plt.ylabel('数值', fontsize=12)
plt.xticks(index + bar_width/2, x_labels)

# 添加图例
plt.legend()

# 调整布局使图形更整洁
plt.tight_layout()

# 保存为PNG文件
plt.savefig('experiment_plot.png', dpi=300, bbox_inches='tight')
print("实验图已保存为 experiment_plot.png")

# 显示图形（可选）
plt.show()