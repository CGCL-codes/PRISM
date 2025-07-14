#include <common.h>
#include <cyclecount.h>
#include <string.h>
#include <mram.h>
#include <defs.h>
#include <assert.h>
#include <barrier.h>

// bitmap data
__dma_aligned __mram_noinit uint64_t bitmap[BITMAP_ROW][BITMAP_COL];   // 2M  
__host uint64_t bitmap_size; // ALIGN8

__dma_aligned __mram_noinit edge_ptr row_ptr[BITMAP_ROW + 64];   // 256K
__dma_aligned __mram_noinit node_t col_idx[BITMAP_ROW << 9];    // 16M
__host uint64_t root_num;
__mram_noinit node_t roots[DPU_ROOT_NUM];   // 1M
__mram_noinit uint64_t ans[DPU_ROOT_NUM];   // 2M
__mram_noinit uint64_t cycle_ct[DPU_ROOT_NUM];   // 2M
__host edge_ptr edge_offset;
__host uint32_t no_partition_flag;

// buffer
__dma_aligned uint64_t buf[NR_TASKLETS][3][BUF_SIZE];  // 12K
__dma_aligned edge_ptr col_buf[NR_TASKLETS][BUF_SIZE];  // 2K

//__mram_noinit uint64_t mram_buf[NR_TASKLETS << 2][BITMAP_COL];  //256K
__host node_t large_degree_num;  //Number of nodes(degree>=16)
// synchronization
BARRIER_INIT(co_barrier, NR_TASKLETS);

// intersection
extern node_t intersect_bitmap(uint64_t (*buf)[BUF_SIZE],node_t __mram_ptr *a, node_t __mram_ptr *b,uint64_t cmp_size);

