#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#ifndef SIMULATOR_H
#define SIMULATOR_H
#define MEM_SIZE 512 * 1024
#define HEAP_START 0x10000
#define STACK_START 0x40000

typedef struct CPU {
    uint8_t mem[MEM_SIZE];
    uint64_t pc;
    uint64_t sp;
}CPU;

CPU cpu;

void EXIT_ERROR();

void memCheck(uint64_t index);

void interpret(uint8_t opcode, uint64_t intIn, double floatIn, int8_t secondParam);

int64_t readMem(int numRead, int ind, int codeHeapStack);

double readMemDouble(int numRead, int ind, int codeHeapStack);

float readMemFloat(int numRead, int ind, int codeHeapStack);

void writeStackVal(int numBytes, int ind, int64_t value);

void writeHeapVal(int numBytes, int ind, int64_t value);

void writeStackMem(int numBytes, int ind, int readInd, int readStackHeap, int writeStackHeap);

void readBinary(FILE* f);

#endif