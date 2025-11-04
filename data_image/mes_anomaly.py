import matplotlib.pyplot as plt
import numpy as np

# 数据准备
categories = ['w/o Back', 'with Back']
msg_rates = [13.75, 3.56]  # 消息速率数据
latencies = [1.21, 2.25]   # 延迟数据

# 创建图形和轴
fig, ax1 = plt.subplots(figsize=(10, 6))

# 设置直方图（左Y轴）
bars = ax1.bar(categories, msg_rates, 
               color=['orange', 'gray'], 
               width=0.4,
               alpha=0.8,
               label='Message Rate (higher is better)')
# ax1.set_ylabel('Message Rate (msg/s)', fontsize=12)
ax1.set_ylim(0, max(msg_rates) * 1.3)

# 添加直方图数值标签
for bar in bars:
    height = bar.get_height()
    ax1.text(bar.get_x() + bar.get_width()/2., height,
             f'{height:.2f}',
             ha='center', va='bottom',
             fontsize=11)

# 创建第二个Y轴（右Y轴）
ax2 = ax1.twinx()
line, = ax2.plot(categories, latencies, 
                color='green', 
                marker='o', 
                linewidth=3, 
                markersize=10,
                label='Latency (lower is better)')
# ax2.set_ylabel('Latency (ms)', fontsize=12)
ax2.set_ylim(0, max(latencies) * 1.5)

# 添加折线图数值标签
for i, txt in enumerate(latencies):
    ax2.annotate(f'{txt:.2f}', 
                (categories[i], latencies[i]),
                textcoords="offset points",
                xytext=(0,10),
                ha='center',
                fontsize=11)

# 添加图例（合并两个轴的图例）
lines = [bars[0], bars[1], line]
labels = ['Msg Rate w/o Back', 'Msg Rate with Back', 'Lat']
ax1.legend(lines, labels, 
          loc='upper right',
          fontsize=11,
          framealpha=0.9,
          ncol=1)

# 设置图表标题和格式
# plt.title('Performance Comparison: Message Rate vs Latency\n(with and without BW_APP)', 
  #        fontsize=14, pad=20)
plt.grid(axis='y', linestyle=':', alpha=0.5)

# 添加分析说明
# plt.figtext(0.5, 0.01, 
        #    "Analysis: BW_APP reduces message rate by ~74% but increases latency by ~86%",
        #    ha="center", 
        #    fontsize=11,
        #    bbox={"facecolor":"white", "alpha":0.8, "pad":5})

# 调整布局防止标签重叠
fig.tight_layout(rect=[0, 0.05, 1, 1])  # 为底部文本留空间

# 保存为PNG
plt.savefig('bw_influent_lat.png', dpi=300, bbox_inches='tight')
plt.show()