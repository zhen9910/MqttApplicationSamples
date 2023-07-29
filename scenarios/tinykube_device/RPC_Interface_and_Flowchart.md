
# RPC Spec and flowchat

1. [Install wasm runtime library](#install-wasm-runtime-library)
2. [Uninstall wasm runtime library](#uninstall-wasm-runtime-library)  
3. [Add a wasm module file](#add-a-wasm-module-file)
4. [Remove a wasm module file](#remove-a-wasm-module-file)
5. [Start a wasm module](#start-a-wasm-module)
6. [Stop a wasm module](#stop-a-wasm-module)
7. [check wasm status](#check-wasm-status)

## Install wasm runtime library
```
      {
        "@type": "Command",
        "name": "InstallWasmRuntimeLib",
        "request": {
          "name": "wasmRuntimeLib",
          "schema": {
            "@type": "Object",
            "fields": [
              {
                "@type": [ "Field", "Indexed" ],
                "name": "runtimeLibName",
                "schema": "string",
                "index": 1
              },
              {
                "@type": [ "Field", "Indexed" ],
                "name": "runtimeType",
                "schema": "string",
                "index": 2
              },
              {
                "@type": [ "Field", "Indexed" ],
                "name": "runtimeLibSize",
                "schema": "integer",
                "index": 3
              },
              {
                "@type": [ "Field", "Indexed" ],
                "name": "runtimeLibContent",
                "schema": {
                  "@type": "Array",
                  "elementSchema": "integer"
                },
                "index": 4
              }
            ]
          }
        },
        "response": {
          "name": "result",
          "schema": "integer"
        }
      },

```
```mermaid
sequenceDiagram;
  participant B as AEW Controller
  participant A as Tinykube device main app

  B->>A: InstallWasmRuntimeLibRequest

  A-->>A: save lib file to local storage
  A-->>A: Load the shared library and get handler with dlopen()
  A-->>A: Obtain the function pointers to execute wasm and get status with dlsym()

  A->>B: InstallWasmRuntimeLibResponse

```

## Uninstall wasm runtime library
```
      {
        "@type": "Command",
        "name": "uninstallWasmRuntimeLib",
        "request": {
          "name": "RuntimeLibName",
          "schema": "string"
        },
        "response": {
          "name": "result",
          "schema": "integer"
        }
      },
```

```mermaid
sequenceDiagram;
  participant B as AEW Controller
  participant A as Tinykube device main app

  B->>A: UninstallWasmRuntimeLibRequest

  A-->>A: remove lib file from local storage
  A-->>A: unload the shared library and close handler with dlclose()

  A->>B: UninstallWasmRuntimeLibResponse
```

## Add a wasm module file
```
      {
        "@type": "Command",
        "name": "addWasmModule",
        "request": {
          "name": "wasmModule",
          "schema": {
            "@type": "Object",
            "fields": [
              {
                "@type": [ "Field", "Indexed" ],
                "name": "wasmModuleName",
                "schema": "string",
                "index": 1
              },
              {
                "@type": [ "Field", "Indexed" ],
                "name": "wasmModuleSize",
                "schema": "integer",
                "index": 2
              },
              {
                "@type": [ "Field", "Indexed" ],
                "name": "wasmModuleContent",
                "schema": {
                  "@type": "Array",
                  "elementSchema": "integer"
                },
                "index": 3
              }
            ]
          }
        },
        "response": {
          "name": "result",
          "schema": "integer"
        }
      },
```
```mermaid
sequenceDiagram;
  participant B as AEW Controller
  participant A as Tinykube device main app

  B->>A: addWasmModuleRequest

  A-->>A: save module file to local storage

  A->>B: addWasmModuleResponse
```

## Remove a wasm module file
```
      {
        "@type": "Command",
        "name": "removeWasmModule",
        "request": {
          "name": "wasmModuleName",
          "schema": "string"
        },
        "response": {
          "name": "result",
          "schema": "integer"
        }
      },
```

```mermaid
sequenceDiagram;
  participant B as AEW Controller
  participant A as Tinykube device main app

  B->>A: removeWasmModuleRequest

  A-->>A: remove module file from local storage

  A->>B: removeWasmModuleResponse
```

## Start a wasm module
```
      {
        "@type": "Command",
        "name": "startWasmModule",
        "request": {
          "name": "wasmModuleName",
          "schema": "string"
        },
        "response": {
          "name": "result",
          "schema": "integer"
        }
      },
```
```mermaid
sequenceDiagram;
  participant B as AEW Controller
  participant A as Tinykube device main app
  participant foo as wasm runtime thread
  
  box Tinykube Enabled Device
  participant A
  participant foo
  end

  B->>A: startWasmModuleRequest
   
  A-->>+foo: Create Thread
  foo-->>A: return
  A->>B: startWasmModuleResponse
  Note over foo: child thread to create wasm runtime and execute wasm module
  foo-->>foo: create_runtime_env
  foo-->>foo: execute_wasm_module
  foo-->>-foo: destroy_runtime_env
  
  ```

## Stop a wasm module
```
      {
        "@type": "Command",
        "name": "stopWasmModule",
        "request": {
          "name": "wasmModuleName",
          "schema": "string"
        },
        "response": {
          "name": "result",
          "schema": "integer"
        }
      },

```
```mermaid
sequenceDiagram;
  participant B as AEW Controller
  participant A as Tinykube device main app
  participant foo as wasm runtime thread
  
  box Tinykube Enabled Device
  participant A
  participant foo
  end

   
  note over foo: wasm module is running
  B->>A: stopWasmModuleRequest
  A-->>foo: Cancel Thread
  foo-->>A: exited
  A-->>A: reset Wasm status
  A->>B: stopWasmModuleResponse
  

```

## check wasm status
```
      {
        "@type": "Command",
        "name": "checkWasmStatus",
        "response": {
          "name": "wasmStatus",
          "schema": {
            "@type": "Object",
            "fields": [
              {
                "@type": [ "Field", "Indexed" ],
                "name": "runtimeLibName",
                "schema": "string",
                "index": 1
              },
              {
                "@type": [ "Field", "Indexed" ],
                "name": "wasmModuleName",
                "schema": "string",
                "index": 2
              },
              {
                "@type": [ "Field", "Indexed" ],
                "name": "runtimeState",
                "schema": "string",
                "index": 3
              },
              {
                "@type": [ "Field", "Indexed" ],
                "name": "startingTime",
                "schema": "dateTime",
                "index": 4
              },
              {
                "@type": [ "Field", "Indexed" ],
                "name": "runningTime",
                "schema": "time",
                "index": 5
              }
            ]
          }
        }
      }
```
```mermaid
sequenceDiagram;
  participant B as AEW Controller
  participant A as Tinykube device main app
  participant foo as wasm runtime thread
  
  box Tinykube Enabled Device
  participant A
  participant foo
  end

   
  note over foo: wasm module is running
  B->>A: checkWasmStatusRequest
  A-->>foo: check_status
  foo-->>A: status
  A->>B: checkWasmStatusResponse
  

```
