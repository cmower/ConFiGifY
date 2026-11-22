#include <cfgy.h>
#include <inttypes.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <config-file>\n", argv[0]);
        return 2;
    }

    cfgy_document *cfg = cfgy_load(argv[1]);
    const char *error = cfgy_error(cfg);
    int64_t count;
    double gain;
    int status = 1;
    if (error) {
        fprintf(stderr, "%s\n", error);
    } else if (!cfgy_read(cfg, &count, "c", "myel1")) {
        fputs("Expected an integer at c.myel1\n", stderr);
    } else if (!cfgy_read(cfg, &gain, "myfloat2")) {
        fputs("Expected a number at myfloat2\n", stderr);
    } else {
        printf("%" PRId64 "\n%g\n", count, gain);
        status = 0;
    }
    cfgy_free(cfg);
    return status;
}
