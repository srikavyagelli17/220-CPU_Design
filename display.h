/* display.h
 * Encoding table, source listings, and the per-step RAM / CPU dump.
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include "assembler.h"

/* Print the ISA table once, for the report. */
void display_isa(void);

/* Print a heading and each source line. */
void display_listing(const char *heading, const char **lines);

/* One trace frame. initial=1 is the dump right after loading.
 * fetch_pc / ir describe the instruction just executed (ignored when initial).
 */
void display_state(int step, const Cpu *cpu, const Image *img, const u16 *prev,
                   int initial, u16 fetch_pc, u16 ir, int fault);

#endif
