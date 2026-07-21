import struct
import sys
from typing import Dict, Optional

# ── VM Constants & Opcodes (Mapped exactly to grrvm.h) ────────────────────
VM_PROGRAM_MEM = 1024

class Opcode:
    # Memory & Stack (OPT_MEMSTACK = 0x00)
    HALT     = 0x00
    PUSH     = 0x01
    POP      = 0x02
    PEEK     = 0x03
    DUP      = 0x04
    SWAP     = 0x05
    ROT      = 0x06
    FPUSH    = 0x07
    CPUSH    = 0x08
    BPUSH    = 0x09

    # Integer Arithmetic (OPT_IMATH = 0x10)
    ADD      = 0x10
    SUB      = 0x11
    MUL      = 0x12
    DIV      = 0x13
    MOD      = 0x14
    INC      = 0x15
    DEC      = 0x16

    # Memory RAM (OPT_MEM = 0x20)
    STORE    = 0x20
    LOAD     = 0x21
    MSET     = 0x22
    ISTORE   = 0x23
    FSTORE   = 0x24
    CSTORE   = 0x25
    BSTORE   = 0x26
    HALLOC   = 0x27
    HFREE    = 0x28
    ISTORE_L = 0x2A
    FSTORE_L = 0x2B
    CSTORE_L = 0x2C
    BSTORE_L = 0x2D
    LOAD_L   = 0x2E
    STORE_L  = 0x2F

    # Control Flow (OPT_CONTROL = 0x30)
    JUMP     = 0x30
    JNZ      = 0x31
    JZ       = 0x32
    CMPEQ    = 0x33
    CMPNEQ   = 0x34
    CMPLT    = 0x35
    CMPLE    = 0x36
    CMPGT    = 0x37
    CMPGE    = 0x38
    CALL     = 0x3A
    RET      = 0x3B

    # I/O (OPT_IO = 0x40)
    OUT      = 0x40
    OUT_LN   = 0x41
    FOUT     = 0x42
    FOUT_LN  = 0x43
    IN       = 0x44

    # Syscall (OPT_SYSCALL = 0x60)
    SYS_READ  = 0x61
    SYS_WRITE = 0x62

# ── Data Structures ───────────────────────────────────────────────────────

class SubroutineCtx:
    def __init__(self, name: str, program_line: int):
        self.name: str = name
        self.program_line: int = program_line
        self.local_var_map: Dict[str, int] = {}
        self.local_var_count: int = 0

    def get_create_local_var(self, var_name: str) -> int:
        if var_name not in self.local_var_map:
            offset = self.local_var_count
            self.local_var_map[var_name] = offset
            self.local_var_count += 1
            return offset
        return self.local_var_map[var_name]

class Assembler:
    def __init__(self, mem_size: int = VM_PROGRAM_MEM):
        self.program: list[int] = []
        self.max_size: int = mem_size
        self._global_start: int = 0

    def emit(self, val: int):
        if len(self.program) >= self.max_size:
            raise MemoryError("Program size exceeds VM capacity.")
        self.program.append(int(val) & 0xFFFFFFFF)

# ── Parsing Helpers ───────────────────────────────────────────────────────

class ParseError(Exception):
    pass

def float_to_int32_bits(value: float) -> int:
    packed = struct.pack('=f', value)
    return struct.unpack('=i', packed)[0]

def to_int8_bits(value: int) -> int:
    packed = struct.pack('=b', value & 0xFF)
    return struct.unpack('=b', packed)[0]

class LineCursor:
    def __init__(self, text: str):
        self.text = text.lstrip()

    def is_empty(self) -> bool:
        return len(self.text) == 0 or self.text.startswith('#')

    def next_word(self) -> Optional[str]:
        self.text = self.text.lstrip()
        if not self.text:
            return None
        parts = self.text.split(maxsplit=1)
        word = parts[0]
        self.text = parts[1] if len(parts) > 1 else ""
        return word

    def next_int(self) -> int:
        word = self.next_word()
        if word is None: raise ParseError("Expected integer.")
        try: return int(word, 0)
        except ValueError: raise ParseError(f"Expected integer, got '{word}'.")

    def next_float(self) -> float:
        word = self.next_word()
        if word is None: raise ParseError("Expected float.")
        try: return float(word)
        except ValueError: raise ParseError(f"Expected float, got '{word}'.")

    def next_char_literal(self) -> int:
        self.text = self.text.lstrip()
        if not self.text.startswith("'"): raise ParseError("Expected opening quote.")
        self.text = self.text[1:]
        if self.text.startswith("'"):
            val = 0
            self.text = self.text[1:]
        else:
            val = ord(self.text[0])
            self.text = self.text[1:]
            if not self.text.startswith("'"): raise ParseError("Expected closing quote.")
            self.text = self.text[1:]
        return val

