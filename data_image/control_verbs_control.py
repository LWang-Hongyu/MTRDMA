import matplotlib.pyplot as plt

# 数据
index = [1000, 2000, 3000, 4000, 5000]  # 假设这是x轴的值
mtrdma = [1000, 2000, 3000, 4000, 5000]
idea = [1000, 2000, 3000, 4000, 5000]

# 创建图形
plt.figure(figsize=(8, 6))

# 绘制Idea的散点折线图
plt.plot(index, idea, color='gray', marker='s', linestyle='-', 
         linewidth=2, markersize=8, label='Idea', markerfacecolor='none')
# 绘制MTRDMA的散点折线图
plt.plot(index, mtrdma, color='orange', marker='x', linestyle='--', 
         linewidth=2, markersize=8, label='MTRDMA')

# 添加标题和标签
# plt.title('Comparison of MTRDMA and Idea', fontsize=14)
# plt.xlabel('Index', fontsize=12)
# plt.ylabel('Value', fontsize=12)

# 添加图例
plt.legend(fontsize=12)

# 设置x轴刻度
plt.xticks(index, fontsize=10)

# 调整布局
plt.tight_layout()

# 保存图形
plt.savefig('comparison_plot.png', dpi=300)

# 显示图形
plt.show()