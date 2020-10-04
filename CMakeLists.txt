CMAKE_MINIMUM_REQUIRED(VERSION 3.15)

project(libgit2java)

if (APPLE)
    set(JNI_MD ${PROJECT_SOURCE_DIR}/include/jni/darwin/)
elseif (WIN32)
    set(JNI_MD ${PROJECT_SOURCE_DIR}/include/jni/mingw/)
elseif (UNIX)
    set(JNI_MD ${PROJECT_SOURCE_DIR}/include/jni/linux/)
endif ()

include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/jni
        ${JNI_MD}
        ${PROJECT_SOURCE_DIR}/include/git2
        ${PROJECT_SOURCE_DIR}/include/git2/sys
)

add_library(
        git2java SHARED
        ${PROJECT_SOURCE_DIR}/moe_lemonneko_nekogit_cmds_CloneCommand.c
)

if (APPLE)
    target_link_libraries(git2java ${PROJECT_SOURCE_DIR}/lib/libgit2.1.0.1.dylib)
elseif (WIN32)
    target_link_libraries(git2java ${PROJECT_SOURCE_DIR}/lib/libgit2.dll)
elseif (UNIX)
    target_link_libraries(git2java ${PROJECT_SOURCE_DIR}/lib/libgit2.so)
endif ()