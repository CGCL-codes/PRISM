#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdio.h>  // for debug

#define PERF
// #define DPU_LOG
// #define CPU_RUN
// #define WRAM_ASYNC
// #define STRONG_SCALABILITY
#define NO_PARTITION_AS_POSSIBLE
// #define MORE_ACCURATE_MODEL
#if defined(CLIQUE4) || defined(CLIQUE5) 
#define BITMAP
#endif

#define DATA_DIR "./data/"
#if defined(SELF)
#define DATA_NAME "self-defined"
#define N (1<<5)
#define M (1<<5)
#elif defined(WV)
#define DATA_NAME "Wiki-Vote"
#define N (1<<13)
#define M (1<<18)
#elif defined(CA)
#define DATA_NAME "ca-AstroPh"
#define N (1<<16)
#define M (1<<20)
#elif defined(CAGQ)
#define DATA_NAME "ca-GrQc_adj"
#define N (1<<16)
#define M (1<<20)
#elif defined(CAHT)
#define DATA_NAME "ca-HepTh_adj"
#define N (1<<16)
#define M (1<<20)
#elif defined(YT)
#define DATA_NAME "com-youtube"
#define N (1<<21)
#define M (1<<23)
#elif defined(PT)
#define DATA_NAME "cit-Patents"
#define N (1<<22)
#define M (1<<26)
#elif defined(LJ)
#define DATA_NAME "soc-LiveJournal1"
#define N (1<<23)
#define M (1<<27)
#elif defined(ORKUT)
#define DATA_NAME "com-orkut"
#define N (1<<23)
#define M (1<<28)
#elif defined(PA)
#define DATA_NAME "roadNet-PA_adj"
#define N (1<<23)
#define M (1<<26)
#elif defined(CH)
#define DATA_NAME "cit-HepPh_adj"
#define N (1<<18)
#define M (1<<23)
#elif defined(SS)
#define DATA_NAME "soc-Slashdot0811_adj"
#define N (1<<18)
#define M (1<<23)
#elif defined(SP)
#define DATA_NAME "simple_csr_graph"
#define N (1<<6)
#define M (1<<8)
#elif defined(FE)
#define DATA_NAME "flickrEdges_adj"
#define N (1<<17)
#define M (1<<23)
#elif defined(TW20)
#define DATA_NAME "twitter20new"
#define N (1<<25)
#define M (1<<29)
#elif defined(NCA)
#define DATA_NAME "roadNet-CA_adj"
#define N (1<<23)
#define M (1<<25)
#elif defined(NTX)
#define DATA_NAME "roadNet-TX_adj"
#define N (1<<23)
#define M (1<<25)
#elif defined(AM0302)
#define DATA_NAME "amazon0302_adj"
#define N (1<<20)
#define M (1<<23)
#elif defined(AM0312)
#define DATA_NAME "amazon0312_adj"
#define N (1<<20)
#define M (1<<23)
#elif defined(AM0505)
#define DATA_NAME "amazon0505_adj"
#define N (1<<20)
#define M (1<<23)
#elif defined(AM0601)
#define DATA_NAME "amazon0601_adj"
#define N (1<<20)
#define M (1<<23)
#elif defined(SC18_4)
#define DATA_NAME "scale18-ef4"
#define N (1<<21)
#define M (1<<25)
#elif defined(SC19_4)
#define DATA_NAME "scale19-ef4"
#define N (1<<21)
#define M (1<<25)
#elif defined(SC20_4)
#define DATA_NAME "scale20-ef4"
#define N (1<<21)
#define M (1<<25)
#elif defined(SC21_4)
#define DATA_NAME "scale21-ef4"
#define N (1<<22)
#define M (1<<25)
#elif defined(SC22_4)
#define DATA_NAME "scale22-ef4"
#define N (1<<23)
#define M (1<<26)
#elif defined(SC23_4)
#define DATA_NAME "scale23-ef4"
#define N (1<<24)
#define M (1<<26)
#elif defined(SC24_4)
#define DATA_NAME "scale24-ef4"
#define N (1<<25)
#define M (1<<27)
#elif defined(SC25_4)
#define DATA_NAME "scale25-ef4"
#define N (33556480)
#define M (1<<29)
#elif defined(P2P04)
#define DATA_NAME "p2p-Gnutella04"
#define N (1<<14)
#define M (1<<19)
#elif defined(P2P31)
#define DATA_NAME "p2p-Gnutella31"
#define N (1<<17)
#define M (1<<20)
//============
#elif defined(Theory_16_25_81_B1k)
#define DATA_NAME "Theory-16-25-81-B1k"
#define N (1<<16)
#define M (1<<19)
#elif defined(Theory_16_25_81_B2k)
#define DATA_NAME "Theory-16-25-81-B2k"
#define N (1<<16)
#define M (1<<19)
#elif defined(Theory_256_625_B1k)
#define DATA_NAME "Theory-256-625-B1k"
#define N (1<<18)
#define M (1<<20)
#elif defined(Theory_256_625_B2k)
#define DATA_NAME "Theory-256-625-B2k"
#define N (1<<18)
#define M (1<<20)
#elif defined(Theory_25_81_B1k)
#define DATA_NAME "Theory-25-81-B1k"
#define N (1<<12)
#define M (1<<14)
#elif defined(Theory_25_81_B2k)
#define DATA_NAME "Theory-25-81-B2k"
#define N (1<<12)
#define M (1<<14)
#elif defined(Theory_5_9_16_25_B1k)
#define DATA_NAME "Theory-5-9-16-25-B1k"
#define N (1<<15)
#define M (1<<19)
#elif defined(Theory_5_9_16_25_B2k)
#define DATA_NAME "Theory-5-9-16-25-B2k"
#define N (1<<15)
#define M (1<<19)
#elif defined(Theory_25_81_256_B1k)
#define DATA_NAME "Theory-25-81-256-B1k"
#define N (1<<20)
#define M (1<<24)
#elif defined(Theory_25_81_256_B2k)
#define DATA_NAME "Theory-25-81-256-B2k"
#define N (1<<20)
#define M (1<<24)
#elif defined(Theory_4_5_9_16_25_B1k)
#define DATA_NAME "Theory-4-5-9-16-25-B1k"
#define N (1<<20)
#define M (1<<24)
#elif defined(Theory_4_5_9_16_25_B2k)
#define DATA_NAME "Theory-4-5-9-16-25-B2k"
#define N (1<<20)
#define M (1<<24)
#elif defined(Theory_9_16_25_81_B1k)
#define DATA_NAME "Theory-9-16-25-81-B1k"
#define N (1<<20)
#define M (1<<24)
#elif defined(Theory_9_16_25_81_B2k)
#define DATA_NAME "Theory-9-16-25-81-B2k"
#define N (1<<20)
#define M (1<<24)
#elif defined(Theory_3_4_5_9_16_25_B1k)
#define DATA_NAME "Theory-3-4-5-9-16-25-B1k"
#define N (1<<23)
#define M (1<<26)
#elif defined(Theory_3_4_5_9_16_25_B2k)
#define DATA_NAME "Theory-3-4-5-9-16-25-B2k"
#define N (1<<23)
#define M (1<<26)
#elif defined(Theory_5_9_16_25_81_B1k)
#define DATA_NAME "Theory-5-9-16-25-81-B1k"
#define N (1<<23)
#define M (1<<26)
#elif defined(Theory_5_9_16_25_81_B2k)
#define DATA_NAME "Theory-5-9-16-25-81-B2k"
#define N (1<<23)
#define M (1<<26)
#elif defined(Theory_81_256_B1k)
#define DATA_NAME "Theory-81-256-B1k"
#define N (1<<20)
#define M (1<<20)
#elif defined(Theory_81_256_B2k)
#define DATA_NAME "Theory-81-256-B2k"
#define N (1<<20)
#define M (1<<20)
#elif defined(MAWI1)
#define DATA_NAME "201512012345"
#define N (1<<25)
#define M (1<<27)
#elif defined(MAWI2)
#define DATA_NAME "201512020000"
#define N (1<<26)
#define M (1<<27)
#elif defined(MAWI3)
#define DATA_NAME "201512020030"
#define N (1<<27)
#define M (1<<28)
#elif defined(MAWI4)
#define DATA_NAME "201512020130"
#define N (1<<27)
#define M (1<<29)
#elif defined(U1a)
#define DATA_NAME "U1a"
#define N (68863500)
#define M (143415000)
#elif defined(V2a)
#define DATA_NAME "V2a"
#define N (55046400)
#define M (117264000)
#elif defined(P1a)
#define DATA_NAME "P1a"
#define N (139364352)
#define M (297864000)
#elif defined(V1r)
#define DATA_NAME "V1r"
#define N (214005760)
#define M (465412096)
#else
#warning "No graph selected, fall back to PP."
#define DATA_NAME "p2p-Gnutella04"
#define N (1<<14)
#define M (1<<17)
#endif
#define DATA_PATH DATA_DIR DATA_NAME ".bin"

