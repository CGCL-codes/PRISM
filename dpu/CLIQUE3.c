#include <dpu_mine.h>

#ifndef WRAM_ASYNC
static ans_t partial_ans[NR_TASKLETS];
static uint64_t partial_cycle[NR_TASKLETS];
static perfcounter_cycles cycles[NR_TASKLETS];

#ifdef BITMAP
static ans_t __imp_clique3_bitmap(sysname_t tasklet_id, node_t second_index) {
    ans_t ans = 0;
    mram_read(mram_bitmap[second_index], bitmap[tasklet_id], sizeof(bitmap[tasklet_id]));
    for (node_t i = 0; i < bitmap_size; i++) {
        uint32_t tmp = bitmap[tasklet_id][i];
        if (tmp) for (node_t j = 0; j < 32; j++) {
            if (tmp & (1 << j)) ans++;
        }
    }
    return ans;
}
#endif

static ans_t __imp_clique3_2(sysname_t tasklet_id, node_t __mram_ptr * root_col, node_t root_size, node_t __mram_ptr * second_col, node_t second_size) {

    if(!second_size)return 0;
    
    node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];

#ifdef NO_RUN   //test cycle without Intersection operation 
    //node_t ans =  intersect_seq_buf_thresh_no_run(tasklet_buf, root_col, root_size, second_col, second_size);
    //node_t ans = 1;
#else
    node_t ans =  intersect_seq_buf_thresh(tasklet_buf, root_col, root_size, second_col, second_size);
#endif

  
    return ans;
}

static ans_t __imp_clique3(sysname_t tasklet_id, node_t root) {
    edge_ptr root_begin = row_ptr[root];  // intended DMA
    edge_ptr root_end = row_ptr[root + 1];  // intended DMA
    node_t root_size = root_end - root_begin;
    if(!root_size)return 0;
    ans_t ans = 0;

    mram_read(&col_idx[edge_offset+2*root_begin],col_buf[tasklet_id],MIN(16,root_end-root_begin)<<(SIZE_EDGE_PTR_LOG+1));

    for (edge_ptr i = 1; i<root_size; i++) {
        ans += __imp_clique3_2(tasklet_id,&col_idx[root_begin],i,&col_idx[col_buf[tasklet_id][2*i]],col_buf[tasklet_id][2*i+1]-col_buf[tasklet_id][2*i]);
    }

    return ans;
}

static ans_t __imp_clique3_partition(sysname_t tasklet_id, node_t root) {
    edge_ptr root_begin = row_ptr[root];  // intended DMA
    edge_ptr root_end = row_ptr[root + 1];  // intended DMA
    node_t root_size = root_end - root_begin;
    if(!root_size)return 0;
    ans_t ans = 0;
    for (edge_ptr i = root_begin + 1; i<root_end; i++) {
        node_t second_root = col_idx[i];  // intended DMA 
        edge_ptr second_begin = row_ptr[second_root];  // intended DMA
        edge_ptr second_end = row_ptr[second_root+1];  // intended DMA
        ans += __imp_clique3_2(tasklet_id,&col_idx[root_begin],i-root_begin,&col_idx[second_begin],second_end-second_begin);
    }
    return ans;
}


