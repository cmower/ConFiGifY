#ifndef CFGY_H
#define CFGY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum cfgy_kind {
    CFGY_NULL, CFGY_BOOL, CFGY_INT, CFGY_FLOAT, CFGY_STRING, CFGY_LIST, CFGY_MAP
};

typedef struct {
    const char *data;
    size_t size;
} cfgy_string;

/* Read-only views, valid until cfgy_free(document). Strings are UTF-8 byte spans.
   integer holds both INT and BOOL (0/1). Child keys are populated for maps. */
typedef struct cfgy_value {
    int32_t kind;
    cfgy_string key;
    int64_t integer;
    double number;
    cfgy_string string;
    size_t count;
    const struct cfgy_value *children;
} cfgy_value;

typedef struct cfgy_document cfgy_document;

/* Each call returns an owned document, including on parse errors. NULL means
   allocation failed. cfgy_error(NULL) and cfgy_free(NULL) are both valid. */
cfgy_document *cfgy_load(const char *path);
cfgy_document *cfgy_parse(const char *utf8, size_t length, const char *source);
const char *cfgy_error(const cfgy_document *document); /* NULL on success */
const cfgy_value *cfgy_root(const cfgy_document *document); /* NULL on error */
void cfgy_free(cfgy_document *document);

/* Look up a NUL-terminated map key; returns NULL for a missing key or wrong kind.
   Iterate children[0..count) to inspect any key, including embedded NULs. */
const cfgy_value *cfgy_get(const cfgy_value *map, const char *key);

#ifdef __cplusplus
}
#endif
#endif
