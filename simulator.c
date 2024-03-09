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
    for(int i = ind + numBytes - 1; i >= ind; i--){
        cpu.mem[i] = value & 0xff;
        value >>= 8;
    }
}

void writeHeapVal(int numBytes, int ind, int64_t value){
    heapCheck(ind + numBytes - 1);
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
        memCheck(ind + numRead - 1);
    }
    else if(codeHeapStack == 1){
        heapCheck(ind + numRead - 1);
    }
    else{
        stackCheck(ind + numRead - 1);
    }
    int64_t ans = 0;
    for(int i = ind; i < ind + numRead; i++){
        ans = (ans << 8) | cpu.mem[i];
    }
    return ans;
}

float readMemFloat(int numRead, int ind, int codeHeapStack){
    if(codeHeapStack == 0){
        memCheck(ind + numRead - 1);
    }
    else if(codeHeapStack == 1){
        heapCheck(ind + numRead - 1);
    }
    else{
        stackCheck(ind + numRead - 1);
    }
    uint32_t ans = 0;
    for(int i = ind; i < ind + numRead; i++){
        ans = (ans << 8) | cpu.mem[i];
    }
    float d = *((float*)&ans);
    return d;
}

double readMemDouble(int numRead, int ind, int codeHeapStack){
    if(codeHeapStack == 0){
        memCheck(ind + numRead - 1);
    }
    else if(codeHeapStack == 1){
        heapCheck(ind + numRead - 1);
    }
    else{
        stackCheck(ind + numRead - 1);
    }
    int64_t ans = 0;
    for(int i = ind; i < ind + numRead; i++){
        ans = (ans << 8) | cpu.mem[i];
    }
    double d = *((double*) &ans);
    return d;
}

int8_t byteCheck(char* in){
    char* ptr;
    long num = strtol(in, &ptr, 10);
    if(ptr[0] != '\0'){
        EXIT_ERROR();
    }
    if(num < -128 || num > 127){
        EXIT_ERROR();
    }
    return num;
}

short shortCheck(char* in){
    char* ptr;
    long num = strtol(in, &ptr, 10);
    if(ptr[0] != '\0'){
        EXIT_ERROR();
    }
    if(num < SHRT_MIN || num > SHRT_MAX){
        EXIT_ERROR();
    }
    return num;
}

int intCheck(char* in){
    char* ptr;
    long num = strtol(in, &ptr, 10);
    if(ptr[0] != '\0'){
        EXIT_ERROR();
    }
    if(num < INT_MIN || num > INT_MAX){
        EXIT_ERROR();
    }
    return num;
}

long long longCheck(char* in){
    char* ptr;
    long long num = strtoll(in, &ptr, 10);
    if(ptr[0] != '\0'){
        EXIT_ERROR();
    }
    if(strcmp(in, "9223372036854775807") != 0 && num == LLONG_MAX){
        EXIT_ERROR();
    }
    else if(strcmp(in, "-9223372036854775808") != 0 && num == LLONG_MIN){
        EXIT_ERROR();
    }
    return num;
}

float floatCheck(char* in){
    char* ptr;
    float num = strtof(in, &ptr);
    if(ptr[0] != '\0'){
        EXIT_ERROR();
    }
    return num;
}

double doubleCheck(char* in){
    char* ptr;
    double num = strtod(in, &ptr);
    if(ptr[0] != '\0'){
        EXIT_ERROR();
    }
    return num;
}

