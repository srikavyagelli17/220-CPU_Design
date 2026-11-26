16-bit CPU simulator with a small assembler. It runs 8^2 and 8! and dumps RAM after every instruction.

Compile:
  make
  gcc -std=c99 -Wall -Wextra -o cpu main.c cpu.c assembler.c display.c programs.c

Run:
  ./cpu
  ./cpu > output.txt

Output:
- Instruction encoding table, printed once
- After load, and after each instruction: registers plus all 32 RAM words
- RAM columns: address, hex, binary, ASCII, and disassembly or data label
- One RESULT line per program, with the expected value and PASS or FAIL

Zip:
  zip -r cpu_project.zip *.c *.h Makefile README.txt output.txt
