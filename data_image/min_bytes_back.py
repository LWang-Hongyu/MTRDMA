import matplotlib.pyplot as plt
import numpy as np

# 准备数据
data = {
    'bytes': [16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192],
    'values': [0.29, 0.59, 1.18, 2.36, 4.72, 9.43, 15.13, 30.53, 61.46, 80.41]
}

# 创建图形
plt.figure(figsize=(12, 6))
plt.style.use('default')
bar_color = '#f77635'

# 创建等间距的x轴位置
x_positions = np.arange(len(data['bytes']))
bar_width = 0.6  # 控制柱子的宽度

# 绘制直方图
bars = plt.bar(x_positions, data['values'], width=bar_width, color=bar_color)

# 在直方图上方标注数值
for bar, value in zip(bars, data['values']):
    height = bar.get_height()
    plt.text(bar.get_x() + bar.get_width()/2., height,
             f'{value:.2f}',
             ha='center', va='bottom', fontsize=9)

# 设置图表标题和坐标轴标签
# plt.title('Bytes vs Values', fontsize=14)
# plt.xlabel('Bytes', fontsize=12)
# plt.ylabel('Values', fontsize=12)

# 设置x轴刻度和标签
plt.xticks(x_positions, data['bytes'])

# 调整y轴范围
plt.ylim(0, max(data['values']) * 1.1)

# 添加网格线
plt.grid(axis='y', color='lightgray', linestyle='--', alpha=0.7)

plt.axhline(y=48.5, color='red', linestyle='--')

# 调整布局
plt.tight_layout()

# 保存图片
plt.savefig('min_bytes_back.png', dpi=300, bbox_inches='tight')
print("图片已保存为 'bytes_values_histogram.png'")

# 显示图形
plt.show()