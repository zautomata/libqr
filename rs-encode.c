#include <assert.h>
#include <stdlib.h>
#include "bitstream.h"
#include "rs.h"

static unsigned int gf_mult(unsigned int a, unsigned int b)
{
        /* Reduce modulo x^8 + x^4 + x^3 + x^2 + 1
         * using the peasant's algorithm
         */
        const unsigned int m = 0x11D;
        unsigned int x = 0;
        int i;

        for (i = 0; i < 8; ++i) {
                x ^= (b & 0x1) ? a : 0;
                a = (a << 1) ^ ((a & 0x80) ? m : 0);
                b >>= 1;
        }

        return x & 0xFF;
}

static unsigned int * make_generator(int k)
{
        unsigned int * g;
        unsigned int a;
        int i, j;

        g = calloc(k, sizeof(*g));
        if (!g)
                return 0;

        g[0] = 1; /* Start with g(x) = 1 */
        a = 1;    /* 2^0 = 1 */

        for (i = 0; i < k; ++i) {
                /* Multiply our poly g(x) by (x + 2^i) */
                for (j = k - 1; j > 0; --j)
                        g[j] = gf_mult(g[j], a) ^ g[j-1];
                g[0] = gf_mult(g[0], a);

                a = gf_mult(a, 2);
        }

        return g;
}

struct bitstream * rs_generate_words(struct bitstream * data,
                                     size_t data_words,
                                     size_t rs_words)
{
        struct bitstream * ec = 0;
        unsigned int * b = 0;
        unsigned int * g;
        size_t n = rs_words;
        int i, r;

        assert(bitstream_remaining(data) >= data_words * 8);

        ec = bitstream_create();
        if (!ec)
                return 0;

        if (bitstream_resize(ec, n * 8) != 0)
                goto fail;

        b = calloc(n, sizeof(*b));
        if (!b)
                goto fail;

        g = make_generator(n);
        if (!g)
                goto fail;

        /* First, prepare the registers (b) with data bits */
        bitstream_seek(data, 0);
        for (i = 0; i < data_words; ++i) {
                unsigned int x = b[n-1] ^ bitstream_read(data, 8);
                for (r = n-1; r > 0; --r)
                        b[r] = b[r-1] ^ gf_mult(g[r], x);
                b[0] = gf_mult(g[0], x);
        }

        /* Read off the registers */
        for (r = 0; r < n; ++r)
                bitstream_write(ec, b[(n-1)-r], 8);

        free(g);
        free(b);
        return ec;
fail:
        free(b);
        bitstream_destroy(ec);
        return 0;
}
