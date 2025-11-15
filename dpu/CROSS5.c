#include <dpu_mine.h>


static ans_t __imp_cross5(sysname_t tasklet_id, node_t root) {
    edge_ptr root_begin = row_ptr[root];  // intended DMA
    edge_ptr root_end = row_ptr[root + 1];  // intended DMA
    ans_t ans = 0;
    node_t common_size = root_end - root_begin;
    if (common_size >= 4)
        ans = (ans_t)common_size * (common_size - 1) * (common_size - 2) * (common_size - 3) / 24;
    return ans;
}

extern void cross5(sysname_t tasklet_id) {
    static ans_t partial_ans[NR_TASKLETS];
    static uint64_t partial_cycle[NR_TASKLETS];
    static perfcounter_cycles cycles[NR_TASKLETS];
    
    node_t i = 0;

    for (i += tasklet_id; i < root_num; i += NR_TASKLETS) {
        node_t root = roots[i];  // intended DMA
#ifdef PERF
        timer_start(&cycles[tasklet_id]);
#endif
        ans[i] = __imp_cross5(tasklet_id, root);  // intended DMA
#ifdef PERF
        cycle_ct[i] = timer_stop(&cycles[tasklet_id]);  // intended DMA
#endif
    }
}
