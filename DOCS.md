# Docs

This library is a low-level abstraction over the GX subsystem of GSP, with support for baremetal code.

## Design goals

- Platform agnostic API, that works the same on both userland and baremetal.
- Fine grained control over low-level data structures, while maintaining a consistent environment.
- Batching of commands, which allows to execute them atomically.
- Support synchronous command execution.
- Command wrapping for ease of use.

### GX subsystem

GSP uses 2 kind of commands: GPU commands, and GX commands. GPU commands are basically data encoding memory addresses pointing to GPU MMIO registers, along with values to write to them. GX commands, instead, interact with different components of the graphics system, and provide core functionalities for graphics programming. GX commands are the focus of this library.

There exist 6 GX commands:

- **RequestDMA (DMA)**: this command initiates a DMA transfer. It's officially used for transferring data to VRAM.
- **MemoryFill (PSC0, PSC1)**: this command sets up the **PSC** units, which are the hardware component responsible for filling a region of memory with a specific value. It's officially used for implementing framebuffer clearing.
- **ProcessCommandList (P3D)**: this command will set a list of GPU commands to be processed by the GPU.
- **DisplayTransfer (PPF)**: this command sets up the **Transfer Engine**. It's officially used for converting GPU framebuffers into LCD framebuffers.
- **TextureCopy (PPF)**: this command also sets up the **Transfer Engine**. It's officially used for copying texture data.
- **FlushCacheRegions**: this command is used for flushing cache on given memory regions.

A client that wants to execute one of these commands must write the relevant command data in a specific memory region, which is shared with the GSP process. Then, the client signals GSP of the changes with an IPC command. Commands are executed asynchronously; on completion GSP writes interrupt data to the shared memory, and signals a kernel event.

Additionally, GSP allows to execute commands in batch, to halt command execution and to retrieve error codes if a command fails.

### Flaws

Unfortunately, the GX subsystem is flawed for multiple reasons, which make writing an abstraction a non-trivial task.

For one, command completion is signaled by the relevant interrupt. However, if a command fails no interrupt is signaled. This is a design flaw, and a terrible one:

- The kernel event is not triggered, meaning the client has no chance to know the command has been executed. If the client always expects a command to complete, there's a chance an invalid command will cause a client-side hang (this issue can be probably worked around by implementing a, nonethelessly complex, timeout-based system).
- The error codes are ultimately useless, because the client can't know when to check for them.
- No interrupt is signaled for `FlushCacheRegions` anyway, which needs special handling.

Moreover, if a command fails, an error flag is set in shared memory. It just so happens that the halt flag is part of the same byte in shared memory, and GSP fails to check for it by checking for equality. This implies that, if the error bit is set, GSP will never be able to detect if command handling has halted, and will always try to execute the next command. This can be worked around, as there's a different halt flag on a different location with similar behaviour. When implementing command batching an abstraction has to take care of that.

Lastly, applications would benefit from having multiple command queues, especially on the 3DS which has 2 screens. However, the shared memory region contains only 1 queue-like structure for sending GX commands. An abstraction that wants to leverage multiple command queues is therefore enforced to implement a state machine, which has questionable results in code quality.

## Initialization and cleanup

To initialize the library, simply call `ctrgxInit`. To cleanup the library, call `ctrgxExit`. An application has to call `ctrgxExit` for as many times as `ctrgxInit` has been called.

All functions are thread safe, with regard to both other threads and the internal worker thread, except for state accessors:

- `ctrgxGetIntrQueue`
- `ctrgxGetCmdQueue`
- `ctrgxGetCmdBuffer`

An application must call `ctrgxLock` to acquire exclusive access on internal state, this includes adding commands to the currently set command buffer. Once the application no longer needs to access internal state, `ctrgxUnlock` must be called to release exclusive access.

## Command buffer

"Command buffer" is, in reality, a misnomer. It's actually a queue-like structure, akin to the one exposed by GSP. The name "buffer" was chosen to differentiate it from the "command queue", which is used to reference the GSP one.

Command buffers are the central parts of this library. Each command buffer holds GX commands, which are split into **command batches**. In the most simple form, each command batch contains one command, and each batch represents that command.

Asynchronous commands require a command buffer to be run. To set a command buffer, call `ctrgxExchangeCmdBuffer`. This function returns the current command buffer. A halt is performed if flushing is not requested.

An application may setup a "static" command buffer, which uses static buffers internally, or a "dynamic" one, which is initialized through `ctrgxCmdBufferAlloc`, and offloads initialization to the library, at the cost of using heap memory. Dynamic command buffers must be freed with `ctrgxCmdBufferFree` once they're not needed anymore.

`ctrgxCmdBufferClear` resets the content of a command buffer. Commands are inserted with `ctrgxCmdBufferAdd`, while batches are marked with `ctrgxCmdBufferFinalize`, where it's possible to set a callback that will be executed on batch completion.

A finalized command buffer is a command buffer whose batches are all finalized. Command execution will fail if the currently set command buffer is not finalized.

## Flushing and halting

Once command processing has started batches are processed continuously, eg. when one batch has completed, the next one is processed, until all batches are completed. Once all batches are completed, command execution is halted, and can be restarted by calling `ctrgxFlushBufferedCommands` or, if the application has added new commands to the command buffer, by passing `true` for `exec` in `ctrgxUnlock`. `ctrgxWaitCompletion` will block until the command buffer becomes empty.

Applications are allowed to halt the execution by calling `ctrgxHalt`. If an application passes `true` for `wait`, then the function returns only after execution has halted. Otherwise, halting will be requested, and the function returns immediately. This makes it possible for applications to run code inbetween the halting process:

```c
ctrgxHalt(false); // Request halt
// Run some code
ctrgxHalt(true); // Wait until execution has halted
```

Each command batch is executed atomically, that is, all commands in a batch must be processed before the application has a chance to halt execution.

## Synchronous execution

Sometimes an application has to wait for a command to be completed to continue, for example when using `TextureCopy` when copying some data. A command may be executed with `ctrgxExecSync`: this function halts execution of the current command buffer, executes the given command, wait for completion, and then resume the execution of the command buffer.