#include <common.h>
#include <timer.h>
#include <assert.h>
#include <stdio.h>
#include <dpu.h>

extern void data_transfer(struct dpu_set_t set, bitmap_t bitmap, int base);
extern void prepare_graph();
extern void data_bm_transfer(struct dpu_set_t set, int base);
extern ans_t clique2(Graph *g, node_t root);
extern ans_t KERNEL_FUNC(Graph *g, node_t root);
extern Graph *global_g;
extern bitmap_t bitmap;
Graph *g = NULL;
ans_t ans[N] = {0};
ans_t result[N] = {0};
Timer timer = {0};
uint64_t cycle_ct[N] = {0};
uint64_t cycle_ct_dpu[EF_NR_DPUS][NR_TASKLETS] = {0};
node_t large_degree_num[EF_NR_DPUS] = {0};

// prepare DPU
ans_t total_ans = 0;
#ifdef PERF
    uint64_t total_cycle_ct = 0;
#endif
uint32_t BM_DPUS;
node_t BM_NUMS;

static void collect_dpu_batch(struct dpu_set_t set, int base, int current_batch_size);
static void report_and_output_results();

int main() {
    printf("NR_DPUS: %u, NR_TASKLETS: %u, DPU_BINARY: %s, PATTERN: %s\n", NR_DPUS, NR_TASKLETS, DPU_BINARY, PATTERN_NAME);

    // Task allocation and graph partitioning
    printf("Selecting graph: %s\n", DATA_PATH);
    start(&timer, 0, 0);
    g = malloc(sizeof(Graph));
    global_g = g;
    prepare_graph();
    stop(&timer, 0);
    printf("Data prepare ");
    print(&timer, 0, 1);

int batch_count = 1;
int base = 0;
int current_batch_size = NR_DPUS; // Number of DPUs to process in one batch
#ifdef V_NR_DPUS
batch_count = (V_NR_DPUS + NR_DPUS - 1) / NR_DPUS; // Round up
#endif

// Allocate DPU set
struct dpu_set_t set;
int prev_batch_size = -1;
bool set_valid = false;

// DPU batch processing loop
for (int index = 0; index < batch_count; index++) {
    HERE_OKF(" batch index %d begin...", index);
#ifdef V_NR_DPUS
    base = index * NR_DPUS;
    current_batch_size = ((base + NR_DPUS) <= V_NR_DPUS) ? NR_DPUS : (V_NR_DPUS - base);
#endif

    uint32_t bm_start = base;
    uint32_t bm_end = base + current_batch_size - 1;
    // Check if this batch is fully within the BM_DPUS region
    if (bm_end < BM_DPUS) {
        //// Fully inside BM region
        if (current_batch_size != prev_batch_size) {
            if (set_valid) DPU_ASSERT(dpu_free(set));
            DPU_ASSERT(dpu_alloc(current_batch_size, NULL, &set));
            set_valid = true;
            prev_batch_size = current_batch_size;
        }
        data_bm_transfer(set, base);
        start(&timer, 0, index);
        DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
        stop(&timer, 0);
        collect_dpu_batch(set, base, current_batch_size);
        
    }
    // Fully inside normal region
    else if (bm_start >= BM_DPUS) {
        if (current_batch_size != prev_batch_size) {
            if (set_valid) DPU_ASSERT(dpu_free(set));
            DPU_ASSERT(dpu_alloc(current_batch_size, NULL, &set));
            set_valid = true;
            prev_batch_size = current_batch_size;
        }
        data_transfer(set, bitmap, base);
        start(&timer, 0, index);
        DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
        stop(&timer, 0);
        collect_dpu_batch(set, base, current_batch_size);
    }
    // Cross-region case: split into two parts
    else {
        int bm_part = BM_DPUS - base;
        int normal_part = current_batch_size - bm_part;

        if (set_valid) {
            DPU_ASSERT(dpu_free(set));
        }
        // BM region part
        struct dpu_set_t set_bm;
        DPU_ASSERT(dpu_alloc(bm_part, NULL, &set_bm));
        data_transfer(set_bm, bitmap, base);
        DPU_ASSERT(dpu_launch(set_bm, DPU_ASYNCHRONOUS)); // Launch asynchronously

        // Normal region part
        struct dpu_set_t set_normal;
        DPU_ASSERT(dpu_alloc(normal_part, NULL, &set_normal));
        data_transfer(set_normal, bitmap, base + bm_part);
        DPU_ASSERT(dpu_launch(set_normal, DPU_ASYNCHRONOUS)); // Launch asynchronously

        // Synchronize and collect
        DPU_ASSERT(dpu_sync(set_bm));
        collect_dpu_batch(set_bm, base, bm_part);
        DPU_ASSERT(dpu_free(set_bm));

        DPU_ASSERT(dpu_sync(set_normal));
        collect_dpu_batch(set_normal, base + bm_part, normal_part);
        DPU_ASSERT(dpu_free(set_normal));
        
        prev_batch_size = -1;
        set_valid = false;

    }
}
printf("DATA-NAME:"DATA_NAME"\n");
printf("DPU ");
print(&timer, 0, 1);


if (set_valid) {
    DPU_ASSERT(dpu_free(set));
}
    report_and_output_results();    
    
    assert(bitmap != NULL);
    free(bitmap);
    free(g);
    return 0;
}




