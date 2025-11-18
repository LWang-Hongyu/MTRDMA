.PHONY: clean

APPS    := rdma_client rdma_server

CFLAGS  := -Wall -g 
LDLIBS  := ${LDLIBS} -I/home/phx/MTRDMA/rdma-core-58mlnx43/build/include -L/home/phx/MTRDMA/rdma-core-58mlnx43/build/lib -lpthread -lrdmacm -libverbs -lcrypto
# -I/home/gxy/paper/rdma-core-58mlnx43/build/include -L$(LD_LIBRARY_PATH)
# export LD_LIBRARY_PATH=/home/gxy/rdma/source/rdma-core-58mlnx43/build/lib
all: ${APPS}

clean:
	rm -f ${APPS}