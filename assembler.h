/* assembler.h
 * Two-pass assembler: labels, .ORG, and .WORD into a 32-word image.
 */

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "cpu.h"

#define MAX_LABELS 32
#define LABEL_LEN  16

typedef struct {
    char name[LABEL_LEN];
    u16 addr;
} Label;

typedef struct {
    Label lab[MAX_LABELS];
    int nlabels;
} SymTab;

typedef struct {
    u16 ram[RAM_WORDS];
    int kind[RAM_WORDS]; /* KIND_EMPTY, KIND_CODE, or KIND_DATA */
    SymTab sym;
    int error;
} Image;

/* Assemble a NULL-terminated list of source lines. Returns 0 on success. */
int assemble(const char **lines, Image *img);

/* Name of the first label at addr, or 0 if that word has no label. */
const char *label_at(const Image *img, u16 addr);

/* Address of a label, or -1 if it was never defined. */
int label_addr(const Image *img, const char *name);

#endif
