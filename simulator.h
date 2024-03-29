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

int8_t byteCheck(char* in);

short shortCheck(char* in);

int intCheck(char* in);

long long longCheck(char* in);

float floatCheck(char* in);

double doubleCheck(char* in);

void interpret(uint8_t opcode, int64_t intIn, double floatIn, int8_t secondParam);

int64_t readMem(int numRead, int ind);

double readMemDouble(int numRead, int ind);

float readMemFloat(int numRead, int ind);

void writeStackVal(int numBytes, int ind, int64_t value);

void writeHeapVal(int numBytes, int ind, int64_t value);

void readBinary(FILE* f);

#endif