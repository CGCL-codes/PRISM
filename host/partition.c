#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <dpu.h>
#include <dpu_types.h>

Graph *global_g=NULL;
Heap * heap;
bitmap_t bitmap = {0};
double workload[N]={0};
node_t eff_num[N]={0};
edge_ptr offset = 0;
uint32_t no_partition_flag = 1; //true

extern uint32_t BM_DPUS;
extern node_t BM_NUMS;
uint64_t op_bitmap[BITMAP_ROW][BITMAP_COL]={0};  //bitmap transfer


static int deg_cmp(const void *a, const void *b) {
    node_t x = *(node_t *)a;
    node_t y = *(node_t *)b;
    return global_g->row_ptr[y + 1] - global_g->row_ptr[y] - (global_g->row_ptr[x + 1] - global_g->row_ptr[x]);
}
static int node_t_cmp(const void *a, const void *b) {
    node_t x = *(node_t *)a;
    node_t y = *(node_t *)b;
    return x - y;
}
static int workload_cmp(const void *a, const void *b) {
    node_t x = *(node_t *)a;
    node_t y = *(node_t *)b;
    return workload[y] - workload[x];
}

#ifdef MORE_ACCURATE_MODEL
static inline double predict_workload(Graph *g, node_t root) {
    double deg = g->row_ptr[root + 1] - g->row_ptr[root];
    edge_ptr l = g->row_ptr[root], r = g->row_ptr[root + 1];
    while (l < r) {
        edge_ptr mid = (l + r) >> 1;
        if (g->col_idx[mid] < root) {
            l = mid + 1;
        }
        else {
            r = mid;
        }
    }
    double eff_deg = eff_deg = l - g->row_ptr[root];
#ifndef PRUNING_OP
    if (deg > MRAM_BUF_SIZE) {
        printf(ANSI_COLOR_RED "Error: deg too large\n" ANSI_COLOR_RESET);
        exit(1);
    }
    if (eff_deg > BITMAP_SIZE * 32) {
        printf(ANSI_COLOR_RED "Error: eff_deg too large\n" ANSI_COLOR_RESET);
        exit(1);
    }
#endif
    double avg_deg = 0;
    for (edge_ptr i = g->row_ptr[root]; i < g->row_ptr[root + 1]; i++) {
        node_t neighbor = g->col_idx[i];
        avg_deg += g->row_ptr[neighbor + 1] - g->row_ptr[neighbor];
    }
    avg_deg /= g->row_ptr[root + 1] - g->row_ptr[root];
    double n = global_g->n;
    (void)deg;
    (void)eff_deg;
    (void)avg_deg;
    (void)n;
#if defined(CLIQUE2)
    return eff_deg;
#elif defined(CLIQUE3)
    return eff_deg * eff_deg * avg_deg + 100;
#elif defined(CLIQUE4)
    return eff_deg * eff_deg * eff_deg * avg_deg * avg_deg * avg_deg + 100;
#elif defined(CLIQUE5)
    return eff_deg * eff_deg * eff_deg * eff_deg * avg_deg * avg_deg * avg_deg * avg_deg * avg_deg * avg_deg + 100;
#elif defined(CYCLE4)
    return eff_deg * eff_deg * avg_deg + 100;
#elif defined(HOUSE5)
    return eff_deg * deg * avg_deg * (2 + deg / n + avg_deg / n) + 100;
#elif defined(TRI_TRI6)
    return eff_deg * eff_deg * avg_deg * (deg + 3 * avg_deg + (deg + avg_deg) * avg_deg / n) + 100;
#elif defined(TELE5)
    return eff_deg * eff_deg * eff_deg * avg_deg * avg_deg * avg_deg + 100;
#endif
}
#else
static inline double predict_workload(Graph *g, node_t root) {
    double deg = g->row_ptr[root + 1] - g->row_ptr[root];
    edge_ptr l = g->row_ptr[root], r = g->row_ptr[root + 1];
    while (l < r) {
        edge_ptr mid = (l + r) >> 1;
        if (g->col_idx[mid] < root) {
            l = mid + 1;
        }
        else {
            r = mid;
        }
    }
    double eff_deg = l - g->row_ptr[root];
    eff_num[root]=eff_deg;
#ifndef PRUNING_OP
    if (deg > MRAM_BUF_SIZE) {
        printf(ANSI_COLOR_RED "Error: deg too large\n" ANSI_COLOR_RESET);
        exit(1);
    }
    if (eff_deg > BITMAP_SIZE * 32) {
        printf(ANSI_COLOR_RED "Error: eff_deg too large\n" ANSI_COLOR_RESET);
        exit(1);
    }
#endif
    double avg_deg = (double)global_g->m / global_g->n;
    double n = global_g->n;
    (void)deg;
    (void)eff_deg;
    (void)avg_deg;
    (void)n;
#if defined(CLIQUE2)
    return eff_deg;
#elif defined(CLIQUE3)
    return eff_deg * eff_deg + 100;
#elif defined(CLIQUE4)
    return eff_deg * eff_deg * eff_deg + 100;
#elif defined(CLIQUE5)
    return eff_deg * eff_deg * eff_deg * eff_deg + 100;
#elif defined(CYCLE4)
    return eff_deg * eff_deg + 100;
#elif defined(HOUSE5)
    return eff_deg * deg * (2 + deg / n + avg_deg / n) + 100;
#elif defined(TRI_TRI6)
    return eff_deg * eff_deg * (deg + 3 * avg_deg + (deg + avg_deg) * avg_deg / n) + 100;
#elif defined(HOOF4)
    return eff_deg * eff_deg + 100; 
#elif defined(TELE5)
    return eff_deg * eff_deg * eff_deg + 100;
#endif
}
#endif

