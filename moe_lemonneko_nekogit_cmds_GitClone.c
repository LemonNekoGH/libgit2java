#include <stdio.h>
#include <string.h>

#include "include/moe_lemonneko_nekogit_cmds_GitClone.h"
#include "include/git2.h"
#include "include/libgit2java.h"

#ifndef CLONE_DEBUG
#define CLONE_DEBUG 0
#endif

static jobject global_fetch_progress;
static jobject global_checkout_progress;
static jobject global_handle_error;

static void call_on_progress(
        jobject cb,
        char *signature,
        unsigned int received_progress,
        unsigned int index_progress,
        unsigned int checkout_progress) {
    // null check
    if (!(global_fetch_progress && global_checkout_progress)) {
        printf("error:\nglobal refs is null.\n");
        return;
    }

    JNIEnv *env = get_jni_env();

    if (env) {
        jclass cls_on_progress_listener = (*env)->GetObjectClass(env, cb);
        jmethodID method_on_progress = (*env)->GetMethodID(env, cls_on_progress_listener, "progress", signature);

        if (strcmp(signature, "(I)V") == 0) {
            (*env)->CallVoidMethod(env, global_fetch_progress, method_on_progress, checkout_progress);
        } else if (strcmp(signature, "(II)V") == 0) {
            (*env)->CallVoidMethod(env, global_fetch_progress, method_on_progress, received_progress, index_progress);
        }
    } else {
        printf("cannot get jni env.\n");
    }

    release_jni_env();
}

void call_handle_error(char *message) {
    // null check
    if (!message || !global_handle_error) {
        printf("error:\nmessage argument or global refs is null.\n");
        return;
    }

    JNIEnv *env = get_jni_env();

    if (env) {
        // construct the exception object
        jclass class_clone_exception = (*env)->FindClass(env, "moe/lemonneko/nekogit/exceptions/CloneException");
        jmethodID constructor_clone_exception = (*env)->GetMethodID(env, class_clone_exception, "<init>",
                                                                    "(Ljava/lang/String;)V");
        jstring exception_message = (*env)->NewStringUTF(env, message);
        jobject obj_clone_exception = (*env)->NewObject(env, class_clone_exception, constructor_clone_exception,
                                                        exception_message);
        // call handle error callback
        jclass class_call_back = (*env)->GetObjectClass(env, global_handle_error);
        jmethodID method_handle_error = (*env)->GetMethodID(env, class_call_back, "handleError",
                                                            "(Ljava/lang/Throwable;)V");
        (*env)->CallVoidMethod(env, global_handle_error, method_handle_error, obj_clone_exception);
        // please do not delete local ref.
    } else {
        printf("cannot get jni env\n");
    }

    release_jni_env();
}

int fetch_progress(const git_indexer_progress *stats, void *payload) {
    unsigned int receive_progress = compute_progress(stats->received_objects, stats->total_objects);
    unsigned int index_progress = compute_progress(stats->indexed_objects, stats->total_objects);
    call_on_progress(global_fetch_progress, "(II)V", receive_progress, index_progress, 0);
    return 0;
}

void checkout_progress(const char *path, size_t cur, size_t tot, void *payload) {
    unsigned int checkout_progress = compute_progress(cur, tot);
    call_on_progress(global_checkout_progress, "(I)V", 0, 0, checkout_progress);
}

JNIEXPORT void JNICALL Java_moe_lemonneko_nekogit_cmds_GitClone_doClone(
        JNIEnv *env,
        jclass class_git_clone,
        jstring url,
        jstring path,
        jobject fetch_cb,
        jobject checkout_cb,
        jobject error_cb) {
    // init libgit2
    git_libgit2_init();
    // get jvm
    if (!global_vm) {
        (*env)->GetJavaVM(env, &global_vm);
    }
    // save global ref
    global_fetch_progress = (*env)->NewGlobalRef(env, fetch_cb);
    global_checkout_progress = (*env)->NewGlobalRef(env, checkout_cb);
    global_handle_error = (*env)->NewGlobalRef(env, error_cb);

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
    error = git_clone(&cloned_repo, converted_url, converted_path, &clone_opts);

    if (error != 0) {
        const git_error *err = git_error_last();

        if (err) {
            call_handle_error(err->message);
        }
    } else if (cloned_repo) {
        git_repository_free(cloned_repo);
    }

    git_libgit2_shutdown();

    (*env)->DeleteGlobalRef(env, global_fetch_progress);
    (*env)->DeleteGlobalRef(env, global_checkout_progress);
    (*env)->DeleteGlobalRef(env, global_handle_error);
}