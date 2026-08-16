/* processor.c
 *
 * The processor of our mini computer.
 * It has 256 integer registers x0-x255, 32 vector registers v0-v31
 * (8 integer elements each), a PC and the four flags N Z C V.
 * Every instruction is 4 bytes :  opcode  dest  src1  src2
 */

#include <stdio.h>
#include "processor.h"
#include "memory.h"

int Register[256];
int VRegister[32][VEC_ELEMENTS];
int PC, opcode, dest, src1, src2;
int N, Z, C, V;
int end_of_simulation = 0;

/* safety counter, so that a wrong program does not loop for ever */
static long executed = 0;
#define MAX_STEPS 1000000

/* ------------------------------------------------------------------ */
/* flags                                                              */
/* ------------------------------------------------------------------ */

/* sign bit (MSB) of a 32 bit number */
static int sign_of(int x)
{
    return (x >> 31) & 1;
}

static void flags_for_add(int a, int b, int result)
{
    unsigned int ua = (unsigned int)a;
    unsigned int ub = (unsigned int)b;
    unsigned int ur = (unsigned int)result;

    Z = (result == 0) ? 1 : 0;
    N = sign_of(result);

    /* carry : unsigned result became smaller than one of the inputs */
    C = (ur < ua || ur < ub) ? 1 : 0;

    /* overflow : both inputs have same sign but result sign is different */
    if (sign_of(a) == sign_of(b) && sign_of(result) != sign_of(a))
        V = 1;
    else
        V = 0;
}

static void flags_for_sub(int a, int b, int result)
{
    Z = (result == 0) ? 1 : 0;
    N = sign_of(result);

    /* as given in the problem statement : C is 1 if operand 1 is bigger
     * than operand 2 (compared as unsigned numbers) */
    C = ((unsigned int)a > (unsigned int)b) ? 1 : 0;

    /* overflow : the two operands have different signs and the result
     * sign is same as the sign of operand 2 */
    if (sign_of(a) != sign_of(b) && sign_of(result) == sign_of(b))
        V = 1;
    else
        V = 0;
}

