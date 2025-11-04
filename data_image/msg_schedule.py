import matplotlib.pyplot as plt
import numpy as np

# 数据准备
msg_sizes = ['64B', '128B', '512B', '1KB', '2KB', '4KB', '1MB', '1GB']
msg_rates = [13.26, 13.13, 11.82, 8.96, 5.61, 3.59, 3.55, 3.55]
avg_latencies = [1.25, 1.26, 1.27, 1.42, 1.93, 2.33, 2.31, 2.32]

# 创建图形和坐标轴
fig, ax1 = plt.subplots(figsize=(10, 6))

# 设置标题和标签
# plt.title("Message Rate and Average Latency vs Message Size", pad=20)

# 第一条Y轴（左侧）- 消息速率
color = 'tab:orange'
# ax1.set_xlabel('Msg size')
# ax1.set_ylabel('Msg Rate (million msgs/s)', color=color)
ax1.plot(msg_sizes, msg_rates, color=color, marker='o', linestyle='-', linewidth=2, label='Msg Rate')
ax1.tick_params(axis='y', labelcolor=color)

# 第二条Y轴（右侧）- 平均延迟
ax2 = ax1.twinx()
color = 'tab:gray'
# ax2.set_ylabel('Avg Latency (μs)', color=color)
ax2.plot(msg_sizes, avg_latencies, color=color, marker='s', linestyle='--', linewidth=2, label='Avg Latency')
ax2.tick_params(axis='y', labelcolor=color)

# 调整布局防止标签重叠
fig.tight_layout()

# 添加图例
lines1, labels1 = ax1.get_legend_handles_labels()
lines2, labels2 = ax2.get_legend_handles_labels()
ax1.legend(lines1 + lines2, labels1 + labels2, loc='center right')

# 保存为PNG文件
plt.savefig('message_rate_latency.png', dpi=300, bbox_inches='tight')

print("图表已保存为 message_rate_latency.png")