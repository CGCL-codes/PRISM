#include <common.h>
#include <mram.h>
#include <defs.h>


extern node_t intersect_seq_buf_thresh(node_t (*buf)[BUF_SIZE], node_t __mram_ptr *a, node_t a_size, node_t __mram_ptr *b, node_t b_size) {        
    node_t ans = 0;
    if (a_size > b_size) {
        node_t __mram_ptr *tmp = a;
        a = b;
        b = tmp;
        node_t tmp_size = a_size;
        a_size = b_size;
        b_size = tmp_size;
    }
    if (a_size < (b_size >> 4)&& a_size < BUF_SIZE) {
        node_t i = 0;
        if (((uint64_t)a) & 4) {
            a--;
            i = 1; 
            a_size++;
        }
        
        mram_read(a, buf[0], ALIGN8((a_size)<<SIZE_NODE_T_LOG));
        for (; i < a_size; i++) { 
            node_t a_val = buf[0][i];
            node_t l = 0, r = b_size;
            while (l <r) {
                node_t mid = l + ((r - l) >> 1);;
                node_t b_val = b[mid];  // intended DMA
                if (a_val == b_val) {
                    ans++;
                    break;
                }
                else if (a_val < b_val) {
                    r = mid;
                }
                else {
                    l = mid + 1;
                }
            }
        }
        return ans;
    }
    
node_t *a_buf = buf[0];
node_t *b_buf = buf[1];

node_t i = 0, j = 0;


while (a_size > 0 && b_size > 0) {

if (((uint64_t)a) & 4) {
    a--;
    i = 1;
    a_size++;
}
if (((uint64_t)b) & 4) {
    b--;
    j = 1;
    b_size++;
}
    node_t a_chunk_size = MIN(BUF_SIZE, a_size);
    node_t b_chunk_size = MIN(BUF_SIZE, b_size);

    mram_read(a, a_buf, ALIGN8(a_chunk_size << SIZE_NODE_T_LOG));
    mram_read(b, b_buf, ALIGN8(b_chunk_size << SIZE_NODE_T_LOG));

    while (i < a_chunk_size && j < b_chunk_size) {
        if (a_buf[i] == b_buf[j]) {
            ans++;
            i++;
            j++;
        } else if (a_buf[i] < b_buf[j]) {
            i++;
        } else {
            j++;
        }
    }

    a += i;
    b += j;
    a_size -= i;
    b_size -= j;
    i=j=0;
}
    return ans;
}

extern node_t intersect_seq_buf_thresh_no_run(node_t (*buf)[BUF_SIZE], node_t __mram_ptr *a, node_t a_size, node_t __mram_ptr *b, node_t b_size, node_t threshold) {        
    node_t ans = 0;
    node_t i = 0, j = 0;
    if (((uint64_t)a) & 4) {
        a--;
        i = 1;
        a_size++;
    }
    if (((uint64_t)b) & 4) {
        b--;
        j = 1;
        b_size++;
    }

    // 缓冲区
    node_t *a_buf = buf[0];
    node_t *b_buf = buf[1];

    // 遍历 a 和 b，全量 MRAM 读取，不执行任何逻辑
    node_t a_remain = a_size;
    node_t b_remain = b_size;
    node_t __mram_ptr *a_ptr = a;
    node_t __mram_ptr *b_ptr = b;

    while (a_remain > 0 || b_remain > 0) {
        if (a_remain > 0) {
            node_t a_chunk = MIN(a_remain, BUF_SIZE);
            mram_read(a_ptr, a_buf, ALIGN8(a_chunk << SIZE_NODE_T_LOG));
            a_ptr += a_chunk;
            a_remain -= a_chunk;
        }
        if (b_remain > 0) {
            node_t b_chunk = MIN(b_remain, BUF_SIZE);
            mram_read(b_ptr, b_buf, ALIGN8(b_chunk << SIZE_NODE_T_LOG));
            b_ptr += b_chunk;
            b_remain -= b_chunk;
        }
    }
    return ans;
}


