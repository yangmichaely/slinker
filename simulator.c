#include "simulator.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <regex.h>

int codeStart;
int dataStart;

int main(int argc, char** argv){
    FILE* f = fopen(argv[1], "rb\0");
    if(f == NULL){
        fprintf(stderr, "%s", "Invalid slinker filepath\n");
        exit(-1);
    }
    char o = argv[1][strlen(argv[1]) - 1];
    char k = argv[1][strlen(argv[1]) - 2];
    char l = argv[1][strlen(argv[1]) - 3];
    char s = argv[1][strlen(argv[1]) - 4];
    char dot = argv[1][strlen(argv[1]) - 5];
    if(k != 'k' || l != 'l' || dot != '.' || o != 'o' || s != 's'){
        fprintf(stderr, "%s", "Invalid slinker filepath\n");
        exit(-1);
    }
    fread(&codeStart, sizeof(int), 1, f);
    codeStart = codeStart >> 24;
    fread(&dataStart, sizeof(int), 1, f);
    dataStart = dataStart >> 24;
    readBinary(f);
    fclose(f);
    return 0;
}

void EXIT_ERROR(){
    fprintf(stderr, "%s\n", "Simulation error");
    exit(1);
}

void memCheck(uint64_t index){
    if(index < 0 || index >= HEAP_START){
        EXIT_ERROR();
    }
}

void stackCheck(uint64_t index){
    if(index < STACK_START || index >= MEM_SIZE){
        EXIT_ERROR();
    }
}

void heapCheck(uint64_t index){
    if(index < HEAP_START || index >= STACK_START){
        EXIT_ERROR();
    }
}

void writeStackVal(int numBytes, int ind, int64_t value){
    stackCheck(ind + numBytes - 1);
    for(int i = numBytes - 1; i >= 0; i--){
        cpu.mem[ind + i] = value >> (((numBytes - 1 - i) * 8) & 0xff);
    }
}

void writeStackMem(int numBytes, int ind, int readInd, int readStackHeap, int writeStackHeap){
    int64_t value;
    if(readStackHeap == 0){
        value = readMem(numBytes, readInd, 1);
    }
    else{
        value = readMem(numBytes, readInd, 2);
    }
    if(writeStackHeap == 0){
        stackCheck(ind + numBytes - 1);
    }
    else{
        heapCheck(ind + numBytes - 1);
    }
    for(int i = numBytes - 1; i >= 0; i--){
        cpu.mem[ind + i] = value >> (((numBytes - 1 - i) * 8) & 0xff);
    }
}

int64_t readMem(int numRead, int ind, int codeHeapStack){
    if(codeHeapStack == 0){
        memCheck(ind + numRead);
    }
    else if(codeHeapStack == 1){
        heapCheck(ind + numRead - 1);
    }
    else{
        stackCheck(ind + numRead - 1);
    }
    int64_t ans = 0;
    for(int i = numRead; i > 0; i--){
        ans |= cpu.mem[ind + i] << ((numRead - i) * 8);
    }
    return ans;
}

