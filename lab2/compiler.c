/* compiler.c
 *
 * Converts our simple language into byte code (program.byte).
 * It works in two passes:
 *      pass 1 : find all the labels and remember which instruction number
 *               they point to
 *      pass 2 : convert every instruction into 4 bytes and write the file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "compiler.h"

#define MAX_LINE   200
#define MAX_TOKEN  10
#define MAX_LABELS 50
#define MAX_INST   64      /* instruction memory is 256 bytes = 64 instructions */

/* one entry for every label found in the program */
struct label {
    char name[40];
    int  index;            /* instruction number of the next instruction */
};

static struct label label_table[MAX_LABELS];
static int label_count = 0;

/* branch suffix table, position in this array is the branch code */
static char *suffix_table[15] = {
    "EQ", "NE", "CS", "CC", "MI", "PL", "VS", "VC",
    "HI", "LS", "GE", "LT", "GT", "LE", "AL"
};

/* ------------------------------------------------------------------ */
/* small helper functions                                             */
/* ------------------------------------------------------------------ */

/* removes the comment part and puts one space around every special
 * symbol, so that later we can simply cut the line on spaces.
 * "x1=x2+10" becomes " x1 = x2 + 10 "
 */
static void clean_line(char *in, char *out)
{
    int i = 0, j = 0;

    while (in[i] != '\0' && in[i] != '\n' && in[i] != '%') {
        char c = in[i];

        if (c == '\t' || c == '\r')
            c = ' ';

        if (c == ';') {                 /* ; is optional, just throw it away */
            out[j++] = ' ';
        } else if (c == '=' || c == '[' || c == ']' ||
                   c == '+' || c == '-' || c == '*' || c == '/') {
            out[j++] = ' ';
            out[j++] = c;
            out[j++] = ' ';
        } else {
            out[j++] = c;
        }
        i++;
    }
    out[j] = '\0';
}

/* cuts the line on spaces, copies the pieces in token[] and returns
 * how many pieces were found
 */
static int split_line(char *line, char token[MAX_TOKEN][40])
{
    int count = 0;
    char *p = strtok(line, " ");

    while (p != NULL && count < MAX_TOKEN) {
        strcpy(token[count], p);
        count++;
        p = strtok(NULL, " ");
    }
    return count;
}

/* is this token a variable like x0 ... x255 ? */
static int is_variable(char *t)
{
    int i;

    if (t[0] != 'x' && t[0] != 'X')
        return 0;
    if (t[1] == '\0')
        return 0;
    for (i = 1; t[i] != '\0'; i++)
        if (!isdigit(t[i]))
            return 0;
    return 1;
}

/* variable number of x12 is 12 */
static int variable_number(char *t, int line_no)
{
    int n = atoi(t + 1);

    if (n < 0 || n > 255) {
        printf("Compile error (line %d): variable %s is out of range x0-x255\n",
               line_no, t);
        exit(1);
    }
    return n;
}

/* value of a constant, both 10 and 0xA are accepted */
static int constant_value(char *t, int line_no)
{
    long v = strtol(t, NULL, 0);

    if (v < 0 || v > 255) {
        printf("Compile error (line %d): constant %s must be between 0 and 255\n",
               line_no, t);
        exit(1);
    }
    return (int)v;
}

/* returns the branch code (0-14) if the token is a branch like BEQ,
 * otherwise returns -1
 */
static int branch_code(char *t)
{
    int i;

    if (t[0] != 'B' && t[0] != 'b')
        return -1;
    if (strlen(t) != 3)
        return -1;

    for (i = 0; i < 15; i++)
        if (toupper(t[1]) == suffix_table[i][0] &&
            toupper(t[2]) == suffix_table[i][1])
            return i;

    return -1;
}

/* returns the instruction number of a label, -1 if not found */
static int find_label(char *name)
{
    int i;

    for (i = 0; i < label_count; i++)
        if (strcmp(label_table[i].name, name) == 0)
            return label_table[i].index;

    return -1;
}

