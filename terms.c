/**
* Copyright (c) 2017
* Mikhail Baynov m.baynov@gmail.com
* Distributed under the GNU GPL v2

Limitations:
0xFFFF max init section address length
0xFFFF max contract length
0xFF max variables count

gcc terms.c -Wno-incompatible-pointer-types-discards-qualifiers -o terms
*/
#ifndef MAIN_FUNC
#define MAIN_FUNC

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>

typedef unsigned char uint8_t;
#define _GNU_SOURCE

#include "emasm.c"


//TODO: store abi inside contract
//TODO synonyms table
//TODO optional words in macros == get take    if ^1 [is] zero
//TODO  ; == equal equals =
//      ; == get take

//TODO Check return before function(), warn if absent

//TODO  more beautiful way for "related record"

//TODO ?jumps only inside scope


//TODO remove "constant" before functions, detect SSTORE automatically

//TODO hex addition, mul, sub, div
//TODO EMASM: remove duplicate refjump jumpdest
//TODO EMASM:  CODECOPY const strings data to beginning of file   CONST_STRING
//TODO EMASM:  CODECOPY const vars         to beginning of file   CONST_VAR

//TODO hash method signatures to call other contracts
//TODO add 0 jump JUMPDEST JUMP

//TODO EMASM: disassemble SSTORESEQ, MSTORESEQ
//TODO add payable fallback() by default
//TODO 1+2*3 -> 1 + 2 * 3   remove spaces in conditions



#define VERSION_CUSTOM_REVISIONS
#define VERSION_MASTER_SIGN





#define TERMS_COMPILER_OUTPUT_FILE "terms_compiler.out"
#define TERMS_VARIABLE_ARRAYS_OFFSET 0x010000
#define TERMS_EVM_CONSTANT_NAME "CONSTANT"
#define TERMS_VARIABLE_SEQUENCE_MODIFIER " SEQUENCE"



char* hex_from_string(const char *str) {
  if (!str) return 0;
  char *out = NEW_LINE;
  out[0] = '0';
  out[1] = 'x';
  char *p = out + 2;
  size_t ii=0;
  while (str[ii]) {
    sprintf(p, "%02x", str[ii]);
    p += 2;
    ++ii;
  }
  p[0] = 0;
  REALLOC_STR(out);
  return out;
}

char* crop_filename_extension(const char* filename) {
  char *out = strdup(filename);
  size_t len = strlen(out);
  size_t i=len;
  while (i>0) {
    if (out[i] == '.') {
      out[i] = 0;
      break;
    }
    --i;
  }
  return out;
}


int is_variable(const char *word) {
  if (!word) return 0;
  if ((word[0] >= 'A' && word[0] <= 'Z') || word[0] == '_' || word[0] == '#') return 1;
  return 0;
}

size_t words_from_sentence_line(const char *line, char ***words_arr) {   //returns word_count
  if (!line) return 0;
  size_t words_count = 0;
  size_t i=0;
  while (i < strlen(line)) {
    if (i+1 < strlen(line)) {
    if (line[i] == '/' && line[i+1] == '/') {
        *(*words_arr + words_count) = 0;
        return words_count;
      }
    }
    char *word = NEW_LINE;
    size_t inside_braces = 0;
    bool inside_commas = 0;
    size_t y=0;
    while (line[i] != '\n' && i < strlen(line)) {
      if (line[i] == '(') ++inside_braces;
      if (line[i] == ')') --inside_braces;
      if (line[i] == '\"') inside_commas = !inside_commas;
      if (inside_braces || inside_commas) {
        word[y] = line[i];
        ++y;
      }
      else {
        if (line[i] == ' ') break;
        if (line[i] != '\n') {
          word[y] = line[i];
          ++y;
        }
      }
      ++i;
    }
    word[i] = 0;
    if (y) {
      *(*words_arr + words_count) = word;
      size_t is_comment_line = !strcmp(**words_arr, ";");
      if (words_count > 1 + is_comment_line && strcmp(**words_arr, ">>")) {
        char *prev_word = *(*words_arr + words_count - 1);
        if ((prev_word[strlen(prev_word)-1] != ',') &&
            is_variable(prev_word) && is_variable(word)) {
          sprintf(*(*words_arr + words_count - 1), "%s %s", prev_word, word);
          *(*words_arr + words_count) = 0;
          --words_count;
        }
      }
      ++words_count;
    }
    ++i;
  }
  *(*words_arr + words_count) = 0;
  return words_count;
}


