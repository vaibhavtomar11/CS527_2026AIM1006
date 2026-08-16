#ifndef PROCESSOR_H
#define PROCESSOR_H

/* one vector register keeps 8 integers of 32 bit = 256 bit */
#define VEC_ELEMENTS 8

/* Vector arithmetic has three kinds of second operand (vector register,
 * integer register, constant) but the opcode table gives only two opcodes
 * (register / constant).  A vector register number needs only 5 bits, so
 * bit 7 of the operand 1 byte is used as a flag:
 *      flag set   -> the second operand is an integer register x0-x255
 *      flag clear -> the second operand is a vector register   v0-v31
 * The compiler sets this bit, the processor reads it.
 */
#define VEC_SCALAR_FLAG 0x80

extern int Register[256];               /* integer registers x0 - x255 */
extern int VRegister[32][VEC_ELEMENTS]; /* vector registers  v0 - v31  */

extern int PC, opcode, dest, src1, src2;
extern int N, Z, C, V;                  /* the four global flags */
extern int end_of_simulation;

void reset(void);
void fetch(void);
void decode(void);
void execute(void);

void print_state(void);                 /* only for checking the result */

#endif
