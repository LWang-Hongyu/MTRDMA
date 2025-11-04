import matplotlib.pyplot as plt
import numpy as np

# 更新后的数据
message_sizes = [16, 32, 64, 128, 256, 512, 1024, 2048, 4096]
wo_back = [0.84, 1.19, 2.56, 5, 13.6, 19.94, 50.25, 79.73, 97.02]
with_back = [0.41, 0.81, 1.62, 3.2, 6.35, 12.67, 20.64, 33.37, 48.81]
with_back_mtrdma = [0.87, 1.67, 2.55, 4.94, 13.32, 20.21, 47.22, 47.56, 47.98]

# 创建图形
plt.figure(figsize=(12, 7))
plt.style.use('default')  # 使用更现代的样式

# 设置柱状图位置和宽度
x = np.arange(len(message_sizes))
width = 0.25

# 绘制柱状图
bars1 = plt.bar(x - width, wo_back, width, color='#55433b', label='w/o Back')
bars2 = plt.bar(x, with_back, width, color='#bca79d', label='with Back')
bars3 = plt.bar(x + width, with_back_mtrdma, width, color='#f77635', label='with Back + MTRDMA')

# 添加标签和标题
# plt.xlabel('Message Size (bytes)', fontsize=12)
# plt.ylabel('Value', fontsize=12)
# plt.title('Performance Comparison by Message Size (Updated Data)', fontsize=14)
plt.xticks(x, message_sizes)
plt.legend()

# 添加数值标签（可选）
for bars in [bars1, bars2, bars3]:
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.1f}',
                ha='center', va='bottom', fontsize=8)

# 调整y轴范围
plt.ylim(0, max(wo_back)*1.1)

# 添加网格线
plt.grid(axis='y', linestyle='--', alpha=0.7)

# 在数据未48.5的位置画一根红色的虚线
plt.axhline(y=48.5, color='red', linestyle='--')

# 保存为PNG
plt.tight_layout()
plt.savefig('updated_experiment_results.png', dpi=300, bbox_inches='tight')
print("更新后的实验图已保存为 updated_experiment_results.png")

# 显示图形
plt.show()