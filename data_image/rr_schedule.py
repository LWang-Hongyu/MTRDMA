import matplotlib.pyplot as plt
import numpy as np

# 数据准备
data = {
    "w/o": {
        "Msg size": [16, 32, 64, 128, 256, 0],
        "Msg rate": [9.97, 9.93, 9.74, 9.76, 9.6]
    },
    "1KB": {
        "Msg size": [16, 32, 64, 128, 256, 1024],
        "Msg rate": [5.8, 5.8, 5.78, 5.78, 5.78, 5.76]
    },
    "2KB": {
        "Msg size": [16, 32, 64, 128, 256, 2048],
        "Msg rate": [3.95, 3.95, 3.95, 3.95, 3.95, 3.9]
    },
    "4KB": {
        "Msg size": [16, 32, 64, 128, 256, 4096],
        "Msg rate": [2.47, 2.47, 2.47, 2.47, 2.47, 2.33]
    }
}

# 设置图形
plt.figure(figsize=(10, 6))  # 调整图形大小以减少图片比例
# plt.xlabel("Environment", labelpad=25)  # 调整xlabel的位置
# plt.ylabel("Message Rate (Msg/s)", labelpad=10)

# 从浅到深的橘色渐变
colors = plt.cm.Oranges(np.linspace(0.3, 0.9, len(data["w/o"]["Msg size"]) - 1))

# 使用RGBA值表示灰色 (0.6, 0.6, 0.6, 1.0)
dark_gray_rgba = np.array([0.35, 0.35, 0.35, 1.0])

# 将灰色添加到颜色数组末尾
colors = np.vstack([colors, dark_gray_rgba])

# 绘制分组直方图
bar_width = 0.6  # 调整条形宽度以减少不同env之间的间距
envs = list(data.keys())
x = np.arange(len(envs))  # 环境分组位置

for i, env in enumerate(envs):
    msg_sizes = data[env]["Msg size"]
    msg_rates = data[env]["Msg rate"]
    
    # 为每个消息大小绘制一个条形，颜色逐渐加深
    for j, (size, rate) in enumerate(zip(msg_sizes, msg_rates)):
        plt.bar(x[i] + j*bar_width/len(msg_sizes), 
                rate, 
                width=bar_width/len(msg_sizes), 
                color=colors[j],
                label=f'{size}B' if i == 2 else '')  # 只在第一组添加图例
        
    # 添加环境标签
    plt.text(x[i] + len(msg_sizes)/2*bar_width/len(msg_sizes)-0.05, -0.5, env, ha='center')

# 添加图例
handles, labels = plt.gca().get_legend_handles_labels()
unique_labels = list(dict.fromkeys(labels))  # 去重
unique_labels[-1] = "Back"
plt.legend(handles[:len(unique_labels)], unique_labels)

plt.xticks([])  # 隐藏默认的x轴标签
plt.grid(axis='y', linestyle='--', alpha=0.7)
plt.tight_layout()

# 保存为PNG文件
plt.savefig('message_rate_histogram.png', dpi=300, bbox_inches='tight')
print("图表已保存为 message_rate_histogram.png")

# 如果要同时显示图表，可以取消下面这行的注释
# plt.show()