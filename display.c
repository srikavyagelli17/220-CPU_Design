/* display.c
 * All printing: ISA table, listings, registers, and the 32-word RAM dump.
 * ASCII is the high byte then the low byte. '.' stands for non-printable.
 */

#include "display.h"

#include <stdio.h>

static int bslen(const char *s)
{
    int n = 0;
    while (s[n] != 0) {
        n++;
    }
    return n;
}

static void bapp(char *b, int cap, const char *s)
{
    int n = bslen(b);
    int i = 0;

    while (s[i] != 0 && n < cap - 1) {
        b[n] = s[i];
        n++;
        i++;
    }
    b[n] = 0;
}

static void bapp_ch(char *b, int cap, char c)
{
    int n = bslen(b);
    if (n < cap - 1) {
        b[n] = c;
        b[n + 1] = 0;
    }
}

static void bapp_dec(char *b, int cap, int v)
{
    char tmp[16];
    int i = 0;
    int neg = 0;

    if (v < 0) {
        neg = 1;
        v = -v;
    }
    if (v == 0) {
        tmp[i] = '0';
        i++;
    }
    while (v > 0 && i < (int)sizeof tmp) {
        tmp[i] = (char)('0' + (v % 10));
        i++;
        v /= 10;
    }
    if (neg) {
        bapp_ch(b, cap, '-');
    }
    while (i > 0) {
        i--;
        bapp_ch(b, cap, tmp[i]);
    }
}

static void bapp_hex2(char *b, int cap, unsigned v)
{
    const char *hex = "0123456789ABCDEF";
    bapp(b, cap, "0x");
    bapp_ch(b, cap, hex[(v >> 4) & 0xFu]);
    bapp_ch(b, cap, hex[v & 0xFu]);
}

static char glyph(int c)
{
    if (c >= 32 && c <= 126) {
        return (char)c;
    }
    return '.';
}

static void print_bits(unsigned v, int nbits)
{
    int i;
    for (i = nbits - 1; i >= 0; i--) {
        printf("%c", (v & (1u << i)) ? '1' : '0');
    }
}

/* opcode rd rs imm, as in: 0110 01 10 00000000 */
static void print_fields(u16 w)
{
    print_bits((unsigned)((w >> 12) & 0xFu), 4);
    printf(" ");
    print_bits((unsigned)((w >> 10) & 0x3u), 2);
    printf(" ");
    print_bits((unsigned)((w >> 8) & 0x3u), 2);
    printf(" ");
    print_bits((unsigned)(w & 0xFFu), 8);
}

/* 16 bits in groups of four: 0000 0000 0100 0000 */
static void print_nibbles(u16 w)
{
    print_bits((unsigned)((w >> 12) & 0xFu), 4);
    printf(" ");
    print_bits((unsigned)((w >> 8) & 0xFu), 4);
    printf(" ");
    print_bits((unsigned)((w >> 4) & 0xFu), 4);
    printf(" ");
    print_bits((unsigned)(w & 0xFu), 4);
}

static int sex_disp(unsigned imm)
{
    imm &= 0xFFu;
    if (imm & 0x80u) {
        return (int)imm - 256;
    }
    return (int)imm;
}

static void bapp_reg(char *b, int cap, unsigned reg)
{
    bapp(b, cap, "R");
    bapp_ch(b, cap, (char)('0' + (reg & 3u)));
}

/* Label name if this address has one, otherwise a 2-digit hex address. */
static void bapp_target(char *b, int cap, const Image *img, unsigned addr)
{
    const char *lab = label_at(img, (u16)addr);
    if (lab != 0) {
        bapp(b, cap, lab);
    } else {
        bapp_hex2(b, cap, addr);
    }
}

static void bapp_rr(char *b, int cap, const char *op, unsigned rd, unsigned rs)
{
    bapp(b, cap, op);
    bapp(b, cap, " ");
    bapp_reg(b, cap, rd);
    bapp(b, cap, ", ");
    bapp_reg(b, cap, rs);
}