extern node_t intersect_seq_buf_thresh_withcbuf(node_t (*buf)[BUF_SIZE], node_t __mram_ptr *a, node_t a_size, node_t __mram_ptr *b, node_t b_size, node_t __mram_ptr *c, node_t threshold) {
    if(a_size==0||b_size==0)return 0;
    
    if (a_size > b_size) {
        node_t __mram_ptr *tmp = a;
        a = b;
        b = tmp;
        node_t tmp_size = a_size;
        a_size = b_size;
        b_size = tmp_size;
    }
    if (a_size < (b_size >> 4)&& a_size < BUF_SIZE) {
        node_t ans = 0;
        node_t i = 0;
        if (((uint64_t)a) & 4) {
            a--;
            i = 1;
            a_size++;
        }
        mram_read(a, buf, ALIGN8(a_size << SIZE_NODE_T_LOG));
        node_t *c_buf = buf[1];
        for (; i < a_size; i++) {
            node_t a_val = buf[0][i];
            if (a_val >= threshold) break;
            node_t l = 0, r = b_size;
            while (l < r) {
                node_t mid = (l + r) >> 1;
                node_t b_val = b[mid];  // intended DMA
                if (a_val == b_val) {
                    c_buf[ans++] = a_val;
                    break;
                }
                else if (a_val < b_val) {
                    r = mid;
                }
                else {
                    l = mid + 1;
                }
            }
        }
        if(ans) mram_write(c_buf, c, ALIGN8(ans << SIZE_NODE_T_LOG));
        return ans;
    }
    node_t *a_buf = buf[0];
    node_t *b_buf = buf[1];
    node_t *c_buf = buf[2];
    node_t i = 0, j = 0, k = 0, ans = 0;
    if (((uint64_t)a) & 4) {
        a--;
        i = 1;
        a_size++;
    }
    if (((uint64_t)b) & 4) {
        b--;
        j = 1;
        b_size++;
    }
    mram_read(a, a_buf, ALIGN8(MIN(a_size, BUF_SIZE) << SIZE_NODE_T_LOG));
    mram_read(b, b_buf, ALIGN8(MIN(b_size, BUF_SIZE) << SIZE_NODE_T_LOG));

    while (i < a_size && j < b_size) {
        if (i == BUF_SIZE) {
            a_size -= i;
            a += i;
            mram_read(a, a_buf, ALIGN8(MIN(a_size, BUF_SIZE) << SIZE_NODE_T_LOG));
            i = 0;
        }
        if (j == BUF_SIZE) {
            b_size -= j;
            b += j;
            mram_read(b, b_buf, ALIGN8(MIN(b_size, BUF_SIZE) << SIZE_NODE_T_LOG));
            j = 0;
        }

        if (a_buf[i] >= threshold || b_buf[j] >= threshold) break;

        if (a_buf[i] == b_buf[j]) {
            c_buf[k++] = a_buf[i];
            ans++;
            i++;
            j++;
            if (k == BUF_SIZE) {
                mram_write(c_buf, c, k << SIZE_NODE_T_LOG);
                c += k;
                k = 0;
            }
        }
        else if (a_buf[i] < b_buf[j]) {
            i++;
        }
        else {
            j++;
        }
    }
    if (k) mram_write(c_buf, c, ALIGN8(k << SIZE_NODE_T_LOG));
    return ans;
}

extern node_t intersect_from_buf(node_t *a, uint32_t a_size, node_t *b, uint32_t b_size) {
    node_t ans = 0;
    uint32_t i = 0, j = 0;

    while (i < a_size && j < b_size) {
        if (a[i] == b[j]) {
            ans++;
            i++;
            j++;
        } else if (a[i] < b[j]) {
            i++;
        } else {
            j++;
        }
    }

    return ans;
}

#ifdef BITMAP
extern void intersect_bitmap(node_t *a, node_t *b, node_t *c, node_t bitmap_size) {
    for (node_t i = 0; i < bitmap_size; i++) {
        c[i] = a[i] & b[i];
    }
}
#endif