# ── Emitters ──────────────────────────────────────────────────────────────

def emit_store(asm: Assembler, opcode: int, cursor: LineCursor):
    if opcode in (Opcode.ISTORE, Opcode.BSTORE):
        asm.emit(opcode); asm.emit(cursor.next_int()); asm.emit(cursor.next_int())
    elif opcode == Opcode.FSTORE:
        asm.emit(opcode); asm.emit(float_to_int32_bits(cursor.next_float())); asm.emit(cursor.next_int())
    elif opcode == Opcode.CSTORE:
        asm.emit(opcode); asm.emit(cursor.next_char_literal()); asm.emit(cursor.next_int())

def emit_store_load_local(asm: Assembler, opcode: int, cursor: LineCursor, sub: SubroutineCtx):
    if opcode in (Opcode.ISTORE_L, Opcode.BSTORE_L):
        val = cursor.next_int()
        var_name = cursor.next_word()
        if not var_name: raise ParseError("Expected variable name.")
        asm.emit(opcode); asm.emit(val); asm.emit(sub.get_create_local_var(var_name))
    elif opcode == Opcode.FSTORE_L:
        val = float_to_int32_bits(cursor.next_float())
        var_name = cursor.next_word()
        if not var_name: raise ParseError("Expected variable name.")
        asm.emit(opcode); asm.emit(val); asm.emit(sub.get_create_local_var(var_name))
    elif opcode == Opcode.CSTORE_L:
        val = cursor.next_char_literal()
        var_name = cursor.next_word()
        if not var_name: raise ParseError("Expected variable name.")
        asm.emit(opcode); asm.emit(val); asm.emit(sub.get_create_local_var(var_name))
    elif opcode in (Opcode.STORE_L, Opcode.LOAD_L):
        var_name = cursor.next_word()
        if not var_name: raise ParseError("Expected variable name.")
        asm.emit(opcode); asm.emit(sub.get_create_local_var(var_name))

# ── Main Compilation Loop ─────────────────────────────────────────────────

