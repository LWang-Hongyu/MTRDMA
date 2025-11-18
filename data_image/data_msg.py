import matplotlib.pyplot as plt
import numpy as np

# 消息率数据
message_sizes = [16, 32, 64, 128, 256, 512, 1024, 2048, 4096]
wo_back = [6.53, 4.65, 5.0, 4.87, 6.63, 4.86, 6.13, 4.86, 2.96]
with_back = [3.19, 3.17, 3.16, 3.12, 3.10, 3.09, 2.51, 2.0, 1.48]
with_back_mtrdma = [6.83, 6.5, 4.97, 4.82, 6.5, 4.93, 5.56, 2.86, 1.46]

# 创建图形
plt.figure(figsize=(12, 7))
plt.style.use('default')  # 使用带网格线的现代样式

# 设置柱状图位置和宽度
x = np.arange(len(message_sizes))
width = 0.25

# 绘制柱状图（使用您指定的颜色）
bars1 = plt.bar(x - width, wo_back, width, color='#55433b', label='w/o Back')
bars2 = plt.bar(x, with_back, width, color='#bca79d', label='with Back')
bars3 = plt.bar(x + width, with_back_mtrdma, width, color='#f77635', label='with Back + MTRDMA')

# 添加标签和标题
# plt.xlabel('Message Size (bytes)', fontsize=12)
# plt.ylabel('Message Rate', fontsize=12)
# plt.title('Message Rate Comparison by Message Size', fontsize=14, pad=20)
plt.xticks(x, message_sizes)
plt.legend(loc='upper right')

# 添加数值标签
def add_value_labels(bars):
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}',
                ha='center', va='bottom', fontsize=8)

add_value_labels(bars1)
add_value_labels(bars2)
add_value_labels(bars3)

# 调整y轴范围
plt.ylim(0, max(wo_back)*1.15)

# 添加水平网格线
plt.grid(axis='y', linestyle='--', alpha=0.7)

# 自定义x轴标签旋转角度
plt.xticks(rotation=45, ha='right')

# 调整边距
plt.tight_layout()

# 保存为PNG
plt.savefig('message_rate_comparison.png', dpi=300, bbox_inches='tight')
print("消息率对比图已保存为 message_rate_comparison.png")

# 显示图形
plt.show()