# KDVM (KD Virtual Machine)

KDVM is a minimal, Turing-complete, stack-based virtual machine written in pure C. It features a custom instruction set, an integrated text-to-bytecode assembler, and a modular, struct-based execution engine. 

Designed for simplicity and speed, KDVM acts as a perfect educational foundation for understanding low-level computing, compiler design, and CPU architecture.

## 🚀 Features

* **Turing Complete:** Supports arbitrary memory access (RAM) and conditional branching.
* **Stack-Based Execution:** Uses a standard LIFO stack for all arithmetic and logic evaluations.
* **Type-Punned Floating Point:** Seamlessly supports both 32-bit signed integers and IEEE-754 floating-point math using the exact same memory arrays via C union bit-casting.
* **Subroutines:** Features a dedicated Call Stack, allowing for modular functions and `CALL`/`RET` flows.
* **No Global State:** The entire VM is encapsulated inside a `VM` struct, allowing multiple independent machines to run simultaneously within the same host program.
* **Native I/O:** Built-in opcodes for reading from and writing to the standard terminal.

## 🏗️ Architecture

KDVM uses a hybrid Harvard architecture concept with three distinct memory planes:
1. **PROGRAM (1024 words):** Read-only instruction memory.
2. **STACK (256 words):** The primary scratchpad for math and variable manipulation.
3. **RAM (256 words):** Random-access memory for persistent data storage across loops and subroutines.
4. **CALL STACK (64 words):** Dedicated storage for tracking Instruction Pointer (`PC`) return addresses.

## 🛠️ Building and Running

KDVM is highly portable and can be compiled with any standard C compiler. It is specifically tested and optimized for **TCC (Tiny C Compiler)**.

### Compilation
Compile the multi-file project into a single executable:
```bash
tcc -o kdvm main.c loader.c vm.c

```

### Usage

Write your assembly instructions in a `.kdm` file, then pass it to the VM:

```bash
./kdvm my_program.kdm

```

## 📜 Instruction Set Architecture (ISA)

### Base & Stack Manipulation

* `HALT` - Stop execution cleanly.
* `POP` - Remove the top value from the stack.
* `DUP` - Duplicates the top value of the stack and pushing it.
* `SWAP` - Swaps 2 top values in the stack
* `ROT` - Rotates the top 3 values in the stack. It floats the last value to the top, and sinks the first 2 values.

### Data Types

* `PUSH <val>` - Pushes an integer onto the stack (bit-casted).
* `FPUSH <val>` - Pushes a float onto the stack (bit-casted).
* `BPUSH <val>` - Pushes a byte onto the stack (bit-casted).
* `CPUSH <val>` - Pushes a char onto the stack (bit-casted).

### Integer Arithmetic

* `ADD` / `SUB` / `MUL` / `DIV` - Pops top two values, performs math, pushes result.

### Memory Access

* `STORE` - Pops value, pops address -> Saves value to `RAM[address]`.
* `LOAD` - Pops address -> Pushes `RAM[address]` to the stack.

* `I_STORE <integer-value> <addr>` - Stores integer value directly to `RAM[addr]`.
* `F_STORE <float-value> <addr>` - Stores float value directly to `RAM[addr]`.


### Control Flow

* `JUMP <addr>` - Unconditionally jump `PC` to target address relative to sub-routine.
* `JNZ <addr>` - Pops condition. If != 0, jump `PC` to target address relative to sub-routine.
* `JZ <addr>` - Pops condition. If == 0, jump `PC` to target address relative to sub-routine.
* `CMPEQ` - Pops two values. Pushes `1` if equal, `0` otherwise.
* `CMPNEQ` - Pops two values. Pushes `1` if not equal, `0` otherwise.
* `CMPLT` - Pops two values. Pushes `1` if A < B, `0` otherwise.
* `CMPLE` - Pops two values. Pushes `1` if A <= B, `0` otherwise.
* `CMPGT` - Pops two values. Pushes `1` if A > B, `0` otherwise.
* `CMPGE` - Pops two values. Pushes `1` if A >= B, `0` otherwise.

### Subroutines & I/O

* `::<subroutine>` - is how we define a sub-routine. It is basically just an index. All sub-routine must end with a `RET`
* `CALL <subroutine>` - Push current `PC` to Call Stack, jump to target.
* `RET` - Pop address from Call Stack and return `PC` there.
* `OUT` - Pops and prints an integer to the terminal.
* `IN` - Pauses execution, waits for user integer input, pushes to stack.

The program starts with a main sub-routine and it should be named ::_global.
This is a reserved subroutine name so that the VM knows where to start executing the program.

## 💻 Example Code (`test.kdm`)

Here is an example program that uses functions and comparisons to check if 15 is less than 20:

```text
# --- MAIN PROGRAM ---

# --- SUBROUTINE:item_a Print the boolean result (1 = True, 0 = False)
::print
    OUT
    HALT
    RET

# --- SUBROUTINE: is_less_than (Index 8) ---
::is_less_than
    CMPLT   # Pushes 1 if 15 < 20
    RET     # Returns to main
    
::_global
    PUSH 15
    PUSH 20

    # Call the "::is_less_than" subroutine
    CALL is_less_than
    CALL print
    RET

```
