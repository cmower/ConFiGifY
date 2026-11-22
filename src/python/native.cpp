#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "cfgy.h"

static PyObject* convert(const cfgy_value& node, PyObject* mapping) {
    switch (node.kind) {
        case CFGY_NULL: Py_RETURN_NONE;
        case CFGY_BOOL: return PyBool_FromLong(static_cast<long>(node.integer));
        case CFGY_INT: return PyLong_FromLongLong(node.integer);
        case CFGY_FLOAT: return PyFloat_FromDouble(node.number);
        case CFGY_STRING: return PyUnicode_FromStringAndSize(node.string.data, node.string.size);
        case CFGY_LIST: {
            PyObject* list = PyList_New(node.count);
            if (!list) return nullptr;
            for (size_t i = 0; i < node.count; ++i) {
                auto* item = convert(node.children[i], mapping);
                if (!item) { Py_DECREF(list); return nullptr; }
                PyList_SET_ITEM(list, i, item);  // steals the new reference
            }
            return list;
        }
        case CFGY_MAP: {
            PyObject* dict = PyDict_New();
            if (!dict) return nullptr;
            for (size_t i = 0; i < node.count; ++i) {
                const auto& child = node.children[i];
                auto* key = PyUnicode_FromStringAndSize(child.key.data, child.key.size);
                auto* item = key ? convert(child, mapping) : nullptr;
                const bool ok = item && PyDict_SetItem(dict, key, item) == 0;
                Py_XDECREF(key);
                Py_XDECREF(item);
                if (!ok) { Py_DECREF(dict); return nullptr; }
            }
            auto* result = PyObject_CallFunctionObjArgs(mapping, dict, nullptr);
            Py_DECREF(dict);
            return result;
        }
    }
    PyErr_SetString(PyExc_RuntimeError, "unknown cfgy value type");
    return nullptr;
}

static PyObject* parse(PyObject*, PyObject* args) {
    PyObject *text, *mapping;
    const char* source;
    if (!PyArg_ParseTuple(args, "UsO", &text, &source, &mapping)) return nullptr;
    Py_ssize_t size;
    const char* utf8 = PyUnicode_AsUTF8AndSize(text, &size);
    if (!utf8) return nullptr;
    cfgy_document* doc = cfgy_parse(utf8, size, source);
    if (!doc) return PyErr_NoMemory();
    PyObject* result = nullptr;
    if (const char* error = cfgy_error(doc)) PyErr_SetString(PyExc_ValueError, error);
    else result = convert(*cfgy_root(doc), mapping);
    cfgy_free(doc);
    return result;
}

static PyMethodDef methods[] = {{"parse", parse, METH_VARARGS, nullptr}, {nullptr}};
static PyModuleDef module = {PyModuleDef_HEAD_INIT, "_cfgy", nullptr, -1, methods};
PyMODINIT_FUNC PyInit__cfgy() { return PyModule_Create(&module); }