void interpret(uint8_t opcode, uint64_t intIn, double floatIn, int8_t secondParam){
    int8_t val8;
    int16_t val16;
    int32_t val32;
    int64_t val64;
    switch(opcode){
        case 0:
            writeStackVal(1, cpu.sp, intIn);
            cpu.sp++;
            cpu.pc += 2;
            break;
        case 1:
            writeStackVal(2, cpu.sp, intIn);
            cpu.sp += 2;
            cpu.pc += 3;
            break;
        case 2:
            writeStackVal(4, cpu.sp, intIn);
            cpu.sp += 4;
            cpu.pc += 5;
            break;
        case 3:
            writeStackVal(8, cpu.sp, intIn);
            cpu.sp += 8;
            cpu.pc += 9;
            break;
        case 4:
            val64 = *((int64_t*)&floatIn);
            writeStackVal(4, cpu.sp, intIn);
            cpu.sp += 4;
            cpu.pc += 5;
            break;
        case 5:
            val64 = *((int64_t*)&floatIn);
            writeStackVal(8, cpu.sp, intIn);
            cpu.sp += 8;
            cpu.pc += 9;
            break;
        case 6:
            writeStackMem(1, cpu.sp, intIn, 0, 0);
            cpu.sp += 1;
            cpu.pc += 4;
            break;
        case 7:
            writeStackMem(2, cpu.sp, intIn, 0, 0);
            cpu.sp += 1;
            cpu.pc += 4;
            break;
        case 8:
        case 10:
            writeStackMem(4, cpu.sp, intIn, 0, 0);
            cpu.sp += 1;
            cpu.pc += 4;
            break;
        case 9:
        case 11:
            writeStackMem(8, cpu.sp, intIn, 0, 0);
            cpu.sp += 1;
            cpu.pc += 4;
            break;
        case 12:
            for(int i = 0; i < secondParam; i++){
                cpu.mem[cpu.sp + i] = cpu.mem[intIn + i];
            }
            cpu.sp += secondParam;
            cpu.pc += 5;
            break;
        case 13:
            writeStackMem(1, cpu.sp, cpu.sp - 1, 1, 0);
            cpu.sp++;
            cpu.pc++;
            break;
        case 14:
            writeStackMem(2, cpu.sp, cpu.sp - 2, 1, 0);
            cpu.sp += 2;
            cpu.pc++;
            break;
        case 15:
        case 17:
            writeStackMem(4, cpu.sp, cpu.sp - 4, 1, 0);
            cpu.sp += 4;
            cpu.pc++;
            break;
        case 16:
        case 18:
            writeStackMem(8, cpu.sp, cpu.sp - 8, 1, 0);
            cpu.sp += 8;
            cpu.pc++;
            break;
        case 19:
            writeStackVal(1, cpu.sp - 1, 0);
            cpu.sp--;
            cpu.pc++;
            break;
        case 20:
            writeStackVal(2, cpu.sp - 2, 0);
            cpu.sp -= 2;
            cpu.pc++;
            break;
        case 21:
        case 23:
            writeStackVal(4, cpu.sp - 4, 0);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 22:
        case 24:
            writeStackVal(8, cpu.sp - 8, 0);
            cpu.sp -= 8;
            cpu.pc++;
            break;
        case 25:
            writeStackMem(1, cpu.sp, cpu.sp - 1, 0, 1);
            writeStackVal(1, cpu.sp - 1, 0);
            cpu.sp--;
            cpu.pc += 4;
            break;
        case 26:
            writeStackMem(2, cpu.sp, cpu.sp - 2, 0, 1);
            writeStackVal(2, cpu.sp - 2, 0);
            cpu.sp -= 2;
            cpu.pc += 4;
            break;
        case 27:
        case 29:
            writeStackMem(4, cpu.sp, cpu.sp - 4, 0, 1);
            writeStackVal(4, cpu.sp - 4, 0);
            cpu.sp -= 4;
            cpu.pc += 4;
            break;
        case 28:
        case 30:
            writeStackMem(8, cpu.sp, cpu.sp - 8, 0, 1);
            writeStackVal(8, cpu.sp - 8, 0);
            cpu.sp -= 8;
            cpu.pc += 4;
            break;
        case 31:
            stackCheck(cpu.sp + secondParam - 1);
            heapCheck(intIn + secondParam - 1);
            cpu.sp -= secondParam;
            for(int i = 0; i < secondParam; i++){
                cpu.mem[intIn + i] = cpu.mem[cpu.sp + i];
            }
            cpu.pc += 5;
            break;
        case 32:
            val64 = readMem(1, cpu.sp - 1, 2);
            writeStackMem(1, cpu.sp - 1, cpu.sp - 2, 0, 0);
            writeStackVal(1, cpu.sp - 2, val64);
            cpu.pc++;
            break;
        case 33:
            val64 = readMem(2, cpu.sp - 2, 2);
            writeStackMem(2, cpu.sp - 2, cpu.sp - 4, 0, 0);
            writeStackVal(2, cpu.sp - 4, val64);
            cpu.pc++;
            break;
        case 34:
        case 36:
            val64 = readMem(4, cpu.sp - 4, 2);
            writeStackMem(4, cpu.sp - 4, cpu.sp - 8, 0, 0);
            writeStackVal(4, cpu.sp - 8, val64);
            cpu.pc++;
            break;
        case 35:
        case 37:
            val64 = readMem(8, cpu.sp - 8, 2);
            writeStackMem(8, cpu.sp - 8, cpu.sp - 16, 0, 0);
            writeStackVal(8, cpu.sp - 16, val64);
            cpu.pc++;
            break;
        case 38:
            val16 = (int16_t) readMem(1, cpu.sp - 1, 2);
            writeStackVal(2, cpu.sp - 1, val16);
            cpu.sp++;
            cpu.pc++;
            break;
        case 39:
        case 41:
            val32 = (int32_t) readMem(1, cpu.sp - 1, 2);
            writeStackVal(4, cpu.sp - 1, val32);
            cpu.sp += 3;
            cpu.pc++;
            break;
        case 40:
        case 42:
            val64 = (int64_t) readMem(1, cpu.sp - 1, 2);
            writeStackVal(8, cpu.sp - 1, val64);
            cpu.sp += 7;
            cpu.pc++;
            break;
        case 43:
            val8 = (int8_t) readMem(2, cpu.sp - 2, 2);
            writeStackVal(1, cpu.sp - 2, val8);
            cpu.sp--;
            cpu.pc++;
            break;
        case 44:
        case 46:
            val32 = (int32_t) readMem(2, cpu.sp - 2, 2);
            writeStackVal(4, cpu.sp - 2, val32);
            cpu.sp += 2;
            cpu.pc++;
            break;
        case 45:
        case 47:
            val64 = (int64_t) readMem(2, cpu.sp - 2, 2);
            writeStackVal(8, cpu.sp - 2, val64);
            cpu.sp += 6;
            cpu.pc++;
            break;
        case 48:
        case 59:
            val8 = (int8_t) readMem(4, cpu.sp - 4, 2);
            writeStackVal(1, cpu.sp - 4, val8);
            cpu.sp -= 3;
            cpu.pc++;
            break;
        case 49:
        case 60:
            val16 = (int16_t) readMem(4, cpu.sp - 4, 2);
            writeStackVal(2, cpu.sp - 4, val16);
            cpu.sp -= 2;
            cpu.pc++;
            break;
        case 50:
        case 52:
        case 62:
        case 63:
            val64 = (int64_t) readMem(4, cpu.sp - 4, 2);
            writeStackVal(8, cpu.sp - 4, val64);
            cpu.sp += 4;
            cpu.pc++;
            break;
        case 51:
        case 61:
            val32 = (int32_t) readMem(4, cpu.sp - 4, 2);
            writeStackVal(4, cpu.sp - 4, val32);
            cpu.pc++;
            break;
        case 53:
        case 64:
            val8 = (int8_t) readMem(8, cpu.sp - 8, 2);
            writeStackVal(1, cpu.sp - 8, val8);
            cpu.sp -= 7;
            cpu.pc++;
            break;
        case 54:
        case 65:
            val16 = (int16_t) readMem(8, cpu.sp - 8, 2);
            writeStackVal(2, cpu.sp - 8, val16);
            cpu.sp -= 6;
            cpu.pc++;
            break;
        case 55:
        case 57:
        case 66:
        case 68:
            val32 = (int32_t) readMem(8, cpu.sp - 8, 2);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 3;
            cpu.pc++;
            break;
        case 58:
        case 67:
            val64 = (int64_t) readMem(8, cpu.sp - 8, 2);
            writeStackVal(8, cpu.sp - 8, val64);
            cpu.pc++;
            break;
        case 69:
            
    }
}