static void print_bitmap(uint64_t op_bitmap[BITMAP_ROW][BITMAP_COL], int row_limit, int col_limit_bits) {
    for (int i = 0; i < row_limit; i++) {
        printf("Row %d: ", i);
        for (int j = 0; j < col_limit_bits; j++) {
            int word_idx = j >> 6;
            int bit_idx = j & 63;
            uint64_t word = op_bitmap[i][word_idx];
            int bit = (word >> bit_idx) & 1;
            printf("%d", bit);
        }
        printf("\n");
    }
}

static const uint8_t bitcount_table[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

static inline uint32_t count64_lookup(uint64_t x) {
    return bitcount_table[x & 0xFF] +
           bitcount_table[(x >> 8) & 0xFF] +
           bitcount_table[(x >> 16) & 0xFF] +
           bitcount_table[(x >> 24) & 0xFF] +
           bitcount_table[(x >> 32) & 0xFF] +
           bitcount_table[(x >> 40) & 0xFF] +
           bitcount_table[(x >> 48) & 0xFF] +
           bitcount_table[(x >> 56) & 0xFF];
}

static void verify_bitmap_intersection(uint64_t op_bitmap[BITMAP_ROW][BITMAP_COL], int bm_nums) {
    uint32_t total_bm_ans = 0;

    for (int i = 1; i < bm_nums; i++) {
        uint32_t common = 0;
        for (node_t e = global_g->row_ptr[i]; e < global_g->row_ptr[i+1]; e++) {
            int j = global_g->col_idx[e];
            int bound = j;
            int word_limit = (bound + 63) >> 6;
            for (int w = 0; w < word_limit; w++) {
                uint64_t a = op_bitmap[i][w];
                uint64_t b = op_bitmap[j][w];
                if ((w + 1) * 64 > bound) {
                    int remain = bound - (w * 64);
                    uint64_t mask = ((uint64_t)1 << remain) - 1;
                    a &= mask;
                    b &= mask;
                }
                common += count64_lookup(a & b);
            }
        }
        if (common > 0) {
            printf("i = %d, ans = %u\n", i, common);
        }
        total_bm_ans += common;
    }

    printf("bm total ans = %u\n", total_bm_ans);
}

static void init_op_bitmap(uint64_t op_bitmap[BITMAP_ROW][BITMAP_COL], node_t bm_nums, const Graph*g) {
 
    memset(op_bitmap, 0, sizeof(uint64_t) * BITMAP_ROW * BITMAP_COL);

    for (node_t i = 0; i < bm_nums; i++) {
 
        for (edge_ptr e = g->row_ptr[i]; e < g->row_ptr[i + 1]; e++) {
            node_t neighbor = g->col_idx[e];
            if(neighbor>=BM_NUMS)break;
            op_bitmap[i][neighbor >> 6] |= (1ULL << (neighbor & 63));
        }
    }

}

static node_t count_high_degree_nodes(Graph *g, node_t threshold) {
    node_t l = 0, r = g->n;
    while (l < r) {
        node_t mid = l + ((r - l) >> 1);
        node_t degree = g->row_ptr[mid + 1] - g->row_ptr[mid];
        if (degree > threshold) {
            l = mid + 1;
        } else {
            r = mid;
        }
    }

    node_t count = l;

    count &= ~63;

    // limit to 8192
    if (count > 8192) {
        count = 8192;
    }

    return count;
}

static void init_bm_num(){
    node_t node_count = count_high_degree_nodes(global_g, global_g->n*(0.0046));
    int denom = global_g->n / EF_NR_DPUS;
    if (denom == 0) denom = 1;  // avoid 0
    int predict_dpu_num = (node_count / denom) * 3;
    BM_NUMS = node_count;
    BM_DPUS = predict_dpu_num;
    //BM_NUMS = 0;
    //BM_DPUS = 0;
    BM_DPUS &= ~63;
    if(!BM_DPUS)BM_NUMS=0;
    BM_DPUS = MIN(BM_DPUS,BM_NUMS);
    printf("BM_NUMS = %u\n", BM_NUMS);
    printf("BM_DPUS = %u\n", BM_DPUS);
}

static void read_input() {
    FILE *fin = fopen(DATA_PATH, "rb");
    node_t n;
    edge_ptr m;
    edge_ptr *row_ptr = global_g->row_ptr;
    node_t *col_idx = global_g->col_idx;
if (fread(&n, sizeof(node_t), 1, fin) != 1) {
    perror("fread n failed");
    exit(EXIT_FAILURE);
}
if (fread(&m, sizeof(edge_ptr), 1, fin) != 1) {
    perror("fread m failed");
    exit(EXIT_FAILURE);
}
if (fread(row_ptr, sizeof(edge_ptr), n, fin) != n) {
    perror("fread row_ptr failed");
    exit(EXIT_FAILURE);
}
if (fread(col_idx, sizeof(node_t), m, fin) != m) {
    perror("fread col_idx failed");
    exit(EXIT_FAILURE);
}
    row_ptr[n] = m;
    fclose(fin);
    global_g->n = n;
    global_g->m = m;
}

static void data_renumber() {
    static node_t rank[N];
    static node_t renumbered[N];
    static uint32_t out_deg[N] = {0};
    static uint32_t in_deg[N] = {0};
    static bool valid[N] = {0};
    node_t new_n = 0;

    Graph *tmp_g = malloc(sizeof(Graph));
    memcpy(tmp_g, global_g, sizeof(Graph));


    for (node_t i = 0; i < global_g->n; i++) {
        out_deg[i] = tmp_g->row_ptr[i + 1] - tmp_g->row_ptr[i];
        for (edge_ptr j = tmp_g->row_ptr[i]; j < tmp_g->row_ptr[i + 1]; j++) {
            node_t v = tmp_g->col_idx[j];
            in_deg[v]++;
        }
    }

    for (node_t i = 0; i < global_g->n; i++) {
        if (out_deg[i] + in_deg[i] > 2) {
            valid[i] = true;
            rank[new_n++] = i;
        }
    }

    qsort(rank, new_n, sizeof(node_t), deg_cmp);

    for (node_t i = 0; i < new_n; i++) {
        renumbered[rank[i]] = i;
    }

    edge_ptr cur = 0;
    for (node_t i = 0; i < new_n; i++) {
        node_t node = rank[i];
        global_g->row_ptr[i] = cur;
        for (edge_ptr j = tmp_g->row_ptr[node]; j < tmp_g->row_ptr[node + 1]; j++) {
            node_t neighbor = tmp_g->col_idx[j];
            if (valid[neighbor]) {
                global_g->col_idx[cur++] = renumbered[neighbor];
            }
        }
        qsort(global_g->col_idx + global_g->row_ptr[i], cur - global_g->row_ptr[i], sizeof(node_t), node_t_cmp);
    }
    global_g->row_ptr[new_n] = cur;
    global_g->n = new_n;
    global_g->m = cur;

    free(tmp_g);
}


static inline bool check_in_bitmap(node_t n, uint32_t bitmap[N >> 5]) {
    return bitmap[n >> 5] & (1 << (n & 31));
}


static bool update_alloc_info(uint32_t dpu_id, node_t n, edge_ptr *m_count, bitmap_t bitmap) {
    // check condition
    if (global_g->root_num[dpu_id] == DPU_ROOT_NUM) {
        return false;
    }
    if(global_g->row_ptr[n + 1] - global_g->row_ptr[n]==0) //without edge
    {
        return true;
    }
    edge_ptr dpu_m_count = m_count[dpu_id];
    if (!check_in_bitmap(n, bitmap[dpu_id])) {
        dpu_m_count += global_g->row_ptr[n + 1] - global_g->row_ptr[n];
    }
    for (edge_ptr i = global_g->row_ptr[n]; i < global_g->row_ptr[n + 1]; i++) {
        node_t neighbor = global_g->col_idx[i];
        if (!check_in_bitmap(neighbor, bitmap[dpu_id])) {
            dpu_m_count += global_g->row_ptr[neighbor + 1] - global_g->row_ptr[neighbor];
        }
    }
    if (dpu_m_count > DPU_M) {
        return false;
    }

    // allocate
    global_g->roots[dpu_id][global_g->root_num[dpu_id]++] = n;
    m_count[dpu_id] = dpu_m_count;
    bitmap[dpu_id][n >> 5] |= (1 << (n & 31));
    for (edge_ptr i = global_g->row_ptr[n]; i < global_g->row_ptr[n + 1]; i++) {
        node_t neighbor = global_g->col_idx[i];
        bitmap[dpu_id][neighbor >> 5] |= (1 << (neighbor & 31));
    }
    return true;
}

uint32_t heap_pop(Heap *heap);
void heap_push(Heap *heap, uint32_t dpu_id, double workload);
void heap_init(Heap *heap);
void heap_free(Heap *heap);
Heap *heap_create(uint32_t capacity);
static void cut_edge();

static void data_allocate(bitmap_t bitmap) {
    memset(bitmap, 0, (size_t)(N >> 3) * EF_NR_DPUS);
    for (uint32_t i = 0; i < EF_NR_DPUS; i++) {
        global_g->root_num[i] = 0;
        global_g->roots[i] = malloc(DPU_ROOT_NUM * sizeof(node_t));
    }
    init_bm_num();

    static edge_ptr m_count[EF_NR_DPUS];   // edges put in dpu
    static node_t allocate_rank[N];
    static double dpu_workload[EF_NR_DPUS];
    
    for (node_t i = 0; i < global_g->n; i++) {
        allocate_rank[i] = i;
        workload[i] = predict_workload(global_g, i);
    }
    qsort(allocate_rank, global_g->n, sizeof(node_t), workload_cmp);
    
    //bm
    heap = heap_create(BM_DPUS);
    heap_init(heap);
    for (node_t i = 0; i < global_g->n; i++) {
        node_t node = allocate_rank[i];
        if(node>=BM_NUMS)continue;
        uint32_t cur_dpu = heap_pop(heap);
        global_g->roots[cur_dpu][global_g->root_num[cur_dpu]++] = node;
        dpu_workload[cur_dpu] += workload[node];
        heap_push(heap, cur_dpu, dpu_workload[cur_dpu]);

    }
    heap_free(heap);
    memset(dpu_workload,0,sizeof(dpu_workload));
    //normal
#ifdef PRUNING_OP
    cut_edge();
#endif
    heap = heap_create(EF_NR_DPUS-BM_DPUS);
    heap_init(heap);

    uint32_t full_dpu_ct = 0;
    for (node_t i = 0; i <  global_g->n; i++) {
    node_t node = allocate_rank[i];
    if(node<BM_NUMS)continue;
    while (full_dpu_ct != (EF_NR_DPUS - BM_DPUS)) {
        uint32_t cur_dpu = heap_pop(heap);
        if (update_alloc_info(cur_dpu+BM_DPUS, node, m_count, bitmap)) {
            dpu_workload[cur_dpu] += workload[node];
            heap_push(heap, cur_dpu, dpu_workload[cur_dpu]);
            break;
        } else {
            full_dpu_ct++;
        }
    }

    if (full_dpu_ct == (EF_NR_DPUS - BM_DPUS)) {
        printf(ANSI_COLOR_RED "Error: not enough DPUs\n" ANSI_COLOR_RESET);
        heap_free(heap);
        exit(1);
    }
}
    heap_free(heap);
}


//rCSR format
static void data_compact(struct dpu_set_t set, bitmap_t bitmap,int base) {
    edge_ptr(*dpu_row_ptr)[DPU_N * 2];
    dpu_row_ptr = malloc(NR_DPUS * DPU_N * 2 * sizeof(edge_ptr));
    node_t(*dpu_col_idx)[DPU_M * 2];
    dpu_col_idx = malloc(NR_DPUS * DPU_M * 2 * sizeof(node_t));
    node_t(*dpu_roots)[DPU_ROOT_NUM];
    dpu_roots = malloc(NR_DPUS * DPU_ROOT_NUM * sizeof(node_t));
    static uint64_t processed_row_size[NR_DPUS];
    memset(processed_row_size, 0, NR_DPUS * sizeof(uint64_t));
    static uint64_t processed_col_size[NR_DPUS];
    memset(processed_col_size, 0, NR_DPUS * sizeof(uint64_t));
    static uint64_t tmp_row_size[NR_DPUS];
    static uint64_t tmp_col_size[NR_DPUS];
    
    struct dpu_set_t dpu;
    uint32_t each_dpu;
    DPU_ASSERT(dpu_load(set, DPU_ALLOC_BINARY, NULL));

    uint64_t mode = 0;
    DPU_ASSERT(dpu_broadcast_to(set, "mode", 0, &mode, sizeof(uint64_t), DPU_XFER_DEFAULT));
    DPU_FOREACH(set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, bitmap[each_dpu+base]));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "bitmap", 0, (N >> 5) * sizeof(uint32_t), DPU_XFER_DEFAULT));

    static uint32_t zero[N >> 5];
    memset(zero, 0, sizeof(zero));

    DPU_ASSERT(dpu_broadcast_to(set, "involve_bitmap", 0, zero, sizeof(zero), DPU_XFER_DEFAULT));
    uint64_t start = 0;
    while (start < global_g->n) {
            //HERE_OKF("initialize ok");
        uint64_t size = 0;
        while (start + size < global_g->n && global_g->row_ptr[start + size + 1] - global_g->row_ptr[start] < PARTITION_M && size + 32 < PARTITION_M) {
            size++;
        }
        DPU_ASSERT(dpu_broadcast_to(set, "start", 0, &start, sizeof(uint64_t), DPU_XFER_DEFAULT));
        DPU_ASSERT(dpu_broadcast_to(set, "size", 0, &size, sizeof(uint64_t), DPU_XFER_DEFAULT));
        DPU_ASSERT(dpu_broadcast_to(set, "row_ptr", 0, &global_g->row_ptr[start], ALIGN8((size + 1) * sizeof(edge_ptr)), DPU_XFER_DEFAULT));
        DPU_ASSERT(dpu_broadcast_to(set, "col_idx", 0, &global_g->col_idx[global_g->row_ptr[start]], ALIGN8((global_g->row_ptr[start + size] - global_g->row_ptr[start]) * sizeof(node_t)), DPU_XFER_DEFAULT));
        DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
        start += size;
    }

    mode = 1;
    DPU_ASSERT(dpu_broadcast_to(set, "mode", 0, &mode, sizeof(uint64_t), DPU_XFER_DEFAULT));
    uint64_t n_size = global_g->n;
    DPU_ASSERT(dpu_broadcast_to(set, "size", 0, &n_size, sizeof(uint64_t), DPU_XFER_DEFAULT));
    DPU_FOREACH(set, dpu, each_dpu) {
        uint64_t root_num = global_g->root_num[each_dpu+base];
        DPU_ASSERT(dpu_copy_to(dpu, "root_size", 0, &root_num, sizeof(uint64_t)));
    }
    DPU_FOREACH(set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, global_g->roots[each_dpu+base]));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "roots", 0, DPU_ROOT_NUM * sizeof(node_t), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
    DPU_FOREACH(set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_roots[each_dpu]));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, "roots", 0, DPU_ROOT_NUM * sizeof(node_t), DPU_XFER_DEFAULT));

    mode = 2;
    DPU_ASSERT(dpu_broadcast_to(set, "mode", 0, &mode, sizeof(uint64_t), DPU_XFER_DEFAULT));

    start = 0;
    while (start < global_g->n) {
        uint64_t size = 0;
        while (start + size < global_g->n && global_g->row_ptr[start + size + 1] - global_g->row_ptr[start] < PARTITION_M && size + 32 < PARTITION_M) {
            size++;
        }
        DPU_ASSERT(dpu_broadcast_to(set, "start", 0, &start, sizeof(uint64_t), DPU_XFER_DEFAULT));
        DPU_ASSERT(dpu_broadcast_to(set, "size", 0, &size, sizeof(uint64_t), DPU_XFER_DEFAULT));
        DPU_FOREACH(set, dpu, each_dpu) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, &processed_col_size[each_dpu]));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "processed_offset", 0, sizeof(uint64_t), DPU_XFER_DEFAULT));
        //DPU_ASSERT(dpu_broadcast_to(set, "eff_num", 0, &eff_num[start], ALIGN8((size) * sizeof(edge_ptr)), DPU_XFER_DEFAULT));
        DPU_ASSERT(dpu_broadcast_to(set, "row_ptr", 0, &global_g->row_ptr[start], ALIGN8((size + 1) * sizeof(edge_ptr)), DPU_XFER_DEFAULT));
        DPU_ASSERT(dpu_broadcast_to(set, "col_idx", 0, &global_g->col_idx[global_g->row_ptr[start]], ALIGN8((global_g->row_ptr[start + size] - global_g->row_ptr[start]) * sizeof(node_t)), DPU_XFER_DEFAULT));
        DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
        DPU_FOREACH(set, dpu, each_dpu) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, &tmp_row_size[each_dpu]));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, "processed_row_size", 0, sizeof(uint64_t), DPU_XFER_DEFAULT));
        DPU_FOREACH(set, dpu, each_dpu) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, &tmp_col_size[each_dpu]));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, "processed_col_size", 0, sizeof(uint64_t), DPU_XFER_DEFAULT));
        uint64_t max_row_size = 0;
        uint64_t max_col_size = 0;
        DPU_FOREACH(set, dpu, each_dpu) {
            if (tmp_row_size[each_dpu] > max_row_size) {
                max_row_size = tmp_row_size[each_dpu];
            }
            if (tmp_col_size[each_dpu] > max_col_size) {
                max_col_size = tmp_col_size[each_dpu];
            }
        }
        if (max_row_size != 0) {
            DPU_FOREACH(set, dpu, each_dpu) {
                DPU_ASSERT(dpu_prepare_xfer(dpu, &dpu_row_ptr[each_dpu][processed_row_size[each_dpu]]));
            }
            DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, "processed_row_ptr", 0, ALIGN8(max_row_size * sizeof(edge_ptr)), DPU_XFER_DEFAULT));
        }
        if (max_col_size != 0) {
            DPU_FOREACH(set, dpu, each_dpu) {
                DPU_ASSERT(dpu_prepare_xfer(dpu, &dpu_col_idx[each_dpu][processed_col_size[each_dpu]]));
            }
            DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, "processed_col_idx", 0, ALIGN8(max_col_size * sizeof(node_t)), DPU_XFER_DEFAULT));
        }
        DPU_FOREACH(set, dpu, each_dpu) {
            processed_row_size[each_dpu] += tmp_row_size[each_dpu];
            processed_col_size[each_dpu] += tmp_col_size[each_dpu];
        }
        start += size;
    }
    for (uint32_t i = 0; i < NR_DPUS; i++) {
        dpu_row_ptr[i][processed_row_size[i]] = processed_col_size[i];
    }

    //real data xfer 
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));
    DPU_FOREACH(set, dpu, each_dpu) {
        uint64_t root_num = global_g->root_num[each_dpu+base];
        DPU_ASSERT(dpu_copy_to(dpu, "root_num", 0, &root_num, sizeof(uint64_t)));
    }
    DPU_FOREACH(set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_roots[each_dpu]));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "roots", 0, DPU_ROOT_NUM * sizeof(node_t), DPU_XFER_DEFAULT));
    DPU_FOREACH(set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_row_ptr[each_dpu]));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "row_ptr", 0, DPU_N * sizeof(edge_ptr), DPU_XFER_DEFAULT));
    DPU_FOREACH(set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_col_idx[each_dpu]));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "col_idx", 0, DPU_M * sizeof(node_t), DPU_XFER_DEFAULT));

    free(dpu_row_ptr);
    free(dpu_col_idx);
    free(dpu_roots);

}