int string_is_method_name(const char *str) {
  if (!str) return 0;
  size_t len = strlen(str);
  bool is_open = 0;
  bool is_closed = 0;
  size_t closer_location = 0;
  size_t y=0;
  while (y < len) {
    if (is_open) {
      if (str[y] == '(') return 0;
      if (str[y] == ')') {
        is_closed = true;
        ++y;
        break;
      }
    } else if (str[y] == ' ' || str[y] == '/' || str[y] == ';' || str[y] == '.') return 0;
    if (str[y] == '(') is_open = true;
    ++y;
  }
  if (is_closed) {
    while (y < len) {
      if (!(str[y] == ' ' || str[y] == '/' || str[y] == ';' || str[y] == '.' || str[y] == '\n')) return 0;
      ++y;
    }
    return 1;
  }
  return 0;
}


// event Transfer(address indexed _from, address indexed _to, uint256 _value);
int parse_method_name(const char *string, char** signature, char**event_name, char*** type_args,
  char*** argument_names, size_t** event_indexed1_unindexed2_arr) {

  if (!string) return 0;
  char *signature_p = *signature;
  char *str = NEW_LINE;
  strcpy(str, string);
  size_t len = strlen(string);
  bool is_open = 0;
  bool is_closed = 0;
  size_t closer_location = 0;
  size_t y=0;
  char **args = NEW_ARR;  //raw comma separated args
  size_t args_count = 0;
  char *buffer;
  size_t buffer_count = 0;
  while (y < len) {
    if (is_open) {
      if (str[y] == '(') {
        FREE(str);
        return 0;
      }
      if (str[y] == ')') {
        buffer[buffer_count] = 0;
        args[args_count] = buffer;
        buffer = NEW_LINE;
        buffer_count = 0;
        ++args_count;
        args[args_count] = 0;
        ++y;
        str[y] = 0;
        is_closed = true;
        break;
      }
      if (str[y] == ',') {
        buffer[buffer_count] = 0;
        args[args_count] = buffer;
        buffer = NEW_LINE;
        buffer_count = 0;
        ++args_count;
      }
      buffer[buffer_count] = str[y];
      ++buffer_count;
    }
    else {
      if (str[y] != ' ' && str[y] != '\t') {
        *signature_p = str[y];
        ++signature_p;
      }
    }
    if (str[y] == '(') {
      is_open = true;
      buffer = NEW_LINE;
      buffer_count = 0;
      *event_name = strdup(*signature);
      *(*event_name + strlen(*event_name) - 1) = 0;  //delete '('
    }
    ++y;
  }
  if (is_closed) {
    while (y < len) {
      if (!(str[y] == ' ' || str[y] == '\t' || str[y] == '/' || str[y] == ';' || str[y] == '.' || str[y] == '\n' || str[y] == 0)) {
        FREE(str);
        return 0;
      }
      ++y;
    }
    FREE(str);
    // Inside braces: parse raw comma separated args
    size_t ii=0;
    args_count=0;
    while (args[ii]) {
      size_t i=0;
      char *type_arg = NEW_LINE;
      char *second_arg = NEW_LINE;
      char *third_arg = NEW_LINE;
      size_t letter_count=0;
      while (i < strlen(args[ii]) && (args[ii][i] == ' ' || args[ii][i] == '\t' || args[ii][i] == ',')) ++i;
      while (i < strlen(args[ii]) && (args[ii][i] != ' ' && args[ii][i] != '\t')) {
        type_arg[letter_count] = args[ii][i];
        ++letter_count;
        ++i;
      };
      type_arg[letter_count] = 0;
      REALLOC_STR(type_arg);

      letter_count=0;
      while (i < strlen(args[ii]) && (args[ii][i] == ' ' || args[ii][i] == '\t')) ++i;
      while (i < strlen(args[ii]) && (args[ii][i] != ' ' && args[ii][i] != '\t')) {
        second_arg[letter_count] = args[ii][i];
        ++letter_count;
        ++i;
      };
      second_arg[letter_count] = 0;
      REALLOC_STR(second_arg);

      letter_count=0;
      while (i < strlen(args[ii]) && (args[ii][i] == ' ' || args[ii][i] == '\t')) ++i;
      while (i < strlen(args[ii]) && (args[ii][i] != ' ' && args[ii][i] != '\t')) {
        third_arg[letter_count] = args[ii][i];
        ++letter_count;
        ++i;
      };
      third_arg[letter_count] = 0;
      REALLOC_STR(third_arg);

      // Add strings to arrays
      *(*type_args + args_count) = type_arg;
      if (third_arg[0]) {
        *(*argument_names + ii) = third_arg;
        if (*event_indexed1_unindexed2_arr)  *(*event_indexed1_unindexed2_arr + ii) = 1;  //indexed

      }
      else {
        *(*argument_names + ii) = second_arg;
        if (*event_indexed1_unindexed2_arr)  *(*event_indexed1_unindexed2_arr + ii) = 2;  //unindexed
      }
      if (!second_arg[0] && !third_arg[0]) {
        FREE(*(*type_args + args_count));
        *(*type_args + args_count) = 0;
        FREE(*(*argument_names + ii));
        *(*argument_names + ii) = 0;
      }
      ++args_count;
      ++ii;
    }
    *(*argument_names + ii) = 0;
    *(*type_args + args_count) = 0;

// print_arr("type_args", *type_args);
// print_arr("argument_names", *argument_names);
// getchar();
    // Add type_args to method signature
    ii=0;
    while (*(*type_args + ii)) {
      strcpy(signature_p, *(*type_args + ii));
      signature_p += strlen(signature_p);
      ++ii;
      if (*(*type_args + ii)) {
        *signature_p = ',';
        ++signature_p;
      }
    }
    *signature_p = ')';
    return 1;
  }
  FREE(str);
  return 0;
}


