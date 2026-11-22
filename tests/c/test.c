#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cfgy.h>
#include <assert.h>
#include <string.h>

int main(int argc, char **argv) {
    assert(argc == 2);
    cfgy_document *doc = cfgy_load(argv[1]);
    assert(!cfgy_error(doc));
    const cfgy_value *root = cfgy_root(doc);
    const cfgy_value *wide = cfgy_get(root, "wide");
    assert(wide && wide->kind == CFGY_INT && wide->integer == INT64_C(9007199254740993));
    assert(cfgy_get(cfgy_get(root, "group"), "a.b")->integer == 7);
    assert(cfgy_get(root, "absent") == NULL);
    const cfgy_value *items = cfgy_get(root, "mixed");
    assert(items->count == 7 && items->children[0].kind == CFGY_NULL);
    assert(items->children[1].kind == CFGY_BOOL && items->children[1].integer == 1);
    assert(items->children[6].kind == CFGY_LIST && items->children[6].count == 0);
    assert(cfgy_get(root, "absolute")->children[1].number == 2.5);
    cfgy_document *bad = cfgy_parse("x: sqrt(-1)", 11, "broken");
    assert(!cfgy_root(bad) && strstr(cfgy_error(bad), "broken:1:4:"));
    cfgy_free(bad);
    const char invalid[] = "x: '\xed\xa0\x80'";
    bad = cfgy_parse(invalid, sizeof(invalid) - 1, "bytes");
    assert(cfgy_root(bad) == NULL && strstr(cfgy_error(bad), "invalid UTF-8"));
    cfgy_free(bad);
    assert(wide->integer == INT64_C(9007199254740993));
    cfgy_free(doc);
    cfgy_free(NULL);
}