/* writes one instruction (4 bytes) in the byte code file */
static void emit(FILE *out, int opcode, int b1, int b2, int b3)
{
    fprintf(out, "%02X %02X %02X %02X\n", opcode & 0xFF, b1 & 0xFF,
            b2 & 0xFF, b3 & 0xFF);
}

/* ------------------------------------------------------------------ */
/* pass 1 : collect the labels                                        */
/* ------------------------------------------------------------------ */
static void pass_one(FILE *in)
{
    char raw[MAX_LINE], clean[MAX_LINE * 3];
    char token[MAX_TOKEN][40];
    int count;
    int inst_no = 0;
    int line_no = 0;

    while (fgets(raw, MAX_LINE, in) != NULL) {
        line_no++;

        /* a label must start at the very first character of the line */
        if (raw[0] == '.') {
            clean_line(raw, clean);
            count = split_line(clean, token);
            if (count != 1) {
                printf("Compile error (line %d): a label line can not have "
                       "anything else\n", line_no);
                exit(1);
            }
            if (label_count >= MAX_LABELS) {
                printf("Compile error: too many labels\n");
                exit(1);
            }
            strcpy(label_table[label_count].name, token[0]);
            label_table[label_count].index = inst_no;
            label_count++;
            continue;
        }

        clean_line(raw, clean);
        count = split_line(clean, token);
        if (count > 0)                  /* a real instruction */
            inst_no++;
    }

    if (inst_no >= MAX_INST) {
        printf("Compile error: program has %d instructions, instruction "
               "memory can hold only %d\n", inst_no, MAX_INST);
        exit(1);
    }
}

