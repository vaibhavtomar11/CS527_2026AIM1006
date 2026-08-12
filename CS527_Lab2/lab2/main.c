/* main.c
 *
 * Single executable which first compiles the program and then runs it
 * on our computer system.
 *
 * Usage :  ./mycomputer  <program file>
 */

#include <stdio.h>
#include "compiler.h"
#include "memory.h"
#include "processor.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s <program file>\n", argv[0]);
        return 1;
    }

    compile(argv[1]);       /* source program  ->  program.byte */

    initialize();           /* load program.byte and data.byte  */
    reset();                /* clear registers and PC           */

    /* The assignment sheet writes while(end_of_simulation), but then the
     * loop would never run even once, so the condition has to be the
     * other way round. */
    while (end_of_simulation == 0) {
        fetch();
        decode();
        execute();
    }

    finalize();             /* write data memory back to data.byte */
    print_state();          /* just to see the result on screen    */

    return 0;
}
