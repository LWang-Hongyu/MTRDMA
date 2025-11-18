import matplotlib.pyplot as plt
import numpy as np

# 数据准备
message_sizes = [16, 32, 64, 256, 512, 1024]
wo_back = [1.15, 1.11, 1.19, 1.67, 1.62, 1.89]
with_back = [2.2, 2.26, 2.19, 3.2, 3.22, 3.41]
with_back_mtrdma = [1.16, 1.21, 1.22, 1.62, 1.74, 2.05]

# 设置图形
plt.figure(figsize=(10, 6))
plt.style.use('default')  # 使用白色背景

# 设置柱状图的位置和宽度
x = np.arange(len(message_sizes))
width = 0.25

# 绘制柱状图
bars1 = plt.bar(x - width, wo_back, width, color='#55433b', label='w/o Back')
bars2 = plt.bar(x, with_back, width, color='#bca79d', label='with Back')
bars3 = plt.bar(x + width, with_back_mtrdma, width, color='#f77635', label='with Back + MTRDMA')

# 在柱状图上添加数值标签
def autolabel(bars):
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}',
                ha='center', va='bottom')

autolabel(bars1)
autolabel(bars2)
autolabel(bars3)

# 添加标签和标题
# plt.xlabel('Message Size (bytes)', fontsize=12)
# plt.ylabel('Value', fontsize=12)
# plt.title('Performance Comparison by Message Size', fontsize=14)
plt.xticks(x, message_sizes)
plt.legend()

# 调整布局
plt.tight_layout()

# 保存为PNG
plt.savefig('MTRDMA_lat.png', dpi=300, bbox_inches='tight')
print("实验图已保存为 experiment_results.png")

# 显示图形
plt.show()