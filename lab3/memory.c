/* memory.c
 *
 * Instruction memory and data memory of our computer system.
 * Both memories are byte addressable, that is why they are char arrays.
 *
 * program.byte and data.byte keep four hex bytes per line, separated by
 * space, for example :   FF FF FF FF
 *
 * A 32 bit number is stored in little endian order (lowest byte first),
 * the same way a normal x86 machine does it. So the number 5 is written
 * in the file as   05 00 00 00
 */

#include <stdio.h>
#include <stdlib.h>
#include "memory.h"

char Instruction[256];
char Data[4096];

/* reads a file of hex bytes into the given array */
static int load_file(char *name, char *array, int size)
{
    FILE *fp;
    int value, i = 0;

    fp = fopen(name, "r");
    if (fp == NULL)
        return 0;                       /* file not present */

    while (i < size && fscanf(fp, "%x", &value) == 1) {
        array[i] = (char)(value & 0xFF);
        i++;
    }
    fclose(fp);
    return i;                           /* how many bytes were loaded */
}

void initialize(char *program_file, char *data_file)
{
    int i, n;

    for (i = 0; i < 256; i++)
        Instruction[i] = 0;
    for (i = 0; i < 4096; i++)
        Data[i] = 0;

    n = load_file(program_file, Instruction, 256);
    if (n == 0) {
        printf("Error: %s not found\n", program_file);
        exit(1);
    }

    n = load_file(data_file, Data, 4096);
    if (n == 0)
        printf("Note: %s not found, data memory starts with all zero\n",
               data_file);
}

void finalize(char *data_file)
{
    FILE *fp;
    int i;

    fp = fopen(data_file, "w");
    if (fp == NULL) {
        printf("Error: can not write %s\n", data_file);
        return;
    }

    for (i = 0; i < 4096; i = i + 4)
        fprintf(fp, "%02X %02X %02X %02X\n",
                (unsigned char)Data[i], (unsigned char)Data[i + 1],
                (unsigned char)Data[i + 2], (unsigned char)Data[i + 3]);

    fclose(fp);
    printf("Data memory written back to %s\n", data_file);
}

int read_word(int address)
{
    unsigned char b0, b1, b2, b3;

    if (address < 0 || address + 3 >= 4096) {
        printf("Error: read from address %d is outside data memory\n", address);
        return 0;
    }

    b0 = (unsigned char)Data[address];
    b1 = (unsigned char)Data[address + 1];
    b2 = (unsigned char)Data[address + 2];
    b3 = (unsigned char)Data[address + 3];

    return (int)(b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));
}

void write_word(int address, int value)
{
    if (address < 0 || address + 3 >= 4096) {
        printf("Error: write to address %d is outside data memory\n", address);
        return;
    }

    Data[address]     = (char)(value & 0xFF);
    Data[address + 1] = (char)((value >> 8) & 0xFF);
    Data[address + 2] = (char)((value >> 16) & 0xFF);
    Data[address + 3] = (char)((value >> 24) & 0xFF);
}
