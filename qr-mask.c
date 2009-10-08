#include <limits.h>
#include <stdlib.h>
#include "qr-bitmap.h"
#include "qr-mask.h"

struct qr_bitmap * qr_mask_apply(const struct qr_bitmap * orig,
                                 unsigned int mask)
{
        struct qr_bitmap * bmp;
        int i, j;

        if (mask & ~0x7)
                return 0;

        bmp = qr_bitmap_clone(orig);
        if (!bmp)
                return 0;

        /* Slow version for now; we can optimize later */

        for (i = 0; i < bmp->height; ++i) {
                unsigned char * p = bmp->bits + i * bmp->stride;

                for (j = 0; j < bmp->width; ++j) {
                        int bit = j % CHAR_BIT;
                        size_t off = j / CHAR_BIT;
                        int t;

                        switch (mask) {
                        case 0: t = (i + j) % 2; break;
                        case 1: t = i % 2; break;
                        case 2: t = j % 3; break;
                        case 3: t = (i + j) % 3; break;
                        case 4: t = (i/2 + j/3) % 2; break;
                        case 5: t = ((i*j) % 2) + ((i*j) % 3); break;
                        case 6: t = (((i*j) % 2) + ((i*j) % 3)) % 2; break;
                        case 7: t = (((i*j) % 3) + ((i+j) % 2)) % 2; break;
                        }

                        p[off] ^= (t == 0) << bit;
                }
        }

        return bmp;
}
