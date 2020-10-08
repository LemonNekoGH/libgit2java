//
// Created by LemonNeko on 2020/10/8.
//
#include "git2.h"
#include "libgit2java.h"

#include <stdio.h>

int main() {
    git_libgit2_init();

    progress_data pd = {{0}};
    git_repository *cloned_repo = NULL;
    git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
    git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

    const char *url = "https://github.com/LemonNekoGH/neko-logger";
    const char *path = "neko-logger";
    int error;

    /* Set up options */
    checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
    checkout_opts.progress_cb = checkout_progress;
    checkout_opts.progress_payload = &pd;
    clone_opts.checkout_opts = checkout_opts;
    clone_opts.fetch_opts.callbacks.transfer_progress = &fetch_progress;
    clone_opts.fetch_opts.callbacks.payload = &pd;

    /* Do the clone */
    error = git_clone(&cloned_repo, url, path, &clone_opts);
    if (error) {
        const git_error *git_error = git_error_last();
        if (git_error->message) {
            printf("error: message: %s\n", git_error->message);
        } else {
            printf("error code: %d, no further information.\n", git_error->klass);
        }
    }

    git_repository_free(cloned_repo);
    git_libgit2_shutdown();

    return error;
}