/* cpu.c
 * Fetch, decode, and execute one instruction per call.
 * Arithmetic is 16-bit and wraps. LOADI/ADDI immediates are sign-extended.
 */

#include "cpu.h"

/* Sign-extend an 8-bit immediate to 16 bits. */
static u16 sx8(u16 imm)
{
    imm = (u16)(imm & 0xFFu);
    if (imm & 0x80u) {
        return (u16)(imm | 0xFF00u);
    }
    return imm;
}

/* Z is 1 exactly when v is zero. */
static void set_z(Cpu *cpu, u16 v)
{
    cpu->z = (v == 0);
}

/* Reject addresses outside the 32-word RAM. */
static int bad_addr(u16 addr)
{
    return addr >= RAM_WORDS;
}

void cpu_clear(Cpu *cpu)
{
    int i;

    for (i = 0; i < NREGS; i++) {
        cpu->r[i] = 0;
    }
    for (i = 0; i < RAM_WORDS; i++) {
        cpu->ram[i] = 0;
    }
    cpu->pc = 0;
    cpu->ir = 0;
    cpu->z = 0;
    cpu->halted = 0;
}

void cpu_load(Cpu *cpu, const u16 *ram)
{
    int i;

    cpu_clear(cpu);
    for (i = 0; i < RAM_WORDS; i++) {
        cpu->ram[i] = ram[i];
    }
}

int cpu_step(Cpu *cpu, StepResult *step)
{
    u16 word;
    u16 op, rd, rs, imm;
    u16 result;

    step->ran = 0;
    step->halted = 0;
    step->fault = FAULT_NONE;
    step->fetch_pc = cpu->pc;
    step->ir = cpu->ir;

    if (cpu->halted) {
        step->halted = 1;
        return 0;
    }
    if (bad_addr(cpu->pc)) {
        step->fault = FAULT_PC;
        cpu->halted = 1;
        return 0;
    }

    /* Fetch: IR gets the word at PC, then PC advances to the next word. */
    word = cpu->ram[cpu->pc];
    cpu->ir = word;
    step->ir = word;
    step->fetch_pc = cpu->pc;
    step->ran = 1;
    cpu->pc = (u16)(cpu->pc + 1);

    op = (u16)((word >> 12) & 0xFu);
    rd = (u16)((word >> 10) & 0x3u);
    rs = (u16)((word >> 8) & 0x3u);
    imm = (u16)(word & 0xFFu);

    switch (op) {
    case OP_HALT:
        cpu->halted = 1;
        step->halted = 1;
        return 0;

    case OP_LOADI: /* rd = sign_extend(imm) */
        result = sx8(imm);
        cpu->r[rd] = result;
        set_z(cpu, result);
        break;

    case OP_LOAD: /* rd = RAM[addr] */
        if (bad_addr(imm)) {
            step->fault = FAULT_ADDR;
            cpu->halted = 1;
            return 0;
        }
        result = cpu->ram[imm];
        cpu->r[rd] = result;
        set_z(cpu, result);
        break;

    case OP_STORE: /* RAM[addr] = rd */
        if (bad_addr(imm)) {
            step->fault = FAULT_ADDR;
            cpu->halted = 1;
            return 0;
        }
        cpu->ram[imm] = cpu->r[rd];
        break;

    case OP_ADD:
        result = (u16)(cpu->r[rd] + cpu->r[rs]);
        cpu->r[rd] = result;
        set_z(cpu, result);
        break;

    case OP_SUB:
        result = (u16)(cpu->r[rd] - cpu->r[rs]);
        cpu->r[rd] = result;
        set_z(cpu, result);
        break;

    case OP_MUL: /* low 16 bits of the product; multiply in unsigned to avoid overflow */
        result = (u16)((unsigned)cpu->r[rd] * (unsigned)cpu->r[rs]);
        cpu->r[rd] = result;
        set_z(cpu, result);
        break;

    case OP_ADDI:
        result = (u16)(cpu->r[rd] + sx8(imm));
        cpu->r[rd] = result;
        set_z(cpu, result);
        break;

    case OP_MOV:
        result = cpu->r[rs];
        cpu->r[rd] = result;
        set_z(cpu, result);
        break;

    case OP_CMP: /* Z from equality; rd and rs stay as they are */
        cpu->z = (cpu->r[rd] == cpu->r[rs]);
        break;

    case OP_JMP:
        if (bad_addr(imm)) {
            step->fault = FAULT_ADDR;
            cpu->halted = 1;
            return 0;
        }
        cpu->pc = imm;
        break;

    case OP_JZ:
        if (cpu->z) {
            if (bad_addr(imm)) {
                step->fault = FAULT_ADDR;
                cpu->halted = 1;
                return 0;
            }
            cpu->pc = imm;
        }
        break;

    case OP_JNZ:
        if (!cpu->z) {
            if (bad_addr(imm)) {
                step->fault = FAULT_ADDR;
                cpu->halted = 1;
                return 0;
            }
            cpu->pc = imm;
        }
        break;

    default:
        step->fault = FAULT_OP;
        cpu->halted = 1;
        return 0;
    }

    return 1;
}
