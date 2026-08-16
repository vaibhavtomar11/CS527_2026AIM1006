# CS527 Lab 3 - Mini computer system with vector instructions

A small computer system written in C. The program written in our simple
language is first compiled into byte code, and then the byte code is
executed by a simulated processor. Everything runs from one executable.

Lab 3 adds **vector registers and vector instructions** on top of Lab 2.
The processor now has

* 256 integer registers `x0 ... x255`, each 32 bit
* 32 vector registers `v0 ... v31`, each 256 bit = **8 integers of 32 bit**

## How to build and run

```
make                                          # creates the executable "mycomputer"

./mycomputer tests/array_add.prog tests/array_add.data data.byte
```

The executable takes the files on the command line:

```
./mycomputer <program file> [input data file] [output data file]
```

| Argument | Meaning | Default |
|---|---|---|
| program file | source program, it is compiled into `program.byte`. If the name ends with `.byte` it is taken as already compiled byte code and the compiler is not called | must be given |
| input data file | data memory at the start of the simulation | `data.byte` |
| output data file | data memory at the end of the simulation | `data.byte` |

So both files of the specification can be given directly:

```
./mycomputer program.byte data.byte          # run a byte code file
```

Because the input and the output file are separate arguments, a test file
is never destroyed. The helper script uses the same idea:

```
./run_test.sh array_add                      # tests/array_add.{prog,data} -> data.byte
```

`make clean` removes the object files, the executable and `program.byte`.

## Files

| File | What it does |
|---|---|
| `main.c` | reads the command line, calls `compile()`, then `initialize()`, `reset()` and the fetch / decode / execute loop, and `finalize()` at the end |
| `compiler.c/h` | two pass compiler, source program -> `program.byte` |
| `processor.c/h` | integer and vector registers, PC, N Z C V flags, `reset() fetch() decode() execute()` |
| `memory.c/h` | `Instruction[256]`, `Data[4096]`, loading and saving the byte files, 32 bit word read/write |
| `Makefile` | multi file build |
| `tests/` | example programs (`.prog`) and their input data memory (`.data`) |

## The language

Everything of Lab 2 still works (integer arithmetic, memory read / write,
labels, branches, `Read` / `Write`, comments with `%`). A name which starts
with `v` is a vector register and makes the instruction a vector
instruction.

```
v3 = v1 + v4        % for(i=0; i<8; i++) v3[i] = v1[i] + v4[i]
v3 = v1 - 15        % for(i=0; i<8; i++) v3[i] = v1[i] - 15
v3 = v1 * x4        % for(i=0; i<8; i++) v3[i] = v1[i] * x4
v1 = [x2]           % load  8 words from address x2 , x2+4 ... x2+28
v1 = [32]           % load  8 words from address 32 , 36  ... 60
[x2] = v1           % store 8 words at   address x2 , x2+4 ... x2+28
[32] = v1           % store 8 words at   address 32 , 36  ... 60
```

The first operand of a vector operation is always a vector, the second one
can be a vector, an integer register or a constant. Only `+ - *` are
allowed for vectors.

## How the compiler works

**Pass 1** reads the whole file and only looks for labels. A label must be
the first character of a line and it is stored together with the number of
the instruction which comes after it.

**Pass 2** reads the file again and produces four bytes for every
instruction. For a branch the offset is `label instruction number - current
instruction number`, so a backward jump gives a negative offset which is
stored as a signed byte (for example -6 is stored as `FA`).

Before a line is parsed it is "cleaned": everything after `%` is removed,
tabs become spaces, `;` is dropped and one space is put around `= [ ] + - *
/`. After that the line is simply cut on spaces, so `v1=v2+10` and
`v1 = v2 + 10` are both accepted.

## Byte code used

Every instruction is 4 bytes: `opcode  dest  operand1  operand2`

