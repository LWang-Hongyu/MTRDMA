#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_TENANT_NUM 3000
#define QPS_CHECK_INTERVAL 10000   // 10ms
#define PRINT_INTERVAL 1000000     // 1s
#define BTENANT_ENABLE_TIME 100000 // 100ms

struct mtrdma_shm_context
{
    uint32_t next_tenant_id;
    uint32_t tenant_num;
    uint32_t active_tenant_num;
    uint64_t active_qps_num;
    uint32_t max_qps_limit;

    uint32_t active_qps_per_tenant[MAX_TENANT_NUM];

    pthread_mutex_t mtrdma_thread_lock[MAX_TENANT_NUM];
    pthread_cond_t mtrdma_thread_cond[MAX_TENANT_NUM];
    pthread_mutex_t lock;
};

void main()
{
    struct mtrdma_shm_context *shm_ctx = NULL;
    int shm_fd;

    printf("Init mtrdma_shm\n");
    shm_fd = shm_open("/home/phx/MTRDMA/mtrdma-shm", O_CREAT | O_RDWR, 0666);

    if (shm_fd == -1)
    {
        printf("Open mtrdma_shm is failed\n");
        exit(1);
    }

    if (ftruncate(shm_fd, sizeof(struct mtrdma_shm_context)) < 0)
        printf("ftruncate error\n");

    shm_ctx = (struct mtrdma_shm_context *)mmap(0, sizeof(struct mtrdma_shm_context), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (shm_ctx == MAP_FAILED)
    {
        printf("Error mapping shared memory mtrdma_shm");
        exit(1);
    }

    shm_ctx->next_tenant_id = 0;
    shm_ctx->tenant_num = 0;
    shm_ctx->active_tenant_num = 0;
    shm_ctx->active_qps_num = 0;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(shm_ctx->lock), &attr);

    pthread_condattr_t attrcond;
    pthread_condattr_init(&attrcond);
    pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED);

    for (uint32_t i = 0; i < MAX_TENANT_NUM; i++)
    {
        pthread_mutex_init(&(shm_ctx->mtrdma_thread_lock[i]), &attr);
        pthread_cond_init(&(shm_ctx->mtrdma_thread_cond[i]), &attrcond);
    }

    uint32_t NIC_QPS_CAPA = 8;
    uint32_t MAX_MSG_RATE = 12400; // Kpps
    uint64_t NIC_LINK_BW = 100000; // Mbps
    uint32_t MSEN_QP_LIMIT = 1;
    uint32_t MAX_SIM_BTENANT_NUM = 1;

    struct timeval now;
    struct timeval qps_check_timer;
    struct timeval print_timer;

    gettimeofday(&qps_check_timer, NULL);
    gettimeofday(&print_timer, NULL);

    struct timeval last_enable_btenant_time;
    gettimeofday(&last_enable_btenant_time, NULL);
    uint32_t post_stop_num = 0;
    uint32_t can_post_num = 0;

    uint32_t last_enable_btenant_idx = -1;
    uint32_t last_add_qp_tenant = -1;

    printf("mtrdma_NIC_QPS_CAPA: %d, mtrdma_NIC_LINK_BW: %ld, \ 
            mtrdma_NIC_MSG_RATE: %d mtrdma_MSEN_QP_LIMIT: %d  \
            mtrdma_MAX_SIM_BTENANT_NUM: %d\n",
           NIC_QPS_CAPA, NIC_LINK_BW, MAX_MSG_RATE, MSEN_QP_LIMIT, MAX_SIM_BTENANT_NUM);
    while (true)
    {
        uint32_t atn = 0, aqn = 0;
        uint64_t max_msg_size = 0;
        uint32_t tnum = 0;

        can_post_num = 0;
        for (uint32_t i = 0; i < MAX_TENANT_NUM; i++)
        {
            tnum++;
            if (shm_ctx->active_qps_per_tenant[i])
            {
                atn++;
                aqn += shm_ctx->active_qps_per_tenant[i];
            }

            if (tnum == shm_ctx->tenant_num)
                break;
        }

        uint32_t fu_qp_num;
        if (max_msg_size == 0)
            fu_qp_num = NIC_QPS_CAPA;
        else
            fu_qp_num = (NIC_LINK_BW * 0.7 / (double)(max_msg_size * 8)) / (double)(MAX_MSG_RATE / 1000.0);

        if (fu_qp_num == 0)
            fu_qp_num = 1;

        shm_ctx->active_tenant_num = atn;
        shm_ctx->active_qps_num = aqn;
        shm_ctx->max_qps_limit = NIC_QPS_CAPA > fu_qp_num ? fu_qp_num : NIC_QPS_CAPA;
        // printf("Instant Global Tenant Num: %d, Active Tenant Num: %d, Active QPs Num: %ld, Active Resp Read Tenant Num: %d,  Delay Sensitive Num: %d, Msg Senstivie Num: %d, Bandwidth Sesitive Num: %d, MAX_QPS_LIMIT: %d\n", shm_ctx->tenant_num, shm_ctx->active_tenant_num, shm_ctx->active_qps_num, shm_ctx->active_rrtenant_num, shm_ctx->active_dtenant_num, shm_ctx->active_mtenant_num, shm_ctx->active_tenant_num - shm_ctx->active_stenant_num, shm_ctx->max_qps_limit);
        qps_check_timer = now;

        gettimeofday(&now, NULL);
        uint64_t t = (now.tv_sec - print_timer.tv_sec) * 1000000 + (now.tv_usec - print_timer.tv_usec);

        if (t > PRINT_INTERVAL)
        {
            printf("Current Global Tenant Num: %d, Active Tenant Num: %d, MAX_QPS_LIMIT: %d\n", shm_ctx->tenant_num, shm_ctx->active_tenant_num, shm_ctx->max_qps_limit);

            print_timer = now;
        }

        usleep(QPS_CHECK_INTERVAL);
    }
}
