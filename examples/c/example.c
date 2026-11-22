#include <cfgy.h>
#include <inttypes.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    cfgy_document *doc = cfgy_load(argv[1]);
    const char *error = cfgy_error(doc);
    if (error) fprintf(stderr, "%s\n", error);
    const cfgy_value *root = cfgy_root(doc);
    const cfgy_value *count = cfgy_get(cfgy_get(root, "c"), "myel1");
    const cfgy_value *gain = cfgy_get(root, "myfloat2");
    int ok = count && count->kind == CFGY_INT && gain && gain->kind == CFGY_FLOAT;
    if (ok) printf("%" PRId64 "\n%g\n", count->integer, gain->number);
    cfgy_free(doc);
    return ok ? 0 : 1;
}