char* revision_trim_filename(const char *filename) {
  if (!filename) return NULL;
  if (!memcmp(filename, "terms.", 6)) {
    char *out = NEW_WORD;
    strcpy(out, filename + 6);
    size_t i=0;
    if (string_ends_with(out, ".txt")) {
      out[strlen(out)-4]=0;
      return out;
    }
  }
  return 0;
}


void parse_macro_argument(const char *orig, char **var_name_str, char **var_index_str) {
  bool inside_quotes = 0;
  bool is_array_member = false;
  size_t count = 0;
  size_t z=0;
  while(orig[z]) {
    if (orig[z] == '\"') {
      inside_quotes = !inside_quotes;
    }
    if (inside_quotes) {
      *(*var_name_str + z) = orig[z];
    }
    else {
      if (is_array_member) {
        *(*var_index_str + count) = orig[z];
        ++count;
      }
      else {
        *(*var_name_str + z) = orig[z];
      }
      if (orig[z] == '#') {
        *(*var_name_str + z) = 0;
        size_t zz = z - 1;
        while (zz) {
          if (*(*var_name_str + zz) == ' ') {
            *(*var_name_str + zz) = 0;
            --zz;
            continue;
          } else break;
        }
        is_array_member = true;
      }
    }
    ++z;
  }
  *(*var_name_str + z) = 0;

  // printf("{%s} : {%s}  +  {%s}\n", orig, *var_name_str, *var_index_str);
  // getchar();
}



int sentence_matches_macro(const char **words_arr, const char **macro_arr) {
  if (!words_arr || !macro_arr) return 0;
  size_t i=0;
  while (1) {
    char *str1 = macro_arr[i];
    char *str2 = words_arr[i];
    if ((!str1 && str2) || (!str2 && str1)) {
      return 0;
    }
    if (!str1 && !str2) {
      return 1;
    }
    if (str1[0] == '^' && str2) {
      if ( !string_is_method_name(str2) && !is_variable(str2) &&
       !(str2[0] >= '0' && str2[0] <= '9') && !(str2[0] == '\"') ) {
         if (!strcasecmp(str1, str2)) {
           ++i;
           continue;
         } else return 0;
      }
      ++i;
      continue;
    }
    if (!strcasecmp(str1, str2)) {
      ++i;
      continue;
    } else return 0;
  }
  return 0;
}


