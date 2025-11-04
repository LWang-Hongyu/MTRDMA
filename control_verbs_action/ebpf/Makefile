CLANG ?= clang
CFLAGS := -g -O2 -Wall

ARCH := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/')

BPF_CFLAGS := -g -O2 -Wall
BPF_CFLAGS += -target bpf -D__TARGET_ARCH_$(ARCH)

INCLUDES := -I/usr/include/bpf \
            -I/usr/include/linux \
            -I/usr/include

all: rdma_monitor

%.bpf.o: %.bpf.c
	$(CLANG) $(BPF_CFLAGS) $(INCLUDES) -c $< -o $@

%.skel.h: %.bpf.o 
	bpftool gen skeleton $< > $@

rdma_monitor: rdma_monitor.c rdma_monitor.skel.h
	$(CC) $(CFLAGS) -o $@ $< -lbpf -lelf

clean:
	rm -f *.o *.skel.h rdma_monitor

.PHONY: all clean