/* Disassemble one instruction word into out. */
static void disasm(u16 w, const Image *img, char *out, int cap)
{
    unsigned op = (unsigned)((w >> 12) & 0xFu);
    unsigned rd = (unsigned)((w >> 10) & 0x3u);
    unsigned rs = (unsigned)((w >> 8) & 0x3u);
    unsigned imm = (unsigned)(w & 0xFFu);

    out[0] = 0;
    switch (op) {
    case OP_HALT:
        bapp(out, cap, "HALT");
        break;
    case OP_LOADI:
        bapp(out, cap, "LOADI ");
        bapp_reg(out, cap, rd);
        bapp(out, cap, ", ");
        bapp_dec(out, cap, sex_disp(imm));
        break;
    case OP_LOAD:
        bapp(out, cap, "LOAD ");
        bapp_reg(out, cap, rd);
        bapp(out, cap, ", [");
        bapp_target(out, cap, img, imm);
        bapp(out, cap, "]");
        break;
    case OP_STORE:
        bapp(out, cap, "STORE ");
        bapp_reg(out, cap, rd);
        bapp(out, cap, ", [");
        bapp_target(out, cap, img, imm);
        bapp(out, cap, "]");
        break;
    case OP_ADD:
        bapp_rr(out, cap, "ADD", rd, rs);
        break;
    case OP_SUB:
        bapp_rr(out, cap, "SUB", rd, rs);
        break;
    case OP_MUL:
        bapp_rr(out, cap, "MUL", rd, rs);
        break;
    case OP_ADDI:
        bapp(out, cap, "ADDI ");
        bapp_reg(out, cap, rd);
        bapp(out, cap, ", ");
        bapp_dec(out, cap, sex_disp(imm));
        break;
    case OP_MOV:
        bapp_rr(out, cap, "MOV", rd, rs);
        break;
    case OP_CMP:
        bapp_rr(out, cap, "CMP", rd, rs);
        break;
    case OP_JMP:
        bapp(out, cap, "JMP ");
        bapp_target(out, cap, img, imm);
        break;
    case OP_JZ:
        bapp(out, cap, "JZ ");
        bapp_target(out, cap, img, imm);
        break;
    case OP_JNZ:
        bapp(out, cap, "JNZ ");
        bapp_target(out, cap, img, imm);
        break;
    default:
        bapp(out, cap, "???");
        break;
    }
}

/* Code is disassembled. Data shows DATA and its label. Empty shows --. */
static void decode_cell(const Image *img, u16 addr, char *out, int cap)
{
    const char *lab = label_at(img, addr);
    char inst[64];

    out[0] = 0;
    if (img->kind[addr] == KIND_DATA) {
        bapp(out, cap, "DATA");
        if (lab != 0) {
            bapp(out, cap, " ");
            bapp(out, cap, lab);
        }
        return;
    }
    if (img->kind[addr] == KIND_CODE) {
        if (lab != 0) {
            bapp(out, cap, lab);
            bapp(out, cap, ": ");
        }
        disasm(img->ram[addr], img, inst, 64);
        bapp(out, cap, inst);
        return;
    }
    if (lab != 0) {
        bapp(out, cap, lab);
        return;
    }
    bapp(out, cap, "--");
}

static const char *fault_text(int fault)
{
    if (fault == FAULT_PC) {
        return "PC out of range";
    }
    if (fault == FAULT_OP) {
        return "unknown opcode";
    }
    if (fault == FAULT_ADDR) {
        return "address out of range";
    }
    return "unknown fault";
}