/* checks the branch condition, code is the value from the branch table */
static int condition_true(int code)
{
    switch (code) {
        case 0x0: return Z == 1;                    /* EQ */
        case 0x1: return Z == 0;                    /* NE */
        case 0x2: return C == 1;                    /* CS */
        case 0x3: return C == 0;                    /* CC */
        case 0x4: return N == 1;                    /* MI */
        case 0x5: return N == 0;                    /* PL */
        case 0x6: return V == 1;                    /* VS */
        case 0x7: return V == 0;                    /* VC */
        case 0x8: return (C == 1 && Z == 0);        /* HI */
        case 0x9: return (C == 0 || Z == 1);        /* LS */
        case 0xA: return (N == V);                  /* GE */
        case 0xB: return (N != V);                  /* LT */
        case 0xC: return (Z == 0 && N == V);        /* GT */
        case 0xD: return (Z == 1 || N != V);        /* LE */
        case 0xE: return 1;                         /* AL */
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* vector instructions                                                */
/* ------------------------------------------------------------------ */

/* the byte code may come from a file, so the register number is checked */
static int vector_ok(int number)
{
    if (number >= 0 && number < 32)
        return 1;

    printf("Error: v%d is not a vector register (PC = %d)\n", number, PC);
    end_of_simulation = 1;
    return 0;
}

/* one arithmetic instruction works on all the 8 elements of a vector.
 * The second operand is the same for every element when it is a constant
 * or an integer register, and element by element when it is a vector.
 */
static void vector_arithmetic(void)
{
    int i, a, b;
    int first    = src1 & ~VEC_SCALAR_FLAG;     /* operand 1 is v0 - v31   */
    int scalar   = src1 &  VEC_SCALAR_FLAG;     /* operand 2 is x register */
    int constant = (opcode >= 0x29);            /* operand 2 is a constant */
    int operation = constant ? opcode - 8 : opcode;   /* 0x21 / 0x22 / 0x23 */

    if (!vector_ok(dest) || !vector_ok(first))
        return;
    if (!constant && !scalar && !vector_ok(src2))
        return;

    for (i = 0; i < VEC_ELEMENTS; i++) {
        a = VRegister[first][i];

        if (constant)
            b = src2;                           /* v3 = v1 + 15  */
        else if (scalar)
            b = Register[src2];                 /* v3 = v1 * x4  */
        else
            b = VRegister[src2][i];             /* v3 = v1 + v4  */

        switch (operation) {
            case 0x21: VRegister[dest][i] = a + b; break;
            case 0x22: VRegister[dest][i] = a - b; break;
            case 0x23: VRegister[dest][i] = a * b; break;
        }
    }
}

/* v<number> = 8 words starting at address */
static void vector_load(int number, int address)
{
    int i;

    if (!vector_ok(number))
        return;

    for (i = 0; i < VEC_ELEMENTS; i++)
        VRegister[number][i] = read_word(address + i * 4);
}

/* 8 words starting at address = v<number> */
static void vector_store(int address, int number)
{
    int i;

    if (!vector_ok(number))
        return;

    for (i = 0; i < VEC_ELEMENTS; i++)
        write_word(address + i * 4, VRegister[number][i]);
}

/* ------------------------------------------------------------------ */
/* the four functions of the processor                                */
/* ------------------------------------------------------------------ */

void reset(void)
{
    int i, j;

    for (i = 0; i < 256; i++)
        Register[i] = 0;

    for (i = 0; i < 32; i++)
        for (j = 0; j < VEC_ELEMENTS; j++)
            VRegister[i][j] = 0;

    PC = 0;
    opcode = 0;
    dest = 0;
    src1 = 0;
    src2 = 0;
    N = 0;
    Z = 0;
    C = 0;
    V = 0;
    end_of_simulation = 0;
    executed = 0;
}

void fetch(void)
{
    if (PC < 0 || PC + 3 > 255) {
        printf("Error: PC = %d is outside instruction memory\n", PC);
        end_of_simulation = 1;
        return;
    }

    opcode = (unsigned char)Instruction[PC];
    dest   = (unsigned char)Instruction[PC + 1];
    src1   = (unsigned char)Instruction[PC + 2];
    src2   = (unsigned char)Instruction[PC + 3];
}

void decode(void)
{
    /* nothing to do at this time, kept because the design asks for it */
}

void execute(void)
{
    int a, b, result;
    int offset;
    int next_pc;

    if (end_of_simulation)
        return;

    next_pc = PC + 4;                   /* normally we go to next instruction */

    switch (opcode) {

        case 0x00:                      /* opcode 0 means program is over */
            end_of_simulation = 1;
            return;

        /* ---------------- add ---------------- */
        case 0x01:                      /* dest = src1 + src2 (variable)  */
        case 0x09:                      /* dest = src1 + src2 (constant)  */
            a = Register[src1];
            b = (opcode == 0x01) ? Register[src2] : src2;
            result = a + b;
            Register[dest] = result;
            flags_for_add(a, b, result);
            break;

        /* ---------------- subtract ---------------- */
        case 0x02:
        case 0x0A:
            a = Register[src1];
            b = (opcode == 0x02) ? Register[src2] : src2;
            result = a - b;
            Register[dest] = result;
            flags_for_sub(a, b, result);
            break;

        /* ---------------- multiply ---------------- */
        case 0x03:
        case 0x0B:
            a = Register[src1];
            b = (opcode == 0x03) ? Register[src2] : src2;
            Register[dest] = a * b;      /* multiply does not change flags */
            break;

        /* ---------------- divide ---------------- */
        case 0x04:
        case 0x0C:
            a = Register[src1];
            b = (opcode == 0x04) ? Register[src2] : src2;
            if (b == 0) {
                printf("Error: divide by zero at PC = %d\n", PC);
                end_of_simulation = 1;
                return;
            }
            Register[dest] = a / b;
            break;

        /* ---------------- memory read ---------------- */
        case 0x05:                      /* dest = [ src1 ]  address in reg  */
            Register[dest] = read_word(Register[src1]);
            break;

        case 0x0D:                      /* dest = [ src2 ]  address constant */
            Register[dest] = read_word(src2);
            break;

        /* ---------------- memory write ---------------- */
        case 0x06:                      /* [ dest ] = src1  address in reg   */
            write_word(Register[dest], Register[src1]);
            break;

        case 0x0E:                      /* [ src2 ] = src1  address constant */
            write_word(src2, Register[src1]);
            break;

        /* ---------------- data movement ---------------- */
        case 0x07:                      /* dest = constant */
            Register[dest] = src2;
            break;

        case 0x0F:                      /* dest = another variable */
            Register[dest] = Register[src2];
            break;

        /* ---------------- vector arithmetic ---------------- */
        case 0x21:                      /* v_dest = v_src1 + v_src2 / x_src2 */
        case 0x22:                      /* v_dest = v_src1 - v_src2 / x_src2 */
        case 0x23:                      /* v_dest = v_src1 * v_src2 / x_src2 */
        case 0x29:                      /* v_dest = v_src1 + constant        */
        case 0x2A:                      /* v_dest = v_src1 - constant        */
        case 0x2B:                      /* v_dest = v_src1 * constant        */
            vector_arithmetic();
            break;

        /* ---------------- vector memory read ---------------- */
        case 0x25:                      /* v_dest = [ src1 ]  address in reg  */
            vector_load(dest, Register[src1]);
            break;

        case 0x2C:                      /* v_dest = [ src2 ]  address constant */
            vector_load(dest, src2);
            break;

        /* ---------------- vector memory write ---------------- */
        case 0x26:                      /* [ dest ] = v_src1  address in reg   */
            vector_store(Register[dest], src1);
            break;

        case 0x2E:                      /* [ src2 ] = v_src1  address constant */
            vector_store(src2, src1);
            break;

        /* ---------------- branch ---------------- */
        default:
            if (opcode >= 0x10 && opcode <= 0x1E) {
                /* the offset byte is a signed number */
                offset = (int)(signed char)src2;

                if (condition_true(opcode - 0x10)) {
                    /* offset is counted in instructions and one
                     * instruction is 4 bytes long */
                    next_pc = PC + offset * 4;
                }
            } else {
                printf("Error: unknown opcode %02X at PC = %d\n", opcode, PC);
                end_of_simulation = 1;
                return;
            }
            break;
    }

    PC = next_pc;

    executed++;
    if (executed > MAX_STEPS) {
        printf("Error: program did not stop after %ld instructions\n",
               executed);
        end_of_simulation = 1;
    }
}

/* ------------------------------------------------------------------ */
/* helper used by main only to show the result on screen               */
/* ------------------------------------------------------------------ */
void print_state(void)
{
    int i, j, empty;

    printf("\nSimulation over after %ld instructions\n", executed);
    printf("Flags : Z=%d N=%d C=%d V=%d\n", Z, N, C, V);

    printf("Integer registers which are not zero :\n");
    for (i = 0; i < 256; i++)
        if (Register[i] != 0)
            printf("   x%-3d = %d\n", i, Register[i]);

    printf("Vector registers which are not zero :\n");
    for (i = 0; i < 32; i++) {
        empty = 1;
        for (j = 0; j < VEC_ELEMENTS; j++)
            if (VRegister[i][j] != 0)
                empty = 0;

        if (!empty) {
            printf("   v%-3d =", i);
            for (j = 0; j < VEC_ELEMENTS; j++)
                printf(" %d", VRegister[i][j]);
            printf("\n");
        }
    }
}
