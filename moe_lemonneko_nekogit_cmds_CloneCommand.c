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
static jobject global_on_error;

static void call_on_progress(unsigned int progress) {
    JNIEnv *env;
    int status = (*global_vm)->GetEnv(global_vm, (void **) &env, JNI_VERSION_10);
    if (status != JNI_OK) {
        (*global_vm)->AttachCurrentThread(global_vm, (void **) &env, NULL);
    }
    jclass cls_on_progress_listener = (*env)->GetObjectClass(env, global_on_progress);
    jmethodID method_on_progress = (*env)->GetMethodID(env, cls_on_progress_listener, "onProgress", "(I)V");

    (*env)->CallVoidMethod(env, global_on_progress, method_on_progress, progress);
    (*global_vm)->DetachCurrentThread(global_vm);
}

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
        if (CLONE_DEBUG) {
            printf("c-end: progress %u%%\n", index_percent);
        }
        if (global_vm) {
            call_on_progress(index_percent);
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
        jobject on_progress,
        jobject on_error) {
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
    global_on_error = (*env)->NewGlobalRef(env, on_error);

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
        if (err && CLONE_DEBUG) {
            printf("c-end: error code %d: %s\n", err->klass, err->message);
        } else {
            printf("c-end: error code %d: no detailed info\n", error);
        }
        if (err) {
            jclass class_clone_exception = (*env)->FindClass(env, "moe/lemonneko/nekogit/exceptions/CloneException");
            jmethodID constructor_clone_exception = (*env)->GetMethodID(env, class_clone_exception, "<init>",
                                                                        "(Ljava/lang/String;)V");

            jstring arg_message = (*env)->NewStringUTF(env, err->message);
            jobject clone_exception = (*env)->NewObject(env, class_clone_exception, constructor_clone_exception,
                                                        arg_message);

            jclass class_on_error_listener = (*env)->GetObjectClass(env, global_on_error);
            jmethodID method_on_error = (*env)->GetMethodID(env, class_on_error_listener, "onError",
                                                            "(Ljava/lang/Exception;)V");
            (*env)->CallVoidMethod(env, global_on_error, method_on_error, clone_exception);

            (*env)->DeleteLocalRef(env, clone_exception);
        }
    } else if (cloned_repo) {
        printf("c-end: freeing repository...\n");
        call_on_progress(100);
    }

    git_repository_free(cloned_repo);

    if (CLONE_DEBUG) {
        printf("c-end: destroying libgit2...\n");
    }

    git_libgit2_shutdown();

    if (CLONE_DEBUG) {
        printf("c-end: deleting global refs...\n");
    }
    (*env)->DeleteGlobalRef(env, global_on_progress);
    (*env)->DeleteGlobalRef(env, global_on_error);

    return error;
}