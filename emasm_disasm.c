/**
* Copyright (c) 2017
* Mikhail Baynov m.baynov@gmail.com
* Distributed under the GNU GPL v2
*/


char* callname_from_hex(const char *push4_str) {
  FILE *file = fopen(EMASM_CALLNAME_FILE,"r");
  if (file==NULL) {
      printf("ERROR: Can't open %s\n", EMASM_CALLNAME_FILE);
      exit(1);
  }
  char *line = NEW_LINE;
  fseek(file, 0, SEEK_SET);
  bool loader_exists = false;
  while (fgets(line, 4096, file)) {
    size_t i=0;
    while (line[i] != 0 && line[i] != '\n' && line[i] != ';') {
      if (i < strlen(line)-1) {
        if (line[i] == '/' && line[i+1] == '/') break;
      }
      //trim excess symbols in line
      while ( ((line[i] >= '0' && line[i] <= '9') ||
               (line[i] >= 'a' && line[i] <= 'z') ||
               (line[i] >= 'A' && line[i] <= 'Z') ||
                line[i] == '(' || line[i] == ')' || line[i] == ',')
            ) {
        ++i;
      }
      line[i] = 0;
    }
    if (line) {
      char *hash = keccak_hash(line);
      hash[8] = 0;
      if (!strcmp(push4_str, hash)) {
        return line;
      }
    }
  }
  fclose(file);
  FREE(line);
  return 0;
}

int callname_exists(const char *str) {
  FILE *file = fopen(EMASM_CALLNAME_FILE,"r");
  if (file==NULL) {
      printf("WARNING: Can't open %s used for disassembling\n", EMASM_CALLNAME_FILE);
  }
  char *line = NEW_LINE;
  fseek(file, 0, SEEK_SET);
  bool loader_exists = false;
  while (fgets(line, 4096, file)) {
    size_t i=0;
    while (line[i] != 0 && line[i] != '\n' && line[i] != ';') {
      if (i < strlen(line)-1) {
        if (line[i] == '/' && line[i+1] == '/') break;
      }
      //trim excess symbols in line
      while ( ((line[i] >= '0' && line[i] <= '9') ||
               (line[i] >= 'a' && line[i] <= 'z') ||
               (line[i] >= 'A' && line[i] <= 'Z') ||
                line[i] == '(' || line[i] == ')' || line[i] == ',')
            ) {
        ++i;
      }
      line[i] = 0;
    }
    if (line) {
      if (!strcmp(str, line)) {
        return 1;
      }
    }
  }
  fclose(file);
  FREE(line);
  return 0;
}


char* func_at_offset(size_t offset, const struct tag *func_arr) {
  for (size_t i = 1; func_arr[i].name[0] != 0; ++i) {
    if (func_arr[i].offset == offset) {
      return (char*)func_arr[i].name;
    }
  }
  return 0;
}


