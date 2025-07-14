#include <common.h>
#include <mram.h>
#include <defs.h>


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


// intersection
extern node_t intersect_bitmap(uint64_t (*buf)[BUF_SIZE], uint64_t __mram_ptr *a, uint64_t __mram_ptr *b, node_t threshold)
{
    node_t ans = 0;
    uint64_t cmp_size = threshold >> 6;         // = threshold / 64，
    uint64_t remain = threshold & 63;           // = threshold % 64

    uint64_t *a_buf = buf[0]; 
    uint64_t *b_buf = buf[1];

    uint64_t offset = 0;

    while (cmp_size > 0)
    {
        uint64_t batch_size = MIN(cmp_size, BUF_SIZE);

        mram_read(a + offset, a_buf, batch_size << SIZE_BITMAP_LOG);  
        mram_read(b + offset, b_buf, batch_size << SIZE_BITMAP_LOG);

        for (uint64_t i = 0; i < batch_size; ++i)
        {
            uint64_t val = a_buf[i] & b_buf[i];
            ans += count64_lookup(val);
        }

        cmp_size -= batch_size;
        offset += batch_size;
    }

    // 处理最后一部分不足 64 bit 的尾部
    if (remain > 0)
    {
        uint64_t a_last, b_last;
        mram_read(a + offset, &a_last, sizeof(uint64_t));
        mram_read(b + offset, &b_last, sizeof(uint64_t));

        uint64_t mask = ((uint64_t)1 << remain) - 1;
        uint64_t val = (a_last & b_last) & mask;
        ans += count64_lookup(val);
    }

    return ans;
}