#ifndef NR_DPUS
#warning "No NR_DPUS defined, fall back to 1."
#define NR_DPUS 1
#endif
#ifndef NR_TASKLETS
#warning "No NR_TASKLETS defined, fall back to 1."
#define NR_TASKLETS 1
#endif
#ifndef DPU_BINARY
#warning "No DPU_BINARY defined, fall back to bin/dpu."
#define DPU_BINARY "bin/dpu"
#endif
#ifndef DPU_ALLOC_BINARY
#warning "No DPU_ALLOC_BINARY defined, fall back to bin/dpu_alloc."
#define DPU_ALLOC_BINARY "bin/dpu_alloc"
#endif
#ifndef DPU_BM_BINARY
#warning "No DDPU_BM_BINARY defined, fall back to bin/dpu_bm."
#define DPU_BINARY "bin/dpu_bm"
#endif

#if defined(CLIQUE2)
#define KERNEL_FUNC clique2
#define PATTERN_NAME "clique2"
#elif defined(CLIQUE3)
#define PRUNING_OP
#define KERNEL_FUNC clique3
#define KERNEL_FUNC_BM clique3_bm
#define PATTERN_NAME "clique3"
#elif defined(CLIQUE4)
#define KERNEL_FUNC clique4
#define PATTERN_NAME "clique4"
#elif defined(CLIQUE5)
#define KERNEL_FUNC clique5
#define PATTERN_NAME "clique5"
#elif defined(CYCLE4)
#define KERNEL_FUNC cycle4
#define PATTERN_NAME "cycle4"
#elif defined(HOUSE5)
#define KERNEL_FUNC house5
#define PATTERN_NAME "house5"
#elif defined(TRI_TRI6)
#define KERNEL_FUNC tri_tri6
#define PATTERN_NAME "tri_tri6"
#elif defined(HOOF4)
#define KERNEL_FUNC hoof4
#define PATTERN_NAME "hoof4"
#elif defined(TELE5)
#define KERNEL_FUNC tele5
#define PATTERN_NAME "tele5"
#else
#warning "No kernel function selected, fall back to clique2."
#define KERNEL_FUNC clique2
#define PATTERN_NAME "clique2"
#endif

