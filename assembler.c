#include "assembler.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h> 
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <regex.h>
#include <float.h>

int lines;

int main(int argc, char** argv){
    FILE* fp = fopen(argv[1], "r\0");
    if(fp == NULL){
        fprintf(stderr, "%s", "Invalid slinker filepath\n");
        exit(1);
    }
    char k = argv[1][strlen(argv[1]) - 1];
    char l = argv[1][strlen(argv[1]) - 2];
    char s = argv[1][strlen(argv[1]) - 3];
    char dot = argv[1][strlen(argv[1]) - 4];
    if(k != 'k' || l != 'l' || dot != '.' || s != 's'){
        fprintf(stderr, "%s", "Invalid slinker filepath\n");
        exit(1);
    }
    firstPass(fp);
    // displayList();
    char* outFile = (char*) calloc(sizeof(char) * strlen(argv[1]) + 2, 1);
    for(int i = 0; i < strlen(argv[1]); i++){
        outFile[i] = argv[1][i];
    }
    outFile[strlen(argv[1])] = 'o';
    outFile[strlen(argv[1]) + 1] = '\0';
    read(fp, outFile);
    exit(0);
}

void insert(char* name, int address, int nameLength){
    if(listHead != NULL){
        addr* lk = (addr*) malloc(sizeof(addr));
        lk -> address = address;
        lk -> name = (char*) malloc(sizeof(char) * (nameLength + 1));
        strncpy(lk -> name, name, nameLength);
        lk -> name[nameLength] = '\0';
        lk -> next = listHead;
        listHead = lk;
    }
    else{
        listHead = (addr*) malloc (sizeof(addr));
        listHead -> address = address;
        listHead -> name = (char*) malloc(sizeof(char) * (nameLength + 1));
        strcpy(listHead -> name, name);
        listHead -> next = NULL;
    }
}

int search(char* name, int line){
    addr *temp = listHead;
    while(temp != NULL) {
        if (strcmp(temp -> name, name) == 0) {
            return temp -> address;
        }
        temp=temp->next;
    }
    EXIT_ERROR(line + 1);
}

void freeList(addr* listHead){
    addr* tmp;
    while (listHead != NULL){
        tmp = listHead;
        listHead = listHead->next;
        free(tmp -> name);
        free(tmp);
    }
}

void displayList() {
   addr* temp;
   if (listHead == NULL) {
      printf("List is empty.\n");
      return;
   }
   printf("elements of list are :\n");
   temp = listHead;
   while (temp != NULL) {
      printf("name: %s     address: %d\n", temp->name, temp->address);
      temp = temp->next;
   }
}

void EXIT_ERROR(int line){
    fprintf(stderr, "%s%d\n", "Error on line ", line);
    exit(1);
}

void byteCheck(int num, int line){
    if(num < -128 || num > 127){
        EXIT_ERROR(line + 1);
    }
}

void shortCheck(int num, int line){
    if(num < -128 || num > 127){
        EXIT_ERROR(line + 1);
    }
}

void intCheck(int num, int line){
    if(num < -128 || num > 127){
        EXIT_ERROR(line + 1);
    }
}

void longCheck(long long num, int line){
    if(num < LONG_MIN || num > LONG_MAX){
        EXIT_ERROR(line + 1);
    }
}

void twentyFourCheck(long num, int line){
    if(num < 0 || num > 16777215){
        EXIT_ERROR(line + 1);
    }
}

