#ifndef MEMORY_H
#define MEMORY_H

#define MEM_SIZE 256

extern unsigned char Instruction[MEM_SIZE];
extern unsigned char Data[MEM_SIZE];

/* Reads program.byte into Instruction[] and data.byte into Data[] */
void initialize(void);

/* Writes the final contents of Data[] back out to data.byte */
void finalize(void);

#endif
