import matplotlib.pyplot as plt
import numpy as np

# 数据准备
env_values = ["w/o", 1, 5, 10, 15, 20]
# bw_app_obser = [97.66, 49.11, 16.33, 8.93, 6.10, 4.66]
# bw_app_back = [0, 49.11, 81.66, 89.31, 91.53, 93.56]

# bw_app_obser = [97.66, 49.11, 16.33, 8.93, 4.66]
# bw_app_back = [0, 49.11, 81.66, 89.31, 93.56]

bw_app_obser = [97.66, 47.44, 47.23, 47.02, 46.89, 46.53]
bw_app_back = [0, 47.44, 47.31, 47.19, 47.01, 46.84]

# bw_app_obser = [13.61, 13.11, 11.45, 11.32, 11.61]
# bw_app_back = [0, 13.22, 45.45, 44.75, 43.96]

# 设置柱状图参数
bar_width = 0.35  # 柱状图宽度
index = np.arange(len(env_values))  # x轴位置

# 创建图形
plt.figure(figsize=(10, 6))

# 绘制柱状图
bar1 = plt.bar(index - bar_width/2, bw_app_obser, bar_width, 
               color='orange', label='Obser')
bar2 = plt.bar(index + bar_width/2, bw_app_back, bar_width, 
               color='gray', label='Back')

# 添加标签和标题
# plt.xlabel('env', fontsize=12)
# plt.ylabel('Value', fontsize=12)
# plt.title('Comparison of BW_APP Obser and Back by env', fontsize=14)
plt.xticks(index, env_values)  # 设置x轴刻度标签为env值
plt.legend()

# # 在48.5的位置画一根红色虚线
# plt.axhline(y=48.5, color='red', linestyle='--')

plt.axhline(y=48.5, color='red', linestyle='--')

# 调整布局
plt.tight_layout()

# 保存为PNG文件
plt.savefig('qp_anomaly.png', dpi=300, bbox_inches='tight')
print("图表已保存为 data_qp_comparison.png")

# 显示图形
plt.show()