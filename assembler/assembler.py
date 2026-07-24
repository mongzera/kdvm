import shlex
import struct
import sys
from typing import Dict, List, Optional, Tuple

# ── VM Memory & Limits ───────────────────────────────────────────────────
VM_PROGRAM_MEM = 1024


class PrimitiveType:
    TYPE_INT = 0x0
    TYPE_FLOAT = 0x1
    TYPE_CHAR = 0x2
    TYPE_BYTE = 0x3
    TYPE_ADDRESS = 0x4


class WordState:
    WORD_AVAILABLE = 0x0
    WORD_OPEN = 0x1  # Used for ::_data
    WORD_CONSTANT = 0x2  # Used for ::_const_data


class Opcode:
    # Memory & Stack
    HALT = 0x00
    PUSH = 0x01
    POP = 0x02
    PEEK = 0x03
    DUP = 0x04
    SWAP = 0x05
    ROT = 0x06
    FPUSH = 0x07
    CPUSH = 0x08
    BPUSH = 0x09

    # Integer Arithmetic
    ADD = 0x10
    SUB = 0x11
    MUL = 0x12
    DIV = 0x13
    MOD = 0x14
    INC = 0x15
    DEC = 0x16

    # Memory (RAM)
    STORE = 0x20
    LOAD = 0x21
    MSET = 0x22
    ISTORE = 0x23
    FSTORE = 0x24
    CSTORE = 0x25
    BSTORE = 0x26
    HALLOC = 0x27
    HFREE = 0x28
    ISTORE_L = 0x2A
    FSTORE_L = 0x2B
    CSTORE_L = 0x2C
    BSTORE_L = 0x2D
    LOAD_L = 0x2E
    STORE_L = 0x2F

    # Control Flow & Subroutines
    JUMP = 0x30
    JNZ = 0x31
    JZ = 0x32
    CMPEQ = 0x33
    CMPNEQ = 0x34
    CMPLT = 0x35
    CMPLE = 0x36
    CMPGT = 0x37
    CMPGE = 0x38
    CALL = 0x3A
    RET = 0x3B

    # I/O & Syscalls
    OUT = 0x40
    OUT_LN = 0x41
    FOUT = 0x42
    FOUT_LN = 0x43
    IN = 0x44
    SYS_READ = 0x61
    SYS_WRITE = 0x62


def float_to_bits(f: float) -> int:
    return struct.unpack("<I", struct.pack("<f", f))[0]


def format_bin32(val: int) -> str:
    """Formats a 32-bit int into grouped 8-bit binary strings."""
    b = f"{val & 0xFFFFFFFF:032b}"
    return f"{b[:8]} {b[8:16]} {b[16:24]} {b[24:32]}"


def tokenize_line(line: str) -> List[str]:
    try:
        return shlex.split(line, posix=False)
    except ValueError:
        return line.split()


def parse_char_or_int(token: str) -> int:
    token = token.strip()
    if token.startswith("'") and token.endswith("'"):
        content = token[1:-1]
        if not content:
            return ord(" ")
        try:
            decoded = bytes(content, "utf-8").decode("unicode_escape")
            return ord(decoded[0])
        except Exception:
            return ord(content[0])
    return int(token, 0)


class PrimitiveValueBin:

    def __init__(self, prim_type: int, word_state: int, data_value: int):
        self.type = prim_type
        self.word_state = word_state
        self.data = data_value & 0xFFFFFFFF

    def pack(self) -> bytes:
        return struct.pack("<BBHI", self.type, self.word_state, 0, self.data)


class SubroutineCtx:

    def __init__(self, name: str, program_line: int):
        self.name: str = name
        self.program_line: int = program_line
        self.local_var_map: Dict[str, int] = {}
        self.local_var_count: int = 0

    def get_or_create_local(self, var_name: str) -> int:
        if var_name not in self.local_var_map:
            offset = self.local_var_count
            self.local_var_map[var_name] = offset
            self.local_var_count += 1
            return offset
        return self.local_var_map[var_name]


