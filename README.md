# GRRVM (Gamat-Ruyeras-Robinson Virtual Machine)

GRRVM is a minimal, Turing-complete, stack-based virtual machine written in pure C. It features a custom instruction set, an integrated text-to-bytecode assembler, and a modular, struct-based execution engine.

Designed for simplicity and speed, GRRVM acts as a perfect educational foundation for understanding low-level computing, compiler design, and CPU architecture.

## 🚀 Features

* **Turing Complete:** Supports arbitrary memory access (RAM) and conditional branching.
* **Stack-Based Execution:** Uses a standard LIFO stack for all arithmetic and logic evaluations.
* **Type-Punned Floating Point:** Seamlessly supports both 32-bit signed integers and IEEE-754 floating-point math using the exact same memory arrays via C union bit-casting.
* **Subroutines:** Features a dedicated Call Stack, allowing for modular functions and `CALL`/`RET` flows.
* **No Global State:** The entire VM is encapsulated inside a `VM` struct, allowing multiple independent machines to run simultaneously within the same host program.
* **Variables:** The VM supports variables localized within subroutines.
* **Native I/O:** Built-in opcodes for reading from and writing to the standard terminal.

---

## 🏗️ Architecture

GRRVM uses a hybrid Harvard architecture concept with three distinct memory planes:

1. **PROGRAM (1024 words):** Read-only instruction memory.
2. **STACK (256 words):** The primary scratchpad for math and variable manipulation.
3. **RAM (1024 words):** Random-access memory for persistent data storage across loops and subroutines. This consists of `GLOBAL[256]`, `HEAP[384]`, and `STACK[384]` = 1024 words. It auto-sizes itself depending on the RAM specified.
4. **CALL STACK (64 words):** Dedicated storage for tracking Instruction Pointer (`PC`) return addresses.
5. **STACK FRAME (STACK_RAM_SIZE):** For a localized memory allocation of each subroutine. This is allocated in the RAM stack, not the `VM_STACK`.

---

## Installation & Setup for GCC-ARM-NONE-EABI on Linux

```
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
git clone https://github.com/raspberrypi/pico-sdk.git ~/.pico-sdk
export PICO_SDK_PATH=~/.pico-sdk
wget https://raw.githubusercontent.com/raspberrypi/pico-sdk/master/external/pico_sdk_import.cmake
export PICO_SDK_PATH=~/pico-sdk

mkdir -p build && cd build
cmake ..
make -j$(nproc)

```

## 🛠️ Building and Running

GRRVM is highly portable and can be compiled with any standard C compiler. It is specifically tested and optimized for **TCC (Tiny C Compiler)**.
However it now supports GCC as well.

### Compilation

Compile the multi-file project into a single executable using the build script:

```bash
./build.sh # for TCC
./build_gcc.sh #for GCC


# for Raspberry Pico Compilation
# only run this once
mkdir -p build && cd build #if no /build folder yet
cmake ..


# to build for Raspberry Pi Pico
make -j$(nproc)

```

### Usage

Write your assembly instructions in a `.grr` text file, then pass it to the VM (which compiles and executes it as a `.grrm` binary representation):

```bash
./grrvm /tests/<testfile>.grr

```

---

## 📜 Instruction Set Architecture (ISA)

### Base & Stack Manipulation

* `HALT` - Stop execution cleanly.
* `POP` - Remove the top value from the stack.
* `DUP` - Duplicates the top value of the stack and pushes it.
* `SWAP` - Swaps the top two values in the stack.
* `ROT` - Rotates the top three values in the stack (floats the third value to the top, sinking the first two).

## Data Types & Memory

### Primitive Data Types
|DATA TYPE | SIZE (bit length)|
| --- | --- |
|`BYTE`|8|
|`CHAR`|16|
|`INT`|32|
|`FLOAT`|32|

### Push Data to Local Stack
| INSTRUCTION | DESCRITION | USAGE |
|---|---|---|
|`PUSH`|Pushes an integer onto the stack (bit-casted). |`PUSH <val>` |
|`FPUSH`| Pushes a float onto the stack (bit-casted). |`FPUSH <val>`|
|`BPUSH`| Pushes a byte onto the stack (bit-casted). |`BPUSH <val>`|
|`CPUSH`| Pushes a char onto the stack (bit-casted). |`CPUSH <val>`|

### Arithmetic & Type Promotion

GRRVM features dynamic **Implicit Type Promotion**. When performing math operations on mixed data types, the VM automatically "levels up" the narrower type to match the wider type to prevent precision loss.

> **Promotion Hierarchy:** `BYTE` → `CHAR` → `INT` → `FLOAT`


* `ADD` / `SUB` / `MUL` / `DIV` - Pops the top two values, determines their highest common type, performs the math, and pushes the correctly typed result back onto the stack.
*(Example: If you `ADD` an `INT` and a `FLOAT`, the VM dynamically casts the integer to a float, performs floating-point addition, and pushes a `FLOAT` result.)*

### Memory Access

**Global Scope Store**

* `STORE` - Pops value, pops address -> Saves value to `RAM[address]`.
* `ISTORE <integer-value> <addr>` - Stores integer value directly to `RAM[addr]`.
* `FSTORE <float-value> <addr>` - Stores float value directly to `RAM[addr]`.
* `CSTORE <char-value> <addr>` - Stores char value directly to `RAM[addr]`.
* `BSTORE <byte-value> <addr>` - Stores byte value directly to `RAM[addr]`.
* `LOAD` - Pops address -> Pushes the value `RAM[address]` to the stack.

**Local Scope Store**