void interpret(uint8_t opcode, int64_t intIn, double floatIn, int8_t secondParam){
    int8_t val8;
    int16_t val16;
    int32_t val32;
    int64_t val64;
    double valDouble;
    float valFloat;
    char valChar;
    int32_t tmp32;
    int64_t tmp64;
    char* in = (char*) calloc (sizeof(char) * 50, 1);
    switch(opcode){
        case 0:
            val8 = (int8_t) intIn;
            writeStackVal(1, cpu.sp, val8);
            cpu.sp++;
            cpu.pc += 2;
            break;
        case 1:
            val16 = (int16_t) intIn;
            writeStackVal(2, cpu.sp, val16);
            cpu.sp += 2;
            cpu.pc += 3;
            break;
        case 2:
            val32 = (int32_t) intIn;
            writeStackVal(4, cpu.sp, val32);
            cpu.sp += 4;
            cpu.pc += 5;
            break;
        case 3:
            writeStackVal(8, cpu.sp, intIn);
            cpu.sp += 8;
            cpu.pc += 9;
            break;
        case 4:
            valFloat = (float) floatIn;
            val32 =  *((int32_t*)&valFloat);
            writeStackVal(4, cpu.sp, val32);
            cpu.sp += 4;
            cpu.pc += 5;
            break;
        case 5:
            val64 = *((int64_t*)&floatIn);
            writeStackVal(8, cpu.sp, val64);
            cpu.sp += 8;
            cpu.pc += 9;
            break;
        case 6:
            val8 = readMem(1, intIn, 1);
            writeStackVal(1, cpu.sp, val8);
            cpu.sp += 1;
            cpu.pc += 4;
            break;
        case 7:
            val16 = readMem(2, intIn, 1);
            writeStackVal(2, cpu.sp, val16);
            cpu.sp += 2;
            cpu.pc += 4;
            break;
        case 8:
            val32 = readMem(4, intIn, 1);
            writeStackVal(4, cpu.sp, val32);
            cpu.sp += 4;
            cpu.pc += 4;
            break;
        case 9:
            val64 = readMem(8, intIn, 1);
            writeStackVal(8, cpu.sp, val64);
            cpu.sp += 8;
            cpu.pc += 4;
            break;
        case 10:
            valFloat = readMemFloat(4, intIn, 1);
            val32 = *((int32_t*)&valFloat);
            writeStackVal(4, cpu.sp, val32);
            cpu.sp += 4;
            cpu.pc += 4;
            break;
        case 11:
            valDouble = readMemDouble(8, intIn, 1);
            val64 = *((int64_t*) &valDouble);
            writeStackVal(8, cpu.sp, val64);
            cpu.sp += 8;
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
            val8 = readMem(1, cpu.sp - 1, 2);
            writeStackVal(1, cpu.sp, val8);
            cpu.sp++;
            cpu.pc++;
            break;
        case 14:
            val16 = readMem(2, cpu.sp - 2, 2);
            writeStackVal(2, cpu.sp, val16);
            cpu.sp += 2;
            cpu.pc++;
            break;
        case 15:
            val32 = readMem(4, cpu.sp - 4, 2);
            writeStackVal(4, cpu.sp, val32);
            cpu.sp += 4;
            cpu.pc++;
            break;
        case 16:
            val64 = readMem(8, cpu.sp - 8, 2);
            writeStackVal(8, cpu.sp, val64);
            cpu.sp += 8;
            cpu.pc++;
            break;
        case 17:
            valFloat = readMemFloat(4, cpu.sp - 4, 2);
            val32 = *((int32_t*)&valFloat);
            writeStackVal(4, cpu.sp, val32);
            cpu.sp += 4;
            cpu.pc++;
            break;
        case 18:
            valDouble = readMemDouble(8, cpu.sp - 48, 2);
            val64 = *((int64_t*)&valFloat);
            writeStackVal(8, cpu.sp, val64);
            cpu.sp += 8;
            cpu.pc++;
            break;
        case 19:
            //writeStackVal(1, cpu.sp - 1, 0);
            cpu.sp--;
            cpu.pc++;
            break;
        case 20:
            //writeStackVal(2, cpu.sp - 2, 0);
            cpu.sp -= 2;
            cpu.pc++;
            break;
        case 21:
        case 23:
            //writeStackVal(4, cpu.sp - 4, 0);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 22:
        case 24:
            //writeStackVal(8, cpu.sp - 8, 0);
            cpu.sp -= 8;
            cpu.pc++;
            break;
        case 25:
            val8 = readMem(1, cpu.sp - 1, 2);
            writeHeapVal(1, intIn, val8);
            //writeStackMem(1, intIn, cpu.sp - 1, 1, 1);
            //writeStackVal(1, cpu.sp - 1, 0);
            cpu.sp--;
            cpu.pc += 4;
            break;
        case 26:
            //writeStackMem(2, intIn, cpu.sp - 2, 1, 1);
            //writeStackVal(2, cpu.sp - 2, 0);
            val16 = readMem(2, cpu.sp - 2, 2);
            writeHeapVal(2, intIn, val16);
            cpu.sp -= 2;
            cpu.pc += 4;
            break;
        case 27:
            //writeStackMem(4, intIn, cpu.sp - 4, 1, 1);
            //writeStackVal(4, cpu.sp - 4, 0);
            val32 = readMem(4, cpu.sp - 4, 2);
            writeHeapVal(4, intIn, val32);
            cpu.sp -= 4;
            cpu.pc += 4;
            break;
        case 28:
            //writeStackMem(8, intIn, cpu.sp - 8, 1, 1);
            //writeStackVal(8, cpu.sp - 8, 0);
            val64 = readMem(8, cpu.sp - 8, 2);
            writeHeapVal(8, intIn, val64);
            cpu.sp -= 8;
            cpu.pc += 4;
            break;
        case 29:
            valFloat = readMemFloat(4, cpu.sp - 4, 2);
            val32 = *((int32_t*)&valFloat);
            writeHeapVal(8, intIn, val32);
            //writeStackVal(8, cpu.sp - 8, 0);
            cpu.sp -= 4;
            cpu.pc += 4;
            break;
        case 30:
            valDouble = readMemDouble(8, cpu.sp - 8, 2);
            val64 = *((int64_t*)&valDouble);
            writeHeapVal(8, intIn, val64);
            //writeStackVal(8, cpu.sp - 8, 0);
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
            val8 = readMem(1, cpu.sp - 1, 2);
            int8_t tmp8 = readMem(1, cpu.sp - 2, 2);
            writeStackVal(1, cpu.sp - 2, val8);
            writeStackVal(1, cpu.sp - 1, tmp8);
            cpu.pc++;
            break;
        case 33:
            val16 = readMem(2, cpu.sp - 2, 2);
            int16_t tmp16 = readMem(2, cpu.sp - 4, 2);
            writeStackVal(2, cpu.sp - 4, val16);
            writeStackVal(2, cpu.sp - 2, tmp16);
            cpu.pc++;
            break;
        case 34:
            val32 = readMem(4, cpu.sp - 4, 2);
            tmp32 = readMem(4, cpu.sp - 8, 2);
            writeStackVal(4, cpu.sp - 8, val32);
            writeStackVal(4, cpu.sp - 4, tmp32);
            cpu.pc++;
            break;
        case 35:
            val64 = readMem(8, cpu.sp - 8, 2);
            tmp64 = readMem(8, cpu.sp - 16, 2);
            writeStackVal(8, cpu.sp - 16, val64);
            writeStackVal(8, cpu.sp - 8, tmp64);
            cpu.pc++;
            break;
        case 36:
            valFloat = readMemFloat(4, cpu.sp - 4, 2);
            val32 = *((int32_t*)&valFloat);
            float tmpFloat = readMemFloat(4, cpu.sp - 8, 2);
            tmp32 = *((int32_t*)&tmpFloat);
            writeStackVal(4, cpu.sp - 8, val32);
            writeStackVal(4, cpu.sp - 4, tmp32);
            cpu.pc++;
            break;
        case 37:
            valDouble = readMemFloat(8, cpu.sp - 8, 2);
            val64 = *((int64_t*)&valDouble);
            double tmpDouble = readMemDouble(8, cpu.sp - 16, 2);
            tmp64 = *((int64_t*)&tmpDouble);
            writeStackVal(4, cpu.sp - 8, val64);
            writeStackVal(4, cpu.sp - 4, tmp64);
            cpu.pc++;
            break;
        case 38:
            val16 = (int16_t) readMem(1, cpu.sp - 1, 2);
            writeStackVal(2, cpu.sp - 1, val16);
            cpu.sp++;
            cpu.pc++;
            break;
        case 39:
            val32 = (int32_t) readMem(1, cpu.sp - 1, 2);
            writeStackVal(4, cpu.sp - 1, val32);
            cpu.sp += 3;
            cpu.pc++;
            break;
        case 40:
            val64 = (int64_t) readMem(1, cpu.sp - 1, 2);
            writeStackVal(8, cpu.sp - 1, val64);
            cpu.sp += 7;
            cpu.pc++;
            break;
        case 41:
            val8 = readMem(1, cpu.sp - 1, 2);
            valFloat = (float) val8;
            val32 = *((int32_t*)&valFloat);
            writeStackVal(4, cpu.sp - 1, val32);
            cpu.sp += 3;
            cpu.pc++;
            break;
        case 42:
            val8 = readMem(1, cpu.sp - 1, 2);
            valDouble = (double) val8;
            val64 = *((int64_t*)&valDouble);
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
            val32 = (int32_t) readMem(2, cpu.sp - 2, 2);
            writeStackVal(4, cpu.sp - 2, val32);
            cpu.sp += 2;
            cpu.pc++;
            break;
        case 45:
            val64 = (int64_t) readMem(2, cpu.sp - 2, 2);
            writeStackVal(8, cpu.sp - 2, val64);
            cpu.sp += 6;
            cpu.pc++;
            break;
        case 46:
            val16 = readMem(2, cpu.sp - 2, 2);
            valFloat = (float) val16;
            val32 = *((int32_t*)&valFloat);
            writeStackVal(4, cpu.sp - 1, val32);
            cpu.sp += 2;
            cpu.pc++;
            break;
        case 47:
            val16 = readMem(2, cpu.sp - 2, 2);
            valDouble = (double) val16;
            val64 = *((int64_t*)&valDouble);
            writeStackVal(8, cpu.sp - 1, val64);
            cpu.sp += 6;
            cpu.pc++;
            break;
        case 48:
            val8 = (int8_t) readMem(4, cpu.sp - 4, 2);
            writeStackVal(1, cpu.sp - 4, val8);
            cpu.sp -= 3;
            cpu.pc++;
            break;
        case 49:
            val16 = (int16_t) readMem(4, cpu.sp - 4, 2);
            writeStackVal(2, cpu.sp - 4, val16);
            cpu.sp -= 2;
            cpu.pc++;
            break;
        case 50:
            val64 = (int64_t) readMem(4, cpu.sp - 4, 2);
            writeStackVal(8, cpu.sp - 4, val64);
            cpu.sp += 4;
            cpu.pc++;
            break;
        case 51:
            val32 = readMem(4, cpu.sp - 4, 2);
            valFloat = (float) val32;
            val32 = *((int32_t*)&valFloat);
            writeStackVal(4, cpu.sp - 4, val32);
            cpu.pc++;
            break;
        case 52:
            val32 = readMem(4, cpu.sp - 4, 2);
            valDouble = (double) val32;
            val64 = *((int64_t*)&valDouble);
            writeStackVal(8, cpu.sp - 4, val64);
            cpu.sp += 4;
            cpu.pc++;
            break;
        case 53:
            val8 = (int8_t) readMem(8, cpu.sp - 8, 2);
            writeStackVal(1, cpu.sp - 8, val8);
            cpu.sp -= 7;
            cpu.pc++;
            break;
        case 54:
            val16 = (int16_t) readMem(8, cpu.sp - 8, 2);
            writeStackVal(2, cpu.sp - 8, val16);
            cpu.sp -= 6;
            cpu.pc++;
            break;
        case 55:
            val32 = (int32_t) readMem(8, cpu.sp - 8, 2);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 3;
            cpu.pc++;
            break;
        case 57:
            val64 = readMem(8, cpu.sp - 8, 2);
            valFloat = (float) val64;
            val32 = *((float*)&valFloat);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 58:
            val64 = readMem(8, cpu.sp - 8, 2);
            valDouble = (double) val64;
            val64 = *((double*)&valDouble);
            writeStackVal(8, cpu.sp - 8, val64);
            cpu.pc++;
            break;
        case 59:
            valFloat = readMemFloat(4, cpu.sp - 4, 2);
            val8 = (int8_t) valFloat;
            writeStackVal(1, cpu.sp - 4, val8);
            cpu.sp -= 3;
            cpu.pc++;
            break;
        case 60:
            valFloat = readMemFloat(4, cpu.sp - 4, 2);
            val16 = (int16_t) valFloat;
            writeStackVal(2, cpu.sp - 4, val16);
            cpu.sp -= 2;
            cpu.pc++;
            break;
        case 61:
            valFloat = readMemFloat(4, cpu.sp - 4, 2);
            val32 = (int32_t) valFloat;
            writeStackVal(4, cpu.sp - 4, val32);
            cpu.pc++;
            break;
        case 62:
            valFloat = readMemFloat(4, cpu.sp - 4, 2);
            val64 = (int64_t) valFloat;
            writeStackVal(8, cpu.sp - 4, val64);
            cpu.sp += 4;
            cpu.pc++;
            break;
        case 63:
            valFloat = readMemFloat(4, cpu.sp - 4, 2);
            valDouble = (double) valFloat;
            val64 = *((int64_t*)&valDouble);
            writeStackVal(8, cpu.sp - 4, val64);
            cpu.sp += 4;
            cpu.pc++;
            break;
        case 64:
            valDouble = readMemDouble(8, cpu.sp - 8, 2);
            val8 = (int8_t) valDouble;
            writeStackVal(1, cpu.sp - 8, val8);
            cpu.sp -= 7;
            cpu.pc++;
            break;
        case 65:
            valDouble = readMemDouble(8, cpu.sp - 8, 2);
            val16 = (int16_t) valDouble;
            writeStackVal(2, cpu.sp - 8, val16);
            cpu.sp -= 6;
            cpu.pc++;
            break;
        case 66:
            valDouble = readMemDouble(8, cpu.sp - 8, 2);
            val32 = (int32_t) valDouble;
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 67:
            valDouble = readMemDouble(8, cpu.sp - 8, 2);
            val64 = (int64_t) valDouble;
            writeStackVal(8, cpu.sp - 8, val64);
            cpu.pc++;
            break;
        case 68:
            valDouble = readMemDouble(8, cpu.sp - 8, 2);
            valFloat = (float) valDouble;
            val32 = *((int32_t*)&valFloat);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        //TODO: bounds checking for input
        case 69:
            scanf("%s", in);
            if(strlen(in) != 2){
                EXIT_ERROR();
            }
            val8 = atoi(in);
            writeStackVal(1, cpu.sp, val8);
            cpu.sp++;
            cpu.pc++;
            break;
        case 70:
            scanf("%s", in);
            val8 = byteCheck(in);
            writeStackVal(1, cpu.sp, val8);
            cpu.sp++;
            cpu.pc++;
            break;
        case 71:
            scanf("%s", in);
            val16 = shortCheck(in);
            writeStackVal(2, cpu.sp, val16);
            cpu.sp += 2;
            cpu.pc++;
            break;
        case 72:
            scanf("%s", in);
            val32 = intCheck(in);
            writeStackVal(4, cpu.sp, val32);
            cpu.sp += 4;
            cpu.pc++;
            break;
        case 73:
            scanf("%s", in);
            val64 = longCheck(in);
            writeStackVal(8, cpu.sp, val64);
            cpu.sp += 8;
            cpu.pc++;
            break;
        case 74:
            scanf("%s", in);
            valFloat = floatCheck(in);
            val32 = *((uint32_t*)&valFloat);
            writeStackVal(4, cpu.sp, val32);
            cpu.sp += 4;
            cpu.pc++;
            break;
        case 75:
            scanf("%s", in);
            valDouble = doubleCheck(in);
            val64 = *((uint64_t*)&valDouble);
            writeStackVal(8, cpu.sp, val64);
            cpu.sp += 8;
            cpu.pc++;
            break;
        case 76:
            val8 = readMem(1, cpu.sp - 1, 2);
            printf("%c\n", val8);
            cpu.sp--;
            cpu.pc++;
            break;
        case 77:
            val8 = readMem(1, cpu.sp - 1, 2);
            printf("%hhd\n", val8);
            cpu.sp--;
            cpu.pc++;
            break;
        case 78:
            val16 = readMem(2, cpu.sp - 2, 2);
            printf("%hd\n", val16);
            cpu.sp -= 2;
            cpu.pc++;
            break;
        case 79:
            val32 = readMem(4, cpu.sp - 4, 2);
            printf("%d\n", val32);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 80:
            val64 = readMem(8, cpu.sp - 8, 2);
            printf("%ld\n", val64);
            cpu.sp -= 8;
            cpu.pc++;
            break;
        case 81:
            valFloat = readMemFloat(4, cpu.sp - 4, 2);
            printf("%f\n", valFloat);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 82:
            valDouble = readMemDouble(8, cpu.sp - 8, 2);
            printf("%lf\n", valDouble);
            cpu.sp -= 8;
            cpu.pc++;
            break;
        //TODO: Check for overflow
        case 83:
            val8 = readMem(1, cpu.sp - 1, 2) + readMem(1, cpu.sp - 2, 2);
            writeStackVal(1, cpu.sp - 2, val8);
            cpu.sp--;
            cpu.pc++;
            break;
        case 84:
            val16 = readMem(2, cpu.sp - 2, 2) + readMem(2, cpu.sp - 4, 2);
            writeStackVal(2, cpu.sp - 4, val16);
            cpu.sp -= 2;
            cpu.pc++;
            break;
        case 85:
            val32 = readMem(8, cpu.sp - 4, 2) + readMem(8, cpu.sp - 8, 2);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 86:
            val64 = readMem(8, cpu.sp - 8, 2) + readMem(8, cpu.sp - 16, 2);
            writeStackVal(8, cpu.sp - 16, val64);
            cpu.sp -= 8;
            cpu.pc++;
            break;
        case 87:
            valFloat = readMemFloat(8, cpu.sp - 4, 2) + readMemFloat(8, cpu.sp - 8, 2);
            val32 = *((int32_t*) &valFloat);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 88:
            valDouble = readMemDouble(8, cpu.sp - 8, 2) + readMemDouble(8, cpu.sp - 16, 2);
            val64 = *((int64_t*) &valDouble);
            writeStackVal(8, cpu.sp - 16, val64);
            cpu.sp -= 8;
            cpu.pc++;
            break;
        case 89:
            val8 = readMem(1, cpu.sp - 2, 2) - readMem(1, cpu.sp - 1, 2);
            writeStackVal(1, cpu.sp - 2, val8);
            cpu.sp--;
            cpu.pc++;
            break;
        case 90:
            val16 = readMem(2, cpu.sp - 4, 2) - readMem(2, cpu.sp - 2, 2);
            writeStackVal(2, cpu.sp - 4, val16);
            cpu.sp -= 2;
            cpu.pc++;
            break;
        case 91:
            val32 = readMem(4, cpu.sp - 8, 2) - readMem(4, cpu.sp - 4, 2);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 92:
            val64 = readMem(8, cpu.sp - 16, 2) - readMem(8, cpu.sp - 8, 2);
            writeStackVal(8, cpu.sp - 16, val64);
            cpu.sp -= 8;
            cpu.pc++;
            break;
        case 93:
            valFloat = readMemFloat(4, cpu.sp - 8, 2) - readMemFloat(4, cpu.sp - 4, 2);
            val32 = *((int32_t*)&valFloat);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 94:
            valDouble = readMemDouble(8, cpu.sp - 16, 2) - readMemDouble(8, cpu.sp - 8, 2);
            val64 = *((int64_t*)&valDouble);
            writeStackVal(8, cpu.sp - 16, val64);
            cpu.sp -= 8;
            cpu.pc++;
            break;
        case 95:
            val8 = readMem(1, cpu.sp - 2, 2) * readMem(1, cpu.sp - 1, 2);
            writeStackVal(1, cpu.sp - 2, val8);
            cpu.sp--;
            cpu.pc++;
            break;
        case 96:
            val16 = readMem(2, cpu.sp - 4, 2) * readMem(2, cpu.sp - 2, 2);
            writeStackVal(2, cpu.sp - 4, val16);
            cpu.sp -= 2;
            cpu.pc++;
            break;
        case 97:
            val32 = readMem(4, cpu.sp - 8, 2) * readMem(4, cpu.sp - 4, 2);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 98:
            val64 = readMem(8, cpu.sp - 16, 2) * readMem(8, cpu.sp - 8, 2);
            writeStackVal(8, cpu.sp - 16, val64);
            cpu.sp -= 8;
            cpu.pc++;
            break;
        case 99:
            valFloat = readMemFloat(4, cpu.sp - 8, 2) * readMemFloat(4, cpu.sp - 4, 2);
            val32 = *((int32_t*)&valFloat);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 100:
            valDouble = readMemDouble(8, cpu.sp - 16, 2) * readMemDouble(8, cpu.sp - 8, 2);
            val64 = *((int64_t*)&valDouble);
            writeStackVal(8, cpu.sp - 16, val64);
            cpu.sp -= 8;
            cpu.pc++;
            break;
        case 101:
            val8 = readMem(1, cpu.sp - 2, 2) / readMem(1, cpu.sp - 1, 2);
            writeStackVal(1, cpu.sp - 2, val8);
            cpu.sp--;
            cpu.pc++;
            break;
        case 102:
            val16 = readMem(2, cpu.sp - 4, 2) / readMem(2, cpu.sp - 2, 2);
            writeStackVal(2, cpu.sp - 4, val16);
            cpu.sp -= 2;
            cpu.pc++;
            break;
        case 103:
            val32 = readMem(4, cpu.sp - 8, 2) / readMem(4, cpu.sp - 4, 2);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 104:
            val64 = readMem(8, cpu.sp - 16, 2) / readMem(8, cpu.sp - 8, 2);
            writeStackVal(8, cpu.sp - 16, val64);
            cpu.sp -= 8;
            cpu.pc++;
            break;
        case 105:
            valFloat = readMemFloat(4, cpu.sp - 8, 2) / readMemFloat(4, cpu.sp - 4, 2);
            val32 = *((int32_t*)&valFloat);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 106:
            valDouble = readMemDouble(8, cpu.sp - 16, 2) / readMemDouble(8, cpu.sp - 8, 2);
            val64 = *((int64_t*)&valDouble);
            writeStackVal(8, cpu.sp - 16, val64);
            cpu.sp -= 8;
            cpu.pc++;
            break;
        case 107:
            val8 = readMem(1, cpu.sp - 2, 2) & readMem(1, cpu.sp - 1, 2);
            writeStackVal(1, cpu.sp - 2, val8);
            cpu.sp--;
            cpu.pc++;
            break;
        case 108:
            val16 = readMem(2, cpu.sp - 4, 2) & readMem(2, cpu.sp - 2, 2);
            writeStackVal(2, cpu.sp - 4, val16);
            cpu.sp -= 2;
            cpu.pc++;
            break;
        case 109:
            val32 = readMem(4, cpu.sp - 8, 2) & readMem(4, cpu.sp - 4, 2);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 110:
            val64 = readMem(8, cpu.sp - 16, 2) & readMem(8, cpu.sp - 8, 2);
            writeStackVal(8, cpu.sp - 16, val64);
            cpu.sp -= 8;
            cpu.pc++;
            break;
        case 111:
            val8 = readMem(1, cpu.sp - 2, 2) | readMem(1, cpu.sp - 1, 2);
            writeStackVal(1, cpu.sp - 2, val8);
            cpu.sp--;
            cpu.pc++;
            break;
        case 112:
            val16 = readMem(2, cpu.sp - 4, 2) | readMem(2, cpu.sp - 2, 2);
            writeStackVal(2, cpu.sp - 4, val16);
            cpu.sp -= 2;
            cpu.pc++;
            break;
        case 113:
            val32 = readMem(4, cpu.sp - 8, 2) | readMem(4, cpu.sp - 4, 2);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 114:
            val64 = readMem(8, cpu.sp - 16, 2) | readMem(8, cpu.sp - 8, 2);
            writeStackVal(8, cpu.sp - 16, val64);
            cpu.sp -= 8;
            cpu.pc++;
            break;
        case 115:
            val8 = readMem(1, cpu.sp - 2, 2) ^ readMem(1, cpu.sp - 1, 2);
            writeStackVal(1, cpu.sp - 2, val8);
            cpu.sp--;
            cpu.pc++;
            break;
        case 116:
            val16 = readMem(2, cpu.sp - 4, 2) ^ readMem(2, cpu.sp - 2, 2);
            writeStackVal(2, cpu.sp - 4, val16);
            cpu.sp -= 2;
            cpu.pc++;
            break;
        case 117:
            val32 = readMem(4, cpu.sp - 8, 2) ^ readMem(4, cpu.sp - 4, 2);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.sp -= 4;
            cpu.pc++;
            break;
        case 118:
            val64 = readMem(8, cpu.sp - 16, 2) ^ readMem(8, cpu.sp - 8, 2);
            writeStackVal(8, cpu.sp - 16, val64);
            cpu.sp -= 8;
            cpu.pc++;
            break;
        case 119:
            val8 = ~readMem(1, cpu.sp - 2, 2);
            writeStackVal(1, cpu.sp - 2, val8);
            cpu.pc++;
            break;
        case 120:
            val16 = ~readMem(2, cpu.sp - 4, 2);
            writeStackVal(2, cpu.sp - 4, val16);
            cpu.pc++;
            break;
        case 121:
            val32 = ~readMem(4, cpu.sp - 8, 2);
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.pc++;
            break;
        case 122:
            val64 = ~readMem(8, cpu.sp - 16, 2);
            writeStackVal(8, cpu.sp - 16, val64);
            cpu.pc++;
            break;
        case 123:
            val8 = readMem(1, cpu.sp - 2, 2) >> intIn;
            writeStackVal(1, cpu.sp - 2, val8);
            cpu.pc += 2;
            break;
        case 124:
            val16 = ~readMem(2, cpu.sp - 4, 2) >> intIn;
            writeStackVal(2, cpu.sp - 4, val16);
            cpu.pc += 2;
            break;
        case 125:
            val32 = ~readMem(4, cpu.sp - 8, 2) >> intIn;
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.pc += 2;
            break;
        case 126:
            val64 = ~readMem(8, cpu.sp - 16, 2) >> intIn;
            writeStackVal(8, cpu.sp - 16, val64);
            cpu.pc += 2;
            break;
        case 127:
            val8 = readMem(1, cpu.sp - 2, 2) << intIn;
            writeStackVal(1, cpu.sp - 2, val8);
            cpu.pc += 2;
            break;
        case 128:
            val16 = ~readMem(2, cpu.sp - 4, 2) << intIn;
            writeStackVal(2, cpu.sp - 4, val16);
            cpu.pc += 2;
            break;
        case 129:
            val32 = ~readMem(4, cpu.sp - 8, 2) << intIn;
            writeStackVal(4, cpu.sp - 8, val32);
            cpu.pc += 2;
            break;
        case 130:
            val64 = ~readMem(8, cpu.sp - 16, 2) << intIn;
            writeStackVal(8, cpu.sp - 16, val64);
            cpu.pc += 2;
            break;
        case 131:
            memCheck(intIn);
            cpu.pc = intIn;
            break;
        case 132:
            memCheck(cpu.pc + intIn);
            cpu.pc += intIn;
            break;
        case 133:
            val32 = readMem(4, cpu.sp - 4, 2);
            memCheck(val32);
            cpu.pc = val32;
            break;
        case 134:
            val32 = readMem(4, cpu.sp - 4, 2);
            if(val32 == 0){
                cpu.pc = intIn;
            }
            else{
                cpu.pc += 4;
            }
            break;
        case 135:
            val32 = readMem(4, cpu.sp - 4, 2);
            if(val32 != 0){
                cpu.pc = intIn;
            }
            else{
                cpu.pc += 4;
            }
            break;
        case 136:
            val32 = readMem(4, cpu.sp - 4, 2);
            if(val32 > 0){
                cpu.pc = intIn;
            }
            else{
                cpu.pc += 4;
            }
            break;
        case 137:
            val32 = readMem(4, cpu.sp - 4, 2);
            if(val32 < 0){
                cpu.pc = intIn;
            }
            else{
                cpu.pc += 4;
            }
            break;
        case 138:
            val32 = readMem(4, cpu.sp - 4, 2);
            if(val32 >= 0){
                cpu.pc = intIn;
            }
            else{
                cpu.pc += 4;
            }
            break;
        case 139:
            val32 = readMem(4, cpu.sp - 4, 2);
            if(val32 <= 0){
                cpu.pc = intIn;
            }
            else{
                cpu.pc += 4;
            }
            break;
        case 140:
            writeStackVal(8, cpu.sp, cpu.pc + 4);
            cpu.sp += 4;
            cpu.pc = intIn;
            break;
        case 141:
            cpu.sp -= 4;
            cpu.pc = readMem(8, cpu.sp, 2);
            break;
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
    while(cpu.pc < HEAP_START && cpu.pc >= 0 && cpu.sp < MEM_SIZE && cpu.sp >= STACK_START){
        uint8_t opcode = cpu.mem[cpu.pc];
        int64_t intIn = 0;
        int8_t secondParam = 0;
        double floatIn = 0;
        switch(opcode){
            case 0:
            case 123 ... 130:
            case 132:
                intIn = readMem(1, cpu.pc + 1, 0);
                break;
            case 1:
                intIn = readMem(2, cpu.pc + 1, 0);
                break;
            case 2:
                intIn = readMem(4, cpu.pc + 1, 0);
                break;
            case 3:
                intIn = readMem(8, cpu.pc + 1, 0);
                break;
            case 4:
                floatIn = readMemFloat(4, cpu.pc + 1, 0);
                break;
            case 5:
                floatIn = readMemDouble(8, cpu.pc + 1, 0);
                break;
            case 6 ... 11:
            case 25 ... 30:
            case 131:
            case 134 ... 140:
                intIn = readMem(3, cpu.pc + 1, 0);
                printf("intin: %ld\n", intIn);
                break;
            case 12:
            case 31:
                secondParam = readMem(1, cpu.pc + 4, 0);
                break;
            case 142:
                exit(0);
        }
        // printf("ITERATION START\n");
        // printf("opcode: %d\n", opcode);
        // printf("intIn: %ld\n", intIn);
        // printf("secondParam: %d\n", secondParam);
        // printf("floatIn: %f\n", floatIn);
        interpret(opcode, intIn, floatIn, secondParam);
    }
    EXIT_ERROR();
}