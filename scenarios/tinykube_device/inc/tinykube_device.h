#ifndef __TINYKUBE_COMMON_H__
#define __TINYKUBE_COMMON_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wasm_export.h"

#define WASM_MODULES_PATH "./wasm-module"

enum WASM_RUNTIME_STATE {
    WAMR_RUNTIME_NOT_CREATED = 0,
    WAMR_RUNTIME_CREATED = 1,
};

enum WASM_MODULE_STATE {
    WASM_MODULE_STOPPED = 0,
    WASM_MODULE_RUNNING = 1,
};

struct wasm_runtime_and_module_status {
    enum WASM_RUNTIME_STATE runtime_state;
    enum WASM_MODULE_STATE module_state;
    char *runtime_lib_name;
    char *module_name;
    time_t starting_time;
    char *runtime_heap_buf;
    uint32_t runtime_heap_size;
    wasm_module_t module;
    wasm_module_inst_t module_inst;
    wasm_exec_env_t exec_env;
    char *load_buffer;
};

struct wasm_runtime_thread_args {
    unsigned heap_size;
    char *module_name;
};

#define MAX_WASM_MODULE_NUM 1
#define MAX_WASM_RUNTIME_NUM 1

#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

typedef void* (*ExecutorHandler)(void *);
typedef void* (*ResetStatusHandler)(void *);
typedef void* (*GetModuleStatusHandler)(void *);

int create_wamr_runtime(uint32_t heap_size);
int destroy_wamr_runtime();
int add_wasm_module(char *wasm_module_name, int32_t *wasm_module_content, size_t wasm_module_size);
int remove_wasm_module(char *wasm_module_name);
int start_wasm_module(char *wasm_module_name);
int stop_wasm_module(char *wasm_module_name);

int start_wasm_module_v2(char *wasm_module_name);
int stop_wasm_module_v2(char *wasm_module_name);

int open_runtime_lib();
#endif /* __TINYKUBE_COMMON_H__ */
