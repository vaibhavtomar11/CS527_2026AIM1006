#include <stdio.h>
#include <stdlib.h>
#include "processor.h"
#include "memory.h"

int Register[NUM_REGISTERS];
int PC;
int opcode, dest, src1, src2;
int end_of_simulation;

void reset(void) {
    for (int i = 0; i < NUM_REGISTERS; i++) Register[i] = 0;
    PC = 0;
    opcode = dest = src1 = src2 = 0;
    end_of_simulation = 0;
}

void fetch(void) {
    opcode = Instruction[PC];
    dest   = Instruction[PC + 1];
    src1   = Instruction[PC + 2];
    src2   = Instruction[PC + 3];
    PC += 4;
}

void decode(void) {
    /* empty for this lab, reserved for future use */
}

void execute(void) {
    switch (opcode) {
        case 0: /* halt */
            end_of_simulation = 1;
            break;
        case 1: /* add */
            Register[dest] = Register[src1] + Register[src2];
            break;
        case 2: /* subtract */
            Register[dest] = Register[src1] - Register[src2];
            break;
        case 3: /* multiply */
            Register[dest] = Register[src1] * Register[src2];
            break;
        case 4: /* divide */
            if (Register[src2] == 0) {
                fprintf(stderr, "Runtime error: division by zero at PC=%d\n", PC - 4);
                exit(1);
            }
            Register[dest] = Register[src1] / Register[src2];
            break;
        case 5: /* memory read: dest reg <- Data[src1 = address] */
            Register[dest] = Data[src1];
            break;
        case 6: /* memory write: Data[src1 = address] <- dest reg */
            Data[src1] = (unsigned char)Register[dest];
            break;
        case 7: /* data movement: dest reg <- constant (src1) */
            Register[dest] = src1;
            break;
        default:
            fprintf(stderr, "Runtime error: unknown opcode %d at PC=%d\n", opcode, PC - 4);
            exit(1);
    }
}
