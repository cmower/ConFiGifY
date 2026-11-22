#include "mex.h"
#include "cfgy.h"
#include <string.h>

static mxArray *bytes(const char *data, size_t length) {
    mxArray *result = mxCreateNumericMatrix(1, length, mxUINT8_CLASS, mxREAL);
    if (length) memcpy(mxGetData(result), data, length);
    return result;
}

/* Return map keys as bytes, so quoted keys need not be MATLAB identifiers. */
static mxArray *convert(const cfgy_value *n) {
    switch (n->kind) {
        case CFGY_NULL: return mxCreateDoubleMatrix(0, 0, mxREAL);
        case CFGY_BOOL: return mxCreateLogicalScalar(n->integer != 0);
        case CFGY_INT: {
            mxArray *result = mxCreateNumericMatrix(1, 1, mxINT64_CLASS, mxREAL);
            *(int64_t *)mxGetData(result) = n->integer;
            return result;
        }
        case CFGY_FLOAT: return mxCreateDoubleScalar(n->number);
        case CFGY_STRING: return bytes(n->string.data, n->string.size);
        case CFGY_LIST: {
            mxArray *items = mxCreateCellMatrix(1, n->count);
            for (size_t i = 0; i < n->count; ++i)
                mxSetCell(items, i, convert(&n->children[i]));
            return items;
        }
        case CFGY_MAP: {
            const char *names[] = {"keys", "values"};
            mxArray *map = mxCreateStructMatrix(1, 1, 2, names);
            mxArray *keys = mxCreateCellMatrix(1, n->count);
            mxArray *values = mxCreateCellMatrix(1, n->count);
            mxSetField(map, 0, "keys", keys);
            mxSetField(map, 0, "values", values);
            for (size_t i = 0; i < n->count; ++i) {
                const cfgy_value *child = &n->children[i];
                mxSetCell(keys, i, bytes(child->key.data, child->key.size));
                mxSetCell(values, i, convert(child));
            }
            return map;
        }
    }
    return NULL;
}

void mexFunction(int nlhs, mxArray *out[], int nrhs, const mxArray *in[]) {
    if (nlhs != 2 || nrhs != 2 || !mxIsUint8(in[0]) || !mxIsUint8(in[1]))
        mexErrMsgIdAndTxt("cfgy:Arguments", "Expected two UTF-8 byte arrays and two outputs");
    size_t size = mxGetNumberOfElements(in[1]);
    char *source = mxCalloc(size + 1, 1);
    if (size) memcpy(source, mxGetData(in[1]), size);
    if (strlen(source) != size) mexErrMsgIdAndTxt("cfgy:Arguments", "NUL in source name");
    cfgy_document *doc = cfgy_parse(mxGetData(in[0]), mxGetNumberOfElements(in[0]), source);
    mxFree(source);
    const char *error = cfgy_error(doc);
    out[0] = error ? mxCreateDoubleMatrix(0, 0, mxREAL) : convert(cfgy_root(doc));
    out[1] = bytes(error, error ? strlen(error) : 0);
    cfgy_free(doc);
}