//=================================//
static void collect_dpu_batch(struct dpu_set_t set, int base, int current_batch_size) {
    bool fine = true;
    bool finished, failed;
    struct dpu_set_t dpu;
    int each_dpu;

    DPU_FOREACH(set, dpu, each_dpu) {
        if (each_dpu >= current_batch_size)
            break;

        DPU_ASSERT(dpu_status(dpu, &finished, &failed));
        if (failed) {
            printf("DPU: %u failed\n", each_dpu);
            fine = false;
            break;
        }

        // ====== Collect answers ======
        int idx = each_dpu + base;
        uint32_t root_cnt = g->root_num[idx];

        uint64_t *dpu_ans = (uint64_t *)malloc(ALIGN8(root_cnt * sizeof(uint64_t)));
        DPU_ASSERT(dpu_copy_from(dpu, "ans", 0, dpu_ans, ALIGN8(root_cnt * sizeof(uint64_t))));
        for (node_t k = 0; k < root_cnt; k++) {
            node_t cur_root = g->roots[idx][k];
            result[cur_root] = dpu_ans[k];
            total_ans += dpu_ans[k];
        }
        free(dpu_ans);
        

#ifdef PERF
        // ====== Collect performance data ======
        uint64_t *dpu_cycle_ct = (uint64_t *)malloc(ALIGN8(root_cnt * sizeof(uint64_t)));
        DPU_ASSERT(dpu_copy_from(dpu, "cycle_ct", 0, dpu_cycle_ct, ALIGN8(root_cnt * sizeof(uint64_t))));
        DPU_ASSERT(dpu_copy_from(dpu, "large_degree_num", 0, large_degree_num[idx], sizeof(node_t)));

        for (node_t k = 0, cur_thread = 0; k < root_cnt; k++) {
            node_t cur_root = g->roots[idx][k];
            cycle_ct[cur_root] = dpu_cycle_ct[k];
            if (g->row_ptr[cur_root + 1] - g->row_ptr[cur_root] >= BRANCH_LEVEL_THRESHOLD) {
                for (uint32_t i = 0; i < NR_TASKLETS; i++) {
                    cycle_ct_dpu[idx][i] += dpu_cycle_ct[k] / NR_TASKLETS;
                }
            } else {
                cycle_ct_dpu[idx][cur_thread] += dpu_cycle_ct[k];
                cur_thread = (cur_thread + 1) % NR_TASKLETS;
            }
            total_cycle_ct += dpu_cycle_ct[k];
        }
        free(dpu_cycle_ct);
#endif
    }

    if (!fine) {
        printf(ANSI_COLOR_RED "Some failed\n" ANSI_COLOR_RESET);
    }
}

static void report_and_output_results() {
    printf("DPU ans: %lu\n", total_ans);
#ifdef PERF
    printf("Lower bound: %f\n", (double)total_cycle_ct / NR_DPUS / NR_TASKLETS / 350000);
#endif

#ifdef V_NR_DPUS
    printf(ANSI_COLOR_GREEN "[INFO] Finished in VIRTUAL DPU mode (V_NR_DPUS = %d)\n" ANSI_COLOR_RESET, V_NR_DPUS);
#else
    printf(ANSI_COLOR_GREEN "[INFO] Finished in PHYSICAL DPU mode (NR_DPUS = %d)\n" ANSI_COLOR_RESET, NR_DPUS);
#endif

#ifdef PERF
#ifdef NO_RUN
    FILE *fp = fopen("./result/" PATTERN_NAME "_" DATA_NAME "_NO_RUN.txt", "w");
#else
    FILE *fp = fopen("./result/" PATTERN_NAME "_" DATA_NAME ".txt", "w");
#endif

    if (fp == NULL) {
        perror("fopen");
        return;
    }

    fprintf(fp, "NR_DPUS: %u, NR_TASKLETS: %u, DPU_BINARY: %s, PATTERN: %s\n",
            NR_DPUS, NR_TASKLETS, DPU_BINARY, PATTERN_NAME);
    fprintf(fp, "N: %u, M: %u, avg_deg: %f\n", g->n, g->m, (double)g->m / g->n);

     uint64_t total_dpu_cycle = 0;
     uint64_t max_dpu_cycle = 0;
    for (uint32_t i = 0; i < EF_NR_DPUS; i++) {
#ifndef STRONG_SCALABILITY
        if(i%2560 == 0 ){
            total_dpu_cycle+=max_dpu_cycle;
            max_dpu_cycle=0;
        }
#endif
        for (uint32_t j = 0; j < NR_TASKLETS; j++) {
            fprintf(fp, "DPU: %u, tasklet: %u, cycle: %lu, root_num: %lu\n",
                    i, j, cycle_ct_dpu[i][j], g->root_num[i]);
            max_dpu_cycle = MAX(cycle_ct_dpu[i][j], max_dpu_cycle);
        }
    }
    total_dpu_cycle += max_dpu_cycle;
    printf("DPU CYCLE TIME : %.2f ms\n", total_dpu_cycle * 2.85e-6);

    for (uint32_t i = 0; i < EF_NR_DPUS; i++) {
        float ratio = (float)large_degree_num[i] / g->root_num[i];
        fprintf(fp, "DPU: %u, large_degree_num: %u, root_num: %lu, ratio: %.2f\n",
                i, large_degree_num[i], g->root_num[i], ratio);
    }

    fclose(fp);
#endif
}