static void data_xfer(struct dpu_set_t set,int base) {
       struct dpu_set_t dpu;
        uint32_t each_dpu;

        DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));
        node_t max_root_num = 0;
        DPU_FOREACH(set, dpu, each_dpu) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, &global_g->root_num[each_dpu+base]));
            if(global_g->root_num[each_dpu+base] > max_root_num) {
                max_root_num = global_g->root_num[each_dpu+base];
            }
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "root_num", 0, sizeof(uint64_t), DPU_XFER_DEFAULT));
        DPU_FOREACH(set, dpu, each_dpu) {
            if(global_g->root_num[each_dpu+base])
            DPU_ASSERT(dpu_prepare_xfer(dpu, global_g->roots[each_dpu+base]));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "roots", 0, ALIGN8(max_root_num * sizeof(node_t)), DPU_XFER_DEFAULT));
        DPU_FOREACH(set, dpu, each_dpu) {
            if(global_g->root_num[each_dpu+base])
            DPU_ASSERT(dpu_prepare_xfer(dpu, global_g->row_ptr));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "row_ptr", 0, ALIGN8((global_g->n + 1) * sizeof(edge_ptr)), DPU_XFER_DEFAULT));
        DPU_FOREACH(set, dpu, each_dpu) {
            if(global_g->root_num[each_dpu+base])
            DPU_ASSERT(dpu_prepare_xfer(dpu, global_g->col_idx));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "col_idx", 0, ALIGN8(global_g->m * sizeof(node_t) * 3), DPU_XFER_DEFAULT));

        //re_col
        DPU_ASSERT(dpu_broadcast_to(set, "edge_offset", 0, &offset, sizeof(edge_ptr), DPU_XFER_DEFAULT));

}