char* emasm_disasm(const char* in) {
  char *out = NEW_HUGE_ARR;
  char *out_p = out;
  if (strlen(in) == 0 || strlen(in) % 2 > 0) {
     printf("ERROR: Wrong input\n");
     return 0;
  }
  FILE *file=fopen(EMASM_OUTPUT_FILE,"w");
  if (file==0) {
    printf("ERROR: Can't open %s\n", EMASM_OUTPUT_FILE);
    exit(1);
  }
  uint8_t *bytes;
  size_t bytes_len;
  hex2bin(in, &bytes, &bytes_len);

  char *instr = NEW_WORD;
  char *operand = NEW_LINE;
  char **instructions = NEW_HUGE_ARR;
  char **operands = NEW_HUGE_ARR;
  size_t line_count = 0;

  bool init_part = true;
  bool fallback_dispatcher_used = false;
  bool inside_fallback_dispatcher = false;
  struct tag *func_arr = NEW_ARR;
  size_t func_count = 1;
  size_t i=0;
  while (i < bytes_len) {
    //detect init part
    if (i > 4 && init_part) {
      if (bytes[i-4] == CODECOPY &&
        bytes[i-3] == PUSH1 &&
        bytes[i-2] == 0x00 &&
        bytes[i-1] == RETURN) {
        init_part = false;
        bytes += i;
        bytes_len -= i;
        i=0;
      }
    }
    //detect fallback dispatcher
    if (i+8 < bytes_len) {
      if (bytes[i] == PUSH1 &&
        bytes[i+1] == 0xe0 &&
        bytes[i+2] == PUSH1 &&
        bytes[i+3] == 0x02 &&
        bytes[i+4] == EXP &&
        bytes[i+5] == PUSH1 &&
        bytes[i+6] == 0x00 &&
        bytes[i+7] == CALLDATALOAD &&
        bytes[i+8] == DIV) {
          fallback_dispatcher_used = true;
          inside_fallback_dispatcher = true;
      }
    }
    char *opcode = NULL;
    if (bytes[i] == PUSH4) {
      if (i+4 < bytes_len) {
        asprintf(instructions + line_count, "PUSH");
        char push_str[9];
        sprintf(push_str, "%02x%02x%02x%02x", bytes[i+1], bytes[i+2], bytes[i+3], bytes[i+4]);
        char *callname = callname_from_hex(push_str);
        if (callname) {
          asprintf(operands + line_count, "HASH:%s  ;0x%s", callname, push_str);
        } else {
          asprintf(operands + line_count, "0x%s", push_str);
        }

        //if inside fallback dispatcher, add to func_arr
        if (inside_fallback_dispatcher) {
          if (callname) {
            strcpy(func_arr[func_count].name, callname);
          } else {
            sprintf(func_arr[func_count].name, "UNKNOWN_FUNC(%s)", push_str);
          }
          ++func_count;
        }
        ++line_count;
        i += 5;
        continue;
      }
    }  //not PUSH4
    else if (bytes[i] >= PUSH1 && bytes[i] <= PUSH3 &&
       (bytes[i+bytes[i]-PUSH1+2] == JUMP ||
        bytes[i+bytes[i]-PUSH1+2] == JUMPI)) {

        opcode = opcode_from_byte(bytes[i+bytes[i]-PUSH1+2]);
        size_t jump_addr = 0;
        size_t jump_len = bytes[i]-PUSH1+1;
        for (size_t y = 0; y < jump_len; ++y) {
          jump_addr += bytes[i+1+y] << (bytes[i]-PUSH1-y)*8;
        }
        if (inside_fallback_dispatcher) {
          func_arr[func_count-1].offset = jump_addr;
        }

        if (fallback_dispatcher_used) {
          char *name = func_at_offset(jump_addr, func_arr);
          if (name) {
            asprintf(instructions + line_count, "%s", opcode);
            asprintf(operands + line_count, "%s", name);
            ++line_count;
            i += jump_len + 2;
            continue;
          }
        }
        char *tag_modifier = init_part ? "entry" : "tag";
        asprintf(instructions + line_count, "%s", opcode);
        asprintf(operands + line_count, "%s%zx", tag_modifier, jump_addr);
        ++line_count;
        i += jump_len + 2;
        continue;
    }
    else {      //not PUSH4, not JUMP, not JUMPI
      opcode = opcode_from_byte(bytes[i]);
      if (bytes[i] == JUMPDEST) {
        if (inside_fallback_dispatcher) {
          strcpy(func_arr[func_count].name, "()");
          func_arr[func_count].offset = i;
          func_count++;
          inside_fallback_dispatcher = false;
        }
        char *name = func_at_offset(i, func_arr);
        if (name) {
          asprintf(instructions + line_count, "%s", opcode);
          asprintf(operands + line_count, "%s", name);
        } else {
          char *tag_modifier = init_part ? "entry" : "tag";
          asprintf(instructions + line_count, "%s", opcode);
          asprintf(operands + line_count, "%s%zx", tag_modifier, i);
        }
        ++line_count;
        ++i;
        continue;
      }
      else if (bytes[i] >= PUSH1 && bytes[i] <= PUSH32) {
        char *push_hex = NEW_LINE;
        char *push_hex_p = push_hex;
        size_t max = bytes[i] - PUSH1;
        for (size_t tmp = 0; tmp <= max; ++tmp) {
          ++i;
          sprintf(push_hex_p, "%02x", bytes[i]);
          push_hex_p = push_hex + strlen(push_hex);
        }
        asprintf(instructions + line_count, "PUSH");
        asprintf(operands + line_count, "0x%s", push_hex);
        FREE(push_hex);
        ++line_count;
        ++i;
        continue;
      } else {
        asprintf(instructions + line_count, "%s", opcode);
        ++line_count;
      }
    }
    // FREE(opcode);
    ++i;
  }
  REALLOC_ARR(instructions);
//  REALLOC_ARR(operands);
//TODO FIX MEMORY LEAK IN instructions and operands arrays


  // Replacements

  init_part = true;
  for (size_t i = 0; i < line_count; ++i) {
    // replace init
    if (i+8 < line_count) {
      if ( comp(instructions[i], "PUSH")
        && comp(instructions[i+1], "DUP1")
        && comp(instructions[i+2], "PUSH")
        && comp(instructions[i+3], "PUSH")
        && comp(instructions[i+4], "CODECOPY")
        && comp(instructions[i+5], "PUSH")
        && comp(instructions[i+6], "RETURN"))
      {
        init_part = false;
        instructions[i] = strdup("INIT");
        // asprintf(instructions + i, "INIT");
        operands[i] = 0;
        size_t task = i + 6;
        if (comp(instructions[i+7], "STOP")) ++task;
        while (i < task) {
          ++i;
          instructions[i] = 0;
          operands[i] = 0;
        }
      }
    }
    // replace fallback
    size_t n = i + 5 + (func_count - 2)*4;
    if (i+n < line_count) {
      if ( comp(instructions[i], "PUSH") && comp(operands[i], "0xe0")
        && comp(instructions[i+1], "PUSH") && comp(operands[i+1], "0x02")
        && comp(instructions[i+2], "EXP")
        && comp(instructions[i+3], "PUSH") && comp(operands[i+3], "0x00")
        && comp(instructions[i+4], "CALLDATALOAD")
        && comp(instructions[i+5], "DIV"))
      {
        asprintf(instructions + i, "FALLBACK");
        asprintf(operands + i, "()");
        while (i < n) {
          ++i;
          instructions[i] = 0;
          operands[i] = 0;
        }
      }
    }

    //replace BACKJUMP
    if (i+2 < line_count) {
      if ( comp(instructions[i], "PUSH") && comp(operands[i], "0x00")
        && comp(instructions[i+1], "MLOAD")
        && (comp(instructions[i+2], "JUMP") || comp(instructions[i+2], "JUMPI")))
      {
        asprintf(instructions + i, "BACK%s", instructions[i+2]);
        operands[i] = 0;
        size_t task = i + 2;
        while (i < task) {
          ++i;
          instructions[i] = 0;
          operands[i] = 0;
        }
      }
    }

    if (i < line_count) {
      if ( comp(instructions[i], "PUSH")
        && comp(instructions[i+1], "PUSH") && comp(operands[i+1], "0x00")
        && comp(instructions[i+2], "MSTORE")
        && (comp(instructions[i+3], "JUMP") || comp(instructions[i+3], "JUMPI"))
        && comp(instructions[i+4], "JUMPDEST"))
      {
        asprintf(instructions + i, "REF%s", instructions[i+3]);
        operands[i] = strdup(operands[i+3]);
        size_t task = i + 4;
        while (i < task) {
          ++i;
          instructions[i] = 0;
          operands[i] = 0;
        }
      }
    }

  } //for


  // CONDITIONNOT, CONDITIONYES, CONDITIONEND
  char **jumpi_tags = NEW_BIG_ARR;
  char **end_tags = NEW_BIG_ARR;
  char **yes_tags = NEW_BIG_ARR;
  size_t jumpi_tags_count = 0;
  for (size_t i = 0; i < line_count; ++i) {
    if (!instructions[i]) continue;
    if (!strcmp(instructions[i], "JUMPI")) {
      if (-1 == string_index_in_arr(operands[i], jumpi_tags)) {
        asprintf(&jumpi_tags[jumpi_tags_count], "%s", operands[i]);
        ++jumpi_tags_count;
      }
    }
    else if (!strcmp(instructions[i], "JUMP")) {
      size_t n=i+1;
      while (!instructions[n]) ++n;
      if (!strcmp(instructions[n], "JUMPDEST") && string_index_in_arr(operands[n], jumpi_tags) != -1) {
        if (-1 == string_index_in_arr(instructions[i], end_tags)) {
          asprintf(&jumpi_tags[jumpi_tags_count], "%s", operands[i]);
          ++jumpi_tags_count;
        }
      }
    }
  }
  jumpi_tags[jumpi_tags_count] = 0;


  print_arr("jumpi_tags", jumpi_tags);
  print_arr("yes_tags", yes_tags);
  print_arr("end_tags", end_tags);


  for (size_t i = 0; i < line_count; ++i) {
    if (!instructions[i]) continue;
    if (!strcmp(instructions[i], "JUMPI")) {
      size_t n=i+1;
      while (!instructions[n]) ++n;
      if (!strcmp(instructions[n], "JUMP")) {
        size_t m=n+1;
        while (!instructions[m]) ++m;
        if (!strcmp(instructions[m], "JUMPDEST") && !strcmp(operands[i], operands[m])) {
          FREE(instructions[i]);
          FREE(instructions[n]);
          FREE(instructions[m]);
          instructions[i] = strdup("CONDITIONYES");
          instructions[n] = 0;
          instructions[m] = 0;
          FREE(operands[i]);
          FREE(operands[m]);
          operands[i] = operands[n];
          operands[n] = 0;
          operands[m] = 0;
        }
      }
    }
    if (!strcmp(instructions[i], "JUMP")) {
      size_t m=i+1;
      while (!instructions[m]) ++m;
      size_t m_index = string_index_in_arr(operands[m], jumpi_tags);
      if (!strcmp(instructions[m], "JUMPDEST") && m_index != -1) {
        FREE(instructions[i]);
        FREE(instructions[m]);
        instructions[i] = strdup("CONDITIONYES");
        instructions[m] = 0;
        char *tag_to_replace = operands[m];
        char *replace_with_tag = operands[i];
        // FREE(jumpi_tags[m_index]);
        // jumpi_tags[m_index] = strdup(operands[i]);
        for (size_t ii = 0; ii < line_count; ++ii) {
          if (!instructions[ii]) break;
          if (!memcmp(instructions[ii], "JUMP", 4) && !strcmp(operands[ii], tag_to_replace)) {
            FREE(operands[ii]);
            operands[ii] = strdup(operands[i]);
          }
        }
        FREE(operands[m]);
      }
    }
  }

  for (size_t i = 0; i < line_count; ++i) {
    if (!instructions[i]) continue;
    if (!strcmp(instructions[i], "JUMPI")) {
      FREE(instructions[i]);
      instructions[i] = strdup("CONDITIONNOT");
    }
    if (!strcmp(instructions[i], "JUMPDEST")) {
      if (string_index_in_arr(operands[i], jumpi_tags) != -1) {
        FREE(instructions[i]);
        instructions[i] = strdup("CONDITIONEND");
      }
    }
  }

  // for (size_t i = 0; i < line_count; ++i) {
  //   if (instructions[i]) {
  //     printf("%s", instructions[i]);
  //     if (operands[i]) {
  //       printf(" %s\n", operands[i]);
  //     } else printf("\n");
  //   }
  // }
  //
  // getchar();
  //
  // for (size_t i = 1; i < func_count; ++i) {
  //   printf("func_count[%zu] %s %04x\n", i, func_arr[i].name, func_arr[i].offset);
  // }



  // Save to out
  for (size_t i = 0; i < line_count; ++i) {
   if (!instructions[i]) continue;
    if (comp(instructions[i], "INIT") ||
        comp(instructions[i], "FALLBACK") ||
        comp(instructions[i], "JUMPDEST")) {
          sprintf(out_p, "\n");
          out_p = out + strlen(out);
    }
    sprintf(out_p, "%s", instructions[i]);
    out_p = out + strlen(out);
    if (operands[i]) {
      sprintf(out_p, " %s", operands[i]);
    }
    out_p = out + strlen(out);
    sprintf(out_p, "\n");
    out_p = out + strlen(out);
  }
  fseek(file, 0, SEEK_SET);
  fputs(out, file);
  fclose(file);


  FREE(instr);
  FREE(operand);
  for (size_t i = 0; i < line_count; ++i) {
    FREE(instructions[i]);
    FREE(operands[i]);
  }
  FREE(instructions);
  FREE(operands);
  func_arr[1] = (const struct tag){0};
  REALLOC_TAG_ARR(func_arr);
  REALLOC_STR(out);
  return out;
}