void checkValid(int cmdNum, char* cmdParams, int emptyParams, int line){
    switch(cmdNum){
        regex_t regex;
        char* ptr;
        case 13 ... 24:
        case 32 ... 122:
        case 133:
        case 141 ... 142:
            if(emptyParams == 0){
                EXIT_ERROR(line + 1);
            }
            break;
        //SHFT, PUSHB, JRPC
        case 0:
        case 123 ... 130:
        case 132:
            regcomp(&regex, VALID_PARAMETERS[1], REG_EXTENDED);
            if(regexec(&regex, cmdParams, 0, NULL, 0) == 0){
                regfree(&regex);
                int num = atol(cmdParams);
                byteCheck(num, line);
            }
            else{
                regfree(&regex);
                EXIT_ERROR(line + 1);
            }
            break;
        case 1:
            regcomp(&regex, VALID_PARAMETERS[1], REG_EXTENDED);
            if(regexec(&regex, cmdParams, 0, NULL, 0) == 0){
                regfree(&regex);
                int num = atol(cmdParams);
                shortCheck(num, line);
            }
            else{
                regfree(&regex);
                EXIT_ERROR(line + 1);
            }
            break;
        case 2:
            regcomp(&regex, VALID_PARAMETERS[1], REG_EXTENDED);
            if(regexec(&regex, cmdParams, 0, NULL, 0) == 0){
                regfree(&regex);
                int num = atol(cmdParams);
                intCheck(num, line);
            }
            else{
                regfree(&regex);
                EXIT_ERROR(line + 1);
            }
            break;
            
        case 3:
            regcomp(&regex, VALID_PARAMETERS[1], REG_EXTENDED);
            if(regexec(&regex, cmdParams, 0, NULL, 0) == 0){
                regfree(&regex);
                long long num = atoll(cmdParams);
                longCheck(num, line);
            }
            else{
                regfree(&regex);
                EXIT_ERROR(line + 1);
            }
            break;
        case 4: ;
            float numFloat = strtof(cmdParams, &ptr);
            if(ptr != "\0"){
                EXIT_ERROR(line + 1);
            }
            break;
        case 5: ;
            double numDouble = strtod(cmdParams, &ptr);
            if(ptr != "\0"){
                EXIT_ERROR(line + 1);
            }
            break;
        //POP, PUSH, JMP, JZ...etc, CALL memory cases
        case 6 ... 11:
        case 25 ... 30:
        case 131:
        case 134 ... 140:
            regcomp(&regex, VALID_PARAMETERS[2], REG_EXTENDED);
            if(regexec(&regex, cmdParams, 0, NULL, 0) == 0){
                regfree(&regex);
                if(strchr(cmdParams, '+') != NULL){
                    char* add = strchr(cmdParams, '+');
                    char* addr = strtok(cmdParams, "+");
                    long addNum = atol(add);
                    int addrNum = search(addr, line);
                    twentyFourCheck(addrNum + addNum, line);
                }
                else if(strchr(cmdParams, '-') != NULL){
                    char* add = strchr(cmdParams, '-');
                    char* addr = strtok(cmdParams, "-");
                    long addNum = atol(add);
                    int addrNum = search(addr, line);
                    twentyFourCheck(addrNum + addNum, line);
                }
                else{
                    long addrNum = atol(cmdParams);
                    twentyFourCheck(addrNum, line);
                }
            }
            else{
                regfree(&regex);
                EXIT_ERROR(line + 1);
            }
            break;
        //PUSHMM, POPMM
        case 12:
        case 31:
            regcomp(&regex, VALID_PARAMETERS[3], REG_EXTENDED);
            if(regexec(&regex, cmdParams, 0, NULL, 0) == 0){
                regfree(&regex);
                char* param1 = (char*) calloc (sizeof(char) * strlen(cmdParams), 1);
                int i;
                for(i = 0; i < strlen(cmdParams) && cmdParams[i] != ','; i++){
                    param1[i] = cmdParams[i];
                }
                char* param2 = (char*) calloc (sizeof(char) * strlen(cmdParams), 1);
                i+=2;
                int j = 0;
                for(; i < strlen(cmdParams); i++){
                    param2[j++] = cmdParams[i];
                }
                if(strchr(param1, '+') != NULL){
                    char* add = strchr(param1, '+');
                    char* addr = strtok(param1, "+");
                    long addNum = atol(add);
                    int addrNum = search(addr, line);
                    twentyFourCheck(addrNum + addNum, line);
                }
                else if(strchr(param1, '-') != NULL){
                    char* add = strchr(param1, '-');
                    char* addr = strtok(param1, "-");
                    long addNum = atol(add);
                    int addrNum = search(addr, line);
                    twentyFourCheck(addrNum + addNum, line);
                }
                else{
                    long addrNum = atol(cmdParams);
                    twentyFourCheck(addrNum, line);
                }
                int num = atol(param2);
                byteCheck(num, line);
            }
            else{
                regfree(&regex);
                EXIT_ERROR(line + 1);
            }
            break;
        default:
            EXIT_ERROR(line + 1);
            break;
    }
}

