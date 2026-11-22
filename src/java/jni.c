#include <jni.h>
#include <limits.h>
#include <string.h>
#include "cfgy.h"

struct methods { jclass cls; jmethodID make, add; };

static jbyteArray bytes(JNIEnv *env, const char *data, size_t size) {
    if (size > INT_MAX) {
        jclass error = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
        if (error) (*env)->ThrowNew(env, error, "string exceeds Java array size");
        return NULL;
    }
    jbyteArray result = (*env)->NewByteArray(env, (jsize)size);
    if (result && size) (*env)->SetByteArrayRegion(env, result, 0, (jsize)size, (const jbyte *)data);
    return result;
}

static jobject convert(JNIEnv *env, const struct methods *m, const cfgy_value *n) {
    if ((*env)->PushLocalFrame(env, 8) != 0) return NULL;
    jbyteArray text = n->kind == CFGY_STRING ? bytes(env, n->string.data, n->string.size) : NULL;
    jobject value = NULL;
    if (!(*env)->ExceptionCheck(env))
        value = (*env)->CallStaticObjectMethod(env, m->cls, m->make, (jint)n->kind, (jlong)n->integer, n->number, text);
    for (size_t i = 0; i < n->count && !(*env)->ExceptionCheck(env); ++i) {
        const cfgy_value *child = &n->children[i];
        jobject item = convert(env, m, child);
        if ((*env)->ExceptionCheck(env)) break;
        jbyteArray key = n->kind == CFGY_MAP ? bytes(env, child->key.data, child->key.size) : NULL;
        if (!(*env)->ExceptionCheck(env)) (*env)->CallStaticVoidMethod(env, m->cls, m->add, value, key, item);
        (*env)->DeleteLocalRef(env, key);
        (*env)->DeleteLocalRef(env, item);
    }
    return (*env)->PopLocalFrame(env, value);
}

JNIEXPORT jobject JNICALL Java_io_configify_Cfgy_parse(JNIEnv *env, jclass cls, jbyteArray text, jbyteArray source) {
    struct methods m = {cls, NULL, NULL};
    m.make = (*env)->GetStaticMethodID(env, cls, "make", "(IJD[B)Ljava/lang/Object;");
    if (!m.make) return NULL;
    m.add = (*env)->GetStaticMethodID(env, cls, "add", "(Ljava/lang/Object;[BLjava/lang/Object;)V");
    if (!m.add) return NULL;
    jsize length = (*env)->GetArrayLength(env, text);
    jbyte *data = (*env)->GetByteArrayElements(env, text, NULL);
    if (!data) return NULL;
    jbyte *name = (*env)->GetByteArrayElements(env, source, NULL);
    cfgy_document *doc = NULL;
    if (name) {
        doc = cfgy_parse((const char *)data, length, (const char *)name);
        (*env)->ReleaseByteArrayElements(env, source, name, JNI_ABORT);
    }
    (*env)->ReleaseByteArrayElements(env, text, data, JNI_ABORT);
    if (!name) return NULL;

    jobject result = NULL;
    const char *error = cfgy_error(doc);
    if (error) {
        jmethodID fail = (*env)->GetStaticMethodID(env, cls, "fail", "([B)V");
        if (fail) {
            jbyteArray message = bytes(env, error, strlen(error));
            if (message && !(*env)->ExceptionCheck(env)) (*env)->CallStaticVoidMethod(env, cls, fail, message);
        }
    } else {
        result = convert(env, &m, cfgy_root(doc));
    }
    cfgy_free(doc);
    return result;
}
