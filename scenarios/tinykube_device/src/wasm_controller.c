/*
 * Copyright (C) 2023 Microsoft Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wasm_export.h"
#include "bh_read_file.h"

#define WASM_MODULES_PATH "./wasm-module"

enum WAMR_RUNTIME_STATUS {
    WAMR_RUNTIME_NOT_CREATED = 0,
    WAMR_RUNTIME_CREATED = 1,
    // WAMR_RUNTIME_RUNNING = 2,
    // WAMR_RUNTIME_STOPPED = 3,
};

struct wamr_runtime_info {
    char *heap_buf;
    uint32_t heap_size;
    enum WAMR_RUNTIME_STATUS status;
};

enum WASM_MODULE_STATUS {
    WASM_MODULE_EMPTY = 0,
    WASM_MODULE_STOPPED = 1,
    WASM_MODULE_RUNNING = 2,
};

struct wasm_module_info {
    char *module_name;
    enum WASM_MODULE_STATUS status;
    wasm_module_t module;
    wasm_module_inst_t module_inst;
    wasm_exec_env_t exec_env;
};

static struct wamr_runtime_info wamr_runtime_instance;

#define MAX_WASM_MODULE_NUM 1
static struct wasm_module_info wasm_module_instance[MAX_WASM_MODULE_NUM];

int create_wamr_runtime(uint32 heap_size)
{
    if (wamr_runtime_instance.status != WAMR_RUNTIME_NOT_CREATED) {
        printf("wamr runtime has been created already.\n");
        return -1;
    }
    wamr_runtime_instance.heap_buf = malloc(heap_size);
    if (wamr_runtime_instance.heap_buf == NULL) {
        printf("malloc heap buf failed.\n");
        return -1;
    }
    wamr_runtime_instance.heap_size = heap_size;

    // static char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = wamr_runtime_instance.heap_buf;
    init_args.mem_alloc_option.pool.heap_size = heap_size;

    if (!wasm_runtime_full_init(&init_args)) {
        printf("Init runtime environment failed.\n");
        free(wamr_runtime_instance.heap_buf);
        wamr_runtime_instance.heap_buf = NULL;
        wamr_runtime_instance.heap_size = 0;
        return -1;
    }
    wamr_runtime_instance.status = WAMR_RUNTIME_CREATED;
    printf("\n\n------ Created wamr runtime with heapsize %d bytes -------.\n\n", heap_size);
    return 0;
}

int destroy_wamr_runtime()
{
    if (wamr_runtime_instance.status != WAMR_RUNTIME_CREATED) {
        printf("wamr runtime has not been created yet.\n");
        return -1;
    }
    wasm_runtime_destroy();
    free(wamr_runtime_instance.heap_buf);
    wamr_runtime_instance.heap_buf = NULL;
    wamr_runtime_instance.heap_size = 0;
    wamr_runtime_instance.status = WAMR_RUNTIME_NOT_CREATED;
    printf("\n\n------ Destroyed wamr runtime -------.\n\n");
    return 0;
}

int add_wasm_module(char *wasm_module_name, int32_t *wasm_module_content, size_t wasm_module_size)
{
    //file name  = WASM_MODULES_PATH + "/" + wasm_module_name
    char full_file_name[256];
    int result = snprintf(full_file_name, sizeof(full_file_name), "%s/%s", WASM_MODULES_PATH, wasm_module_name);
    if (result < 0 || (size_t)result >= sizeof(full_file_name)) {
        printf("snprintf failed.\n");
        return -1;
    }
    printf("full_file_name = %s\n", full_file_name);
    // write wasm_module_content to full_file_name as binary file
    FILE *fp = fopen(full_file_name, "wb");
    if (fp == NULL) {
        printf("fopen failed.\n");
        return -1;
    }

    char *buffer = malloc(wasm_module_size);
    if (buffer == NULL) {
        printf("malloc failed.\n");
        fclose(fp);
        return -1;
    }
    for (size_t i = 0; i < wasm_module_size; i++) {
        buffer[i] = wasm_module_content[i] & 0xff;
    }
    size_t write_size = fwrite(buffer, 1, wasm_module_size, fp);
    if (write_size != wasm_module_size) {
        printf("fwrite failed.\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    printf("write wasm module to file %s successfully.\n", full_file_name);

    return 0;
}

int remove_wasm_module(char *wasm_module_name)
{
    //file name  = WASM_MODULES_PATH + "/" + wasm_module_name
    char full_file_name[256];
    int result = snprintf(full_file_name, sizeof(full_file_name), "%s/%s", WASM_MODULES_PATH, wasm_module_name);
    if (result < 0 || (size_t)result >= sizeof(full_file_name)) {
        printf("snprintf failed.\n");
        return -1;
    }
    printf("full_file_name = %s\n", full_file_name);
    // remove full_file_name
    int ret = remove(full_file_name);
    if (ret != 0) {
        printf("remove file %s failed.\n", full_file_name);
        return -1;
    }
    printf("remove file %s successfully.\n", full_file_name);

    return 0;
}

int start_wasm_module(char *wasm_module_name)
{
    // static char global_heap_buf[512 * 1024];
    char *buffer, error_buf[128];
    int main_result = 1;

    wasm_module_t module = NULL;
    wasm_module_inst_t module_inst = NULL;
    wasm_exec_env_t exec_env = NULL;
    uint32 buf_size, stack_size = 8092, heap_size = 8092;

    char full_file_name[256];
    int result = snprintf(full_file_name, sizeof(full_file_name), "%s/%s", WASM_MODULES_PATH, wasm_module_name);
    if (result < 0 || (size_t)result >= sizeof(full_file_name)) {
        printf("snprintf failed.\n");
        return -1;
    }
    printf("full_file_name = %s\n", full_file_name);

    if (full_file_name == NULL) {
        printf("No wasm file specified.\n");
        return 0;
    }

    buffer = bh_read_file_to_buffer(full_file_name, &buf_size);

    if (!buffer) {
        printf("Open wasm app file [%s] failed.\n", full_file_name);
        goto fail;
    }

    module = wasm_runtime_load(buffer, buf_size, error_buf, sizeof(error_buf));
    if (!module) {
        printf("Load wasm module failed. error: %s\n", error_buf);
        goto fail;
    }

    const char *wasi_dir = ".";
    wasm_runtime_set_wasi_args_ex(module, &wasi_dir, 1, NULL, 0, NULL, 0, NULL,
                                  0, 0, 1, 2);

    module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                           error_buf, sizeof(error_buf));

    if (!module_inst) {
        printf("Instantiate wasm module failed. error: %s\n", error_buf);
        goto fail;
    }

    exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
    if (!exec_env) {
        printf("Create wasm execution environment failed.\n");
        goto fail;
    }

    if (wasm_application_execute_main(module_inst, 0, NULL)) {
        main_result = wasm_runtime_get_wasi_exit_code(module_inst);
    }
    else {
        printf("call wasm function main failed. error: %s\n",
               wasm_runtime_get_exception(module_inst));
        goto fail;
    }

    wasm_module_instance[0].module_name = wasm_module_name;
    wasm_module_instance[0].status = WASM_MODULE_RUNNING;
    wasm_module_instance[0].module = module;
    wasm_module_instance[0].module_inst = module_inst;
    wasm_module_instance[0].exec_env = exec_env;

    if (buffer)
        BH_FREE(buffer);

    return 0;

fail:
    if (exec_env)
        wasm_runtime_destroy_exec_env(exec_env);
    if (module_inst)
        wasm_runtime_deinstantiate(module_inst);
    if (module)
        wasm_runtime_unload(module);
    if (buffer)
        BH_FREE(buffer);
    // wasm_runtime_destroy();
    // printf(" wasm_runtime_destroy().\n");
    return main_result;
}

int stop_wasm_module(char *wasm_module_name)
{
    wasm_module_instance[0].status = WASM_MODULE_STOPPED;
    wasm_runtime_destroy_exec_env(wasm_module_instance[0].exec_env);
    wasm_runtime_deinstantiate(wasm_module_instance[0].module_inst);
    wasm_runtime_unload(wasm_module_instance[0].module);

    printf("stop_wasm_module().\n");
    return 0;
}