//func begin
extern void clique3( sysname_t tasklet_id )
{
	node_t i = 0;
	large_degree_num = root_num;                            /* if all node is large_degree */
	while ( i < root_num )
	{
		node_t	root		= roots[i];             /* intended DMA */
		node_t	root_begin	= row_ptr[root];        /* intended DMA */
		node_t	root_end	= row_ptr[root + 1];    /* intended DMA */
		node_t	root_size	= root_end - root_begin;
		if ( root_size < BRANCH_LEVEL_THRESHOLD )
		{
			large_degree_num = i;
			break;
		}

#ifdef PERF
		timer_start( &cycles[tasklet_id] );
#endif
		partial_ans[tasklet_id] = 0;

		if ( no_partition_flag )
		{
			// const int	num_dma_threads		= 8;
			// const node_t	max_edges_per_chunk	= 256;  /* 每次最多搬运 256 条边 = 512 元素 */

			// node_t *cb = col_buf[0];                        /* 共享缓冲区，容量 512 个元素 */

			// edge_ptr	root_begin	= row_ptr[root];
			// edge_ptr	root_end	= row_ptr[root + 1];
			// node_t		root_size	= root_end - root_begin;

			// for ( node_t chunk_offset = 1; chunk_offset < root_size; chunk_offset += max_edges_per_chunk )
			// {
			// 	node_t chunk_size = MIN( max_edges_per_chunk, root_size - chunk_offset ); /* 本次最多搬多少边 */

			// 	/* === 1. 并行搬运 === */
			// 	if ( tasklet_id < num_dma_threads )
			// 	{
			// 		node_t	local_chunk_size	= (chunk_size + num_dma_threads - 1) / num_dma_threads;
			// 		node_t	local_start		= tasklet_id * local_chunk_size;
			// 		node_t	local_end		= MIN( (tasklet_id + 1) * local_chunk_size, chunk_size );

			// 		if ( local_start < local_end )
			// 		{
			// 			edge_ptr	mram_src_offset = root_begin + chunk_offset + local_start;
			// 			node_t		len		= local_end - local_start;

			// 			mram_read( &col_idx[edge_offset + 2 * mram_src_offset], cb + 2 * local_start, len << (SIZE_EDGE_PTR_LOG + 1) ); /* len * 2 * sizeof(edge_ptr) */
			// 		}
			// 	}

			// 	barrier_wait( &co_barrier );                                                                                                    /* 所有搬运完成再进入计算 */

			// 	/* === 2. 并行处理 === */
			// 	for ( edge_ptr j = tasklet_id; j < chunk_size; j += NR_TASKLETS )
			// 	{
			// 		partial_ans[tasklet_id] += __imp_clique3_2(
			// 			tasklet_id,
			// 			&col_idx[root_begin],                                                                                           /* 原始 row 起点 */
			// 			chunk_offset + j,                                                                                               /* 真实全局偏移 j */
			// 			&col_idx[cb[2 * j]],
			// 			cb[2 * j + 1] - cb[2 * j]
			// 			);
			// 	}

			// 	barrier_wait( &co_barrier );                                                                                                    /* 等所有线程处理完，再搬下一块 */
			// }

			
			 //without prefetch
			 for ( edge_ptr j = root_begin + tasklet_id + 1; j < root_end; j += NR_TASKLETS )
			 {
			     partial_ans[tasklet_id] += __imp_clique3_2( tasklet_id, &col_idx[root_begin], j - root_begin, &col_idx[col_idx[edge_offset + 2 * j]], col_idx[edge_offset + 2 * j + 1] - col_idx[edge_offset + 2 * j] );
			 }
			 
		}else{
			for ( edge_ptr j = root_begin + tasklet_id + 1; j < root_end; j += NR_TASKLETS )
			{
				node_t		second_root	= col_idx[j];                   /* intended DMA */
				edge_ptr	second_begin	= row_ptr[second_root];         /* intended DMA */
				edge_ptr	second_end	= row_ptr[second_root + 1];     /* intended DMA */
				partial_ans[tasklet_id] += __imp_clique3_2( tasklet_id, &col_idx[root_begin], j - root_begin, &col_idx[second_begin], second_end - second_begin );
			}
		}

#ifdef PERF
		partial_cycle[tasklet_id] = timer_stop( &cycles[tasklet_id] );
#endif
		barrier_wait( &co_barrier );
		if ( tasklet_id == 0 )
		{
			ans_t total_ans = 0;
#ifdef PERF
			uint64_t total_cycle = 0;
#endif
			for ( uint32_t j = 0; j < NR_TASKLETS; j++ )
			{
				total_ans += partial_ans[j];
#ifdef PERF
				total_cycle += partial_cycle[j];
#endif
			}
			ans[i] = total_ans;             /* intended DMA */
#ifdef PERF
			cycle_ct[i] = total_cycle;      /* intended DMA */
#endif
		}
		i++;

		barrier_wait( &co_barrier );
	}

	for ( i += tasklet_id; i < root_num; i += NR_TASKLETS )
	{
		node_t root = roots[i];                                         /* intended DMA */

#ifdef PERF
		timer_start( &cycles[tasklet_id] );
#endif
		if ( no_partition_flag )
			ans[i] = __imp_clique3( tasklet_id, root );             /* intended DMA */
		else
			ans[i] = __imp_clique3_partition( tasklet_id, root );   /* intended DMA */

#ifdef PERF
		cycle_ct[i] = timer_stop( &cycles[tasklet_id] );                /* intended DMA */
#endif
	}
}

