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
    int64_t samples;
    double interval_ms;
    int status = 1;
    if (error) {
        fprintf(stderr, "%s\n", error);
    } else if (!cfgy_read(cfg, &samples, "recording", "samples")) {
        fputs("Expected an integer at recording.samples\n", stderr);
    } else if (!cfgy_read(cfg, &interval_ms, "recording", "interval_ms")) {
        fputs("Expected a number at recording.interval_ms\n", stderr);
    } else {
        printf("%" PRId64 "\n%g\n", samples, interval_ms);
        status = 0;
    }
    cfgy_free(cfg);
    return status;
}
