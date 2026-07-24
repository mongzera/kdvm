import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Assembler {

    // ── VM Memory & Limits ───────────────────────────────────────────────────
    public static final int VM_PROGRAM_MEM = 1024;

    public static class PrimitiveType {
        public static final int TYPE_INT = 0x0;
        public static final int TYPE_FLOAT = 0x1;
        public static final int TYPE_CHAR = 0x2;
        public static final int TYPE_BYTE = 0x3;
        public static final int TYPE_ADDRESS = 0x4;
    }

    public static class WordState {
        public static final int WORD_AVAILABLE = 0x0;
        public static final int WORD_OPEN = 0x1;      // Used for ::_data
        public static final int WORD_CONSTANT = 0x2;  // Used for ::_const_data
    }

    public static class Opcode {
        // Memory & Stack
        public static final int HALT = 0x00, PUSH = 0x01, POP = 0x02, PEEK = 0x03;
        public static final int DUP = 0x04, SWAP = 0x05, ROT = 0x06, FPUSH = 0x07;
        public static final int CPUSH = 0x08, BPUSH = 0x09;

        // Integer Arithmetic
        public static final int ADD = 0x10, SUB = 0x11, MUL = 0x12, DIV = 0x13;
        public static final int MOD = 0x14, INC = 0x15, DEC = 0x16;

        // Memory (RAM)
        public static final int OPT_MEM = 0x20;
        public static final int STORE = OPT_MEM | 0x00, LOAD = OPT_MEM | 0x01, STORE_OFF = OPT_MEM | 0x02, LOAD_OFF = OPT_MEM | 0x03, ISTORE = OPT_MEM | 0x04;
        public static final int FSTORE = OPT_MEM | 0x05, CSTORE = OPT_MEM | 0x06, BSTORE = OPT_MEM | 0x07, H_ALLOC = OPT_MEM | 0x08;
        public static final int H_FREE = OPT_MEM | 0x09, ISTORE_L = OPT_MEM | 0x0A, FSTORE_L = OPT_MEM | 0x0B;
        public static final int CSTORE_L = OPT_MEM | 0x0C, BSTORE_L = OPT_MEM | 0x0D, LOAD_L = OPT_MEM | 0x0E, STORE_L = OPT_MEM | 0x0F;

        // Control Flow & Subroutines
        public static final int JUMP = 0x30, JNZ = 0x31, JZ = 0x32, CMPEQ = 0x33;
        public static final int CMPNEQ = 0x34, CMPLT = 0x35, CMPLE = 0x36, CMPGT = 0x37;
        public static final int CMPGE = 0x38, CALL = 0x3A, RET = 0x3B;

        // I/O & Syscalls
        public static final int OUT = 0x40, OUT_LN = 0x41, FOUT = 0x42, FOUT_LN = 0x43;
        public static final int IN = 0x44, SYS_READ = 0x61, SYS_WRITE = 0x62;
    }

    // ── Structures & Helper Classes ─────────────────────────────────────────

    public static class PrimitiveValueBin {
        public int type;
        public int wordState;
        public int data; // uint32 bit representation stored in 32-bit int

        public PrimitiveValueBin(int type, int wordState, int data) {
            this.type = type;
            this.wordState = wordState;
            this.data = data;
        }

        public byte[] pack() {
            ByteBuffer buf = ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN);
            buf.put((byte) type);
            buf.put((byte) wordState);
            buf.putShort((short) 0);
            buf.putInt(data);
            return buf.array();
        }
    }

    public static class SubroutineCtx {
        public String name;
        public int programLine;
        public Map<String, Integer> localVarMap = new HashMap<>();
        public int localVarCount = 0;

        public SubroutineCtx(String name, int programLine) {
            this.name = name;
            this.programLine = programLine;
        }

        public int getOrCreateLocal(String varName) {
            if (!localVarMap.containsKey(varName)) {
                localVarMap.put(varName, localVarCount);
                return localVarCount++;
            }
            return localVarMap.get(varName);
        }
    }

    public static class DataElement {
        public String type;
        public Object value;

        public DataElement(String type, Object value) {
            this.type = type;
            this.value = value;
        }
    }

    public static class CodeListingItem {
        public int addr;
        public int word;
        public String sourceLine;
        public String operandNote;

        public CodeListingItem(int addr, int word, String sourceLine, String operandNote) {
            this.addr = addr;
            this.word = word;
            this.sourceLine = sourceLine;
            this.operandNote = operandNote;
        }
    }

    public static class RamListingItem {
        public int addr;
        public PrimitiveValueBin prim;
        public String sourceLine;
        public String note;

        public RamListingItem(int addr, PrimitiveValueBin prim, String sourceLine, String note) {
            this.addr = addr;
            this.prim = prim;
            this.sourceLine = sourceLine;
            this.note = note;
        }
    }

    // ── Helper Utility Methods ───────────────────────────────────────────────

    private static String formatBin32(int val) {
        String b = String.format("%32s", Integer.toBinaryString(val)).replace(' ', '0');
        return b.substring(0, 8) + " " + b.substring(8, 16) + " " + b.substring(16, 24) + " " + b.substring(24, 32);
    }

    private static List<String> tokenizeLine(String line) {
        List<String> tokens = new ArrayList<>();
        Matcher m = Pattern.compile("([^\\s\"']+|\"[^\"]*\"|'[^']*')").matcher(line);
        while (m.find()) {
            tokens.add(m.group());
        }
        return tokens;
    }

    private static int parseCharOrInt(String token) {
        token = token.trim();
        if (token.startsWith("'") && token.endsWith("'")) {
            String content = token.substring(1, token.length() - 1);
            if (content.isEmpty()) return 0;
            if (content.startsWith("\\")) {
                switch (content) {
                    case "\\n": return '\n';
                    case "\\t": return '\t';
                    case "\\r": return '\r';
                    case "\\0": return '\0';
                    case "\\\\": return '\\';
                    case "\\'": return '\'';
                    default: return content.charAt(1);
                }
            }
            return content.charAt(0);
        }

        long val;
        if (token.startsWith("0x") || token.startsWith("0X")) {
            val = Long.parseLong(token.substring(2), 16);
        } else {
            val = Long.parseLong(token);
        }
        return (int) (val & 0xFFFFFFFFL);
    }

    private static List<DataElement> parseDataElements(String raw) {
        List<DataElement> results = new ArrayList<>();
        int i = 0;
        raw = raw.trim();
        while (i < raw.length()) {
            char c = raw.charAt(i);
            if (c == ',' || c == ' ' || c == '\t') {
                i++;
                continue;
            }

            if (c == '"') {
                i++;
                StringBuilder sb = new StringBuilder();
                while (i < raw.length() && raw.charAt(i) != '"') {
                    if (raw.charAt(i) == '\\' && i + 1 < raw.length()) {
                        i++;
                        char esc = raw.charAt(i);
                        if (esc == 'n') sb.append('\n');
                        else if (esc == 't') sb.append('\t');
                        else if (esc == 'r') sb.append('\r');
                        else if (esc == '0') sb.append('\0');
                        else sb.append(esc);
                    } else {
                        sb.append(raw.charAt(i));
                    }
                    i++;
                }
                results.add(new DataElement("string", sb.toString()));
                i++;
            } else if (c == '\'') {
                i++;
                int charVal = 0;
                if (i < raw.length() && raw.charAt(i) == '\\') {
                    i++;
                    if (i < raw.length()) {
                        char esc = raw.charAt(i);
                        if (esc == 'n') charVal = '\n';
                        else if (esc == 't') charVal = '\t';
                        else if (esc == 'r') charVal = '\r';
                        else if (esc == '0') charVal = '\0';
                        else charVal = esc;
                        i++;
                    }
                } else if (i < raw.length() && raw.charAt(i) != '\'') {
                    charVal = raw.charAt(i);
                    i++;
                }
                results.add(new DataElement("char", charVal));
                if (i < raw.length() && raw.charAt(i) == '\'') i++;
            } else {
                int start = i;
                while (i < raw.length() && raw.charAt(i) != ',' && raw.charAt(i) != ' ' && raw.charAt(i) != '\t') {
                    i++;
                }
                String token = raw.substring(start, i).trim();
                if (!token.isEmpty()) {
                    if (token.contains(".")) {
                        results.add(new DataElement("float", Float.parseFloat(token)));
                    } else {
                        results.add(new DataElement("int", parseCharOrInt(token)));
                    }
                }
            }
        }
        return results;
    }

    private static int getInstructionWordCount(List<String> tokens) {
        if (tokens.isEmpty()) return 0;
        String cmd = tokens.get(0);

        if (Set.of("ISTORE", "FSTORE", "CSTORE", "BSTORE", "ISTORE_L", "FSTORE_L", "CSTORE_L", "BSTORE_L").contains(cmd)) {
            return 3;
        }
        if (Set.of("PUSH", "FPUSH", "CPUSH", "BPUSH", "JUMP", "JZ", "JNZ", "CALL", "LOAD_L", "STORE_L", "IN").contains(cmd)) {
            return 2;
        }
        if (Set.of("LOAD", "STORE", "OUT").contains(cmd)) {
            return tokens.size() > 1 ? 2 : 1;
        }
        return 1;
    }

    // ── Assembler Core Class ────────────────────────────────────────────────

    public static class AssemblerEngine {
        public List<Integer> program = new ArrayList<>();
        public List<PrimitiveValueBin> ramData = new ArrayList<>();
        public Map<String, Integer> symbolTable = new HashMap<>();
        public Map<String, SubroutineCtx> subroutineMap = new HashMap<>();
        public int globalStart = 0;

        public List<CodeListingItem> codeListing = new ArrayList<>();
        public List<RamListingItem> ramListing = new ArrayList<>();

        public void emit(int val, String sourceLine, String operandNote) {
            if (program.size() >= VM_PROGRAM_MEM) {
                throw new Error("Program size exceeds maximum VM_PROGRAM_MEM limit.");
            }
            int addr = program.size();
            program.add(val);
            codeListing.add(new CodeListingItem(addr, val, sourceLine, operandNote));
        }

        public int allocateRamWord(String symbol, int primType, int wordState, int value, String sourceLine, String note) {
            int addr = ramData.size();
            if (symbol != null && !symbolTable.containsKey(symbol)) {
                symbolTable.put(symbol, addr);
            }
            PrimitiveValueBin prim = new PrimitiveValueBin(primType, wordState, value);
            ramData.add(prim);
            ramListing.add(new RamListingItem(addr, prim, sourceLine, note));
            return addr;
        }
    }

    private static void allocateRamElements(AssemblerEngine asm, String datatype, int wordState, DataElement elem, String symbol, String sourceLine, String notePrefix) {
        if ("byte".equals(datatype)) {
            if ("string".equals(elem.type)) {
                boolean firstChar = true;
                for (char ch : ((String) elem.value).toCharArray()) {
                    String sym = firstChar ? symbol : null;
                    String src = firstChar ? sourceLine : null;
                    asm.allocateRamWord(sym, PrimitiveType.TYPE_BYTE, wordState, ch, src, notePrefix + "[elem: '" + ch + "' (" + (int) ch + ")]");
                    firstChar = false;
                }
            } else {
                asm.allocateRamWord(symbol, PrimitiveType.TYPE_BYTE, wordState, (Integer) elem.value, sourceLine, notePrefix + "[elem: " + elem.value + "]");
            }
        } else if ("half".equals(datatype)) {
            asm.allocateRamWord(symbol, PrimitiveType.TYPE_INT, wordState, ((Integer) elem.value) & 0xFFFF, sourceLine, notePrefix + "[elem: " + elem.value + "]");
        } else if ("word".equals(datatype)) {
            if ("float".equals(elem.type)) {
                asm.allocateRamWord(symbol, PrimitiveType.TYPE_FLOAT, wordState, Float.floatToIntBits((Float) elem.value), sourceLine, notePrefix + "[elem: " + elem.value + "]");
            } else if ("char".equals(elem.type)) {
                asm.allocateRamWord(symbol, PrimitiveType.TYPE_CHAR, wordState, (Integer) elem.value, sourceLine, notePrefix + "[elem: '" + (char) ((Integer) elem.value).intValue() + "']");
            } else {
                asm.allocateRamWord(symbol, PrimitiveType.TYPE_INT, wordState, (Integer) elem.value, sourceLine, notePrefix + "[elem: " + elem.value + "]");
            }
        }
    }

    // ── Dump Writer ─────────────────────────────────────────────────────────

    private static void writeDumpFile(AssemblerEngine asm, String dumpFilename) throws IOException {
        Map<Integer, String> typeNames = Map.of(0, "INT", 1, "FLOAT", 2, "CHAR", 3, "BYTE", 4, "ADDR");
        Map<Integer, String> stateNames = Map.of(0, "AVAIL", 1, "OPEN", 2, "CONST");

        try (PrintWriter writer = new PrintWriter(new FileWriter(dumpFilename, StandardCharsets.UTF_8))) {
            writer.println("=".repeat(105));
            writer.println(" GRR ASSEMBLER DUMP & SOURCE LISTING (JAVA)");
            writer.println("=".repeat(105));
            writer.printf(" Header Metadata:\n");
            writer.printf("   _global_start  : Addr [0x%04X] | 0x%08X | %s\n", asm.globalStart, asm.globalStart, formatBin32(asm.globalStart));
            writer.printf("   Program Words  : %d words (Max capacity: %d)\n", asm.program.size(), VM_PROGRAM_MEM);
            writer.printf("   RAM Data Words : %d words\n", asm.ramData.size());
            writer.println("=".repeat(105) + "\n");

            writer.println("── STATIC MEMORY (RAM Data: ::_data / ::_const_data) ".stripTrailing() + "─".repeat(50));
            writer.println("Addr   Hex (Type/St/Pad/Data)  Data Binary (32-bit)                 ASCII   Type  State  Source / Element Note");
            writer.println("─".repeat(105));

            for (RamListingItem item : asm.ramListing) {
                String hexStr = String.format("%02X %02X 0000 %08X", item.prim.type, item.prim.wordState, item.prim.data);
                String binStr = formatBin32(item.prim.data);
                int charByte = item.prim.data & 0xFF;
                char asciiChar = (charByte >= 32 && charByte <= 126) ? (char) charByte : '.';

                String tStr = String.format("%-5s", typeNames.getOrDefault(item.prim.type, "UNK"));
                String sStr = String.format("%-5s", stateNames.getOrDefault(item.prim.wordState, "UNK"));
                String info = item.sourceLine != null ? item.sourceLine : "    " + item.note;

                writer.printf("[%04X] %s   %s    '%c'   %s %s  %s\n", item.addr, hexStr, binStr, asciiChar, tStr, sStr, info);
            }

            writer.println("\n── PROGRAM MEMORY (Code Section) ".stripTrailing() + "─".repeat(70));
            writer.println("Addr   Hex Word     32-Bit Binary Representation          ASCII / Source Code & Instruction Disassembly");
            writer.println("─".repeat(105));

            for (CodeListingItem item : asm.codeListing) {
                String hexStr = String.format("0x%08X", item.word);
                String binStr = formatBin32(item.word);
                String info = item.sourceLine != null ? "  " + item.sourceLine : (item.operandNote != null ? "      " + item.operandNote : "");
                writer.printf("[%04X] %s   %s %s\n", item.addr, hexStr, binStr, info);
            }
            writer.println("=".repeat(105));
        }
    }

    // ── Main Compilation Pipeline ───────────────────────────────────────────

    public static int compileGrrToBin(String inFilename, String outFilename) {
        List<String> lines = new ArrayList<>();
        try (BufferedReader br = new BufferedReader(new FileReader(inFilename, StandardCharsets.UTF_8))) {
            String l;
            while ((l = br.readLine()) != null) {
                lines.add(l.replace('\u00a0', ' '));
            }
        } catch (IOException e) {
            System.err.println("[ERROR] Could not open source file: " + inFilename);
            return -1;
        }

        AssemblerEngine asm = new AssemblerEngine();
        String currentSection = null;
        int codeWordCounter = 0;

        // ==========================================
        // PASS 1: Allocate RAM Data & Map Code Offsets
        // ==========================================
        for (String line : lines) {
            String clean = line.split("#")[0].trim();
            if (clean.isEmpty()) continue;

            if (clean.startsWith("::")) {
                String header = clean.substring(2).trim();
                if (header.equals("_data") || header.equals("_const_data")) {
                    currentSection = header;
                } else {
                    currentSection = "code";
                    SubroutineCtx subCtx = new SubroutineCtx(header, codeWordCounter);
                    asm.subroutineMap.put(header, subCtx);
                    asm.symbolTable.put(header, codeWordCounter);
                    asm.symbolTable.put("::" + header, codeWordCounter);
                    if (header.equals("_global")) {
                        asm.globalStart = codeWordCounter;
                    }
                }
                continue;
            }

            if ("_data".equals(currentSection) || "_const_data".equals(currentSection)) {
                List<String> parts = tokenizeLine(clean);
                if (parts.size() < 2) continue;

                String datatype = parts.get(0);
                String symbolName = parts.get(1).replaceAll(",$", "");
                int symIdx = clean.indexOf(parts.get(1));
                String rawValues = (symIdx != -1) ? clean.substring(symIdx + parts.get(1).length()).trim() : "";

                int wordState = "_const_data".equals(currentSection) ? WordState.WORD_CONSTANT : WordState.WORD_OPEN;
                List<DataElement> elements = parseDataElements(rawValues);

                // Treats as array if line contains comma, multiple items, OR string literal
                boolean isArray = clean.contains(",")
                                || elements.size() > 1
                                || elements.stream().anyMatch(e -> "string".equals(e.type));

                if (isArray) {
                    // Array Data: Includes Header Length
                    int payloadCount = 0;
                    for (DataElement elem : elements) {
                        payloadCount += "string".equals(elem.type) ? ((String) elem.value).length() : 1;
                    }
                    int totalArraySize = payloadCount + 1;

                    asm.allocateRamWord(symbolName, PrimitiveType.TYPE_INT, wordState, totalArraySize, clean, "Array length (" + totalArraySize + ")");

                    for (DataElement elem : elements) {
                        allocateRamElements(asm, datatype, wordState, elem, null, null, "↳ ");
                    }
                } else {
                    // Single Primitive Data (e.g. word $x 42): NO Header
                    boolean isFirst = true;
                    for (DataElement elem : elements) {
                        String sym = isFirst ? symbolName : null;
                        String lineRef = isFirst ? clean : null;
                        allocateRamElements(asm, datatype, wordState, elem, sym, lineRef, "");
                        isFirst = false;
                    }
                }
            } else if ("code".equals(currentSection)) {
                List<String> tokens = tokenizeLine(clean);
                codeWordCounter += getInstructionWordCount(tokens);
            }
        }

        if (!asm.subroutineMap.containsKey("_global")) {
            System.err.println("[ERROR] No _global subroutine was defined.");
            return -1;
        }

        // ==========================================
        // PASS 2: Emit Code Payload
        // ==========================================
        SubroutineCtx currentSubroutine = null;
        currentSection = null;

        Map<String, Integer> zeroOperandOpcodes = Map.ofEntries(
            Map.entry("HALT", Opcode.HALT), Map.entry("POP", Opcode.POP), Map.entry("DUP", Opcode.DUP),
            Map.entry("SWAP", Opcode.SWAP), Map.entry("ROT", Opcode.ROT), Map.entry("PEEK", Opcode.PEEK),
            Map.entry("ADD", Opcode.ADD), Map.entry("SUB", Opcode.SUB), Map.entry("MUL", Opcode.MUL),
            Map.entry("DIV", Opcode.DIV), Map.entry("MOD", Opcode.MOD), Map.entry("INC", Opcode.INC),
            Map.entry("DEC", Opcode.DEC), Map.entry("H_ALLOC", Opcode.H_ALLOC),
            Map.entry("H_FREE", Opcode.H_FREE), Map.entry("CMPEQ", Opcode.CMPEQ), Map.entry("CMPNEQ", Opcode.CMPNEQ),
            Map.entry("CMPLT", Opcode.CMPLT), Map.entry("CMPLE", Opcode.CMPLE), Map.entry("CMPGT", Opcode.CMPGT),
            Map.entry("CMPGE", Opcode.CMPGE), Map.entry("OUT_LN", Opcode.OUT_LN), Map.entry("FOUT", Opcode.FOUT),
            Map.entry("FOUT_LN", Opcode.FOUT_LN), Map.entry("SYS_READ", Opcode.SYS_READ),
            Map.entry("SYS_WRITE", Opcode.SYS_WRITE), Map.entry("RET", Opcode.RET),
            Map.entry("STORE_OFF", Opcode.STORE_OFF), Map.entry("LOAD_OFF", Opcode.LOAD_OFF)
        );

        for (int lineNum = 1; lineNum <= lines.size(); lineNum++) {
            String clean = lines.get(lineNum - 1).split("#")[0].trim();
            if (clean.isEmpty()) continue;

            if (clean.startsWith("::")) {
                String header = clean.substring(2).trim();
                currentSection = header;
                if (!header.equals("_data") && !header.equals("_const_data")) {
                    currentSubroutine = asm.subroutineMap.get(header);
                }
                continue;
            }

            if ("_data".equals(currentSection) || "_const_data".equals(currentSection)) continue;

            List<String> tokens = tokenizeLine(clean);
            if (tokens.isEmpty()) continue;
            String cmd = tokens.get(0);

            try {
                if (cmd.equals("PUSH")) {
                    asm.emit(Opcode.PUSH, clean, null);
                    asm.emit(resolveVal(asm, tokens.get(1)), null, "└── [val: " + tokens.get(1) + "]");
                } else if (cmd.equals("FPUSH")) {
                    asm.emit(Opcode.FPUSH, clean, null);
                    asm.emit(Float.floatToIntBits(Float.parseFloat(tokens.get(1))), null, "└── [val: " + tokens.get(1) + "]");
                } else if (cmd.equals("CPUSH")) {
                    asm.emit(Opcode.CPUSH, clean, null);
                    asm.emit(parseCharOrInt(tokens.get(1)), null, "└── [char: " + tokens.get(1) + "]");
                } else if (cmd.equals("BPUSH")) {
                    asm.emit(Opcode.BPUSH, clean, null);
                    asm.emit(parseCharOrInt(tokens.get(1)) & 0xFF, null, "└── [byte: " + tokens.get(1) + "]");
                } else if (Set.of("JUMP", "JZ", "JNZ").contains(cmd)) {
                    int op = cmd.equals("JUMP") ? Opcode.JUMP : (cmd.equals("JZ") ? Opcode.JZ : Opcode.JNZ);
                    asm.emit(op, clean, null);
                    int target = resolveJumpTarget(asm, tokens.get(1), currentSubroutine);
                    asm.emit(target, null, "└── [target: " + tokens.get(1) + " -> addr " + target + "]");
                } else if (cmd.equals("CALL")) {
                    asm.emit(Opcode.CALL, clean, null);
                    int target = resolveJumpTarget(asm, tokens.get(1), currentSubroutine);
                    asm.emit(target, null, "└── [target: " + tokens.get(1) + " -> addr " + target + "]");
                } else if (cmd.equals("IN")) {
                    asm.emit(Opcode.IN, clean, null);
                    asm.emit(resolveVal(asm, tokens.get(1)), null, "└── [dest: " + tokens.get(1) + "]");
                } else if (cmd.equals("OUT")) {
                    asm.emit(Opcode.OUT, clean, null);
                    if (tokens.size() > 1) {
                        asm.emit(resolveVal(asm, tokens.get(1)), null, "└── [src: " + tokens.get(1) + "]");
                    }
                } else if (Set.of("LOAD", "STORE").contains(cmd)) {
                    int op = cmd.equals("LOAD") ? Opcode.LOAD : Opcode.STORE;
                    asm.emit(op, clean, null);
                    if (tokens.size() > 1) {
                        asm.emit(resolveVal(asm, tokens.get(1)), null, "└── [addr: " + tokens.get(1) + "]");
                    }
                } else if (Set.of("ISTORE", "FSTORE", "CSTORE", "BSTORE").contains(cmd)) {
                    Map<String, Integer> opMap = Map.of("ISTORE", Opcode.ISTORE, "FSTORE", Opcode.FSTORE, "CSTORE", Opcode.CSTORE, "BSTORE", Opcode.BSTORE);
                    asm.emit(opMap.get(cmd), clean, null);
                    int val = cmd.equals("FSTORE") ? Float.floatToIntBits(Float.parseFloat(tokens.get(1))) : parseCharOrInt(tokens.get(1));
                    asm.emit(val, null, "├── [val: " + tokens.get(1) + "]");
                    asm.emit(resolveVal(asm, tokens.get(2)), null, "└── [dest: " + tokens.get(2) + "]");
                } else if (Set.of("LOAD_L", "STORE_L").contains(cmd)) {
                    if (currentSubroutine == null) throw new SyntaxException("Local memory operations used outside subroutine.");
                    int op = cmd.equals("LOAD_L") ? Opcode.LOAD_L : Opcode.STORE_L;
                    asm.emit(op, clean, null);
                    int offset = currentSubroutine.getOrCreateLocal(tokens.get(1));
                    asm.emit(offset, null, "└── [local: '" + tokens.get(1) + "' -> offset " + offset + "]");
                } else if (Set.of("ISTORE_L", "FSTORE_L", "CSTORE_L", "BSTORE_L").contains(cmd)) {
                    if (currentSubroutine == null) throw new SyntaxException("Local memory operations used outside subroutine.");
                    Map<String, Integer> opMap = Map.of("ISTORE_L", Opcode.ISTORE_L, "FSTORE_L", Opcode.FSTORE_L, "CSTORE_L", Opcode.CSTORE_L, "BSTORE_L", Opcode.BSTORE_L);
                    asm.emit(opMap.get(cmd), clean, null);
                    int val = cmd.equals("FSTORE_L") ? Float.floatToIntBits(Float.parseFloat(tokens.get(1))) : parseCharOrInt(tokens.get(1));
                    asm.emit(val, null, "├── [val: " + tokens.get(1) + "]");
                    int offset = currentSubroutine.getOrCreateLocal(tokens.get(2));
                    asm.emit(offset, null, "└── [local: '" + tokens.get(2) + "' -> offset " + offset + "]");
                } else if (zeroOperandOpcodes.containsKey(cmd)) {
                    asm.emit(zeroOperandOpcodes.get(cmd), clean, null);
                } else {
                    throw new SyntaxException("Unknown instruction '" + cmd + "'");
                }
            } catch (Exception e) {
                System.err.printf("[ASSEMBLER ERROR] Line %d: %s\n", lineNum, e.getMessage());
                return -1;
            }
        }

        // ==========================================
        // PASS 3: Write Output Binary (.grro) & Dump
        // ==========================================
        try (FileOutputStream fos = new FileOutputStream(outFilename)) {
            ByteBuffer header = ByteBuffer.allocate(12).order(ByteOrder.LITTLE_ENDIAN);
            header.putInt(asm.globalStart);
            header.putInt(asm.program.size());
            header.putInt(asm.ramData.size());
            fos.write(header.array());

            ByteBuffer progBuf = ByteBuffer.allocate(asm.program.size() * 4).order(ByteOrder.LITTLE_ENDIAN);
            for (int w : asm.program) progBuf.putInt(w);
            fos.write(progBuf.array());

            for (PrimitiveValueBin prim : asm.ramData) {
                fos.write(prim.pack());
            }

            String dumpFilename = outFilename.contains(".") ? outFilename.substring(0, outFilename.lastIndexOf('.')) + ".dump.txt" : outFilename + ".dump.txt";
            writeDumpFile(asm, dumpFilename);

            System.out.printf("[ASSEMBLER SUCCESS] Compiled '%s' -> '%s'\n", inFilename, outFilename);
            System.out.printf("[DUMP GENERATED]    Wrote listing to '%s'\n", dumpFilename);
            System.out.printf("                     %d code words | %d static RAM words.\n", asm.program.size(), asm.ramData.size());
            return 0;
        } catch (IOException e) {
            System.err.println("[ERROR] Failed writing output file: " + e.getMessage());
            return -1;
        }
    }

    private static int resolveVal(AssemblerEngine asm, String token) {
        if (asm.symbolTable.containsKey(token)) return asm.symbolTable.get(token);
        String cleanTok = token.replaceAll("^[:$]+", "");
        if (asm.symbolTable.containsKey(cleanTok)) return asm.symbolTable.get(cleanTok);
        if (asm.symbolTable.containsKey("$" + cleanTok)) return asm.symbolTable.get("$" + cleanTok);
        return parseCharOrInt(token);
    }

    private static int resolveJumpTarget(AssemblerEngine asm, String token, SubroutineCtx currentSubroutine) {
        if (asm.symbolTable.containsKey(token)) return asm.symbolTable.get(token);
        String cleanTok = token.replaceAll("^[:$]+", "");
        if (asm.symbolTable.containsKey(cleanTok)) return asm.symbolTable.get(cleanTok);
        if (asm.symbolTable.containsKey("$" + cleanTok)) return asm.symbolTable.get("$" + cleanTok);

        int relativeOffset = parseCharOrInt(token);
        if (currentSubroutine != null) {
            return currentSubroutine.programLine + relativeOffset;
        }
        return relativeOffset;
    }

    public static class SyntaxException extends RuntimeException {
        public SyntaxException(String msg) { super(msg); }
    }

    public static void main(String[] args) {
        if (args.length < 2) {
            System.out.println("Usage: java Assembler.java <input.grr> <output.grro>");
            System.exit(1);
        }
        int status = compileGrrToBin(args[0], args[1]);
        System.exit(status == 0 ? 0 : 1);
    }
}