int checkDigits(char* num, int line){
    if(num[0] != '-' && !isdigit(num[0])){
        EXIT_ERROR(line + 1);
    }
    for (int j = 1; j < strlen(num); j++){
        if (!isdigit(num[j])){
            EXIT_ERROR(line + 1);
        }
    }
}

int searchCMD(char* cmdName, int line){
    for(int j = 0; j < 143; j++){
        if(strcmp(VALID_COMMANDS[j], cmdName) == 0){
            return j;
        }
    }
    EXIT_ERROR(line + 1);
}

void firstPass(FILE* fp){
    lines = 0;
    for (char d = getc(fp); d != EOF; d = getc(fp)){
        if (d == '\n'){
            lines++;
        }
    }
    rewind(fp);
    //Data types: byte, ascii, short, int, long, float, double
    //code = 0, data = 1
    int codeOrData = 1;
    int directives = 0;
    int emptyParams = 0;
    int mem = 0;
    for(int i = 0; i < lines; i++){
        char* buffer = (char*) calloc (512 * sizeof(char), 1);
        fgets(buffer, 512, fp);
        buffer[strcspn(buffer, "\n")] = '\0';
        char c = buffer[0];
        if(c == '.'){
            char* buff = strtok(buffer, ".");
            if(strcmp(buff, "code") == 0){
                codeOrData = 0;
            }
            else if(strcmp(buff, "data") == 0){
                codeOrData = 1;
            }
            else if(strcmp(buff, "byte") == 0){
                directives = 0;
            }
            else if(strcmp(buff, "ascii") == 0){
                directives = 1;
            }
            else if(strcmp(buff, "short") == 0){
                directives = 2;
            }
            else if(strcmp(buff, "int") == 0){
                directives = 3;
            }
            else if(strcmp(buff, "long") == 0){
                directives = 4;
            }
            else if(strcmp(buff, "float") == 0){
                directives = 5;
            }
            else if(strcmp(buff, "double") == 0){
                directives = 6;
            }
            else{
                EXIT_ERROR(i + 1);
            }
        }
        else if(c == '\t'){
            char* buff = strtok(buffer, "\t");
            if(codeOrData == 0){
                char* cmdName = (char*) calloc(256 * sizeof(char), 1);
                int j = 0;
                for(j = 0; buff[j] != ' ' && buff[j] != '\n' && buff[j] != '\0' && buff[j] != EOF; j++){
                    cmdName[j] = buff[j];
                }
                int cmdNum = searchCMD(cmdName, i);
                int emptyParams = 1;
                char* cmdParams = (char*) calloc(strlen(buff) * sizeof(char) - strlen(cmdName) * sizeof(char), 1);
                for(int h = j + 1; h < strlen(buff) && buff[h] != '\n' && buff[h] != '\0'; h++){
                    emptyParams = 0;
                    cmdParams[h - j - 1] = buff[h];
                }
                //TODO: fix the checkvalid fucntion
                //TODO: Figure out bounds check on parameters
                checkValid(cmdNum, cmdParams, emptyParams, i);
                if(emptyParams == 0){
                    switch(cmdNum){
                        //PUSH CASES
                        case 0:
                            mem += 2;
                            break;
                        case 1:
                            mem += 3;
                            break;
                        case 2:
                        case 4:
                            mem += 5;
                            break;
                        case 3:
                        case 5:
                            mem += 9;
                            break;
                        case 6 ... 11:
                            mem += 4;
                            break;
                        case 12:
                            mem += 5;
                            break;
                        //POP CASES
                        case 25 ... 30:
                            mem += 4;
                            break;
                        case 31:
                            mem += 5;
                            break;
                        //SHFT CASES
                        case 123 ... 130:
                            mem += 2;
                            break;
                        //JMP CASES
                        case 131:
                            mem += 4;
                            break;
                        case 132:
                            mem += 2;
                            break;
                        case 134 ... 139:
                            mem += 4;
                            break;
                        //CALL CASE
                        case 140:
                            mem += 4;
                            break;
                        default:
                            EXIT_ERROR(i + 1);
                            break;
                    }
                }
                else if (emptyParams == 1){
                    mem++;
                }
                else{
                    EXIT_ERROR(i + 1);
                }
                free(cmdName);
                free(cmdParams);
            }
            //TODO: verify bounds checking for each case
            else if(codeOrData == 1 && directives == 0){
                checkDigits(buff, i);
                int dataVal = atoi(buff);
                if(dataVal < -128 || dataVal > 127){
                    EXIT_ERROR(i + 1);
                }
                mem++;
            }
            else if(codeOrData == 1 && directives == 1){
                //TODO: Verify ascii check
                if(strlen(buffer) != 2){
                    EXIT_ERROR(i + 1);
                }
                mem++;
            }
            else if(codeOrData == 1 && directives == 2){
                checkDigits(buff, i);
                int dataVal = atoi(buff);
                if(dataVal < -32768 || dataVal > 32767){
                    EXIT_ERROR(i + 1);
                }
                mem += 2;
            }
            else if(codeOrData == 1 && directives == 3){
                checkDigits(buff, i);
                int dataVal = atoi(buff);
                if(dataVal < -2147483648 || dataVal > 2147483647){
                    EXIT_ERROR(i + 1);
                }
                mem += 4;
            }
            //TODO: verify bounds checking for 64 bit signed integers
            else if(codeOrData == 1 && directives == 4){
                checkDigits(buff, i);
                int64_t max = 9223372036854775807;
                char *ptr;
                int64_t dataVal = strtoul(buff, &ptr, 10);
                if(dataVal == max && strcmp(buff, "9223372036854775807") != 0){
                    EXIT_ERROR(i + 1);
                }
                mem += 8;
            }
            //TODO: verify bounds for floating point numbers
            else if(codeOrData == 1 && directives == 5){
                char* fptr;
                float value = strtof(buff, &fptr);
                if(fptr[0] != '\0'){
                    EXIT_ERROR(i + 1);
                }
                mem += 4;
            }
            else if(codeOrData == 1 && directives == 6){
                char* dptr;
                double value = strtod(buff, &dptr);
                if(dptr[0] != '\0'){
                    EXIT_ERROR(i + 1);
                }
                mem += 8;
            }
            else{
                EXIT_ERROR(i + 1);
            }
        }
        //TODO: make sure the labels are correctly parsed
        else if(c == ':'){
            char* name = strtok(buffer, ":");
            int found = search(name, -1);
            regex_t regex;
            regcomp(&regex, VALID_PARAMETERS[0], REG_EXTENDED);
            if(strlen(name) > 255 || regexec(&regex, name, 0, NULL, 0) != 0){
                EXIT_ERROR(i + 1);
            }
            regfree(&regex);
            insert(name, mem, strlen(name));
        }
        else if(c != ';'){
            EXIT_ERROR(i + 1);
        }
        free(buffer);
    }
    fseek(fp, -1, SEEK_END);
    if(getc(fp) != '\n'){
        EXIT_ERROR(lines + 1);
    }
}

