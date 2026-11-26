/* programs.c
 * Program A is written in assembly.
 * Program B is the C factorial below, then the same logic in assembly.
 * Each assembly line carries the C (or equivalent) line after ';'.
 *
 * int factorial(void) {
 *     int result = 1;
 *     int n = 8;
 *     int one = 1;
 *     int zero = 0;
 *     while (n != zero) {
 *         result = result * n;
 *         n = n - one;
 *     }
 *     return result;
 * }
 */

#include "programs.h"

const char *prog_pow[] = {
    "; acc = 1; multiply by base (8) exactly exp (2) times",
    "LOADI R2, 1          ; acc = 1",
    "LOADI R0, 8          ; base = 8",
    "LOADI R1, 2          ; exp = 2",
    "LOADI R3, 1          ; one = 1",
    "LOOP: MUL R2, R0     ; acc = acc * base",
    "SUB R1, R3           ; exp = exp - 1",
    "JNZ LOOP             ; repeat while exp != 0",
    "STORE R2, [RESULT]   ; RESULT = acc",
    "HALT",
    ".ORG 0x10",
    "RESULT: .WORD 0",
    0
};

const char *prog_fact_c[] = {
    "int factorial(void) {",
    "    int result = 1;",
    "    int n = 8;",
    "    int one = 1;",
    "    int zero = 0;",
    "    while (n != zero) {",
    "        result = result * n;",
    "        n = n - one;",
    "    }",
    "    return result;",
    "}",
    0
};

const char *prog_fact[] = {
    "LOADI R1, 1              ; int result = 1;",
    "LOADI R0, 8              ; int n = 8;",
    "LOADI R2, 1              ; int one = 1;",
    "LOADI R3, 0              ; int zero = 0;",
    "LOOP: CMP R0, R3         ; while (n != zero) {",
    "JZ DONE                  ;     exit when n == zero",
    "MUL R1, R0               ;     result = result * n;",
    "SUB R0, R2               ;     n = n - one;",
    "JMP LOOP                 ; }",
    "DONE: STORE R1, [RESULT] ; return result;",
    "HALT",
    ".ORG 0x10",
    "RESULT: .WORD 0",
    0
};