def parse_data_elements(raw_args: str) -> List[Tuple[str, any]]:
    results = []
    i = 0
    raw = raw_args.strip()
    while i < len(raw):
        if raw[i] in (",", " ", "\t"):
            i += 1
            continue

        if raw[i] == '"':
            i += 1
            start = i
            while i < len(raw) and raw[i] != '"':
                if raw[i] == "\\" and i + 1 < len(raw):
                    i += 1
                i += 1
            s_val = bytes(raw[start:i], "utf-8").decode("unicode_escape")
            results.append(("string", s_val))
            i += 1
        elif raw[i] == "'":
            i += 1
            if i < len(raw) and raw[i] == "\\":
                char_str = raw[i : i + 2]
                i += 2
                char_val = ord(
                    bytes(char_str, "utf-8").decode("unicode_escape")
                )
            elif i < len(raw):
                char_val = ord(raw[i])
                i += 1
            else:
                break
            results.append(("char", char_val))
            if i < len(raw) and raw[i] == "'":
                i += 1
        else:
            start = i
            while i < len(raw) and raw[i] not in (",", " ", "\t"):
                i += 1
            token = raw[start:i].strip()
            if token:
                if "." in token:
                    results.append(("float", float(token)))
                else:
                    results.append(("int", int(token, 0)))
    return results


def get_instruction_word_count(tokens: List[str]) -> int:
    if not tokens:
        return 0
    cmd = tokens[0]

    if cmd in (
        "ISTORE",
        "FSTORE",
        "CSTORE",
        "BSTORE",
        "ISTORE_L",
        "FSTORE_L",
        "CSTORE_L",
        "BSTORE_L",
    ):
        return 3
    if cmd in (
        "PUSH",
        "FPUSH",
        "CPUSH",
        "BPUSH",
        "JUMP",
        "JZ",
        "JNZ",
        "CALL",
        "LOAD_L",
        "STORE_L",
        "IN",
    ):
        return 2
    if cmd in ("LOAD", "STORE", "OUT"):
        return 2 if len(tokens) > 1 else 1
    return 1


class Assembler:

    def __init__(self):
        self.program: List[int] = []
        self.ram_data: List[PrimitiveValueBin] = []
        self.symbol_table: Dict[str, int] = {}
        self.subroutine_map: Dict[str, SubroutineCtx] = {}
        self._global_start: int = 0

        # Listing metadata for the dump file
        self.code_listing: List[Tuple[int, int, str, str]] = (
            []
        )  # (addr, word, source_line, operand_note)
        self.ram_listing: List[Tuple[int, PrimitiveValueBin, str, str]] = (
            []
        )  # (addr, prim, source_line, note)

    def emit(self, val: int, source_line: str = "", operand_note: str = ""):
        if len(self.program) >= VM_PROGRAM_MEM:
            raise MemoryError("Program size exceeds maximum VM_PROGRAM_MEM limit.")
        addr = len(self.program)
        word = int(val) & 0xFFFFFFFF
        self.program.append(word)
        self.code_listing.append((addr, word, source_line, operand_note))

    def allocate_ram_word(
        self,
        symbol: Optional[str],
        prim_type: int,
        word_state: int,
        value: int,
        source_line: str = "",
        note: str = "",
    ) -> int:
        addr = len(self.ram_data)
        if symbol and symbol not in self.symbol_table:
            self.symbol_table[symbol] = addr
        prim = PrimitiveValueBin(prim_type, word_state, value)
        self.ram_data.append(prim)
        self.ram_listing.append((addr, prim, source_line, note))
        return addr


def write_dump_file(asm: Assembler, dump_filename: str):
    type_names = {0: "INT", 1: "FLOAT", 2: "CHAR", 3: "BYTE", 4: "ADDR"}
    state_names = {0: "AVAIL", 1: "OPEN", 2: "CONST"}

    with open(dump_filename, "w", encoding="utf-8") as f:
        f.write("=" * 105 + "\n")
        f.write(" KDM ASSEMBLER DUMP & SOURCE LISTING\n")
        f.write("=" * 105 + "\n")
        f.write(f" Header Metadata:\n")
        f.write(
            f"   _global_start  : Addr [0x{asm._global_start:04X}] |"
            f" 0x{asm._global_start:08X} | {format_bin32(asm._global_start)}\n"
        )
        f.write(
            f"   Program Words  : {len(asm.program)} words (Max capacity:"
            f" {VM_PROGRAM_MEM})\n"
        )
        f.write(f"   RAM Data Words : {len(asm.ram_data)} words\n")
        f.write("=" * 105 + "\n\n")

        # ── STATIC RAM SECTION ───────────────────────────────────────────────────
        f.write(
            "── STATIC MEMORY (RAM Data: ::_data / ::_const_data) ".ljust(
                105, "─"
            )
            + "\n"
        )
        f.write(
            "Addr   Hex (Type/St/Pad/Data)  Data Binary (32-bit)                  ASCII "
            " Type  State  Source / Element Note\n"
        )
        f.write("─" * 105 + "\n")

        for addr, prim, src, note in asm.ram_listing:
            hex_str = (
                f"{prim.type:02X} {prim.word_state:02X} 0000 {prim.data:08X}"
            )
            bin_str = format_bin32(prim.data)

            # ASCII char decoding for lower byte
            char_byte = prim.data & 0xFF
            ascii_char = chr(char_byte) if 32 <= char_byte <= 126 else "."

            t_str = type_names.get(prim.type, "UNK").ljust(5)
            s_str = state_names.get(prim.word_state, "UNK").ljust(5)

            info = src if src else f"    {note}"
            f.write(
                f"[{addr:04X}] {hex_str}   {bin_str}    '{ascii_char}'   {t_str} {s_str}"
                f"  {info}\n"
            )

        # ── PROGRAM CODE SECTION ─────────────────────────────────────────────────
        f.write(
            "\n── PROGRAM MEMORY (Code Section) ".ljust(105, "─") + "\n"
        )
        f.write(
            "Addr   Hex Word     32-Bit Binary Representation          ASCII / Source"
            " Code & Instruction Disassembly\n"
        )
        f.write("─" * 105 + "\n")

        for addr, word, src, note in asm.code_listing:
            hex_str = f"0x{word:08X}"
            bin_str = format_bin32(word)

            if src:
                info = f"  {src}"
            elif note:
                info = f"      {note}"
            else:
                info = ""

            f.write(f"[{addr:04X}] {hex_str}   {bin_str} {info}\n")

        f.write("=" * 105 + "\n")