/* ------------------------------------------------------------------ */
/* pass 2 : generate the byte code                                    */
/* ------------------------------------------------------------------ */
static int pass_two(FILE *in, FILE *out)
{
    char raw[MAX_LINE], clean[MAX_LINE * 3];
    char token[MAX_TOKEN][40];
    int count;
    int inst_no = 0;
    int line_no = 0;
    int d, s1, s2, code, target, offset;

    while (fgets(raw, MAX_LINE, in) != NULL) {
        line_no++;

        if (raw[0] == '.')              /* label, nothing to generate */
            continue;

        clean_line(raw, clean);
        count = split_line(clean, token);
        if (count == 0)                 /* empty line or full comment line */
            continue;

        /* ---------- branch : BEQ .loop ---------- */
        code = branch_code(token[0]);
        if (code >= 0) {
            if (count != 2 || token[1][0] != '.') {
                printf("Compile error (line %d): branch needs a label\n",
                       line_no);
                exit(1);
            }
            target = find_label(token[1]);
            if (target < 0) {
                printf("Compile error (line %d): label %s not found\n",
                       line_no, token[1]);
                exit(1);
            }
            /* offset is counted in instructions:
             * next instruction = current instruction + offset  */
            offset = target - inst_no;
            emit(out, 0x10 + code, 0, 0, offset);
            inst_no++;
            continue;
        }

        /* ---------- legacy : Read x1 0 ---------- */
        if (strcmp(token[0], "Read") == 0 || strcmp(token[0], "read") == 0) {
            if (count != 3 || !is_variable(token[1])) {
                printf("Compile error (line %d): use  Read <variable> "
                       "<address>\n", line_no);
                exit(1);
            }
            d  = variable_number(token[1], line_no);
            s2 = constant_value(token[2], line_no);
            emit(out, 0x0D, d, 0, s2);
            inst_no++;
            continue;
        }

        /* ---------- legacy : Write x1 4 ---------- */
        if (strcmp(token[0], "Write") == 0 || strcmp(token[0], "write") == 0) {
            if (count != 3 || !is_variable(token[1])) {
                printf("Compile error (line %d): use  Write <variable> "
                       "<address>\n", line_no);
                exit(1);
            }
            s1 = variable_number(token[1], line_no);
            s2 = constant_value(token[2], line_no);
            emit(out, 0x0E, 0, s1, s2);
            inst_no++;
            continue;
        }

        /* ---------- memory write : [x2] = x1  or  [4] = x1 ---------- */
        if (token[0][0] == '[') {
            if (count != 5 || token[2][0] != ']' || token[3][0] != '=' ||
                !is_variable(token[4])) {
                printf("Compile error (line %d): use  [address] = "
                       "<variable>\n", line_no);
                exit(1);
            }
            s1 = variable_number(token[4], line_no);
            if (is_variable(token[1])) {
                d = variable_number(token[1], line_no);
                emit(out, 0x06, d, s1, 0);
            } else {
                s2 = constant_value(token[1], line_no);
                emit(out, 0x0E, 0, s1, s2);
            }
            inst_no++;
            continue;
        }

        /* everything left must be  <variable> = ...  */
        if (!is_variable(token[0]) || count < 3 || token[1][0] != '=') {
            printf("Compile error (line %d): can not understand this line\n",
                   line_no);
            exit(1);
        }
        d = variable_number(token[0], line_no);

        /* ---------- data movement : x1 = 10   or   x1 = x2 ---------- */
        if (count == 3) {
            if (is_variable(token[2])) {
                s2 = variable_number(token[2], line_no);
                emit(out, 0x0F, d, 0, s2);
            } else {
                s2 = constant_value(token[2], line_no);
                emit(out, 0x07, d, 0, s2);
            }
            inst_no++;
            continue;
        }

        /* ---------- memory read : x1 = [x2]  or  x1 = [0] ---------- */
        if (count == 5 && token[2][0] == '[') {
            if (token[4][0] != ']') {
                printf("Compile error (line %d): closing ] is missing\n",
                       line_no);
                exit(1);
            }
            if (is_variable(token[3])) {
                s1 = variable_number(token[3], line_no);
                emit(out, 0x05, d, s1, 0);
            } else {
                s2 = constant_value(token[3], line_no);
                emit(out, 0x0D, d, 0, s2);
            }
            inst_no++;
            continue;
        }

        /* ---------- arithmetic : x1 = x2 + x3   or   x1 = x2 + 10 ---- */
        if (count == 5) {
            if (!is_variable(token[2])) {
                printf("Compile error (line %d): first operand must be a "
                       "variable\n", line_no);
                exit(1);
            }
            s1 = variable_number(token[2], line_no);

            switch (token[3][0]) {
                case '+': code = 0x01; break;
                case '-': code = 0x02; break;
                case '*': code = 0x03; break;
                case '/': code = 0x04; break;
                default:
                    printf("Compile error (line %d): unknown operator %s\n",
                           line_no, token[3]);
                    exit(1);
            }

            if (is_variable(token[4])) {
                s2 = variable_number(token[4], line_no);
            } else {
                s2 = constant_value(token[4], line_no);
                code = code + 8;        /* constant version of the opcode */
            }
            emit(out, code, d, s1, s2);
            inst_no++;
            continue;
        }

        printf("Compile error (line %d): can not understand this line\n",
               line_no);
        exit(1);
    }

    return inst_no;
}

/* ------------------------------------------------------------------ */
void compile(char *source_file)
{
    FILE *in, *out;
    int total;

    in = fopen(source_file, "r");
    if (in == NULL) {
        printf("Error: can not open program file %s\n", source_file);
        exit(1);
    }

    label_count = 0;
    pass_one(in);

    rewind(in);                         /* read the same file again */

    out = fopen("program.byte", "w");
    if (out == NULL) {
        printf("Error: can not create program.byte\n");
        exit(1);
    }

    total = pass_two(in, out);

    fclose(in);
    fclose(out);

    printf("Compiled %s -> program.byte (%d instructions, %d labels)\n",
           source_file, total, label_count);
}
