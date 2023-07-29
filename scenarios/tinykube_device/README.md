# setup
To generate the c files to handle the protobuf payload, install protobuf-c-compiler and libprotobuf-dev. Note that you only need these to generate the files, running the sample only requires the libprotobuf-c-dev package.
```bash
sudo apt-get install protobuf-c-compiler libprotobuf-dev
```

Then, to generate the files, run:
```bash
# from the root folder
protoc-c --c_out=./scenarios/tinykube_device/protobuf --proto_path=./scenarios/tinykube_device/protobuf StartWasmModuleCommandRequest.proto StartWasmModuleCommandResponse.proto DestroyWamrRuntimeCommandResponse.proto
```
# flowchat

```mermaid
sequenceDiagram;
  participant B as AEW Controller
  box Tinykube enabled device
  participant A as Tinykube device main app
  participant foo as wasm runtime thread
  end
  B->>A: Start_Wasm_App_Request
  A->>+foo: Create Thread
  A-->>B: Start_Wasm_App_Response
  Note over foo: child thread to create wasm runtime and execute wasm module
  foo-->>foo: create_runtime_env
  foo-->>foo: execute_wasm_module
  B->>A: Check_Wasm_Status_Request
  A-->>B: Check_Wasm_Status_Response
  foo-->>foo: destroy_runtime_env
  foo-->>-A: Completed
  
```

```mermaid
sequenceDiagram;
  participant B as AEW Controller
  participant A as Tinykube device main app

  B->>A: Start_Wasm_App_Request
  create participant foo as wasm runtime thread
  box Tinykube Enabled Device
  participant A
  participant foo
  end

  A-->>+foo: Create Thread
  A->>B: Start_Wasm_App_Response
  Note over foo: child thread to create wasm runtime and execute wasm module
  foo-->>foo: create_runtime_env
  foo-->>foo: execute_wasm_module
  B->>A: Check_Wasm_Status_Request
  A->>B: Check_Wasm_Status_Response
  foo-->>-foo: destroy_runtime_env
  destroy foo
  foo-->>A: Completed

```