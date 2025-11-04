import matplotlib.pyplot as plt

# 数据准备
labels = ['w/o Interference', 'SR-IOV', 'FreeFlow', 'PeRF', 'MTRDMA']  # 横坐标标签
data_4096 = [96.23, 48.84, 13.34, 40.54, 46.89]       # 第二行数据(4096)
data_8192 = [97.37, 48.91, 26.55, 41.74, 47.23]       # 第三行数据(8192)
colors = ['#f77635', '#55433b']          # 直方图颜色

# 创建图形和坐标轴
fig, ax = plt.subplots(figsize=(8, 6))

# 设置条形图宽度和位置
bar_width = 0.35
x_pos = range(len(labels))

# 绘制两组直方图
bars1 = ax.bar([x - bar_width/2 for x in x_pos], data_4096, bar_width, 
               color=colors[0], label='4096B')
bars2 = ax.bar([x + bar_width/2 for x in x_pos], data_8192, bar_width, 
               color=colors[1], label='8192B')

# 添加标签和标题
# ax.set_xlabel('实现方案')
# ax.set_ylabel('数值')
# ax.set_title('不同实现方案的性能对比')
ax.set_xticks(x_pos)
ax.set_xticklabels(labels)
ax.legend()

# 在条形上方显示数值
def autolabel(bars):
    for bar in bars:
        height = bar.get_height()
        ax.annotate(f'{height:.2f}',
                    xy=(bar.get_x() + bar.get_width() / 2, height),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom')

autolabel(bars1)
autolabel(bars2)

plt.axhline(y=48.5, color='red', linestyle='--')

# 调整布局并保存图片
plt.tight_layout()
plt.savefig('diff_virtual.png', dpi=300, bbox_inches='tight')
plt.show()