void display_isa(void)
{
    printf("CPU SIMULATOR\n");
    printf("WORD 16-bit   RAM 32 words 0x00-0x1F   [opcode:4][rd:2][rs:2][imm/addr:8]\n");
    printf("REGS R0 R1 R2 R3 PC IR Z   IMM signed 8-bit   ADDR unsigned   WRAP 16-bit\n");
    printf("\n");
    printf("OPCODE  MNEMONIC  FORMAT              MEANING\n");
    printf("0000    HALT      HALT                stop execution\n");
    printf("0001    LOADI     LOADI rd, imm8      rd = sign_extend(imm8); set Z\n");
    printf("0010    LOAD      LOAD rd, [addr]     rd = RAM[addr]; set Z\n");
    printf("0011    STORE     STORE rd, [addr]    RAM[addr] = rd\n");
    printf("0100    ADD       ADD rd, rs          rd = rd + rs; set Z\n");
    printf("0101    SUB       SUB rd, rs          rd = rd - rs; set Z\n");
    printf("0110    MUL       MUL rd, rs          rd = low16(rd * rs); set Z\n");
    printf("0111    ADDI      ADDI rd, imm8       rd = rd + sign_extend(imm8); set Z\n");
    printf("1000    MOV       MOV rd, rs          rd = rs; set Z\n");
    printf("1001    CMP       CMP rd, rs          Z = (rd == rs); no register write\n");
    printf("1010    JMP       JMP addr            PC = addr\n");
    printf("1011    JZ        JZ addr             if Z then PC = addr\n");
    printf("1100    JNZ       JNZ addr            if Z is clear then PC = addr\n");
    printf("\n");
    printf("KEY\n");
    printf("* = written this step\n");
    printf("> = executed instruction\n");
    printf("ASCII = high byte, low byte; '.' = non-printable\n");
    printf("header PC = executed instruction    register PC = next fetch\n");
    printf("\n");
}

void display_listing(const char *heading, const char **lines)
{
    int i;

    printf("%s\n", heading);
    if (lines == 0) {
        printf("\n");
        return;
    }
    for (i = 0; lines[i] != 0; i++) {
        printf("    %s\n", lines[i]);
    }
    printf("\n");
}

void display_state(int step, const Cpu *cpu, const Image *img, const u16 *prev,
                   int initial, u16 fetch_pc, u16 ir, int fault)
{
    int i;
    char text[80];

    printf("--------------------------------------------------------------------------------\n");
    if (initial) {
        printf("STEP %02d | PC=0x%02X | (loaded)\n", step, (unsigned)cpu->pc);
    } else {
        disasm(ir, img, text, 80);
        printf("STEP %02d | PC=0x%02X | %s | ", step, (unsigned)fetch_pc, text);
        print_fields(ir);
        printf("\n");
    }

    printf("R0=%5u (0x%04X)  R1=%5u (0x%04X)  R2=%5u (0x%04X)  R3=%5u (0x%04X)\n",
           (unsigned)cpu->r[0], (unsigned)cpu->r[0],
           (unsigned)cpu->r[1], (unsigned)cpu->r[1],
           (unsigned)cpu->r[2], (unsigned)cpu->r[2],
           (unsigned)cpu->r[3], (unsigned)cpu->r[3]);
    if (initial) {
        printf("PC=0x%02X  IR=(none)  Z=%d  HALTED=%d\n",
               (unsigned)cpu->pc, cpu->z, cpu->halted);
    } else {
        printf("PC=0x%02X  IR=0x%04X (", (unsigned)cpu->pc, (unsigned)cpu->ir);
        print_fields(cpu->ir);
        printf(")  Z=%d  HALTED=%d\n", cpu->z, cpu->halted);
    }
    if (fault != FAULT_NONE) {
        printf("FAULT: %s\n", fault_text(fault));
    }

    printf("   ADDR | HEX    | BINARY              | ASCII | DECODED / LABEL\n");
    for (i = 0; i < RAM_WORDS; i++) {
        char mark[3];
        char decoded[80];
        int star = 0;
        /* '>' tracks the header PC: the instruction just executed, or the
         * entry point on the load dump. Register PC may already be the next word. */
        int cursor = initial ? (int)cpu->pc : (int)fetch_pc;
        int here = (i == cursor);
        u16 word = cpu->ram[i];

        if (initial) {
            if (img->kind[i] != KIND_EMPTY) {
                star = 1; /* assembler placed this word */
            }
        } else if (word != prev[i]) {
            star = 1;
        }

        mark[0] = star ? '*' : ' ';
        mark[1] = here ? '>' : ' ';
        mark[2] = 0;

        decode_cell(img, (u16)i, decoded, 80);
        printf("%-2s 0x%02X | 0x%04X | ", mark, i, (unsigned)word);
        print_nibbles(word);
        printf(" | %c%c   | %s\n",
               glyph((int)((word >> 8) & 0xFFu)),
               glyph((int)(word & 0xFFu)),
               decoded);
    }
}