#define node_t uint32_t
#define edge_ptr uint32_t
#define ans_t uint64_t
#define SIZE_NODE_T_LOG 2
#define SIZE_EDGE_PTR_LOG 2
#define SIZE_BITMAP_LOG 3
#define INVALID_NODE ((node_t)(-1))
#define DPU_N ((1<<24)/sizeof(edge_ptr))
#define DPU_M ((1<<25)/sizeof(node_t))
#define DPU_ROOT_NUM ((1<<20)/sizeof(node_t))
#define BITMAP_SIZE 32  // 1024 bits
#define BUF_SIZE  32
#define MRAM_BUF_SIZE 32768
#define BRANCH_LEVEL_THRESHOLD 16
#define PARTITION_M ((1<<22)/sizeof(node_t))

#define BITMAP_ROW 8192
#define BITMAP_COL 128

#define V_NR_DPUS 2560
#ifdef V_NR_DPUS
#define EF_NR_DPUS V_NR_DPUS
#else
#define EF_NR_DPUS NR_DPUS
#endif


typedef struct Graph {
    node_t n;  // number of vertices
    edge_ptr m;  // number of edges
    edge_ptr row_ptr[N];
    node_t col_idx[M];
    uint64_t root_num[EF_NR_DPUS];  // number of search roots allocated to dpu
    node_t *roots[EF_NR_DPUS];
} Graph;
typedef uint32_t(*bitmap_t)[N >> 5];

typedef struct ElementType {
    uint32_t dpu_id;
    double workload;
} ElementType;

typedef struct {
    ElementType *elements;
    uint32_t size;
    uint32_t capacity;
} Heap;


#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#define ALIGN2(x) ALIGN(x, 2)
#define ALIGN4(x) ALIGN(x, 4)
#define ALIGN8(x) ALIGN(x, 8)
#define ALIGN16(x) ALIGN(x, 16)
#define ALIGN_LOWER(x, a) ((x) & ~((a)-1))
#define ALIGN2_LOWER(x) ALIGN_LOWER(x, 2)
#define ALIGN4_LOWER(x) ALIGN_LOWER(x, 4)
#define ALIGN8_LOWER(x) ALIGN_LOWER(x, 8)
#define ALIGN16_LOWER(x) ALIGN_LOWER(x, 16)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define HERE_OK() printf(ANSI_COLOR_GREEN "[OK] %s:%d\n" ANSI_COLOR_RESET , __FILE__, __LINE__); //DEBUG
#define HERE_OKF(fmt, ...) \
    printf(ANSI_COLOR_GREEN "[OK] %s:%d - " fmt "\n" ANSI_COLOR_RESET, __FILE__, __LINE__, ##__VA_ARGS__);

#endif // COMMON_H