| Instruction | Written as | Byte code |
|---|---|---|
| add / sub / mul / div, variable | `x1 = x2 + x3` | `01/02/03/04  dest  src1  src2` |
| add / sub / mul / div, constant | `x1 = x2 + 10` | `09/0A/0B/0C  dest  src1  const` |
| memory read, address in variable | `x1 = [x2]` | `05  dest  addrvar  00` |
| memory read, address constant | `x1 = [0]` / `Read x1 0` | `0D  dest  00  addr` |
| memory write, address in variable | `[x2] = x1` | `06  addrvar  srcvar  00` |
| memory write, address constant | `[4] = x1` / `Write x1 4` | `0E  00  srcvar  addr` |
| data movement, constant | `x1 = 10` | `07  dest  00  const` |
| data movement, variable | `x1 = x2` | `0F  dest  00  srcvar` |
| branch | `BEQ .loop` | `10 + condition code  00  00  offset` |
| **vector add / sub / mul, vector** | `v3 = v1 + v4` | `21/22/23  vdest  vsrc1  vsrc2` |
| **vector add / sub / mul, x register** | `v3 = v1 + x4` | `21/22/23  vdest  vsrc1+80  xsrc2` |
| **vector add / sub / mul, constant** | `v3 = v1 + 15` | `29/2A/2B  vdest  vsrc1  const` |
| **vector memory read, address in variable** | `v1 = [x2]` | `25  vdest  addrvar  00` |
| **vector memory read, address constant** | `v1 = [32]` | `2C  vdest  00  addr` |
| **vector memory write, address in variable** | `[x2] = v1` | `26  addrvar  vsrc  00` |
| **vector memory write, address constant** | `[32] = v1` | `2E  00  vsrc  addr` |
| end of program | (empty memory) | `00 00 00 00` |

The compiler output for the example program of the question paper is exactly
the byte code printed in the question paper (`tests/pdf_example.prog`).

## Assumptions

These points were not fully clear in the question paper, so a choice was
made and it is written here.

### New in Lab 3

1. **How the three kinds of second operand of a vector operation are told
   apart.** The language allows `v3 = v1 + v4`, `v3 = v1 + x4` and
   `v3 = v1 + 15`, but the opcode table gives only two opcodes (register and
   constant). A vector register number needs only 5 bits, so **bit 7 of the
   operand 1 byte** is used as a flag: if it is set, the second operand is an
   integer register `x0-x255`, otherwise it is a vector register. In this way
   the opcodes of the table (`21 22 23` and `29 2A 2B`) are used exactly as
   given, and `v3 = v1 * x4` becomes `23 03 81 04`.
2. **A vector load or store does not change the address register.** The
   pseudo code of the paper is written as
   `for(i=0; i<8; i++, x2 = x2+4) v1[i] = [x2]`, but the value of `x2` is
   kept unchanged, exactly like the constant version where the paper itself
   uses a temporary `tmp`. So `v1 = [x2]` reads the 8 words at
   `x2, x2+4 ... x2+28` and leaves `x2` as it was. The address is increased
   by the program itself (`x2 = x2 + 32`), which keeps the instruction free
   of side effects.
3. **Vector operations do not change the flags N Z C V.** There are 8
   results in one vector instruction, so there is no single result which
   could set the flags. Only the integer add and subtract set the flags, as
   in Lab 2.
4. **Vector element size is 32 bit**, so one vector register of 256 bit
   holds 8 elements and one vector load / store touches 32 bytes of memory.
5. A vector register number is checked both by the compiler and by the
   processor, because the processor can also be started with a byte code
   file which was not produced by our compiler.

### Kept from Lab 2

6. **Branch offset is counted in instructions, not in bytes.** In the given
   example `BAL .loopback` is the 11th instruction and the label is the 5th
   one, and the paper shows the offset `FA` = -6, which is `5 - 11` in
   instruction numbers. So in the processor the jump is done as
   `PC = PC + offset * 4`.
7. **Memory read with a constant address uses opcode `0D`.** The opcode
   table in the paper gives `0C` both for "divide with constant" and for
   "memory read with constant", which cannot work, and `0D` was the only
   free value in that range.
8. **Data movement**: the table says `07` for a variable and `0F` for a
   constant, but the example byte code of the paper shows `x1 = 0` becoming
   `07 01 00 00`, that is `07` with a constant. The example was followed:
   `07` is used for a constant and `0F` for a copy from another variable.
9. **Memory read / write with a register address keep the register number in
   the operand 1 byte**, again following the example (`x4 = [x3]` becomes
   `05 04 03 00` and `[x3] = x2` becomes `06 03 02 00`). The sentence
   "operand 1 is 0" is true for the constant versions and for data movement.
