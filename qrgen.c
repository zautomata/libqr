#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <getopt.h>
#include <qr/bitmap.h>
#include <qr/code.h>
#include <qr/data.h>

#include "qr-bitstream.h"
#include "code-common.h"

struct config {
        int               version;
        enum qr_ec_level  ec;
        enum qr_data_type dtype;
        int               ansi;
        const char *      input;
};

struct qr_code * create(int               version,
                        enum qr_ec_level  ec,
                        enum qr_data_type dtype,
                        const char *      input)
{
        struct qr_data * data;
        struct qr_code * code;
        size_t len;

        len = strlen(input);

        data = qr_create_data(version, dtype, input, len);

        if (!data) {
                /* BUG: this could also indicate OOM or
                 * some other error.
                 */
                fprintf(stderr, "Invalid data\n");
                exit(1);
        }

        code = qr_code_create(ec, data);

        if (!code) {
                perror("Failed to create code");
                qr_free_data(data);
                exit(2);
        }

        return code;
}

void output_pbm(const struct qr_bitmap * bmp, const char * comment)
{
        unsigned char * row;
        int x, y;

        puts("P1");

        if (comment)
                printf("# %s\n", comment);

        printf("%u %u\n",
               (unsigned)bmp->width + 8,
               (unsigned)bmp->height + 8);

        row = bmp->bits;

        for (y = -4; y < (int)bmp->height + 4; ++y) {

                if (y < 0 || y >= (int)bmp->height) {
                        for (x = 0; x < (int)bmp->width + 8; ++x)
                                printf("0 ");
                        putchar('\n');
                        continue;
                }

                printf("0 0 0 0 ");

                for (x = 0; x < (int)bmp->width; ++x) {

                        int mask = 1 << x % CHAR_BIT;
                        int byte = row[x / CHAR_BIT];

                        printf("%c ", (byte & mask) ? '1' : '0');
                }

                puts("0 0 0 0");
                row += bmp->stride;
        }
}

void output_ansi(const struct qr_bitmap * bmp)
{
        const char * out[2] = {
                "  ",
                "\033[7m  \033[0m",
        };

        unsigned char * line;
        int x, y;

        line = bmp->bits;

        for (y = 0; y < bmp->height; ++y) {

                for (x = 0; x < bmp->width; ++x) {

                        int mask = 1 << (x % CHAR_BIT);
                        int byte = line[x / CHAR_BIT];

                        printf("%s", out[!!(byte & mask)]);
                }

                putchar('\n');

                line += bmp->stride;
        }
}

void show_help() {
        fprintf(stderr, "(help message)\n");
}

void set_default_config(struct config * conf)
{
        conf->version = 1;
        conf->ec = QR_EC_LEVEL_M;
        conf->dtype = QR_DATA_NUMERIC;
        conf->ansi = 1;
        conf->input = "01234567";
}

void parse_options(int argc, char ** argv, struct config * conf)
{
        int c;

        for (;;) {
                c = getopt(argc, argv, ":?vetap");

                if (c == -1) /* no more options */
                        break;

                switch (c) {
                case '?': /* help */
                        show_help();
                        exit(0);
                        break;
                case 'v': /* version */
                        if (!optarg) {
                                fprintf(stderr, "No version\n");
                                exit(1);
                        }
                        conf->version = atoi(optarg);
                        if (conf->version < 1 || conf->version > 40) {
                                fprintf(stderr,
                                        "Version must be between 1 and 40\n");
                                exit(1);
                        }
                        break;
                case 'e': /* ec */
                        fprintf(stderr, "XXX: ignored \"ec\"\n"); break;
                case 't': /* type */
                        fprintf(stderr, "XXX: ignored \"type\"\n"); break;
                case 'a': /* ansi */
                        conf->ansi = 1; break;
                case 'p': /* pnm */
                        conf->ansi = 0; break;
                case ':': default:
                        fprintf(stderr, "Unknown option: \"%s\"\n",
                                argv[optind]);
                        exit(1);
                        break;
                }
        }


        if (optind < argc)
                conf->input = argv[optind++];

        if (!conf->input) {
                fprintf(stderr, "No data (try -? for help)\n");
                exit(1);
        }
}

int main(int argc, char ** argv) {

        struct config conf;
        struct qr_code * code;

        set_default_config(&conf);
        parse_options(argc, argv, &conf);

        code = create(conf.version,
                      conf.ec,
                      conf.dtype,
                      conf.input);

        if (conf.ansi)
                output_ansi(code->modules);
        else
                output_pbm(code->modules, "qrgen v0.1");

        qr_code_destroy(code);

        return 0;
}

