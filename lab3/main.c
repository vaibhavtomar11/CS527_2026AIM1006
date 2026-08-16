/* main.c
 *
 * Single executable which first compiles the program and then runs it
 * on our computer system.
 *
 * Usage :  ./mycomputer <program file> [input data file] [output data file]
 *
 *   program file      a source program (compiled into program.byte), or an
 *                     already compiled byte code file whose name ends with
 *                     ".byte" (then the compiler is not called)
 *   input data file   data memory at the start,  default "data.byte"
 *   output data file  data memory at the end,    default "data.byte"
 *
 * Example :  ./mycomputer tests/array_add.prog tests/array_add.data
 *            ./mycomputer program.byte data.byte
 */

#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "memory.h"
#include "processor.h"

/* does the file name end with ".byte" ? */
static int is_byte_code(char *name)
{
    int len = strlen(name);

    return (len > 5 && strcmp(name + len - 5, ".byte") == 0);
}

int main(int argc, char **argv)
{
    char *program_file;
    char *data_in;
    char *data_out;

    if (argc < 2) {
        printf("Usage: %s <program file> [input data file] "
               "[output data file]\n", argv[0]);
        return 1;
    }

    program_file = argv[1];
    data_in  = (argc > 2) ? argv[2] : "data.byte";
    data_out = (argc > 3) ? argv[3] : "data.byte";

    if (!is_byte_code(program_file)) {
        compile(program_file);          /* source program -> program.byte */
        program_file = "program.byte";
    }

    initialize(program_file, data_in);  /* load instruction and data memory */
    reset();                            /* clear registers and PC           */

    /* The assignment sheet writes while(end_of_simulation), but then the
     * loop would never run even once, so the condition has to be the
     * other way round. */
    while (end_of_simulation == 0) {
        fetch();
        decode();
        execute();
    }

    finalize(data_out);     /* write data memory back in a byte file */
    print_state();          /* just to see the result on screen      */

    return 0;
}
