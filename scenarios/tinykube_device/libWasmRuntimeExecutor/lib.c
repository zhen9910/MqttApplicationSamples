#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "tinykube_device.h"
#include "bh_read_file.h"

static struct wamr_runtime_info wamr_runtime_instance;
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

#if 1
int run_wasm_module(char *wasm_module_name)
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
    wasm_module_instance[0].buffer = buffer;

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
    destroy_wamr_runtime();
    wasm_module_instance[0].status = WASM_MODULE_STOPPED;
    wasm_module_instance[0].module_name = NULL;
    wasm_module_instance[0].module = NULL;
    wasm_module_instance[0].module_inst = NULL;
    wasm_module_instance[0].exec_env = NULL;
    wasm_module_instance[0].buffer = NULL;
    return main_result;
}

#else
int run_wasm_module(char *wasm_module_name)
{
    (void)wasm_module_name;
    printf("run_wasm_module() is not implemented yet.\n");
    wasm_module_instance[0].module_name = wasm_module_name;
    destroy_wamr_runtime();
    return 0;
}

#endif
void wasm_runtime_executor(void *thread_args) {
    printf("Thread started: wasm_runtime_executor(%p)\n", thread_args);

    struct wasm_runtime_thread_args *args = (struct wasm_runtime_thread_args *)thread_args;
    wasm_module_instance[0].status = WASM_MODULE_RUNNING;
    printf("args->module_name = %s\n", args->module_name);
    printf("args->heap_size = %d\n", args->heap_size);

    // create wasm runtime
    if (create_wamr_runtime(args->heap_size) != 0) {
        printf("create_wamr_runtime failed.\n");
        return;
    }

    // run wasm module
    if (run_wasm_module(args->module_name) != 0) {
        printf("run_wasm_module failed.\n");
        return;
    }

    printf("Thread stopped: wasm_runtime_executor()\n");
}

void wasm_runtime_stop_module(void *thread_args)
{
    char *module_name = (char *)thread_args;
    printf("[%s]: module_name = %s\n", __func__, module_name);

    if (wasm_module_instance[0].status == WASM_MODULE_STOPPED)
    {
        printf("[%s]: module is already stopped.\n", __func__);
        return;
    }

    if (wasm_module_instance[0].exec_env)
        wasm_runtime_destroy_exec_env(wasm_module_instance[0].exec_env);
    if (wasm_module_instance[0].module_inst)
        wasm_runtime_deinstantiate(wasm_module_instance[0].module_inst);
    if (wasm_module_instance[0].module)
        wasm_runtime_unload(wasm_module_instance[0].module);
    if (wasm_module_instance[0].buffer)
        BH_FREE(wasm_module_instance[0].buffer);

    // wasm_runtime_destroy();
    destroy_wamr_runtime();
    wasm_module_instance[0].status = WASM_MODULE_STOPPED;
    wasm_module_instance[0].module_name = NULL;
    wasm_module_instance[0].module = NULL;
    wasm_module_instance[0].module_inst = NULL;
    wasm_module_instance[0].exec_env = NULL;
    wasm_module_instance[0].buffer = NULL;

    printf("[%s]: unloaded module_name = %s\n", __func__, module_name);

}
