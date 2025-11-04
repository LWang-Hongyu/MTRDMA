import matplotlib.pyplot as plt
import numpy as np

# 数据准备
data_sizes = ['w/o MTRDMA', 'with MTRDMA']
wo_mtrdma = [48.8, 48.61]  # 第二列数据
with_mtrdma = [49.12, 48.61]  # 第三列数据

# 创建图形
plt.figure(figsize=(8, 6))
plt.style.use('seaborn-v0_8-whitegrid')

# 设置柱状图位置和宽度
x = np.arange(len(data_sizes))
width = 0.35

# 绘制柱状图
bars1 = plt.bar(x - width/2, wo_mtrdma, width, color='#55433b', label='1MB')
bars2 = plt.bar(x + width/2, with_mtrdma, width, color='#f77635', label='1GB')

# 添加标签和标题
# plt.xlabel('Data Size', fontsize=12)
# plt.ylabel('Value', fontsize=12)
# plt.title('Performance Comparison with/without MTRDMA', fontsize=14)
plt.xticks(x, data_sizes)
plt.legend()

# 添加数值标签
for bar in bars1 + bars2:
    height = bar.get_height()
    plt.text(bar.get_x() + bar.get_width()/2., height,
            f'{height:.2f}',
            ha='center', va='bottom', fontsize=10)

# 调整y轴范围
plt.ylim(48, 49.5)

# 保存为PNG
plt.tight_layout()
plt.savefig('mtrdma_comparison.png', dpi=300, bbox_inches='tight')
print("MTRDMA对比图已保存为 mtrdma_comparison.png")

# 显示图形
plt.show()