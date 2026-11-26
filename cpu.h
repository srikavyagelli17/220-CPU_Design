/* cpu.h
 * 16-bit word, 32-word von Neumann RAM, registers R0-R3, PC, IR, flag Z.
 * Instruction word: [opcode:4][rd:2][rs:2][imm/addr:8]
 */

#ifndef CPU_H
#define CPU_H

#define RAM_WORDS 32
#define NREGS     4
#define MAX_STEPS 500

typedef unsigned short u16;

#define OP_HALT  0x0 /* HALT                  */
#define OP_LOADI 0x1 /* LOADI rd, imm8        */
#define OP_LOAD  0x2 /* LOAD  rd, [addr]      */
#define OP_STORE 0x3 /* STORE rd, [addr]      */
#define OP_ADD   0x4 /* ADD   rd, rs          */
#define OP_SUB   0x5 /* SUB   rd, rs          */
#define OP_MUL   0x6 /* MUL   rd, rs          */
#define OP_ADDI  0x7 /* ADDI  rd, imm8        */
#define OP_MOV   0x8 /* MOV   rd, rs          */
#define OP_CMP   0x9 /* CMP   rd, rs          */
#define OP_JMP   0xA /* JMP   addr            */
#define OP_JZ    0xB /* JZ    addr            */
#define OP_JNZ   0xC /* JNZ   addr            */

#define KIND_EMPTY 0
#define KIND_CODE  1
#define KIND_DATA  2

#define FAULT_NONE 0
#define FAULT_PC   1
#define FAULT_OP   2
#define FAULT_ADDR 3

typedef struct {
    u16 r[NREGS];
    u16 pc;
    u16 ir;
    int z;       /* 1 when the last flag-writing result was zero */
    int halted;
    u16 ram[RAM_WORDS];
} Cpu;

typedef struct {
    int ran;     /* 1 if an instruction was fetched */
    int halted;  /* 1 if this step executed HALT */
    int fault;   /* FAULT_* */
    u16 fetch_pc;
    u16 ir;
} StepResult;

/* Zero registers, RAM, PC, IR, and Z. */
void cpu_clear(Cpu *cpu);

/* Clear the CPU, then copy 32 words of machine code and data into RAM. */
void cpu_load(Cpu *cpu, const u16 *ram);

/* Fetch, decode, and execute one instruction.
 * Returns 1 to keep running, 0 when halted or faulted.
 * PC is incremented on fetch; jumps overwrite PC during execute.
 */
int cpu_step(Cpu *cpu, StepResult *step);

#endif
