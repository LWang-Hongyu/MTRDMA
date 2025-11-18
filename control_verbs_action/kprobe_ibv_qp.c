#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

#define MAX_COUNT 1000  // 设置你的阈值

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, __u64);
} qp_count SEC(".maps");

SEC("kprobe/mlx5_ibls_create_qp")
int kprobe_ibv_create_qp(struct pt_regs *ctx)
{
    __u32 key = 0;
    __u64 *count, init_val = 1;
    
    // 获取当前计数
    count = bpf_map_lookup_elem(&qp_count, &key);
    if (count) {
        (*count)++;
        
        // 检查是否超过阈值
        if (*count > MAX_COUNT) {
            // 触发事件
            bpf_printk("hello world: ibv_create_qp called %llu times", *count);
            
            // 重置计数器（可选）
            *count = 0;
        }
    } else {
        // 第一次调用，初始化计数器
        bpf_map_update_elem(&qp_count, &key, &init_val, BPF_ANY);
    }
    
    return 0;
}

char _license[] SEC("license") = "GPL";