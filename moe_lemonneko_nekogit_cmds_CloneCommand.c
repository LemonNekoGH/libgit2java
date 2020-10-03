#include <stdio.h>
#include "include/moe_lemonneko_nekogit_cmds_CloneCommand.h"
#include "include/git2.h"

#ifndef CLONE_DEBUG
#define CLONE_DEBUG 1
#endif

typedef struct {
    git_indexer_progress fetch_progress;
    size_t completed_steps;
    size_t total_steps;
    const char *path;
} progress_data;

static JavaVM *global_vm;
static jobject global_on_progress;

static void print_progress(const progress_data *pd) {
    unsigned int index_percent = pd->fetch_progress.total_objects > 0 ?
                                 (100 * pd->fetch_progress.indexed_objects) / pd->fetch_progress.total_objects :
                                 0;

    if (pd->fetch_progress.total_objects &&
        pd->fetch_progress.received_objects == pd->fetch_progress.total_objects) {
        printf("c-end: Resolving deltas %u/%u\r",
               pd->fetch_progress.indexed_deltas,
               pd->fetch_progress.total_deltas);
    } else {
        printf("c-end: progress %u%%\n", index_percent);
        if (global_vm) {
            JNIEnv *env;
            int status = (*global_vm)->GetEnv(global_vm, (void **) &env, JNI_VERSION_10);
            if (status != JNI_OK) {
                (*global_vm)->AttachCurrentThread(global_vm, (void **) &env, NULL);
            }
            jclass cls_on_progress_listener = (*env)->GetObjectClass(env, global_on_progress);
            jmethodID method_on_progress = (*env)->GetMethodID(env, cls_on_progress_listener, "onProgress", "(I)V");

            (*env)->CallVoidMethod(env, global_on_progress, method_on_progress, index_percent);
            (*global_vm)->DetachCurrentThread(global_vm);
        }
    }
}

static int fetch_progress(const git_indexer_progress *stats, void *payload) {
    progress_data *pd = (progress_data *) payload;
    pd->fetch_progress = *stats;
    if (CLONE_DEBUG) {
        printf("c-end: fetch_progress\n");
        print_progress(pd);
    }
    return 0;
}

static void checkout_progress(const char *path, size_t cur, size_t tot, void *payload) {
    progress_data *pd = (progress_data *) payload;
    pd->completed_steps = cur;
    pd->total_steps = tot;
    pd->path = path;
    if (CLONE_DEBUG) {
        printf("c-end: checkout_progress\n");
        print_progress(pd);
    }
}

JNIEXPORT jint JNICALL Java_moe_lemonneko_nekogit_cmds_CloneCommand_doClone(
        JNIEnv *env,
        jobject clone_command,
        jstring url,
        jstring path,
        jobject on_progress) {
    if (CLONE_DEBUG) {
        printf("===========================\n");
        printf("c-end: calling c from java.\n\n");
        printf("c-end: initializing libgit2...\n");
    }
    git_libgit2_init();

    if (CLONE_DEBUG) {
        printf("c-end: get jvm instance and save global refs.\n");
    }
    (*env)->GetJavaVM(env, &global_vm);
    global_on_progress = (*env)->NewGlobalRef(env, on_progress);

    progress_data pd = {{0}};
    git_repository *cloned_repo = NULL;
    git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
    git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

    const char *converted_url = (*env)->GetStringUTFChars(env, url, NULL);
    const char *converted_path = (*env)->GetStringUTFChars(env, path, NULL);
    int error;

    /* Set up options */
    checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
    checkout_opts.progress_cb = checkout_progress;
    checkout_opts.progress_payload = &pd;
    clone_opts.checkout_opts = checkout_opts;
    clone_opts.fetch_opts.callbacks.transfer_progress = &fetch_progress;
    clone_opts.fetch_opts.callbacks.payload = &pd;

    /* Do the clone */

    if (CLONE_DEBUG) {
        printf("c-end: do the clone\n");
        printf("c-end: url = %s\n", converted_url);
        printf("c-end: path = %s\n", converted_path);
    }

    error = git_clone(&cloned_repo, converted_url, converted_path, &clone_opts);

    if (error != 0) {
        const git_error *err = git_error_last();
        if (err) {
            printf("c-end: error code %d: %s\n", err->klass, err->message);
        } else {
            printf("c-end: error code %d: no detailed info\n", error);
        }
    } else if (cloned_repo)
        printf("c-end: freeing repository...\n");
    git_repository_free(cloned_repo);

    if (CLONE_DEBUG) {
        printf("c-end: destroying libgit2...\n");
    }

    git_libgit2_shutdown();

    if (CLONE_DEBUG) {
        printf("c-end: deleting global refs...\n");
    }
    (*env)->DeleteGlobalRef(env, global_on_progress);

    return error;
}