void form_macros_arrays_for_revision(const char *terms_revision,
  char ***constants, char ***constants_bodies,
  char ****macros_head_arr, char ****macros_body_arr) {
    if (!terms_revision) return;

    char *filename;
    asprintf(&filename, "terms.%s.txt", terms_revision);
    print("Opening %s\n", filename);
    char *line = NEW_LINE;
    FILE *file = fopen(filename,"r");
    if (file==NULL) {
        printf("ERROR: No valid terms revision file found\n\n");
        exit(1);
    }
    fseek(file, 0, SEEK_SET);
    size_t constants_count = 0;
    size_t macros_count = 0;
    size_t body_count = 0;
    char **body_arr = NEW_ARR;
    while (fgets(line, 4096, file)) {
      char **head_arr = NEW_ARR;
      if (!line) continue;
      if (strlen(line) < 2) continue;
      // printf("%s\n", line);
      // getchar();

      if (line[0] == '>' && line[1] == '>') {
        char **line_arr = NEW_ARR;
        size_t words_count = words_from_sentence_line(line, &line_arr);
        REALLOC_ARR(line_arr);
        if (words_count > 1) {
          if (line_arr[1]) {
            asprintf(*constants + constants_count, "%s %s", line_arr[1], TERMS_EVM_CONSTANT_NAME);
            asprintf(*constants_bodies + constants_count, "%s", line_arr[2]);
            ++constants_count;
          }
        }
        FREE(line_arr);
      }
      if (line[0] == '#' && line[1] == '#') {
        if (macros_count) {
          body_arr[body_count] = 0;
          *(*macros_body_arr + macros_count - 1) = body_arr;
          body_arr = NEW_ARR;
          body_count = 0;
        }
        size_t words_count = words_from_sentence_line(line, &head_arr);
        REALLOC_ARR(head_arr);
        if (words_count > 1) {
          if (head_arr[0]) {
            *(*macros_head_arr + macros_count) = head_arr + 1;  //##
            ++macros_count;
          }
        }
      }
      else {
        if (macros_count && line) {
          char *line1 = strdup(line);
          size_t i = strlen(line1);
          if (strlen(line1) > 1) {
            do {
              --i;
            } while (i > 0 && (line1[i] == '\n' || line1[i] == '\t' || line1[i] == ' '));
            line1[i+1] = 0;
            body_arr[body_count] = line1;
            ++body_count;
          }
        }
      }
    }
  *(*macros_head_arr + macros_count) = 0;
  *(*macros_body_arr + macros_count) = 0;
  *(*constants + constants_count) = 0;
  *(*constants_bodies + constants_count) = 0;

  FREE(filename);
  FREE(line);
  FREE(body_arr);
}


// #define DEBUG_PRINT 0
// #undef VERSION_CUSTOM_REVISIONS
// #undef VERSION_MASTER_SIGN
// #define print fake_printf               //debug print off
// int fake_printf( const char * format, ... ) { return 0; }


#include "terms_expressions.c"
#include "terms_compiler_abi.c"
#include "terms_compiler.c"





int main(int argc, char *argv[]) {

  const char *in = argv[1];
  const char *in2 = argv[2];
  bool testing_disasm = false;
  //check args
  if (in) {
    if (!strcmp(argv[1], "-h") || !strcmp(argv[1],"--help")) {
      printf("First argument must be filename (.terms file to compile) or opcode hex string (to decompile)\n");
      return 0;
    }
  }
  else {
    printf("Argument missing: filename(compile) or hex string(decompile)\n");
    return 0;
  }
  FILE *file = fopen(in,"r");
  if (file==NULL) {
      printf("ERROR: File not found: %s\n", in);
      return 0;
  }

  char **errors_arr = NEW_ARR;
  terms_compile(crop_filename_extension(in), file, &errors_arr);
  fclose(file);
  if (errors_arr[0]) {
    print_arr("ERROR: INSTRUCTIONS NOT RECOGNIZED", errors_arr);
    exit(1);
  }
  return 0;
}
#endif /* MAIN_FUNC */