void readBinary(FILE* f){
    for(int i = 0; i < MEM_SIZE; i++){
        cpu.mem[i] = 0;
    }
    uint8_t binary = 0;
    cpu.pc = 0;
    cpu.sp = STACK_START;
    int nextMem = 0;
    fseek(f, codeStart, SEEK_SET);
    while(ftell(f) < dataStart){
        fread(&binary, sizeof(binary), 1, f);
        cpu.mem[nextMem] = binary;
        nextMem++;
    }
    fseek(f, dataStart, SEEK_SET);
    nextMem = HEAP_START;
    while(fread(&binary, sizeof(binary), 1, f) == 1){
        cpu.mem[nextMem] = binary;
        nextMem++;
    }
    while(cpu.pc < STACK_START && cpu.pc >= 0 && cpu.sp < HEAP_START && cpu.sp >= STACK_START){
        uint8_t opcode = cpu.mem[cpu.pc];
        int64_t intIn = 0;
        int8_t secondParam = 0;
        double floatIn = 0;
        switch(opcode){
            case 0:
            case 123 ... 130:
            case 132:
                intIn = readMem(cpu.pc, 1, 0);
                break;
            case 1:
                intIn = readMem(cpu.pc, 2, 0);
                break;
            case 2:
                intIn = readMem(cpu.pc, 4, 0);
                break;
            case 3:
                intIn = readMem(cpu.pc, 8, 0);
                break;
            case 4:
                intIn = readMem(cpu.pc, 4, 0);
                memcpy(&floatIn, &intIn, sizeof(floatIn));
                break;
            case 5:
                intIn = readMem(cpu.pc, 8, 0);
                memcpy(&floatIn, &intIn, sizeof(floatIn));
                break;
            case 6 ... 11:
            case 25 ... 30:
            case 131:
            case 134 ... 140:
                intIn = readMem(cpu.pc, 3, 0);
                break;
            case 12:
            case 31:
                intIn = readMem(cpu.pc, 3, 0);
                secondParam = readMem(cpu.pc + 3, 1, 0);
                break;
            case 142:
                exit(0);
        }
        printf("opcode: %d\n", opcode);
        printf("intIn: %ld\n", intIn);
        printf("secondParam: %d\n", secondParam);
        printf("floatIn: %f\n", floatIn);
        interpret(opcode, intIn, floatIn, secondParam);
    }
    EXIT_ERROR();
}