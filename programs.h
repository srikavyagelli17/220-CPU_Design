/* programs.h
 * Embedded assembly sources. Each list ends with a NULL pointer.
 */

#ifndef PROGRAMS_H
#define PROGRAMS_H

/* 8^2 by repeated multiplication. Result word: RESULT. */
extern const char *prog_pow[];

/* C source that Program B was translated from. */
extern const char *prog_fact_c[];

/* 8! hand-translated from prog_fact_c. Result word: RESULT. */
extern const char *prog_fact[];

#endif
