#include <stdio.h>
#include "compiler.h"
#include "processor.h"
#include "memory.h"

int main(int argc, char **argv) {
    const char *srcFile = (argc > 1) ? argv[1] : "program.asm";

    compile(srcFile);
    initialize();
    reset();

    while (!end_of_simulation) {
        fetch();
        decode();
        execute();
    }

    finalize();

    printf("\n--- Simulation complete ---\n");
    printf("Non-zero registers:\n");
    for (int i = 0; i < NUM_REGISTERS; i++) {
        if (Register[i] != 0) printf("  x%d = %d\n", i, Register[i]);
    }

    return 0;
}
