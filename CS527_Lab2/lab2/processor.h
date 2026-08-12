#ifndef PROCESSOR_H
#define PROCESSOR_H

extern int Register[256];
extern int PC, opcode, dest, src1, src2;
extern int N, Z, C, V;                  /* the four global flags */
extern int end_of_simulation;

void reset(void);
void fetch(void);
void decode(void);
void execute(void);

void print_state(void);                 /* only for checking the result */

#endif