10. **A 32 bit value is stored in little endian order** (lowest byte at the
    lowest address), so the number 5 in `data.byte` is `05 00 00 00` and -3
    is `FD FF FF FF`.
11. The C flag of subtraction is implemented exactly as written in the paper,
    `C = 1 if operand 1 is bigger than operand 2` (compared as unsigned).
12. The main loop in the paper is written as `while(end_of_simulation)`, which
    would never run even once, so `while (end_of_simulation == 0)` is used.
13. Data memory is `4096` bytes as given in the code part of the paper (the
    block diagram shows 256 bytes).
14. Opcode `00` stops the simulation. Since instruction memory is cleared to
    zero before loading, a program automatically stops after its last
    instruction.
15. A `divide by zero`, a `PC` outside instruction memory or an address
    outside data memory prints a message and stops the simulation.

## Test programs

| Program | What it does | Expected result |
|---|---|---|
| `array_add.prog` | **sum of two arrays** with vector instructions | see below |
| `fir.prog` | **FIR filter** with vector instructions | see below |
| `vec_basic.prog` | uses every vector instruction once | see below |
| `pdf_example.prog` | the example program of the question paper, only to compare the byte code | `program.byte` is same as in the paper |
| `sum_fixed.prog` | sum of 5 numbers, N fixed inside the program | 150 at address 20 |
| `sum_runtime.prog` | sum of N numbers, N read from address 0 | N=5, numbers 1..5, sum 15 at address 24 |
| `complex_mul.prog` | (2+3i) * (4+5i) | real -7 at address 16, imaginary 22 at address 20 |
| `det3x3.prog` | determinant of `[1 2 3; 4 5 6; 7 8 10]` | -3 at address 36 |
| `branch_flags.prog` | count down loop with `BNE` and a negative compare with `BLT` | 5 at address 0 and 200 at address 4 |
| `legacy_readwrite.prog` | old Lab 1 `Read` / `Write` instructions, kept only to show that the compiler still compiles them | 7 read from address 0, 8 written at address 4 |

In `data.byte` one line is one 32 bit word, so **line number = address / 4**.

### `array_add.prog`

Layout: `N` at address 0, array A from address 4, array B after A, result C
after B. The test data has `N = 16`, `A = 1 2 3 ... 16` and
`B = 10 20 30 ... 160`, so `C = 11 22 33 ... 176` is written from address
`4 + 8N = 132` (line 33 of `data.byte`).

The loop body is only 8 instructions and runs `N/8` times, because one
`v3 = v1 + v2` does 8 additions.

### `fir.prog`

Layout: `N` at address 0, the 8 weights at address 4 ... 0x23, the input
array from address 0x24, the output from address 0x100, exactly as the
paper says. The loop is the pseudo code of the paper, `k` goes from 0 to
`N-8` (not included).

The test data has `N = 16`, weights `1 2 3 ... 8` and input `1 2 3 ... 16`,
so the output is `204 240 276 312 348 384 420 456` at address 0x100
(lines 64 to 71 of `data.byte`).

The 8 multiplications of one output value are done by a single
`v3 = v2 * v1`. The language has no instruction which adds the 8 elements
of one vector together, so the 8 products are stored in a scratch area of
the data memory (address 1024, far away from the input and the output) and
added there with a small loop. The scratch area stays visible in the
written `data.byte`, it is only a work area.

`k` counts **down** from `N-9` to 0 instead of up. The reason is the memory
map of the question paper itself: the input starts at 0x24 and the output at
0x100, so for a big `N` (56 and 64) the last input values and the first
output values share the same addresses. Going backwards, an output value is
written only after all the input values which it covers have already been
read, so the result is correct for every allowed `N`. The program was
checked for `N` = 8, 16, 24, 48, 56 and 64.

### `vec_basic.prog`

Reads two vectors from address 0 and 0x20 and writes the result of every
vector instruction back in the memory from address 0x40 onwards, so all the
opcodes `21 22 23 25 26 29 2A 2B 2C 2E` can be checked in one run.

After every run the program also prints the flags, the integer registers
which are not zero and the vector registers which are not zero, which makes
checking the result easier.
