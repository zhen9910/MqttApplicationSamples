#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// #include "logging.h"
#include "tinykube_device.h"
#include "bh_read_file.h"

#define RUNTIME_LIB_LOG_TAG "RUNTIME_LIB"

static struct wasm_runtime_and_module_status wasm_instance;

void wasm_status_reset()
{
    if (wasm_instance.runtime_heap_buf)
    {
        free(wasm_instance.runtime_heap_buf);
    }
    memset(&wasm_instance, 0, sizeof(struct wasm_runtime_and_module_status));
}

int create_wamr_runtime(uint32 heap_size)
{
    if (wasm_instance.runtime_state != WAMR_RUNTIME_NOT_CREATED) {
        printf("[%s]: wamr runtime has been created already.\n", __func__);
        return -1;
    }
    wasm_instance.runtime_heap_buf = malloc(heap_size);
    if (wasm_instance.runtime_heap_buf == NULL) {
        printf("[%s]: malloc heap buf failed.\n", __func__);
        return -1;
    }
    wasm_instance.runtime_heap_size = heap_size;

    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = wasm_instance.runtime_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = wasm_instance.runtime_heap_size;

    if (!wasm_runtime_full_init(&init_args)) {
        printf("[%s]: Init runtime environment failed.\n", __func__);
        free(wasm_instance.runtime_heap_buf);
        wasm_instance.runtime_heap_buf = NULL;
        wasm_instance.runtime_heap_size = 0;
        return -1;
    }
    wasm_instance.runtime_state = WAMR_RUNTIME_CREATED;
    printf("[%s]: Created wamr runtime with heapsize %d bytes.\n", __func__, heap_size);
    return 0;
}

int destroy_wamr_runtime()
{
    if (wasm_instance.runtime_state != WAMR_RUNTIME_CREATED) {
        printf("[%s]: wamr runtime has not been created yet.\n", __func__);
        return -1;
    }
    wasm_runtime_destroy();
    if (wasm_instance.runtime_heap_buf)
    {
        free(wasm_instance.runtime_heap_buf);
    }
    wasm_instance.runtime_heap_buf = NULL;
    wasm_instance.runtime_heap_size = 0;
    wasm_instance.runtime_state = WAMR_RUNTIME_NOT_CREATED;
    printf("Destroyed wamr runtime.\n");
    return 0;
}

int execute_wasm_app(char *wasm_module_name)
{
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

    wasm_instance.module_name = wasm_module_name;
    wasm_instance.module_state = WASM_MODULE_RUNNING;
    wasm_instance.module = module;
    wasm_instance.module_inst = module_inst;
    wasm_instance.exec_env = exec_env;
    wasm_instance.load_buffer = buffer;

    if (wasm_application_execute_main(module_inst, 0, NULL)) {
        main_result = wasm_runtime_get_wasi_exit_code(module_inst);
    }
    else {
        printf("call wasm function main failed. error: %s\n",
               wasm_runtime_get_exception(module_inst));
        goto fail;
    }

fail:
    if (exec_env)
        wasm_runtime_destroy_exec_env(exec_env);
    if (module_inst)
        wasm_runtime_deinstantiate(module_inst);
    if (module)
        wasm_runtime_unload(module);
    if (buffer)
        BH_FREE(buffer);
    destroy_wamr_runtime();
    wasm_status_reset();
    return main_result;
}

/*
 * thread start routine to create wasm runtime and execute wasm app
*/
void *runtime_controller_start_wasm_app(void *thread_args)
{
    struct wasm_runtime_thread_args *args = (struct wasm_runtime_thread_args *)thread_args;

    printf("[%s]: Thread started!\n", __func__);
    printf("[%s]: args->module_name = %s\n", __func__, args->module_name);
    printf("[%s]: args->heap_size = %d\n", __func__, args->heap_size);

    // create wasm runtime
    if (create_wamr_runtime(args->heap_size) != 0) {
        printf("[%s]: create_wamr_runtime failed.\n", __func__);
        return NULL;
    }

    // execute wasm app
    if (execute_wasm_app(args->module_name) != 0) {
        printf("[%s]: execute_wasm_app failed.\n", __func__);
        return NULL;
    }

    printf("[%s]: Thread stopped: wasm_runtime_executor()\n", __func__);
    return NULL;
}

void runtime_controller_reset_status(char *module_name)
{
    printf("[%s]: module_name = %s\n", __func__, module_name);

    if (wasm_instance.module_state == WASM_MODULE_STOPPED)
    {
        printf("[%s]: module is already stopped.\n", __func__);
        return;
    }
    wasm_status_reset();
    return;
}

void runtime_controller_get_status(struct wasm_runtime_and_module_status *instance)
{
    instance->runtime_state = wasm_instance.runtime_state;
    instance->module_name = wasm_instance.module_name;
    instance->module_state = wasm_instance.module_state;
    instance->runtime_heap_size = wasm_instance.runtime_heap_size;
    instance->starting_time = wasm_instance.starting_time;
}