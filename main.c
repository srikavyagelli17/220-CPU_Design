/* main.c
 * Assemble and run Program A (8^2), then Program B (8!), each in fresh RAM.
 * Prints the ISA table, a full RAM dump after load and after every instruction,
 * and a PASS/FAIL line from the RESULT word.
 */

#include "cpu.h"
#include "assembler.h"
#include "display.h"
#include "programs.h"

#include <stdio.h>

static void snapshot(const Cpu *cpu, u16 *prev)
{
    int i;
    for (i = 0; i < RAM_WORDS; i++) {
        prev[i] = cpu->ram[i];
    }
}

/* Returns 1 when RESULT matches expected. */
static int run_program(const char *title, const char *blurb,
                       const char **c_lines, const char **asm_lines,
                       const char *expr, unsigned expected)
{
    Image img;
    Cpu cpu;
    u16 prev[RAM_WORDS];
    int steps = 0;
    int slot;
    unsigned actual;
    int pass;
    int hit_limit = 0;

    printf("================================================================================\n");
    printf("%s\n", title);
    printf("%s\n", blurb);
    printf("================================================================================\n");
    if (c_lines != 0) {
        display_listing("C function:", c_lines);
    }
    display_listing("Assembly:", asm_lines);

    if (assemble(asm_lines, &img) != 0) {
        printf("RESULT: %s = ?  expected %u  FAIL\n\n", expr, expected);
        return 0;
    }

    cpu_load(&cpu, img.ram);
    snapshot(&cpu, prev); /* same image the load dump shows; nothing has executed */
    display_state(0, &cpu, &img, prev, 1, 0, 0, FAULT_NONE);

    while (steps < MAX_STEPS) {
        StepResult sr;
        int cont = cpu_step(&cpu, &sr);

        if (!sr.ran) {
            if (sr.fault != FAULT_NONE) {
                printf("FAULT before fetch (code %d)\n", sr.fault);
            }
            break;
        }
        steps++;
        display_state(steps, &cpu, &img, prev, 0, sr.fetch_pc, sr.ir, sr.fault);
        snapshot(&cpu, prev);
        if (!cont) {
            break;
        }
    }
    if (steps >= MAX_STEPS && !cpu.halted) {
        hit_limit = 1;
        printf("STOPPED: step limit %d reached\n", MAX_STEPS);
    }

    slot = label_addr(&img, "RESULT");
    if (slot < 0) {
        printf("RESULT: %s = ?  expected %u  FAIL\n\n", expr, expected);
        return 0;
    }
    actual = cpu.ram[slot];
    pass = (!hit_limit && cpu.halted && actual == expected);
    printf("RESULT: %s = %u  expected %u  %s\n\n",
           expr, actual, expected, pass ? "PASS" : "FAIL");
    return pass;
}

int main(void)
{
    int ok_a;
    int ok_b;

    display_isa();
    ok_a = run_program(
        "PROGRAM A: 8^2",
        "Multiply 1 by 8, twice. Answer is stored in RESULT.",
        0, prog_pow, "8^2", 64u);
    ok_b = run_program(
        "PROGRAM B: 8!",
        "Hand-translated from the C function printed below.",
        prog_fact_c, prog_fact, "8!", 40320u);

    if (ok_a && ok_b) {
        printf("BOTH PROGRAMS PASSED\n");
        return 0;
    }
    printf("DONE WITH FAILURES\n");
    return 1;
}