//async
#else
#include <fifo.h>

extern node_t intersect_from_buf( node_t *a, uint32_t a_size, node_t *b, uint32_t b_size );

#define NR_LOADER	1
#define NR_WORKER	(NR_TASKLETS - NR_LOADER)

extern void clique3( sysname_t tasklet_id )
{
	if ( tasklet_id == 0 )
	{
		printf( "fifo initing\n" );
		fifo_init( &global_fifo );
		loader_done_flag = false;
	}
	barrier_wait( &co_barrier );

	if ( tasklet_id < NR_LOADER )
	{
		/* === Loader Tasklet === */
		for ( node_t root_id = tasklet_id; root_id < root_num; root_id += NR_LOADER )
		{
			node_t		root		= roots[root_id];
			edge_ptr	rb		= row_ptr[root];
			edge_ptr	re		= row_ptr[root + 1];
			node_t		root_size	= re - rb;
			if ( !root_size )
				continue;
			int a_idx = allocate_a_buf( root_size );
			if ( a_idx < 0 )
			{
				printf( "[ALLOC FAIL] root_id=%u, root_size=%u\n", root_id, root_size );
				continue;
			} else {
				printf( "[ALLOC] root_id=%u, a_idx=%d, size=%u\n", root_id, a_idx, root_size );
			}
			edge_ptr	aligned_rb	= rb;
			uint8_t		offset_a	= 0;
			if ( aligned_rb % 2 )
			{
				aligned_rb--; offset_a = 1;
			}
			mram_read( &col_idx[aligned_rb], a_buf_pool[a_idx], ALIGN8( (root_size + offset_a) << SIZE_NODE_T_LOG ) );

			int valid_b_cnt = 0;

			job_t jobs[10];

			for ( edge_ptr j = rb; j < re; j++ )
			{
				node_t		second	= col_idx[j];
				edge_ptr	sb	= row_ptr[second];
				edge_ptr	se	= row_ptr[second + 1];
				node_t		b_size	= se - sb;
				if ( !b_size )
					continue;

				int b_idx = allocate_b_buf( b_size );
				if ( b_idx < 0 )
					continue;

				edge_ptr	aligned_sb	= sb;
				uint8_t		offset_b	= 0;
				if ( aligned_sb % 2 )
				{
					aligned_sb--; offset_b = 1;
				}
				mram_read( &col_idx[aligned_sb], b_buf_pool[b_idx], ALIGN8( (b_size + offset_b) << SIZE_NODE_T_LOG ) );

				jobs[valid_b_cnt] = (job_t) {
					.root_id	= root_id,
					.a_index	= a_idx,
					.b_index	= b_idx,
					.a_offset	= offset_a,
					.b_offset	= offset_b,
					.a_size		= root_size,
					.b_size		= b_size,
					.threshold	= second
				};

				valid_b_cnt++;
			}

			if ( valid_b_cnt == 0 )
			{
				release_a_buf( a_idx );
			} else {
				a_buf_table[a_idx].ref_count = valid_b_cnt;
				printf( "[ALLOC] root_id=%u, a_idx=%d, size=%u\n", root_id, a_idx, root_size );

				/* 批量enqueue所有任务 */
				for ( int k = 0; k < valid_b_cnt; k++ )
				{
					while ( !fifo_enqueue( &global_fifo, jobs[k] ) )
					{
						if ( tasklet_id == 0 )
							printf( "[INFO] FIFO full. Waiting...\n" );
					}
				}
			}
		}
		loader_done_flag = true;
	}else    {
		/* === Worker Tasklet === */
		while ( 1 )
		{
			job_t job;
			if ( !fifo_dequeue( &global_fifo, &job ) )
			{
				if ( loader_done_flag )
					break;
				continue;
			}
			node_t	*a	= a_buf_pool[job.a_index] + job.a_offset;
			node_t	*b	= b_buf_pool[job.b_index] + job.b_offset;
			node_t	res	= intersect_from_buf( a, job.a_size, b, job.b_size );

			ans[job.root_id] += res;


			release_b_buf( job.b_index );
			a_buf_table[job.a_index].ref_count--;
			if ( a_buf_table[job.a_index].ref_count == 0 )
			{
				printf( "[RELEASE] a_idx=%d fully released\n", job.a_index );
				release_a_buf( job.a_index );
			}
		}
	}
}

#endif