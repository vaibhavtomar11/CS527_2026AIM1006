#ifndef MEMORY_H
#define MEMORY_H

extern char Instruction[256];       /* instruction memory */
extern char Data[4096];             /* data memory        */

/* the two byte code files are given on the command line */
void initialize(char *program_file, char *data_file);
void finalize(char *data_file);     /* write data memory back to a file */

/* data is always 32 bit, so these two helpers work on 4 bytes */
int  read_word(int address);
void write_word(int address, int value);

#endif