def compile_kdm_to_bin(in_filename: str, out_filename: str) -> int:
    try:
        with open(in_filename, 'r', encoding='utf-8') as file:
            lines = file.readlines()
    except OSError:
        print(f"[ERROR] Could not open input file: {in_filename}")
        return -1

    asm = Assembler()
    subroutine_map: Dict[str, SubroutineCtx] = {}
    current_subroutine: Optional[SubroutineCtx] = None
    subroutine_global: Optional[SubroutineCtx] = None
    defining_subroutine = False

    for line_num, line in enumerate(lines, start=1):
        cursor = LineCursor(line)
        if cursor.is_empty(): continue
        command = cursor.next_word()
        if not command: continue

        try:
            if command.startswith("::"):
                if defining_subroutine: raise ParseError("Cannot start new subroutine without RET.")
                name = command[2:]
                sub = SubroutineCtx(name, len(asm.program))
                subroutine_map[name] = sub
                current_subroutine = sub
                if name == "_global": subroutine_global = sub
                defining_subroutine = True
                continue

            zero_ops = {
                "HALT": Opcode.HALT, "POP": Opcode.POP, "DUP": Opcode.DUP,
                "SWAP": Opcode.SWAP, "ROT": Opcode.ROT, "PEEK": Opcode.PEEK,
                "ADD": Opcode.ADD, "SUB": Opcode.SUB, "MUL": Opcode.MUL,
                "DIV": Opcode.DIV, "MOD": Opcode.MOD, "INC": Opcode.INC, "DEC": Opcode.DEC,
                "STORE": Opcode.STORE, "LOAD": Opcode.LOAD, "MSET": Opcode.MSET,
                "HALLOC": Opcode.HALLOC, "HFREE": Opcode.HFREE,
                "CMPEQ": Opcode.CMPEQ, "CMPNEQ": Opcode.CMPNEQ,
                "CMPLT": Opcode.CMPLT, "CMPLE": Opcode.CMPLE,
                "CMPGT": Opcode.CMPGT, "CMPGE": Opcode.CMPGE,
                "OUT": Opcode.OUT, "OUT_LN": Opcode.OUT_LN,
                "FOUT": Opcode.FOUT, "FOUT_LN": Opcode.FOUT_LN, "IN": Opcode.IN,
                "SYS_READ": Opcode.SYS_READ, "SYS_WRITE": Opcode.SYS_WRITE
            }

            if command in zero_ops: asm.emit(zero_ops[command])
            elif command == "RET":
                defining_subroutine = False
                asm.emit(Opcode.RET)
            elif command in ("PUSH", "FPUSH", "CPUSH", "BPUSH"):
                if command == "PUSH": asm.emit(Opcode.PUSH); asm.emit(cursor.next_int())
                elif command == "FPUSH": asm.emit(Opcode.FPUSH); asm.emit(float_to_int32_bits(cursor.next_float()))
                elif command == "CPUSH": asm.emit(Opcode.CPUSH); asm.emit(cursor.next_char_literal())
                elif command == "BPUSH": asm.emit(Opcode.BPUSH); asm.emit(to_int8_bits(cursor.next_int()))
            elif command in ("JUMP", "JZ", "JNZ"):
                if not current_subroutine: raise ParseError("Jump outside of subroutine.")
                op_map = {"JUMP": Opcode.JUMP, "JZ": Opcode.JZ, "JNZ": Opcode.JNZ}
                asm.emit(op_map[command])
                asm.emit(cursor.next_int() + current_subroutine.program_line)
            elif command == "CALL":
                sub_name = cursor.next_word()
                if not sub_name or sub_name not in subroutine_map: raise ParseError(f"Unknown subroutine '{sub_name}'.")
                asm.emit(Opcode.CALL); asm.emit(subroutine_map[sub_name].program_line)
            elif command in ("ISTORE", "FSTORE", "CSTORE", "BSTORE"):
                op_map = {"ISTORE": Opcode.ISTORE, "FSTORE": Opcode.FSTORE, "CSTORE": Opcode.CSTORE, "BSTORE": Opcode.BSTORE}
                emit_store(asm, op_map[command], cursor)
            elif command in ("ISTORE_L", "FSTORE_L", "CSTORE_L", "BSTORE_L", "STORE_L", "LOAD_L"):
                if not current_subroutine: raise ParseError("Local store/load outside of subroutine.")
                op_map = {"ISTORE_L": Opcode.ISTORE_L, "FSTORE_L": Opcode.FSTORE_L, "CSTORE_L": Opcode.CSTORE_L,
                          "BSTORE_L": Opcode.BSTORE_L, "STORE_L": Opcode.STORE_L, "LOAD_L": Opcode.LOAD_L}
                emit_store_load_local(asm, op_map[command], cursor, current_subroutine)
            else:
                raise ParseError(f"Unknown instruction '{command}'.")

        except (ParseError, MemoryError) as e:
            print(f"[PARSE ERROR] Line {line_num}: {e}")
            return -1

    if not subroutine_global:
        print("[PARSE ERROR] No _global subroutine was defined.")
        return -1

    asm._global_start = subroutine_global.program_line

    # ── Write Binary Representation ───────────────────────────────────────
    try:
        with open(out_filename, 'wb') as bin_file:
            # Write Header: _global_start (4 bytes), program_size (4 bytes)
            bin_file.write(struct.pack('<II', asm._global_start, len(asm.program)))
            # Write Program Payload: array of 32-bit unsigned integers
            for word in asm.program:
                bin_file.write(struct.pack('<I', word))
    except OSError as e:
        print(f"[ERROR] Could not write binary file: {e}")
        return -1

    print(f"Successfully compiled {in_filename} -> {out_filename} ({len(asm.program)} words written).")
    return 0

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 assembler.py <input.grr> <output.grro>")
        sys.exit(1)
    sys.exit(0 if compile_kdm_to_bin(sys.argv[1], sys.argv[2]) == 0 else 1)
