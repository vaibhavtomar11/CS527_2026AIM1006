#include <stdio.h>
#include <stdlib.h>
#include "memory.h"

unsigned char Instruction[MEM_SIZE];
unsigned char Data[MEM_SIZE];

void initialize(void) {
    for (int i = 0; i < MEM_SIZE; i++) {
        Instruction[i] = 0;
        Data[i] = 0;
    }

    FILE *prog = fopen("program.byte", "r");
    if (!prog) {
        fprintf(stderr, "Memory error: cannot open program.byte\n");
        exit(1);
    }
    int val, idx = 0;
    while (idx < MEM_SIZE && fscanf(prog, "%d", &val) == 1) {
        Instruction[idx++] = (unsigned char)val;
    }
    fclose(prog);

    FILE *data = fopen("data.byte", "r");
    if (data) {
        idx = 0;
        while (idx < MEM_SIZE && fscanf(data, "%d", &val) == 1) {
            Data[idx++] = (unsigned char)val;
        }
        fclose(data);
    } else {
        printf("Memory notice: data.byte not found, initializing data memory to 0\n");
    }
}

void finalize(void) {
    FILE *data = fopen("data.byte", "w");
    if (!data) {
        fprintf(stderr, "Memory error: cannot write data.byte\n");
        exit(1);
    }
    for (int i = 0; i < MEM_SIZE; i++) {
        fprintf(data, "%d ", Data[i]);
        if ((i + 1) % 16 == 0) fprintf(data, "\n");
    }
    fclose(data);
    printf("Memory finalized: data.byte written\n");
}