def compile_kdm_to_bin(in_filename: str, out_filename: str) -> int:
    try:
        with open(in_filename, "r", encoding="utf-8") as f:
            lines = f.readlines()
    except OSError:
        print(f"[ERROR] Could not open source file: {in_filename}")
        return -1

    asm = Assembler()
    current_section = None
    code_word_counter = 0

    # ==========================================
    # PASS 1: Allocate RAM Data & Map Code Offsets
    # ==========================================
    for line_num, line in enumerate(lines, start=1):
        clean = line.split("#")[0].strip()
        if not clean:
            continue

        if clean.startswith("::"):
            header = clean[2:].strip()
            if header in ("_data", "_const_data"):
                current_section = header
            else:
                current_section = "code"
                sub_ctx = SubroutineCtx(header, code_word_counter)
                asm.subroutine_map[header] = sub_ctx
                asm.symbol_table[header] = code_word_counter
                asm.symbol_table[f"::{header}"] = code_word_counter
                if header == "_global":
                    asm._global_start = code_word_counter
            continue

        if current_section in ("_data", "_const_data"):
            parts = tokenize_line(clean)
            if len(parts) < 2:
                continue

            datatype = parts[0]
            symbol_name = parts[1]
            raw_values = (
                clean.split(symbol_name, 1)[1].strip()
                if symbol_name in clean
                else ""
            )

            word_state = (
                WordState.WORD_CONSTANT
                if current_section == "_const_data"
                else WordState.WORD_OPEN
            )
            elements = parse_data_elements(raw_values)

            payload_count = sum(
                len(val) if elem_type == "string" else 1
                for elem_type, val in elements
            )
            total_array_size = payload_count + 1

            # 1. Array length header word
            asm.allocate_ram_word(
                symbol_name,
                PrimitiveType.TYPE_INT,
                word_state,
                total_array_size,
                source_line=clean,
                note=f"Array length ({total_array_size})",
            )

            # 2. Payload words
            for elem_type, val in elements:
                if datatype == "byte":
                    if elem_type == "string":
                        for ch in val:
                            asm.allocate_ram_word(
                                None,
                                PrimitiveType.TYPE_BYTE,
                                word_state,
                                ord(ch),
                                note=f"↳ [elem: '{ch}' ({ord(ch)})]",
                            )
                    else:
                        asm.allocate_ram_word(
                            None,
                            PrimitiveType.TYPE_BYTE,
                            word_state,
                            int(val),
                            note=f"↳ [elem: {val}]",
                        )
                elif datatype == "half":
                    asm.allocate_ram_word(
                        None,
                        PrimitiveType.TYPE_INT,
                        word_state,
                        int(val) & 0xFFFF,
                        note=f"↳ [elem: {val}]",
                    )
                elif datatype == "word":
                    if elem_type == "float":
                        asm.allocate_ram_word(
                            None,
                            PrimitiveType.TYPE_FLOAT,
                            word_state,
                            float_to_bits(val),
                            note=f"↳ [elem: {val}]",
                        )
                    else:
                        asm.allocate_ram_word(
                            None,
                            PrimitiveType.TYPE_INT,
                            word_state,
                            int(val) & 0xFFFFFFFF,
                            note=f"↳ [elem: {val}]",
                        )

        elif current_section == "code":
            tokens = tokenize_line(clean)
            code_word_counter += get_instruction_word_count(tokens)

    if "_global" not in asm.subroutine_map:
        print("[ERROR] No _global subroutine was defined.")
        return -1

    # ==========================================
    # PASS 2: Emit Code Payload
    # ==========================================
    current_subroutine: Optional[SubroutineCtx] = None
    current_section = None

    zero_operand_opcodes = {
        "HALT": Opcode.HALT,
        "POP": Opcode.POP,
        "DUP": Opcode.DUP,
        "SWAP": Opcode.SWAP,
        "ROT": Opcode.ROT,
        "PEEK": Opcode.PEEK,
        "ADD": Opcode.ADD,
        "SUB": Opcode.SUB,
        "MUL": Opcode.MUL,
        "DIV": Opcode.DIV,
        "MOD": Opcode.MOD,
        "INC": Opcode.INC,
        "DEC": Opcode.DEC,
        "MSET": Opcode.MSET,
        "HALLOC": Opcode.HALLOC,
        "HFREE": Opcode.HFREE,
        "CMPEQ": Opcode.CMPEQ,
        "CMPNEQ": Opcode.CMPNEQ,
        "CMPLT": Opcode.CMPLT,
        "CMPLE": Opcode.CMPLE,
        "CMPGT": Opcode.CMPGT,
        "CMPGE": Opcode.CMPGE,
        "OUT_LN": Opcode.OUT_LN,
        "FOUT": Opcode.FOUT,
        "FOUT_LN": Opcode.FOUT_LN,
        "SYS_READ": Opcode.SYS_READ,
        "SYS_WRITE": Opcode.SYS_WRITE,
        "RET": Opcode.RET,
    }

    for line_num, line in enumerate(lines, start=1):
        clean = line.split("#")[0].strip()
        if not clean:
            continue

        if clean.startswith("::"):
            header = clean[2:].strip()
            current_section = header
            if header not in ("_data", "_const_data"):
                current_subroutine = asm.subroutine_map[header]
            continue

        if current_section in ("_data", "_const_data"):
            continue

        tokens = tokenize_line(clean)
        if not tokens:
            continue
        cmd = tokens[0]

        def resolve_val(token: str) -> int:
            clean_tok = token.lstrip(":")
            if token in asm.symbol_table:
                return asm.symbol_table[token]
            if clean_tok in asm.symbol_table:
                return asm.symbol_table[clean_tok]
            return parse_char_or_int(token)

        try:
            if cmd == "PUSH":
                asm.emit(Opcode.PUSH, source_line=clean)
                asm.emit(
                    resolve_val(tokens[1]),
                    operand_note=f"└── [val: {tokens[1]}]",
                )
            elif cmd == "FPUSH":
                asm.emit(Opcode.FPUSH, source_line=clean)
                asm.emit(
                    float_to_bits(float(tokens[1])),
                    operand_note=f"└── [val: {tokens[1]}]",
                )
            elif cmd == "CPUSH":
                asm.emit(Opcode.CPUSH, source_line=clean)
                asm.emit(
                    parse_char_or_int(tokens[1]),
                    operand_note=f"└── [char: {tokens[1]}]",
                )
            elif cmd == "BPUSH":
                asm.emit(Opcode.BPUSH, source_line=clean)
                asm.emit(
                    parse_char_or_int(tokens[1]) & 0xFF,
                    operand_note=f"└── [byte: {tokens[1]}]",
                )

            elif cmd in ("JUMP", "JZ", "JNZ"):
                op = (
                    Opcode.JUMP
                    if cmd == "JUMP"
                    else (Opcode.JZ if cmd == "JZ" else Opcode.JNZ)
                )
                asm.emit(op, source_line=clean)
                target = resolve_val(tokens[1])
                asm.emit(
                    target,
                    operand_note=f"└── [target: {tokens[1]} -> addr {target}]",
                )
            elif cmd == "CALL":
                asm.emit(Opcode.CALL, source_line=clean)
                target = resolve_val(tokens[1])
                asm.emit(
                    target,
                    operand_note=f"└── [target: {tokens[1]} -> addr {target}]",
                )

            elif cmd == "IN":
                asm.emit(Opcode.IN, source_line=clean)
                asm.emit(
                    resolve_val(tokens[1]),
                    operand_note=f"└── [dest: {tokens[1]}]",
                )
            elif cmd == "OUT":
                asm.emit(Opcode.OUT, source_line=clean)
                if len(tokens) > 1:
                    asm.emit(
                        resolve_val(tokens[1]),
                        operand_note=f"└── [src: {tokens[1]}]",
                    )
            elif cmd in ("LOAD", "STORE"):
                op = Opcode.LOAD if cmd == "LOAD" else Opcode.STORE
                asm.emit(op, source_line=clean)
                if len(tokens) > 1:
                    asm.emit(
                        resolve_val(tokens[1]),
                        operand_note=f"└── [addr: {tokens[1]}]",
                    )
            elif cmd in ("ISTORE", "FSTORE", "CSTORE", "BSTORE"):
                op_map = {
                    "ISTORE": Opcode.ISTORE,
                    "FSTORE": Opcode.FSTORE,
                    "CSTORE": Opcode.CSTORE,
                    "BSTORE": Opcode.BSTORE,
                }
                asm.emit(op_map[cmd], source_line=clean)
                val = (
                    float_to_bits(float(tokens[1]))
                    if cmd == "FSTORE"
                    else parse_char_or_int(tokens[1])
                )
                asm.emit(val, operand_note=f"├── [val: {tokens[1]}]")
                asm.emit(
                    resolve_val(tokens[2]),
                    operand_note=f"└── [dest: {tokens[2]}]",
                )

            elif cmd in ("LOAD_L", "STORE_L"):
                if not current_subroutine:
                    raise SyntaxError(
                        "Local memory operations used outside subroutine."
                    )
                op = Opcode.LOAD_L if cmd == "LOAD_L" else Opcode.STORE_L
                asm.emit(op, source_line=clean)
                offset = current_subroutine.get_or_create_local(tokens[1])
                asm.emit(
                    offset,
                    operand_note=f"└── [local: '{tokens[1]}' -> offset {offset}]",
                )
            elif cmd in ("ISTORE_L", "FSTORE_L", "CSTORE_L", "BSTORE_L"):
                if not current_subroutine:
                    raise SyntaxError(
                        "Local memory operations used outside subroutine."
                    )
                op_map = {
                    "ISTORE_L": Opcode.ISTORE_L,
                    "FSTORE_L": Opcode.FSTORE_L,
                    "CSTORE_L": Opcode.CSTORE_L,
                    "BSTORE_L": Opcode.BSTORE_L,
                }
                asm.emit(op_map[cmd], source_line=clean)
                val = (
                    float_to_bits(float(tokens[1]))
                    if cmd == "FSTORE_L"
                    else parse_char_or_int(tokens[1])
                )
                asm.emit(val, operand_note=f"├── [val: {tokens[1]}]")
                offset = current_subroutine.get_or_create_local(tokens[2])
                asm.emit(
                    offset,
                    operand_note=f"└── [local: '{tokens[2]}' -> offset {offset}]",
                )

            elif cmd in zero_operand_opcodes:
                asm.emit(zero_operand_opcodes[cmd], source_line=clean)
            else:
                raise SyntaxError(f"Unknown instruction '{cmd}'")

        except (IndexError, ValueError, SyntaxError) as e:
            print(f"[ASSEMBLER ERROR] Line {line_num}: {e}")
            return -1

    # ==========================================
    # PASS 3: Write Output Binary (.grro) & Dump
    # ==========================================
    try:
        with open(out_filename, "wb") as bin_file:
            bin_file.write(
                struct.pack(
                    "<III",
                    asm._global_start,
                    len(asm.program),
                    len(asm.ram_data),
                )
            )
            for word in asm.program:
                bin_file.write(struct.pack("<I", word))
            for prim in asm.ram_data:
                bin_file.write(prim.pack())

        # Generate accompanying dump file
        dump_filename = (
            out_filename.rsplit(".", 1)[0] + ".dump.txt"
            if "." in out_filename
            else out_filename + ".dump.txt"
        )
        write_dump_file(asm, dump_filename)

        print(
            f"[ASSEMBLER SUCCESS] Compiled '{in_filename}' -> '{out_filename}'"
        )
        print(f"[DUMP GENERATED]    Wrote listing to '{dump_filename}'")
        print(
            f"                     {len(asm.program)} code words |"
            f" {len(asm.ram_data)} static RAM words."
        )
        return 0
    except OSError as e:
        print(f"[ERROR] Failed writing output file: {e}")
        return -1


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 assembler.py <input.grr> <output.grro>")
        sys.exit(1)
    sys.exit(0 if compile_kdm_to_bin(sys.argv[1], sys.argv[2]) == 0 else 1)