static void*alloc_bitmap(uint32_t num_bits, size_t num_dpus) {
    uint64_t words_per_dpu = (uint64_t)num_bits/32;
    uint64_t total_words = words_per_dpu * num_dpus;
    uint64_t total_bytes = total_words * sizeof(uint32_t);


    double total_gib = total_bytes / (1024.0 * 1024 * 1024);

    void *bitmap = malloc((size_t)total_bytes);
    if (!bitmap) {
        printf("❌ bitmap allocate false！%.2f GiB (%lu bytes)\n", total_gib, total_bytes);
        exit(1);
    } else {
        printf("✅ bitmap allocate success：%.2f GiB (%lu bytes)\n", total_gib, total_bytes);
        return bitmap;
    }
}

static void cut_edge()
{
    Graph* g = global_g;
    edge_ptr* new_row_ptr = (edge_ptr*) malloc((g->n + 1) * sizeof(edge_ptr));
    node_t* new_col_idx = (node_t*) malloc(g->m * sizeof(node_t)); 

    if (!new_row_ptr || !new_col_idx) {
        printf("Memory allocation failed!\n");
        exit(1);
    }

    edge_ptr col_offset = 0;

    for (node_t i = 0; i < g->n; i++) {
        new_row_ptr[i] = col_offset;
        edge_ptr start = g->row_ptr[i];
        int eff = eff_num[i];
        
        for (int j = 0; j < eff; j++) {
            new_col_idx[col_offset++] = g->col_idx[start + j];
        }
    }
    new_row_ptr[g->n] = col_offset;


    for (edge_ptr i = 0; i < col_offset; i++) {
        g->col_idx[i] = new_col_idx[i];
    }

    for (node_t i = 0; i <= g->n; i++) {
        g->row_ptr[i] = new_row_ptr[i];
    }

    g->m = col_offset;

    free(new_row_ptr);
    free(new_col_idx);
}
static void col_redundant()
{
    offset = global_g->m;
    if (offset & 1) {
            offset += 1;
    } 
    for (edge_ptr j = 0;j<global_g->m;j++) {
        //HERE_OKF(" index %d begin...", j); 
        node_t node = global_g->col_idx[j];
        global_g->col_idx[offset+j*2]=global_g->row_ptr[node];
        global_g->col_idx[offset+j*2+1]=global_g->row_ptr[node+1];;
    }
}

