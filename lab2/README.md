# CS527 Lab 2 - Mini computer system simulator

A small computer system written in C. The program written in our simple
language is first compiled into byte code, and then the byte code is
executed by a simulated processor. Everything runs from one executable.

## How to build and run

```
make                                 # creates the executable "mycomputer"

cp tests/sum_runtime.data data.byte  # input data memory
./mycomputer tests/sum_runtime.prog  # compile + run
```

`finalize()` writes the data memory back into `data.byte`, so the original
input file is kept inside `tests/` and copied every time. The helper script
does both steps together:

```
./run_test.sh sum_runtime
```

`make clean` removes the object files, the executable and `program.byte`.

## Files

| File | What it does |
|---|---|
| `main.c` | calls `compile()`, then `initialize()`, `reset()` and the fetch / decode / execute loop, and `finalize()` at the end |
| `compiler.c/h` | two pass compiler, source program -> `program.byte` |
| `processor.c/h` | registers, PC, N Z C V flags, `reset() fetch() decode() execute()` |
| `memory.c/h` | `Instruction[256]`, `Data[4096]`, loading and saving the byte files, 32 bit word read/write |
| `Makefile` | multi file build |
| `tests/` | example programs (`.prog`) and their input data memory (`.data`) |

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
/`. After that the line is simply cut on spaces, so `x1=x2+10` and
`x1 = x2 + 10` are both accepted.

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
| end of program | (empty memory) | `00 00 00 00` |

The compiler output for the example program of the question paper is exactly
the byte code printed in the question paper (`tests/pdf_example.prog`).

## Assumptions

These points were not fully clear in the question paper, so a choice was
made and it is written here.

1. **Branch offset is counted in instructions, not in bytes.** In the given
   example `BAL .loopback` is the 11th instruction and the label is the 5th
   one, and the paper shows the offset `FA` = -6, which is `5 - 11` in
   instruction numbers. So in the processor the jump is done as
   `PC = PC + offset * 4`.
2. **Memory read with a constant address uses opcode `0D`.** The opcode
   table in the paper gives `0C` both for "divide with constant" and for
   "memory read with constant", which cannot work, and `0D` was the only
   free value in that range.
3. **Data movement**: the table says `07` for a variable and `0F` for a
   constant, but the example byte code of the paper shows `x1 = 0` becoming
   `07 01 00 00`, that is `07` with a constant. The example was followed:
   `07` is used for a constant and `0F` for a copy from another variable.
4. **Memory read / write with a register address keep the register number in
   the operand 1 byte**, again following the example (`x4 = [x3]` becomes
   `05 04 03 00` and `[x3] = x2` becomes `06 03 02 00`). The sentence
   "operand 1 is 0" is true for the constant versions and for data movement.
5. **A 32 bit value is stored in little endian order** (lowest byte at the
   lowest address), so the number 5 in `data.byte` is `05 00 00 00`.
6. **Only add and subtract change the flags.** The paper says
   "every addition/subtraction operation updates the four flags", so
   multiply, divide, memory and data movement instructions leave the flags
   untouched.
7. The C flag of subtraction is implemented exactly as written in the paper,
   `C = 1 if operand 1 is bigger than operand 2` (compared as unsigned).
8. The main loop in the paper is written as `while(end_of_simulation)`, which
   would never run even once, so `while (end_of_simulation == 0)` is used.
9. Data memory is `4096` bytes as given in the code part of the paper (the
   block diagram shows 256 bytes).
10. Opcode `00` stops the simulation. Since instruction memory is cleared to
    zero before loading, a program automatically stops after its last
    instruction.
11. A `divide by zero`, a `PC` outside instruction memory or an address
    outside data memory prints a message and stops the simulation.

## Test programs

| Program | What it does | Expected result |
|---|---|---|
| `pdf_example.prog` | the example program of the question paper, only to compare the byte code | `program.byte` is same as in the paper |
| `sum_fixed.prog` | sum of 5 numbers, N fixed inside the program | 10+20+30+40+50 = 150 at address 20 |
| `sum_runtime.prog` | sum of N numbers, N read from address 0 (uses branches) | N=5, numbers 1..5, sum 15 at address `N*4+4` = 24 |
| `complex_mul.prog` | (2+3i) * (4+5i) | real -7 at address 16, imaginary 22 at address 20 |
| `det3x3.prog` | determinant of `[1 2 3; 4 5 6; 7 8 10]` | -3 at address 36 |
| `fir.prog` | FIR filter `y[n] = x[n] + 2*x[n-1]`, weights are constants | input 1,2,3 gives 1,4,7 from address 0x10 |
| `branch_flags.prog` | count down loop with `BNE` and a negative compare with `BLT` | 5 at address 0 and 200 at address 4 |
| `legacy_readwrite.prog` | old Lab 1 `Read` / `Write` instructions, kept only to show that the compiler still compiles them | 7 read from address 0, 8 written at address 4 |

A negative number is seen as `FF FF FF FF` style bytes in `data.byte`, for
example -3 is `FD FF FF FF`.

After every run the program also prints the flags and all the registers
which are not zero, which makes checking the result easier.
