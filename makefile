CC = gcc
LINK = gcc
DPUCC = dpu-upmem-dpurte-clang
DPULINK = dpu-upmem-dpurte-clang

DPU_DIR := dpu
HOST_DIR := host
BUILD_DIR := bin
OBJ_DIR := obj
INC_DIR := include

NR_DPUS ?=2560
NR_TASKLETS ?= 16
GRAPH ?= WV
PATTERN ?= CLIQUE3
BITMAP_PATTERN ?= CLIQUE3_BM

COMMON_CCFLAGS := -c -Wall -Wextra -g -O2 -I${INC_DIR} \
  -DNR_TASKLETS=${NR_TASKLETS} \
  -DNR_DPUS=${NR_DPUS} \
  -D${GRAPH} \
  -D${PATTERN} \
  -D$(PATTERN)_BM \
  -DDPU_BINARY=\"${BUILD_DIR}/dpu\" \
  -DDPU_ALLOC_BINARY=\"${BUILD_DIR}/dpu_alloc\" \
  -DDPU_BM_BINARY=\"${BUILD_DIR}/dpu_bm\" \
  ${EXTRA_FLAGS}


HOST_CCFLAGS := ${COMMON_CCFLAGS} -std=c11 -mcmodel=large `dpu-pkg-config --cflags dpu` 
DPU_CCFLAGS := ${COMMON_CCFLAGS}

COMMON_LFLAGS := -DNR_TASKLETS=${NR_TASKLETS}
HOST_LFLAGS := ${COMMON_LFLAGS} `dpu-pkg-config --libs dpu`
DPU_LFLAGS := ${COMMON_LFLAGS}

INC_FILE := ${INC_DIR}/common.h ${INC_DIR}/cyclecount.h ${INC_DIR}/timer.h ${INC_DIR}/dpu_mine.h

ifeq ($(PATTERN),CLIQUE3)
  DPU_BM_TARGET = ${BUILD_DIR}/dpu_bm
else
  DPU_BM_TARGET =
endif

.PHONY: all all_before host dpu clean test test_single test_all

all: all_before ${BUILD_DIR}/host ${BUILD_DIR}/dpu ${BUILD_DIR}/dpu_alloc $(DPU_BM_TARGET)

all_before:
	@mkdir -p ${BUILD_DIR}
	@mkdir -p ${OBJ_DIR}
	@mkdir -p ${OBJ_DIR}/${HOST_DIR}
	@mkdir -p ${OBJ_DIR}/${DPU_DIR}
	@mkdir -p result

${BUILD_DIR}/host: ${OBJ_DIR}/${HOST_DIR}/main.o ${OBJ_DIR}/${HOST_DIR}/partition.o ${OBJ_DIR}/${HOST_DIR}/mine.o ${OBJ_DIR}/${HOST_DIR}/set_op.o ${OBJ_DIR}/${HOST_DIR}/heap.o
	@${LINK} $^ ${HOST_LFLAGS}  -o $@

${BUILD_DIR}/dpu: ${OBJ_DIR}/${DPU_DIR}/main.o ${OBJ_DIR}/${DPU_DIR}/set_op.o ${OBJ_DIR}/${DPU_DIR}/${PATTERN}.o
	@${DPULINK} ${DPU_LFLAGS} $^ -o $@

${BUILD_DIR}/dpu_alloc: ${OBJ_DIR}/${DPU_DIR}/partition.o
	@${DPULINK} ${DPU_LFLAGS} $^ -o $@

$(DPU_BM_TARGET): ${OBJ_DIR}/${DPU_DIR}/bitmap.o ${OBJ_DIR}/${DPU_DIR}/bit_op.o ${OBJ_DIR}/${DPU_DIR}/CLIQUE3_BM.o
	@${DPULINK} ${DPU_LFLAGS} $^ -o $@

${OBJ_DIR}/${HOST_DIR}/%.o: ${HOST_DIR}/%.c ${INC_FILE}
	@${CC} ${HOST_CCFLAGS} $< -o $@

${OBJ_DIR}/${DPU_DIR}/%.o: ${DPU_DIR}/%.c ${INC_FILE}
	@${DPUCC} ${DPU_CCFLAGS} $< -o $@

clean:
	@rm -rf ${BUILD_DIR} ${OBJ_DIR}

test:
	@make clean --no-print-directory
	@make all --no-print-directory
	@./${BUILD_DIR}/host

test_single:
	@make clean --no-print-directory
	@NR_DPUS=1 NR_TASKLETS=1 make all --no-print-directory
	@./${BUILD_DIR}/host

test_all:
#####=====SNAP=====#####
	@GRAPH=CA       PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=PT       PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=PA       PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=NCA      PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=NTX      PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=FE    PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=P2P04    PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=P2P31    PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=LJ       PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=AM0302   PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=AM0312   PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=AM0505   PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=AM0601   PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=CH       PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=YT       PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=ORKUT   PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=TW20    PATTERN=CLIQUE3 EXTRA_FLAGS="-DV_NR_DPUS=25600" make test --no-print-directory 


#####=====Theory Datasets=====#####
	@GRAPH=Theory_16_25_81_B1k              PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=Theory_16_25_81_B2k              PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=Theory_256_625_B1k               PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=Theory_256_625_B2k               PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=Theory_5_9_16_25_B1k             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=Theory_5_9_16_25_B2k             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=Theory_25_81_256_B1k             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=Theory_25_81_256_B2k             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=Theory_4_5_9_16_25_B1k           PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=Theory_4_5_9_16_25_B2k           PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=Theory_9_16_25_81_B1k            PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=Theory_9_16_25_81_B2k            PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=Theory_3_4_5_9_16_25_B1k         PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=Theory_3_4_5_9_16_25_B2k         PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=Theory_5_9_16_25_81_B1k          PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=Theory_5_9_16_25_81_B2k          PATTERN=CLIQUE3 make test --no-print-directory
#####=====Graph500=====#####
	@GRAPH=SC18_4             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=SC19_4             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=SC20_4             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=SC21_4             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=SC22_4             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=SC23_4             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=SC24_4             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=SC25_4             PATTERN=CLIQUE3 make test --no-print-directory
# #####=====MAWI=====#####
	@GRAPH=MAWI1             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=MAWI2             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=MAWI3             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=MAWI4             PATTERN=CLIQUE3 make test --no-print-directory
# #####=====GenBank Datasets=====#####
	@GRAPH=U1a             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=V2a             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=P1a             PATTERN=CLIQUE3 make test --no-print-directory
	@GRAPH=V1r             PATTERN=CLIQUE3 make test --no-print-directory

test_sc:
	@EXTRA_FLAGS="-DV_NR_DPUS=640" make test --no-print-directory
	@EXTRA_FLAGS="-DV_NR_DPUS=1280" make test --no-print-directory
	@EXTRA_FLAGS="-DV_NR_DPUS=2560" make test --no-print-directory
	@EXTRA_FLAGS="-DV_NR_DPUS=5120" make test --no-print-directory
	@EXTRA_FLAGS="-DV_NR_DPUS=10240" make test --no-print-directory
	@EXTRA_FLAGS="-DV_NR_DPUS=20480" make test --no-print-directory
	@EXTRA_FLAGS="-DV_NR_DPUS=40960" make test --no-print-directory

