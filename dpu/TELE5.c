#include <dpu_mine.h>

static ans_t __imp_tele5_2(sysname_t tasklet_id, node_t root, node_t second_root) {
    node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];

    edge_ptr root_begin = row_ptr[root];  // intended DMA
    edge_ptr root_end = row_ptr[root + 1];  // intended DMA  TODO: opt
    edge_ptr second_root_begin = row_ptr[second_root];  // intended DMA
    edge_ptr second_root_end = row_ptr[second_root + 1];  // intended DMA
    ans_t ans = 0;
    node_t common_size = intersect_seq_buf_thresh_withcbuf(tasklet_buf, &col_idx[root_begin], root_end - root_begin, &col_idx[second_root_begin], second_root_end - second_root_begin, mram_buf[tasklet_id], second_root);
    for (node_t j = 0; j < common_size; j++) {
        node_t third_root = mram_buf[tasklet_id][j];  // intended DMA
        edge_ptr third_root_begin = row_ptr[third_root];  // intended DMA
        edge_ptr third_root_end = row_ptr[third_root + 1];  // intended DMA
        node_t common_size2 = intersect_seq_buf_thresh_withcbuf(tasklet_buf, mram_buf[tasklet_id], common_size, &col_idx[third_root_begin], third_root_end - third_root_begin, mram_buf[tasklet_id + NR_TASKLETS], third_root);
        ans += common_size2;
    }
    return ans;
}

static ans_t __imp_tele5(sysname_t tasklet_id, node_t root) {
    edge_ptr root_begin = row_ptr[root];  // intended DMA
    edge_ptr root_end = row_ptr[root + 1];  // intended DMA
    ans_t ans = 0;
    for (edge_ptr i = root_begin; i < root_end; i++) {
        node_t second_root = col_idx[i];  // intended DMA
        if (second_root >= root) break;
        ans += __imp_tele5_2(tasklet_id, root, second_root);
    }
    return ans;
}

extern void tele5(sysname_t tasklet_id) {
    static ans_t partial_ans[NR_TASKLETS];
    static uint64_t partial_cycle[NR_TASKLETS];
    static perfcounter_cycles cycles[NR_TASKLETS];
    
    node_t i = 0;
    while (i < root_num) {
        node_t root = roots[i];  // intended DMA
        node_t root_begin = row_ptr[root];  // intended DMA
        node_t root_end = row_ptr[root + 1];  // intended DMA
        if (root_end - root_begin < BRANCH_LEVEL_THRESHOLD) {
            break;
        }
#ifdef PERF
        timer_start(&cycles[tasklet_id]);
#endif
        barrier_wait(&co_barrier);
        partial_ans[tasklet_id] = 0;
        for (edge_ptr j = root_begin + tasklet_id; j < root_end; j += NR_TASKLETS) {
            node_t second_root = col_idx[j];  // intended DMA
            if (second_root >= root) break;
            partial_ans[tasklet_id] += __imp_tele5_2(tasklet_id, root, second_root);
        }
#ifdef PERF
        partial_cycle[tasklet_id] = timer_stop(&cycles[tasklet_id]);
#endif
        barrier_wait(&co_barrier);
        if (tasklet_id == 0) {
            ans_t total_ans = 0;
#ifdef PERF
            uint64_t total_cycle = 0;
#endif
            for (uint32_t j = 0; j < NR_TASKLETS; j++) {
                total_ans += partial_ans[j];
#ifdef PERF
                total_cycle += partial_cycle[j];
#endif
            }
            ans[i] = total_ans;  // intended DMA
#ifdef PERF
            cycle_ct[i] = total_cycle;  // intended DMA
#endif
        }
        i++;
    }

    for (i += tasklet_id; i < root_num; i += NR_TASKLETS) {
        node_t root = roots[i];  // intended DMA
#ifdef PERF
        timer_start(&cycles[tasklet_id]);
#endif
        ans[i] = __imp_tele5(tasklet_id, root);  // intended DMA
#ifdef PERF
        cycle_ct[i] = timer_stop(&cycles[tasklet_id]);  // intended DMA
#endif
    }
}
