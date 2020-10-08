//
// Created by LemonNeko on 2020/10/8.
//

#include "libgit2java.h"

unsigned int compute_progress(unsigned int completed, unsigned int total) {
    return (unsigned int) ((completed * 1.0) / (total * 1.0) * 100);
}

JNIEnv *get_jni_env() {
    if (global_vm) {
        JNIEnv *env;
        int status = (*global_vm)->GetEnv(global_vm, (void **) &env, JNI_VERSION_10);

        if (status != JNI_OK) {
            (*global_vm)->AttachCurrentThread(global_vm, (void **) &env, NULL);
        }
        return env;
    } else {
        return NULL;
    }
}

void release_jni_env() {
    if (global_vm) {
        (*global_vm)->DetachCurrentThread(global_vm);
    }
}
