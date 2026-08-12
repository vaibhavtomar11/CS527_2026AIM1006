#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "compiler.h"

#define OP_ADD        1
#define OP_SUB        2
#define OP_MUL        3
#define OP_DIV        4
#define OP_MEM_READ   5
#define OP_MEM_WRITE  6
#define OP_DATA_MOVE  7

static void writeInstr(FILE *out, int opcode, int dest, int src1, int src2) {
    fprintf(out, "%d %d %d %d\n", opcode, dest, src1, src2);
}

/* strips leading/trailing whitespace (including newline) in place */
static void trim(char *s) {
    char *start = s;
    while (isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);

    int len = (int)strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[len - 1] = '\0';
        len--;
    }
}

void compile(const char *srcFile) {
    FILE *in = fopen(srcFile, "r");
    if (!in) {
        fprintf(stderr, "Compiler error: cannot open source file '%s'\n", srcFile);
        exit(1);
    }

    FILE *out = fopen("program.byte", "w");
    if (!out) {
        fprintf(stderr, "Compiler error: cannot create program.byte\n");
        fclose(in);
        exit(1);
    }

    char line[512];
    int lineNo = 0;

    while (fgets(line, sizeof(line), in)) {
        lineNo++;
        trim(line);

        if (line[0] == '\0') continue;                    /* blank line */
        if (line[0] == '#' ) continue;                     /* comment    */
        if (line[0] == '/' && line[1] == '/') continue;    /* comment    */

        int dest, s1, s2;
        char op;

        /* Read x<dest>, <address> */
        if (sscanf(line, "Read x%d , %d", &dest, &s1) == 2 ||
            sscanf(line, "Read x%d, %d",  &dest, &s1) == 2) {
            writeInstr(out, OP_MEM_READ, dest, s1, 0);
            continue;
        }

        /* Write x<dest>, <address> */
        if (sscanf(line, "Write x%d , %d", &dest, &s1) == 2 ||
            sscanf(line, "Write x%d, %d",  &dest, &s1) == 2) {
            writeInstr(out, OP_MEM_WRITE, dest, s1, 0);
            continue;
        }

        /* x<dest> = x<src1> <op> x<src2> */
        if (sscanf(line, "x%d = x%d %c x%d", &dest, &s1, &op, &s2) == 4) {
            int opcode;
            switch (op) {
                case '+': opcode = OP_ADD; break;
                case '-': opcode = OP_SUB; break;
                case '*': opcode = OP_MUL; break;
                case '/': opcode = OP_DIV; break;
                default:
                    fprintf(stderr, "Compiler error line %d: unknown operator '%c'\n", lineNo, op);
                    fclose(in); fclose(out);
                    exit(1);
            }
            writeInstr(out, opcode, dest, s1, s2);
            continue;
        }

        /* x<dest> = <constant> */
        if (sscanf(line, "x%d = %d", &dest, &s1) == 2) {
            writeInstr(out, OP_DATA_MOVE, dest, s1, 0);
            continue;
        }

        fprintf(stderr, "Compiler error line %d: cannot parse '%s'\n", lineNo, line);
        fclose(in); fclose(out);
        exit(1);
    }

    /* terminator instruction: opcode 0 tells the processor to stop */
    writeInstr(out, 0, 0, 0, 0);

    fclose(in);
    fclose(out);

    printf("Compilation successful: %s -> program.byte\n", srcFile);
}
