#ifndef MEMORY_H
#define MEMORY_H

extern char Instruction[256];       /* instruction memory */
extern char Data[4096];             /* data memory        */

void initialize(void);              /* load program.byte and data.byte  */
void finalize(void);                /* write data memory back to a file */

/* data is always 32 bit, so these two helpers work on 4 bytes */
int  read_word(int address);
void write_word(int address, int value);

#endif
