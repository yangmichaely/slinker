#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

const char* VALID_COMMANDS[143] = {"pushb", "pushs", "pushi", "pushl", "pushf", "pushd", "pushbm", "pushsm", "pushim", "pushlm", "pushfm", 
"pushdm", "pushmm", "dupb", "dups", "dupi", "dupl", "dupf", "dupd", "popb", "pops", "popi", "popl", "popf", "popd", "popbm", "popsm", 
"popim", "poplm", "popfm", "popdm", "popmm", "swapb", "swaps", "swapi", "swapl", "swapf", "swapd", "convbs", "convbi", "convbl", "convbf",
"convbd", "convsb", "convsi", "convsl", "convsf", "convsd", "convib", "convis", "convil", "convif", "convid", "convlb", "convls", "convli",
"bum ass", "convlf", "convld", "convfb", "convfs", "convfi", "convfl", "convfd", "convdb", "convds", "convdi", "convdl", "convdf", "inch",
"inb", "ins", "ini", "inl", "inf", "ind", "outch", "outb", "outs", "outi", "outl", "outf", "outd", "addb", "adds", "addi", "addl", "addf",
"addd", "subb", "subs", "subi", "subl", "subf", "subd", "mulb", "muls", "muli", "mull", "mulf", "muld", "divb", "divs", "divi", "divl", 
"divf", "divd", "and8", "and16", "and32", "and64", "or8", "or16", "or32", "or64", "xor8", "xor16", "xor32", "xor64", "not8", "not16",
"not32", "not64", "shftrb", "shftrs", "shftri", "shftrl", "shftlb", "shftls", "shftli", "shftll", "jmp", "jrpc", "jind", "jz", "jnz", "jgt",
"jlt", "jge", "jle", "call", "ret", "halt"};

const char* VALID_PARAMETERS[36] = {"^[a-zA-Z0-9_]+$", "^[-+]?[0-9]+$", "^[-+]?[0-9]+|:[a-zA-Z0-9_]+\\+[0-9]+|:[a-zA-Z0-9_]+|:[a-zA-Z0-9_]+\\-[0-9]+$",
"^[-+]?[0-9]+|:[a-zA-Z0-9_]+\\+[0-9]+|:[a-zA-Z0-9_]+|:[a-zA-Z0-9_]+\\-[0-9]+, [-+]?[0-9]+$"};

typedef struct addr{
    char* name;
    int address;
    struct addr* next;
} addr;

addr* listHead = NULL;

void insert(char* name, int address, int nameLength);

int search(char* name);

void freeList(addr* head);

void displayList();

void EXIT_ERROR(int line);

void byteCheck(int num, int line);

void shortCheck(int num, int line);

void intCheck(long num, int line);

void longCheck(long long num, int line, char* cmdParams);

int twentyFourCheck(long num);

int searchCMD(char* cmdName, int line);

void firstPass(FILE* fp);

void checkValid(int cmdNum, char* cmdParams, int emptyParams, int line);

void readCode(FILE* fp, FILE* out, char* outfile);

void readData(FILE* fp, FILE* out);

int splitter(char* cmdParams, uint8_t cmdNum, FILE* out);

#endif