void read(FILE* fp, char* outFile){
    FILE* out = fopen(outFile, "wb");
    rewind(fp);
    //code, data, byte, ascii, short, int, long, float, double
    int codeOrData = 1;
    int directives = 0;
    int emptyParams = 0;
    int mem = 0;
    for(int i = 0; i < lines; i++){
        char* buffer = (char*) calloc (512 * sizeof(char), 1);
        fgets(buffer, 512, fp);
        buffer[strcspn(buffer, "\n")] = '\0';
        char c = buffer[0];
        if(c == '.'){
            char* buff = strtok(buffer, ".");
            if(strcmp(buff, "code") == 0){
                codeOrData = 0;
            }
            else if(strcmp(buff, "data") == 0){
                codeOrData = 1;
            }
            else if(strcmp(buff, "byte") == 0){
                directives = 0;
            }
            else if(strcmp(buff, "ascii") == 0){
                directives = 1;
            }
            else if(strcmp(buff, "short") == 0){
                directives = 2;
            }
            else if(strcmp(buff, "int") == 0){
                directives = 3;
            }
            else if(strcmp(buff, "long") == 0){
                directives = 4;
            }
            else if(strcmp(buff, "float") == 0){
                directives = 5;
            }
            else if(strcmp(buff, "double") == 0){
                directives = 6;
            }
        }
        else if(c == '\t'){
            char* buff = strtok(buffer, "\t");
            if(codeOrData == 0){
                char* cmdName = (char*) calloc(256 * sizeof(char), 1);
                int j = 0;
                for(j = 0; buff[j] != ' ' && buff[j] != '\n' && buff[j] != '\0' && buff[j] != EOF; j++){
                    cmdName[j] = buff[j];
                }
                uint8_t cmdNum = searchCMD(cmdName, i);
                int emptyParams = 1;
                char* cmdParams = (char*) calloc(strlen(buff) * sizeof(char) - strlen(cmdName) * sizeof(char), 1);
                for(int h = j + 1; h < strlen(buff) && buff[h] != '\n' && buff[h] != '\0'; h++){
                    emptyParams = 0;
                    cmdParams[h - j - 1] = buff[h];
                }
                fwrite(&cmdNum, sizeof(cmdNum), 1, out);
                if(emptyParams == 0){
                    splitter(cmdParams, cmdNum, out);
                }
                free(cmdName);
                free(cmdParams);
            }
            else if((codeOrData == 1 && directives == 0) || (codeOrData == 1 && directives == 1)){
                int8_t byteNum = atoi(buff);
                fwrite(&byteNum, sizeof(byteNum), 1, out);
            }
            else if(codeOrData == 1 && directives == 2){
                int16_t shortNum = atoi(buff);
                for(int i = 0; i < 2; i++){
                    int8_t byteNum = shortNum >> (i * 8) & 0xff;
                    fwrite(&byteNum, sizeof(byteNum), 1, out);
                }
            }
            else if(codeOrData == 1 && directives == 3){
                int32_t intNum = atoi(buff);
                for(int i = 0; i < 4; i++){
                    int8_t byteNum = intNum >> (i * 8) & 0xff;
                    fwrite(&byteNum, sizeof(byteNum), 1, out);
                }
            }
            else if(codeOrData == 1 && directives == 4){
                int64_t longNum = atoi(buff);
                for(int i = 0; i < 8; i++){
                    int8_t byteNum = longNum >> (i * 8) & 0xff;
                    fwrite(&byteNum, sizeof(byteNum), 1, out);
                }
            }
            else if(codeOrData == 1 && directives == 5){
                float floatNum = atof(buff);
                uint32_t asInt = *((uint32_t*)&floatNum);
                for(int i = 0; i < 4; i++){
                    int8_t byteNum = asInt >> (i * 8) & 0xff;
                    fwrite(&byteNum, sizeof(byteNum), 1, out);
                }
            }
            else if(codeOrData == 1 && directives == 6){
                double doubleNum = atof(buff);
                uint64_t asLongInt = *((uint64_t*)&doubleNum);
                for(int i = 0; i < 4; i++){
                    int8_t byteNum = asLongInt >> (i * 8) & 0xff;
                    fwrite(&byteNum, sizeof(byteNum), 1, out);
                }
            }
        }
        free(buffer);
    }
}

