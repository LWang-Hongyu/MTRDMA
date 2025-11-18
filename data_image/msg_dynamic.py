import matplotlib.pyplot as plt

# 数据整理
data = {
    "时刻": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15],
    "PeRF": [16.38, 16.38, 16.38, 16.38, 16.38, 32.19, 32.19, 32.19, 32.19, 32.19, 70.52, 70.52, 65.55, 41.22, 40.66],
    "MTRDMA": [15.07, 15.07, 15.07, 15.07, 15.07, 30.63, 30.63, 30.63, 30.63, 46.77, 46.77, 46.77, 46.77, 46.77, 46.77]
}

# 创建画布
plt.figure(figsize=(10, 6))

# 绘制PeRF折线（橙色）
plt.plot(data["时刻"], data["PeRF"], 
         marker='o', 
         color='#55433b', 
         linewidth=2,
         label='PeRF')

# 绘制MTRDMA折线（深棕色）
plt.plot(data["时刻"], data["MTRDMA"], 
         marker='o', 
         color='#f77635', 
         linewidth=2,
         label='MTRDMA')

# 添加图表元素
# plt.title('PeRF与MTRDMA随时间变化对比', fontsize=14)
# plt.xlabel('时刻', fontsize=12)
# plt.ylabel('数值', fontsize=12)
plt.grid(True, linestyle='--', alpha=0.6)
plt.legend(fontsize=12)

# 设置坐标轴范围
plt.xlim(0, 16)
plt.ylim(0, 80)

# 保存图片
plt.savefig('msg_dynamic.png', dpi=300, bbox_inches='tight')
plt.close()

print("图片已成功保存为 comparison_result.png")