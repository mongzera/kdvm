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
* `PUSH <val>` - Push an integer onto the stack.
* `POP` - Remove the top value from the stack.

### Integer Arithmetic

* `ADD` / `SUB` / `MUL` / `DIV` - Pops top two values, performs math, pushes result.

### Floating Point

* `FPUSH <val>` - Pushes a float onto the stack (bit-casted).
* `FADD` / `FSUB` / `FMUL` / `FDIV` - Floating point arithmetic.

### Memory Access

* `STORE` - Pops value, pops address -> Saves value to `RAM[address]`.
* `LOAD` - Pops address -> Pushes `RAM[address]` to the stack.

### Control Flow

* `JUMP <addr>` - Unconditionally jump `PC` to target address.
* `JIF <addr>` - Pops condition. If != 0, jump `PC` to target address.
* `CMPEQ` - Pops two values. Pushes `1` if equal, `0` otherwise.
* `CMPLT` - Pops two values. Pushes `1` if A < B, `0` otherwise.

### Subroutines & I/O

* `CALL <addr>` - Push current `PC` to Call Stack, jump to target.
* `RET` - Pop address from Call Stack and return `PC` there.
* `OUT` - Pops and prints an integer to the terminal.
* `FOUT` - Pops and prints a float to the terminal.
* `IN` - Pauses execution, waits for user integer input, pushes to stack.

## 💻 Example Code (`test.kdm`)

Here is an example program that uses functions and comparisons to check if 15 is less than 20:

```text
# --- MAIN PROGRAM ---
PUSH 15
PUSH 20

# Call the "is_less_than" subroutine (Instruction Index 8)
CALL 8

# Print the boolean result (1 = True, 0 = False)
OUT
HALT

# --- SUBROUTINE: is_less_than (Index 8) ---
CMPLT   # Pushes 1 if 15 < 20
RET     # Returns to main

```

```

```
