#include <stdio.h>
#include <stdlib.h>
#include <infiniband/verbs.h>

#define NUM_REGISTRATIONS 1000
#define BUFFER_SIZE 40960

int main()
{
    struct ibv_context *context;
    struct ibv_pd *pd;
    struct ibv_mr *mr;
    void *buffer;

    // 打开第一个可用的 RDMA 设备
    struct ibv_device **device_list = ibv_get_device_list(NULL);
    if (!device_list || !*device_list)
    {
        fprintf(stderr, "No RDMA devices found.\n");
        return 1;
    }
    struct ibv_device *device = device_list[0];
    context = ibv_open_device(device);
    if (!context)
    {
        fprintf(stderr, "Failed to open RDMA device.\n");
        ibv_free_device_list(device_list);
        return 1;
    }
    ibv_free_device_list(device_list);

    // 分配一个保护域
    pd = ibv_alloc_pd(context);
    if (!pd)
    {
        fprintf(stderr, "Failed to allocate protection domain.\n");
        ibv_close_device(context);
        return 1;
    }

    // 分配缓冲区
    buffer = malloc(BUFFER_SIZE);
    if (!buffer)
    {
        fprintf(stderr, "Failed to allocate buffer.\n");
        ibv_dealloc_pd(pd);
        ibv_close_device(context);
        return 1;
    }

    // 循环注册 MR
    while (1)
    {
        mr = ibv_reg_mr(pd, buffer, BUFFER_SIZE, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);
        if (!mr)
        {
            free(buffer);
            ibv_dealloc_pd(pd);
            ibv_close_device(context);
            return 1;
        }
        // 注销 MR
        ibv_dereg_mr(mr);
    }

    // 释放资源
    free(buffer);
    ibv_dealloc_pd(pd);
    ibv_close_device(context);

    printf("Successfully registered and deregistered MR %d times.\n", NUM_REGISTRATIONS);

    return 0;
}