* `STORE_L $<var_name>` - Pops value off stack, then stores the value to a named variable.
* `ISTORE_L <integer-value> $<var_name>` - Stores a local integer value to a named variable.
* `FSTORE_L <float-value> $<var_name>` - Stores a local float value to a named variable.
* `CSTORE_L <char-value> $<var_name>` - Stores a local char value to a named variable.
* `BSTORE_L <byte-value> $<var_name>` - Stores a local byte value to a named variable.
* `LOAD_L $<var_name>` - Resolves local variable offset -> Pushes the value at `RAM[sfp + offset]` to the stack.

**Heap Allocation**
* `H_ALLOC` - Pops 1 value off the stack [size], then it pushes the address of that allocated memory.
* `H_FREE`  - Pops 1 value off the stack [address], this frees the previously allocated memory to the VM to be allocated in the future.

### Control Flow

* `JUMP <addr>` - Unconditionally jump `PC` to target address relative to the subroutine.
* `JNZ <addr>` - Pops condition. If != 0, jump `PC` to target address relative to the subroutine.
* `JZ <addr>` - Pops condition. If == 0, jump `PC` to target address relative to the subroutine.
* `CMPEQ` - Pops two values. Pushes `1` if equal, `0` otherwise.
* `CMPNEQ` - Pops two values. Pushes `1` if not equal, `0` otherwise.
* `CMPLT` - Pops two values. Pushes `1` if A < B, `0` otherwise.
* `CMPLE` - Pops two values. Pushes `1` if A <= B, `0` otherwise.
* `CMPGT` - Pops two values. Pushes `1` if A > B, `0` otherwise.
* `CMPGE` - Pops two values. Pushes `1` if A >= B, `0` otherwise.

### Subroutines & I/O

* `::<subroutine>` - Defines a subroutine index. All subroutines must end with a `RET`.
* `CALL <subroutine>` - Push current `PC` to Call Stack, jump to target.
* `RET` - Pop address from Call Stack and return `PC` there.
* `OUT` - Pops and prints the data to the terminal (automatically distinguishes types).
* `IN <type>` - Pauses execution, waits for user input, and pushes it to the stack.
* *Types:* `[ INT = 0, FLOAT = 1, CHAR = 2, BYTE = 3 ]`

### Special Subroutines [IMPORTANT]
* `::_global` - This is a reserved subroutine name so the VM knows where to begin execution. All user-defined subroutines must be defined before this line since the parser is single-pass.
* `::_const_data` - This is a reserved subroutine where you can declare static constant variables or arrays. Defined before `::_global`.
* `::_data` - This is a reserved subroutine where you can declare static mutable variables or arrays. Defined before `::_global`.

**How to use**

```
::_const_data         # this memory is immutable, will throw an error when modified during runtime
  byte x 10           # size is 8-bit
  half y 15           # size is 16-bit
  word z 1000000000   # size is 32-bit
  word heights 156, 153, 167, 170 # array of 32-bit values
  byte msg "Hello World\0"  # string

::_data         # this memory is mutable, data can be modified during runtime
  byte x 10           # size is 8-bit
  half y 15           # size is 16-bit
  word z 1000000000   # size is 32-bit
  word heights 156, 153, 167, 170 # array of 32-bit values
  byte msg "Hello World\0"  # string

::_global
  # your program here
```

---

## 💻 Example Code (`tests/test_fib2.grr`)

Here is an example program calculating the nth Fibonacci number to demonstrate loops, memory access, and subroutines using the global address space:

```text
# ---------------------------------------------------------
# Subroutine: fib_logic
# ---------------------------------------------------------
::fib_logic
    # 1. Loop Condition (n == 0?)
    DUP             # 0  | Stack: [n, n]
    PUSH 0          # 1  | Stack: [n, n, 0]
    CMPEQ           # 3  | Stack: [n, n==0]
    JNZ 29          # 4  | Jump to POP at offset 29

    # 2. Calculate sum = a + b
    PUSH 0          # 6  | Addr 0
    LOAD            # 8  | Stack: [n, a]
    PUSH 1          # 9  | Addr 1
    LOAD            # 11 | Stack: [n, a, b]
    ADD             # 12 | Stack: [n, sum]

    # 3. Move b to a (RAM[0] = RAM[1])
    # We do this FIRST before overwriting RAM[1]
    PUSH 1          # 13 | Addr 1
    LOAD            # 15 | Stack: [n, sum, b]
    PUSH 0          # 16 | Addr 0
    SWAP            # 18 | Stack: [n, sum, 0, b]
    STORE           # 19 | RAM[0] = b. Stack: [n, sum]

    # 4. Store sum to b (RAM[1] = sum)
    PUSH 1          # 20 | Addr 1
    SWAP            # 22 | Stack: [n, 1, sum]
    STORE           # 23 | RAM[1] = sum. Stack: [n]

    # 5. Decrement n & Loop
    PUSH 1          # 24 | Stack: [n, 1]
    SUB             # 26 | Stack: [n-1]
    JUMP 0          # 27 | Loop back to 0

# ---------------------------------------------------------
# Exit Scope
# ---------------------------------------------------------
    POP             # 29 | Clean up the 0
    RET             # 30 | Return to caller

# ---------------------------------------------------------
# Main Entry Point
# ---------------------------------------------------------
::_global
    # 1. Target Fibonacci Number (type any integer)
    IN 0

    # 2. Initialize RAM[0] = 0 (a)
    PUSH 0
    PUSH 0
    STORE

    # 3. Initialize RAM[1] = 1 (b)
    PUSH 1
    PUSH 1
    STORE

    # 4. Run loop
    CALL fib_logic

    # 5. Output Result
    # If n=5, RAM[0] contains the 5th number (5)
    PUSH 0
    LOAD
    OUT
    HALT

```
