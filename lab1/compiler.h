#ifndef COMPILER_H
#define COMPILER_H

/* Reads the source assembly-like program from srcFile, and
 * generates the byte code in "program.byte" in the current directory. */
void compile(const char *srcFile);

#endif
