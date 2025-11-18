#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

int main(int argc, char **argv) {
    struct bpf_object *obj;
    int prog_fd, map_fd;
    __u32 key = 0;
    __u64 value;
    int err;
    
    // 1. 加载eBPF程序
    obj = bpf_object__open_file("kprobe_ibv_qp.o", NULL);
    if (libbpf_get_error(obj)) {
        fprintf(stderr, "Failed to open eBPF object\n");
        return 1;
    }
    
    // 2. 加载程序到内核
    err = bpf_object__load(obj);
    if (err) {
        fprintf(stderr, "Failed to load eBPF object: %d\n", err);
        return 1;
    }
    
    // 3. 获取程序文件描述符
    prog_fd = bpf_program__fd(bpf_object__find_program_by_name(obj, "kprobe_ibv_create_qp"));
    if (prog_fd < 0) {
        fprintf(stderr, "Failed to find eBPF program\n");
        return 1;
    }
    
    // 4. 获取map文件描述符
    map_fd = bpf_object__find_map_fd_by_name(obj, "qp_count");
    if (map_fd < 0) {
        fprintf(stderr, "Failed to find eBPF map\n");
        return 1;
    }
    
    // 5. 附加kprobe
    struct bpf_link *link = bpf_program__attach(obj);
    if (libbpf_get_error(link)) {
        fprintf(stderr, "Failed to attach eBPF program\n");
        return 1;
    }
    
    printf("eBPF program loaded and attached. Waiting for events...\n");
    
    // 6. 读取并打印计数（可选）
    while (1) {
        err = bpf_map_lookup_elem(map_fd, &key, &value);
        if (!err) {
            printf("Current ibv_create_qp count: %llu\n", value);
        }
        sleep(1);
    }
    
    bpf_link__destroy(link);
    bpf_object__close(obj);
    return 0;
}