void splitter(char* cmdParams, uint8_t cmdNum, FILE* out){
    //TODO: Figure out splitter
    //TODO: Handle commands with params
    switch(cmdNum){
        case 0:
        case 123 ... 130:
        case 132: ;
            int8_t byteNum = atoi(cmdParams);
            fwrite(&byteNum, sizeof(byteNum), 1, out);
            break;
        case 1: ;
            int16_t shortNum = atoi(cmdParams);
            for(int i = 0; i < 2; i++){
                int8_t byteNum = shortNum >> (i * 8) & 0xff;
                fwrite(&byteNum, sizeof(byteNum), 1, out);
            }
            break;
        case 2: ;
            int32_t intNum = atoi(cmdParams);
            for(int i = 0; i < 4; i++){
                int8_t byteNum = intNum >> (i * 8) & 0xff;
                fwrite(&byteNum, sizeof(byteNum), 1, out);
            }
            break;
        case 3: ;
            int64_t longNum = atoi(cmdParams);
            for(int i = 0; i < 8; i++){
                int8_t byteNum = longNum >> (i * 8) & 0xff;
                fwrite(&byteNum, sizeof(byteNum), 1, out);
            }
            break;
        case 4: ;
            float floatNum = atof(cmdParams);
            uint32_t asInt = *((uint32_t*)&floatNum);
            for(int i = 0; i < 4; i++){
                int8_t byteNum = asInt >> (i * 8) & 0xff;
                fwrite(&byteNum, sizeof(byteNum), 1, out);
            }
            break;
        case 5: ;
            double doubleNum = atof(cmdParams);
            uint64_t asLongInt = *((uint64_t*)&doubleNum);
            for(int i = 0; i < 4; i++){
                int8_t byteNum = asLongInt >> (i * 8) & 0xff;
                fwrite(&byteNum, sizeof(byteNum), 1, out);
            }
            break;
        case 6 ... 11:
        case 134 ... 140: ;
            uint32_t address = 0;
            if(strchr(cmdParams, '+') != NULL){
                char* add = strchr(cmdParams, '+');
                char* addr = strtok(cmdParams, "+");
                long addNum = atol(add);
                int addrNum = search(addr, -1);
                address = addrNum + addNum;
            }
            else if(strchr(cmdParams, '-') != NULL){
                char* add = strchr(cmdParams, '-');
                char* addr = strtok(cmdParams, "-");
                long addNum = atol(add);
                int addrNum = search(addr, -1);
                address = addrNum + addNum;
            }
            else{
                char* ptr;
                address = strtoul(cmdParams, &ptr, 10);
            }
            for(int i = 0; i < 3; i++){
                int8_t byteNum = address >> (i * 8) & 0xff;
                fwrite(&byteNum, sizeof(byteNum), 1, out);
            }
            break;
        case 12: ;
            char* param1 = (char*) calloc (sizeof(char) * strlen(cmdParams), 1);
            int i;
            for(i = 0; i < strlen(cmdParams) && cmdParams[i] != ','; i++){
                param1[i] = cmdParams[i];
            }
            char* param2 = (char*) calloc (sizeof(char) * strlen(cmdParams), 1);
            i+=2;
            int j = 0;
            for(; i < strlen(cmdParams); i++){
                param2[j++] = cmdParams[i];
            }
            uint32_t addressNum = 0;
            if(strchr(param1, '+') != NULL){
                char* add = strchr(param1, '+');
                char* addr = strtok(param1, "+");
                long addNum = atol(add);
                int addrNum = search(addr, -1);
                addressNum = addrNum + addNum;
            }
            else if(strchr(param1, '-') != NULL){
                char* add = strchr(param1, '-');
                char* addr = strtok(param1, "-");
                long addNum = atol(add);
                int addrNum = search(addr, -1);
                addressNum = addrNum + addNum;
            }
            else{
                char* ptr;
                addressNum = strtoul(param1, &ptr, 10);
            }
            for(int i = 0; i < 3; i++){
                int8_t byteNum = addressNum >> (i * 8) & 0xff;
                fwrite(&byteNum, sizeof(byteNum), 1, out);
            }
            int8_t param2Num = atoi(param2);
            fwrite(&param2Num, sizeof(param2Num), 1, out);
            break;
        default:
            EXIT_ERROR(-1);
            break;
    }
}