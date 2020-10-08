//
// Created by LemonNeko on 2020/10/8.
//

#ifndef LIBGIT2JAVA_LIBGIT2_H
#define LIBGIT2JAVA_LIBGIT2_H

#include "jni.h"
#include "git2.h"

typedef struct {
    git_indexer_progress fetch_progress;
    size_t completed_steps;
    size_t total_steps;
    const char *path;
} progress_data;

JavaVM *global_vm;

unsigned int compute_progress(unsigned int completed, unsigned int total);

JNIEnv *get_jni_env();

void release_jni_env();

int fetch_progress(const git_indexer_progress *stats, void *payload);

void checkout_progress(const char *path, size_t cur, size_t tot, void *payload);

#endif //LIBGIT2JAVA_LIBGIT2_H
