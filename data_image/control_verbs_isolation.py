import matplotlib.pyplot as plt
import numpy as np

# 数据
without_mtrdma = [97.3, 97.37, 97.42, 97.54, 74.78, 97.51, 97.52, 97.5, 97.74, 97.74]
with_mtrdma = [96.45, 96.66, 96.67, 96.68, 96.45, 96.51, 96.53, 96.52, 96.52, 96.76]
x_values = np.arange(1, len(without_mtrdma) + 1)  # 从1开始的x轴坐标

# 创建图形
plt.figure(figsize=(10, 6))

# 绘制w/o MTRDMA的折线和散点
plt.plot(x_values, without_mtrdma, color='gray', linestyle='-', marker='o', 
         markersize=8, label='w/o MTRDMA')

# 绘制with MTRDMA的折线和散点
plt.plot(x_values, with_mtrdma, color='orange', linestyle='-', marker='x', 
         markersize=8, label='with MTRDMA')

# 添加标题和标签
# plt.title('Performance Comparison: with vs without MTRDMA', fontsize=14)
# plt.xlabel('Measurement Index', fontsize=12)
# plt.ylabel('Performance Value', fontsize=12)

# 添加图例
plt.legend(fontsize=12)

# 调整x轴刻度
plt.xticks(x_values)

# 设置网格
plt.grid(True, linestyle='--', alpha=0.6)

# 保存图像
plt.savefig('performance_comparison.png', dpi=300, bbox_inches='tight')

# 显示图形
plt.show()