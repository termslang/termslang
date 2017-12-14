/**
* Copyright (c) 2017
* Mikhail Baynov m.baynov@gmail.com
* Distributed under the GNU GPL v2
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef unsigned char uint8_t;



#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


void* alloc_big_arr(size_t size) {
  char *out;
  do {
    out = malloc(size);
    size /= 2;
  } while(!out);
  out[0] = 0;
  return (void*)out;
}
#define NEW_WORD        calloc(1, 256)
#define NEW_LINE        calloc(1, 4064)
#define NEW_ARR         alloc_big_arr(10000)
#define NEW_BIG_ARR     alloc_big_arr(100000)
#define NEW_HUGE_ARR    alloc_big_arr(1000000)
#define FREE(p)              do { {p ? free(p) : 1;} p = NULL; } while(0)
#define FREE_ARR(arr)        do { size_t _argu=0; while (arr[_argu]) \
  { free(arr[_argu]); arr[_argu] = NULL; ++_argu; }; free(arr); arr = NULL; } while(0)

#define REALLOC_STR(p)       do {p = realloc(p, (strlen(p)+1)*sizeof(char));} while(0)
#define REALLOC_ARR(arr)     do { size_t _argu=0; while (arr[_argu]) \
  { arr[_argu] = realloc(arr[_argu], (strlen(arr[_argu])+1)*sizeof(char)); ++_argu; };\
    arr = realloc(arr, (_argu+1)*sizeof(char*)); arr[_argu]=0;} while(0)
#define REALLOC_STRPTR_ARR(arr)     do { size_t _argu=0; while (arr[_argu]) \
  { ++_argu; };  arr = realloc(arr, (_argu+1)*sizeof(char*)); arr[_argu]=0;} while(0)




struct tag {
  char name[256];
  size_t offset;
};
#define REALLOC_TAG_ARR(arr)     do { size_t _argu=1; while (arr[_argu].name[0]) \
  {++_argu;} arr = realloc(arr, (_argu+1)*sizeof(struct tag)); } while(0)



#ifndef DEBUG_PRINT
  #define DEBUG_PRINT 1
#define print printf
#endif
void print_arr(const char *str, char **arr) {
  if (!DEBUG_PRINT) return;
  if (!arr) {
    printf("(null)\n");
    return;
  }
  if (str) {printf("%s:\n", str);}
  for (size_t y = 0; arr[y]; ++y) {
    printf("[%03zu] {%s}\n", y, arr[y]);
  }
}


#define EMASM_CALLNAME_FILE "emasm.names.txt"
#define EMASM_OUTPUT_FILE "emasm.out"
#define EMASM_REFJUMP_TAG "REFJUMP_TAG"


#include "emasm.h"
#include "emasm_keccak.c"
#include "emasm_common.c"
#include "emasm_array.c"
#include "emasm_disasm.c"
#include "emasm_compiler.c"

#ifndef MAIN_FUNC
#define MAIN_FUNC
int main(int argc, char *argv[]) {


  const char *in = argv[1];
  const char *filename_arg = argv[2];
  bool testing_disasm = false;
  //check args
  if (in) {
    if (!strcmp(argv[1], "-h") || !strcmp(argv[1],"--help")) {
      printf("First argument must be filename (EVM asm file to compile) or opcode hex string (to disassemble)\n");
      printf("  -t <bytecode>  or --test <bytecode>   to test disassembler\n");
      return 0;
    }
    else if (!strcmp(argv[1], "-t") || !strcmp(argv[1],"--test")) {
      testing_disasm = true;
      emasm_disasm(filename_arg);
      in = EMASM_OUTPUT_FILE;
    }
    if (filename_arg) {
      if (!strcmp(argv[2], "-t") || !strcmp(argv[2],"--test")) {
        testing_disasm = true;
        filename_arg = argv[1];
        emasm_disasm(filename_arg);
        in = EMASM_OUTPUT_FILE;
      }
    }
  }
  else {
    printf("Argument missing: filename(compile) or hex string(disassemble)\n");
    return 0;
  }

  if (testing_disasm) {
    char *file_out;
    FILE *file = fopen(filename_arg, "r");
    if (file) {
      fclose(file);
      file_out = emasm_compile(filename_arg);
      emasm_disasm(file_out);
    } else {
      if (filename_arg[0] == '0' && (filename_arg[1] == 'x' || filename_arg[1] == 'X')) {
        file_out = strdup(filename_arg + 2);
      } else file_out = strdup(filename_arg);
    }
    char *emasm_out = emasm_compile(in);
    // printf("in           %s\n", in);
    // printf("filename_arg %s\n", filename_arg);
    // printf("emasm_out %s\n", emasm_out);
    // printf("file_out  %s\n", file_out);
    if (!strcmp(emasm_out, file_out)) {
      printf("DISASSEMBLER TEST SUCCEEDED\n");
      FREE(emasm_out);
      FREE(file_out);
    } else printf("TEST FAILED:    %s\n", filename_arg);
  } else emasm_compile(in);
  return 0;
}
#endif /* MAIN_FUNC */
