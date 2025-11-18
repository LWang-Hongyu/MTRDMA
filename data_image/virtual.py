import matplotlib.pyplot as plt
import numpy as np

# 数据准备
data = {
    'SR-IOV': {
        '2048 bytes': 33.36,
        '4096 bytes': 63.52,
        '1 QP': 16.28,
        '5 QP': 81.37
    },
    'FreeFlow': {
        '2048 bytes': 7.73,
        '4096 bytes': 15.48,
        '1 QP': 17.8,
        '5 QP': 17.94
    }
}

# 设置图形参数
plt.figure(figsize=(12, 8))
width = 0.2  # 柱状图宽度
x = np.arange(2)  # 两组数据：SR-IOV和FreeFlow
labels = ['SR-IOV', 'FreeFlow']

# 绘制字节数对比的柱状图
bars1 = plt.bar(x - width*1.5, [data['SR-IOV']['2048 bytes'], data['FreeFlow']['2048 bytes']], 
                width, color='#f77635', label='2048 bytes')
bars2 = plt.bar(x - width/2, [data['SR-IOV']['4096 bytes'], data['FreeFlow']['4096 bytes']], 
                width, color='#f77635', label='4096 bytes', alpha=0.7)

# 绘制QP数对比的柱状图
bars3 = plt.bar(x + width/2, [data['SR-IOV']['1 QP'], data['FreeFlow']['1 QP']], 
                width, color='#55433b', label='1 QP')
bars4 = plt.bar(x + width*1.5, [data['SR-IOV']['5 QP'], data['FreeFlow']['5 QP']], 
                width, color='#55433b', label='5 QP', alpha=0.7)

# 添加数值标签
def add_labels(bars):
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}',
                ha='center', va='bottom', fontsize=10)

add_labels(bars1)
add_labels(bars2)
add_labels(bars3)
add_labels(bars4)

# 添加图表元素
# plt.title('Performance Comparison: SR-IOV vs FreeFlow', fontsize=14)
# plt.xlabel('Technology', fontsize=12)
# plt.ylabel('Performance Metric', fontsize=12)
plt.xticks(x, labels, fontsize=11)
plt.legend(bbox_to_anchor=(0.95, 0.85), loc='center right')

# 调整布局并保存图片
plt.tight_layout()
plt.savefig('performance_comparison_with_labels.png', dpi=300, bbox_inches='tight')
print("图片已保存为 performance_comparison_with_labels.png")
plt.show()