void prepare_graph() {
    read_input();  
    data_renumber();    
    bitmap = alloc_bitmap(N, EF_NR_DPUS);
    data_allocate(bitmap); 

    
#ifdef NO_PARTITION_AS_POSSIBLE
    if (global_g->n > DPU_N - 1 || global_g->m * 3 > DPU_M)no_partition_flag=0; //data_compact
#else
    no_partition_flag=0; 
#endif

#ifdef PRUNING_OP
    if(no_partition_flag)col_redundant();
#endif

    init_op_bitmap(op_bitmap, BM_NUMS, global_g);
    //print_bitmap(op_bitmap, 100, 100);  //test
    //verify_bitmap_intersection(op_bitmap,bm_nums); //test

}


void data_transfer(struct dpu_set_t set,bitmap_t bitmap ,int base) {
    
if(no_partition_flag){
    data_xfer(set,base);
}else
{
    data_compact(set, bitmap,base);
    HERE_OKF("data_compact ok");
}
    DPU_ASSERT(dpu_broadcast_to(set, "no_partition_flag", 0, &no_partition_flag, sizeof(uint32_t), DPU_XFER_DEFAULT));

}

static void data_bm_xfer(struct dpu_set_t set,int base) {
       struct dpu_set_t dpu;
        uint32_t each_dpu;

        DPU_ASSERT(dpu_load(set, DPU_BM_BINARY, NULL));
        node_t max_root_num = 0;
        DPU_FOREACH(set, dpu, each_dpu) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, &global_g->root_num[each_dpu+base]));
            if(global_g->root_num[each_dpu+base] > max_root_num) {
                max_root_num = global_g->root_num[each_dpu+base];
            }
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "root_num", 0, sizeof(uint64_t), DPU_XFER_DEFAULT));
        DPU_FOREACH(set, dpu, each_dpu) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, global_g->roots[each_dpu+base]));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "roots", 0, ALIGN8(max_root_num * sizeof(node_t)), DPU_XFER_DEFAULT));
        DPU_FOREACH(set, dpu, each_dpu) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, global_g->row_ptr));
        }

        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "row_ptr", 0, ALIGN8((BM_NUMS + 1) * sizeof(edge_ptr)), DPU_XFER_DEFAULT));
        DPU_FOREACH(set, dpu, each_dpu) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, global_g->col_idx));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "col_idx", 0, ALIGN8(global_g->row_ptr[BM_NUMS] * sizeof(node_t)), DPU_XFER_DEFAULT));

        //op_bitmap
        DPU_FOREACH(set, dpu, each_dpu) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, op_bitmap));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "bitmap", 0, sizeof(uint64_t) * BITMAP_ROW * BITMAP_COL, DPU_XFER_DEFAULT));


}

void data_bm_transfer(struct dpu_set_t set,int base){
    data_bm_xfer(set,base);
}