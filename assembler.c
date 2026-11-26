/* assembler.c
 * Text assembly to machine code. No C string or stdlib helpers.
 * Pass 1 records labels. Pass 2 emits words and resolves names.
 */

#include "assembler.h"

#include <stdio.h>

static int is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static int is_alpha(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

static int is_alnum(char c)
{
    return is_alpha(c) || is_digit(c);
}

static int slen(const char *s)
{
    int n = 0;
    while (s[n] != 0) {
        n++;
    }
    return n;
}

static int streq(const char *a, const char *b)
{
    int i = 0;
    while (a[i] != 0 && a[i] == b[i]) {
        i++;
    }
    return a[i] == 0 && b[i] == 0;
}

static char *skip_space(char *s)
{
    while (is_space(*s)) {
        s++;
    }
    return s;
}

static void trim_inplace(char *s)
{
    int i = 0;
    int j = 0;
    int end;

    while (is_space(s[i])) {
        i++;
    }
    while (s[i] != 0) {
        s[j] = s[i];
        i++;
        j++;
    }
    s[j] = 0;
    end = j;
    while (end > 0 && is_space(s[end - 1])) {
        end--;
        s[end] = 0;
    }
}

/* Copy src into dst and drop a ';' comment plus trailing space. */
static void strip_comment(const char *src, char *dst, int cap)
{
    int i = 0;

    while (src[i] != 0 && src[i] != ';' && i < cap - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
    while (i > 0 && is_space(dst[i - 1])) {
        i--;
        dst[i] = 0;
    }
}

/* Read one whitespace-delimited token. Returns 0 if the line is finished. */
static int take_token(char *s, char *out, int cap, char **rest)
{
    int n = 0;

    s = skip_space(s);
    if (*s == 0) {
        out[0] = 0;
        *rest = s;
        return 0;
    }
    while (*s != 0 && !is_space(*s) && n < cap - 1) {
        out[n] = *s;
        n++;
        s++;
    }
    out[n] = 0;
    *rest = s;
    return 1;
}

/* Split "a, b" into two trimmed operands. b is empty when there is no comma. */
static void split_ops(char *s, char *a, char *b, int cap)
{
    int i = 0;
    int j = 0;

    s = skip_space(s);
    while (*s != 0 && *s != ',' && i < cap - 1) {
        a[i] = *s;
        i++;
        s++;
    }
    a[i] = 0;
    trim_inplace(a);
    if (*s == ',') {
        s++;
    }
    s = skip_space(s);
    while (*s != 0 && j < cap - 1) {
        b[j] = *s;
        j++;
        s++;
    }
    b[j] = 0;
    trim_inplace(b);
}

/* Turn "[RESULT]" into "RESULT". Brackets are optional. */
static void strip_brackets(char *s)
{
    int n;
    int i;

    trim_inplace(s);
    n = slen(s);
    if (n >= 2 && s[0] == '[' && s[n - 1] == ']') {
        for (i = 0; i < n - 2; i++) {
            s[i] = s[i + 1];
        }
        s[n - 2] = 0;
        trim_inplace(s);
    }
}

static int hexval(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

/* Decimal or 0x hex, optional leading + or -. Returns 0 on junk. */
static int parse_num(const char *s, int *out)
{
    int sign = 1;
    int hex = 0;
    int digits = 0;
    unsigned v = 0;

    if (*s == '+' || *s == '-') {
        if (*s == '-') {
            sign = -1;
        }
        s++;
    }
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        hex = 1;
        s += 2;
    }
    if (hex) {
        while (*s != 0) {
            int d = hexval(*s);
            if (d < 0 || v > 0x0FFFFFFFu) {
                return 0;
            }
            v = v * 16u + (unsigned)d;
            digits++;
            s++;
        }
    } else {
        while (*s != 0) {
            if (!is_digit(*s) || v > 429496729u) {
                return 0;
            }
            v = v * 10u + (unsigned)(*s - '0');
            digits++;
            s++;
        }
    }
    if (digits == 0) {
        return 0;
    }
    if (sign < 0) {
        if (v > 32768u) {
            return 0;
        }
        *out = -(int)v;
    } else {
        if (v > 65535u) {
            return 0;
        }
        *out = (int)v;
    }
    return 1;
}

static int is_label_name(const char *s)
{
    int i;

    if (!is_alpha(s[0])) {
        return 0;
    }
    for (i = 1; s[i] != 0; i++) {
        if (!is_alnum(s[i])) {
            return 0;
        }
    }
    return 1;
}

static int parse_reg(const char *s, int *reg)
{
    if ((s[0] == 'R' || s[0] == 'r') &&
        s[1] >= '0' && s[1] <= '3' && s[2] == 0) {
        *reg = s[1] - '0';
        return 1;
    }
    return 0;
}

/* Signed 8-bit immediate, returned already masked to 0..255. */
static int parse_imm(const char *s, int *imm)
{
    int v;

    if (!parse_num(s, &v) || v < -128 || v > 127) {
        return 0;
    }
    *imm = v & 0xFF;
    return 1;
}

static int fail(Image *img, int line, const char *msg, const char *text)
{
    printf("ASSEMBLE FAIL line %d: %s\n", line, msg);
    if (text != 0) {
        printf("    %s\n", text);
    }
    img->error = 1;
    return -1;
}

static void clear_image(Image *img)
{
    int i;

    for (i = 0; i < RAM_WORDS; i++) {
        img->ram[i] = 0;
        img->kind[i] = KIND_EMPTY;
    }
    img->sym.nlabels = 0;
    img->error = 0;
}

static int add_label(Image *img, const char *name, int addr, int line, const char *text)
{
    int i;
    int n;
    Label *lab;

    if (!is_label_name(name)) {
        return fail(img, line, "bad label", text);
    }
    for (i = 0; i < img->sym.nlabels; i++) {
        if (streq(img->sym.lab[i].name, name)) {
            return fail(img, line, "duplicate label", text);
        }
    }
    if (img->sym.nlabels >= MAX_LABELS) {
        return fail(img, line, "too many labels", text);
    }
    n = slen(name);
    if (n >= LABEL_LEN) {
        return fail(img, line, "label too long", text);
    }
    lab = &img->sym.lab[img->sym.nlabels];
    for (i = 0; i < n; i++) {
        lab->name[i] = name[i];
    }
    lab->name[n] = 0;
    lab->addr = (u16)addr;
    img->sym.nlabels++;
    return 0;
}

static u16 enc(unsigned op, unsigned rd, unsigned rs, unsigned imm)
{
    return (u16)((op << 12) | ((rd & 3u) << 10) | ((rs & 3u) << 8) | (imm & 0xFFu));
}

/* Number or label. Forward labels are accepted only while recording (pass 1). */
static int resolve_addr(Image *img, const char *s, int *addr, int need_label)
{
    int v;
    int i;

    if (s[0] == 0) {
        return 0;
    }
    if (parse_num(s, &v)) {
        if (v < 0 || v >= RAM_WORDS) {
            return 0;
        }
        *addr = v;
        return 1;
    }
    if (!is_label_name(s)) {
        return 0;
    }
    if (!need_label) {
        *addr = 0;
        return 1;
    }
    for (i = 0; i < img->sym.nlabels; i++) {
        if (streq(img->sym.lab[i].name, s)) {
            *addr = (int)img->sym.lab[i].addr;
            return 1;
        }
    }
    return 0;
}

/* Walk every source line. emit=0 records labels, emit=1 writes RAM. */
static int walk(const char **lines, Image *img, int emit)
{
    int li;
    int lc = 0;

    for (li = 0; lines[li] != 0; li++) {
        char buf[160];
        char tok[64];
        char mnem[16];
        char op1[48];
        char op2[48];
        char *rest;
        int line = li + 1;

        strip_comment(lines[li], buf, 160);
        if (!take_token(buf, tok, 64, &rest)) {
            continue; /* blank or comment-only line */
        }

        /* A token ending in ':' is a label at the current location. */
        if (tok[0] != 0 && tok[slen(tok) - 1] == ':') {
            tok[slen(tok) - 1] = 0;
            if (!emit) {
                if (add_label(img, tok, lc, line, lines[li]) != 0) {
                    return -1;
                }
            }
            if (!take_token(rest, tok, 64, &rest)) {
                continue; /* label on its own line */
            }
        }

        if (slen(tok) >= (int)sizeof mnem) {
            return fail(img, line, "mnemonic too long", lines[li]);
        }
        {
            int k;
            for (k = 0; tok[k] != 0; k++) {
                mnem[k] = tok[k];
            }
            mnem[k] = 0;
        }
        split_ops(rest, op1, op2, 48);

        if (streq(mnem, ".ORG")) {
            int origin;
            if (op2[0] != 0 || !parse_num(op1, &origin) ||
                origin < 0 || origin >= RAM_WORDS) {
                return fail(img, line, "bad .ORG", lines[li]);
            }
            lc = origin;
            continue;
        }

        if (lc < 0 || lc >= RAM_WORDS) {
            return fail(img, line, "address out of range", lines[li]);
        }

        if (streq(mnem, ".WORD")) {
            int v;
            if (op2[0] != 0 || !parse_num(op1, &v) || v < -32768 || v > 65535) {
                return fail(img, line, "bad .WORD", lines[li]);
            }
            if (emit) {
                if (img->kind[lc] != KIND_EMPTY) {
                    return fail(img, line, "overlap", lines[li]);
                }
                img->ram[lc] = (u16)v;
                img->kind[lc] = KIND_DATA;
            }
            lc++;
            continue;
        }

        if (streq(mnem, "HALT")) {
            if (op1[0] != 0 || op2[0] != 0) {
                return fail(img, line, "HALT takes no operands", lines[li]);
            }
            if (emit) {
                if (img->kind[lc] != KIND_EMPTY) {
                    return fail(img, line, "overlap", lines[li]);
                }
                img->ram[lc] = enc(OP_HALT, 0, 0, 0);
                img->kind[lc] = KIND_CODE;
            }
            lc++;
            continue;
        }

        if (streq(mnem, "LOADI") || streq(mnem, "ADDI")) {
            int rd, imm, op;
            if (!parse_reg(op1, &rd) || !parse_imm(op2, &imm)) {
                return fail(img, line, "bad immediate instruction", lines[li]);
            }
            op = streq(mnem, "LOADI") ? OP_LOADI : OP_ADDI;
            if (emit) {
                if (img->kind[lc] != KIND_EMPTY) {
                    return fail(img, line, "overlap", lines[li]);
                }
                img->ram[lc] = enc((unsigned)op, (unsigned)rd, 0, (unsigned)imm);
                img->kind[lc] = KIND_CODE;
            }
            lc++;
            continue;
        }

        if (streq(mnem, "LOAD") || streq(mnem, "STORE")) {
            int rd, addr, op;
            strip_brackets(op2);
            if (!parse_reg(op1, &rd) || !resolve_addr(img, op2, &addr, emit)) {
                return fail(img, line, "bad memory instruction", lines[li]);
            }
            op = streq(mnem, "LOAD") ? OP_LOAD : OP_STORE;
            if (emit) {
                if (img->kind[lc] != KIND_EMPTY) {
                    return fail(img, line, "overlap", lines[li]);
                }
                img->ram[lc] = enc((unsigned)op, (unsigned)rd, 0, (unsigned)addr);
                img->kind[lc] = KIND_CODE;
            }
            lc++;
            continue;
        }

        if (streq(mnem, "ADD") || streq(mnem, "SUB") || streq(mnem, "MUL") ||
            streq(mnem, "MOV") || streq(mnem, "CMP")) {
            int rd, rs, op;
            if (!parse_reg(op1, &rd) || !parse_reg(op2, &rs)) {
                return fail(img, line, "bad register instruction", lines[li]);
            }
            if (streq(mnem, "ADD")) {
                op = OP_ADD;
            } else if (streq(mnem, "SUB")) {
                op = OP_SUB;
            } else if (streq(mnem, "MUL")) {
                op = OP_MUL;
            } else if (streq(mnem, "MOV")) {
                op = OP_MOV;
            } else {
                op = OP_CMP;
            }
            if (emit) {
                if (img->kind[lc] != KIND_EMPTY) {
                    return fail(img, line, "overlap", lines[li]);
                }
                img->ram[lc] = enc((unsigned)op, (unsigned)rd, (unsigned)rs, 0);
                img->kind[lc] = KIND_CODE;
            }
            lc++;
            continue;
        }

        if (streq(mnem, "JMP") || streq(mnem, "JZ") || streq(mnem, "JNZ")) {
            int addr, op;
            strip_brackets(op1);
            if (op2[0] != 0 || !resolve_addr(img, op1, &addr, emit)) {
                return fail(img, line, "bad branch", lines[li]);
            }
            if (streq(mnem, "JMP")) {
                op = OP_JMP;
            } else if (streq(mnem, "JZ")) {
                op = OP_JZ;
            } else {
                op = OP_JNZ;
            }
            if (emit) {
                if (img->kind[lc] != KIND_EMPTY) {
                    return fail(img, line, "overlap", lines[li]);
                }
                img->ram[lc] = enc((unsigned)op, 0, 0, (unsigned)addr);
                img->kind[lc] = KIND_CODE;
            }
            lc++;
            continue;
        }

        return fail(img, line, "unknown mnemonic", lines[li]);
    }
    return 0;
}

int assemble(const char **lines, Image *img)
{
    clear_image(img);
    if (lines == 0) {
        img->error = 1;
        printf("ASSEMBLE FAIL: no source\n");
        return -1;
    }
    if (walk(lines, img, 0) != 0) {
        return -1;
    }
    if (walk(lines, img, 1) != 0) {
        return -1;
    }
    return 0;
}

const char *label_at(const Image *img, u16 addr)
{
    int i;

    for (i = 0; i < img->sym.nlabels; i++) {
        if (img->sym.lab[i].addr == addr) {
            return img->sym.lab[i].name;
        }
    }
    return 0;
}

int label_addr(const Image *img, const char *name)
{
    int i;

    for (i = 0; i < img->sym.nlabels; i++) {
        if (streq(img->sym.lab[i].name, name)) {
            return (int)img->sym.lab[i].addr;
        }
    }
    return -1;
}
