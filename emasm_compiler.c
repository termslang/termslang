/**
* Copyright (c) 2017
* Mikhail Baynov m.baynov@gmail.com
* Distributed under the GNU GPL v2
*/


size_t parse_push_bytes(const char *operand, char **hex_bytes) {
  if (!*hex_bytes) *hex_bytes = NEW_ARR;
  if (!memcmp(operand, "HASH:", 5)) {
    char *body = operand + 5;
    size_t i;
    for (i = 0; i < strlen(body); ++i) {
      if (body[i] == ' ' || body[i] == '\t') {
        body[i] = 0;
      }
    }
    char hash[65];// = calloc(1, 65);
    strcpy(hash, keccak_hash(body));
    hash[8] = 0;
    char *hex_bytes_p = *hex_bytes;
    strcpy(*hex_bytes, hash);
    return 4;
  }
  if(is_hex(operand)) {
    uint8_t *bytes = NEW_LINE;
    size_t bytes_len = 0;
    hex2bin(operand, &bytes, &bytes_len);
    if (bytes_len > 32) {
      printf("ERROR: PUSH count > 32\n");
      exit(1);
    }
    char *hex_bytes_p = *hex_bytes;
    for (size_t y = 0; y < bytes_len; ++y) {
      sprintf(hex_bytes_p, "%02x", bytes[y]);
      hex_bytes_p += 2;
    }
    REALLOC_STR(*hex_bytes);
    // FREE(bytes);
    return bytes_len;
  }
  else if (memcmp(operand, "HASH:", 5)) {    //if not compares
    printf("ERROR: wrong PUSH operand: {%s}\n", operand);
    exit(1);
  }
  REALLOC_STR(*hex_bytes);
  return 0;
}

size_t string_index_in_tag_arr(const char *str, const struct tag *tags_arr) {
  size_t i;
  for (i = 1; tags_arr[i].name[0]; ++i) {
    if (!strcmp(str, tags_arr[i].name)) {
      return i;
    }
  }
  if (memcmp(str, EMASM_REFJUMP_TAG, strlen(EMASM_REFJUMP_TAG))) {
    if (str[0] == 0) {
      printf("WARNING: EMPTY JUMP TAG\n");
    } else {
      printf("ERROR: JUMP TAG NOT PRESENT: %s\n", str);
      exit(1);
    }
  }
  return 0;
}


char* add_loader(char***init_seq_arr, char*** seq_arr) {
  char *init_wr = NEW_ARR;
  char *wr_p = init_wr;
  size_t i;
  for (i = 1; *(*init_seq_arr + i); ++i) {
    strcpy(wr_p, *(*init_seq_arr + i));
    wr_p += strlen(*(*init_seq_arr + i));
  }
  char *wr = NEW_ARR;
  wr_p = wr;
  for (i = 1; *(*seq_arr + i); ++i) {
    strcpy(wr_p, *(*seq_arr + i));
    wr_p += strlen(*(*seq_arr + i));
  }
  size_t len = strlen(wr)/2;
  size_t init_len = 0;
  char *str2 = NEW_ARR;
  char *str2_p = str2;
  if (len <= 0xff) {
    sprintf(str2_p, "%s60%02lx80", init_wr, len);
  } else {
    sprintf(str2_p, "%s61%04lx80", init_wr, len);
  }
  str2_p += strlen(str2_p);
  init_len = strlen(str2)/2 + 8;
  if (init_len > 0xff) {
         sprintf(str2_p, "61%04lx6000396000f3", init_len + 1);
  } else sprintf(str2_p, "60%02lx6000396000f3", init_len);
  REALLOC_STR(str2);
  **seq_arr = str2;
  wr_p = wr;
  for (i = 0; *(*seq_arr + i) != 0; ++i) {
    strcpy(wr_p, *(*seq_arr + i));
    wr_p += strlen(*(*seq_arr + i));
  }
  return wr;
}


void update_tags_offset(const char **seq_arr, struct tag **tags_arr) {
  size_t len = 0;
  size_t tags_c = 1;
  size_t i;
  for (i = 1; seq_arr[i]; ++i) {
    len += strlen(seq_arr[i])/2;
    if (strlen(seq_arr[i]) > 2) continue;
    uint8_t* bytes = malloc(1);
    hex2bin(seq_arr[i], &bytes, NULL);
    if (bytes[0] == JUMPDEST) {
      (*(*tags_arr + tags_c)).offset = len - 1;
      ++tags_c;
    }
  }
}


int test_jumpdest(const char **seq_arr, const struct tag *tags_arr) {   //0 = success
  size_t t = 1;
  size_t len = 0;
  size_t i;
  for (i = 1; seq_arr[i]; ++i) {
    len += strlen(seq_arr[i])/2;
    uint8_t* bytes = malloc(1);
    hex2bin(seq_arr[i], &bytes, NULL);
    if (bytes[0] == JUMPDEST) {
      // printf("JUMPDEST 0x%04lx  ?=  ", len - 1);
      // printf("0x%04lx", tags_arr[t].offset);
      // printf(" (%s)\n", tags_arr[t].name);
      if (tags_arr[t].offset != len - 1) {
        // printf("JUMPDEST TEST MISMATCH, LOOPING AGAIN\n");
        return 1;
      }
      ++t;
    }
  }


  // test JUMPREF
  size_t refjump_tag_no = 0;
  for (i = 1; seq_arr[i]; ++i) {
    if (string_ends_with(seq_arr[i], "600052")
    && ((string_ends_with(seq_arr[i+1], "56") || string_ends_with(seq_arr[i+1], "57")))
    && !strcmp(seq_arr[i+2], "5b")) {
      if (!strcmp(seq_arr[i], "600052")) return 1;

      ++refjump_tag_no;
      char *tag;
      asprintf(&tag, "%s%zu", EMASM_REFJUMP_TAG, refjump_tag_no);
      size_t index = string_index_in_tag_arr(tag, tags_arr);
      if (index) {
        char *new_str;
        struct tag backdest = tags_arr[index];
        if (backdest.offset <= 0xff) {
               asprintf(&new_str, "60%02lx600052", backdest.offset);
        } else asprintf(&new_str, "61%04lx600052", backdest.offset);

        // printf("backdest %s\n", backdest.name);
        // printf("%s ?= \n%s\n\n", seq_arr[i], new_str);
        if (strcmp(seq_arr[i], new_str)) {
          FREE(new_str);
          return 1;
        }
        FREE(new_str);
      }
      FREE(tag);
    }
  }

  return 0;
}


void write_jumps_seq(const struct tag *tags_arr, const size_t *jumps_arr, char*** seq_arr) {
  size_t jumps = 0;
  size_t i;
  for (i = 1; *(*seq_arr + i); ++i) {
    char *str = *(*seq_arr + i);

    // JUMP JUMPI
    if ((string_ends_with(str, "56") && !string_ends_with(str, "60005156")) ||
        (string_ends_with(str, "57") && !string_ends_with(str, "60005157"))) {
      char *new_str;
      struct tag dest = tags_arr[jumps_arr[jumps]];
      char *last = str + strlen(str) - 2;

      //tag doesn't exist or empty tag
      if (jumps_arr[jumps] == 0) {
        asprintf(&new_str, "%s", last);
      }
      else {
        if (dest.offset <= 0xff) {
               asprintf(&new_str, "60%02lx%s", dest.offset, last);
        } else asprintf(&new_str, "61%04lx%s", dest.offset, last);
      }
      FREE(*(*seq_arr + i));
      *(*seq_arr + i) = new_str;
      ++jumps;
    }
  }

  // JUMPREF
  size_t refjump_tag_no = 0;
  for (i = 1; *(*seq_arr + i); ++i) {
    if (string_ends_with(*(*seq_arr + i), "600052")) {

      ++refjump_tag_no;
      char *tag;
      asprintf(&tag, "%s%zu", EMASM_REFJUMP_TAG, refjump_tag_no);
      size_t index = string_index_in_tag_arr(tag, tags_arr);
      if (index) {
        char *new_str;
        struct tag backdest = tags_arr[index];
        if (backdest.offset <= 0xff) {
               asprintf(&new_str, "60%02lx600052", backdest.offset);
        } else asprintf(&new_str, "61%04lx600052", backdest.offset);

        FREE(*(*seq_arr + i));
        *(*seq_arr + i) = new_str;
      }

      // printf("\n\n******************************LOG:");
      // printf("\nseq_arr:\n");
      // for (i = 1; *(*seq_arr + i) != 0; ++i) {
      //   printf("[%02zu] %s\n", i, *(*seq_arr + i));}
      // printf("*******************************END\n\n");
    }
  }

}


char* hex_from_instr(const char *instr, const char *operand) {
  if (!instr) return NULL;
  char *out = "";
  if (!strcmp(instr, "BACKJUMP")) asprintf(&out, "60005156");
  if (!strcmp(instr, "BACKJUMPI")) asprintf(&out, "60005157");
  if (!memcmp(instr, "PUSH", 4)) {
    char *hex_bytes;
    size_t n = 0x5f + parse_push_bytes(operand, &hex_bytes);
    asprintf(&out, "%02zx%s", n, hex_bytes);
    free(hex_bytes);
  }
  if (!memcmp(instr, "DUP", 3)) {
    size_t n = 0x7f;
    size_t nn = atoi_from_string(instr + 3);
    if (nn != -1 && nn > 0 && nn <= 16) n += nn;
    asprintf(&out, "%02zx", n);
  }
  if (!memcmp(instr, "SWAP", 4)) {
    size_t n = 0x8f;
    size_t nn = atoi_from_string(instr + 4);
    if (nn != -1 && nn > 0 && nn <= 16) n += nn;
    asprintf(&out, "%02zx", n);
  }
  if (!strcmp(instr, "STOP")) asprintf(&out, "00");
  if (!strcmp(instr, "ADD"))  asprintf(&out, "01");
  if (!strcmp(instr, "MUL")) asprintf(&out, "02");
  if (!strcmp(instr, "SUB")) asprintf(&out, "03");
  if (!strcmp(instr, "DIV")) asprintf(&out, "04");
  if (!strcmp(instr, "SDIV")) asprintf(&out, "05");
  if (!strcmp(instr, "MOD")) asprintf(&out, "06");
  if (!strcmp(instr, "SMOD")) asprintf(&out, "07");
  if (!strcmp(instr, "ADDMOD")) asprintf(&out, "08");
  if (!strcmp(instr, "MULMOD")) asprintf(&out, "09");
  if (!strcmp(instr, "EXP")) asprintf(&out, "0a");
  if (!strcmp(instr, "SIGNEXTEND")) asprintf(&out, "0b");

  if (!strcmp(instr, "LT")) asprintf(&out, "10");
  if (!strcmp(instr, "GT")) asprintf(&out, "11");
  if (!strcmp(instr, "SLT")) asprintf(&out, "12");
  if (!strcmp(instr, "SGT")) asprintf(&out, "13");
  if (!strcmp(instr, "EQ")) asprintf(&out, "14");
  if (!strcmp(instr, "ISZERO")) asprintf(&out, "15");
  if (!strcmp(instr, "AND")) asprintf(&out, "16");
  if (!strcmp(instr, "OR")) asprintf(&out, "17");
  if (!strcmp(instr, "XOR")) asprintf(&out, "18");
  if (!strcmp(instr, "NOT")) asprintf(&out, "19");
  if (!strcmp(instr, "BYTE")) asprintf(&out, "1a");

  if (!strcmp(instr, "SHA3")) asprintf(&out, "20");

  if (!strcmp(instr, "ADDRESS")) asprintf(&out, "30");
  if (!strcmp(instr, "BALANCE")) asprintf(&out, "31");
  if (!strcmp(instr, "ORIGIN")) asprintf(&out, "32");
  if (!strcmp(instr, "CALLER")) asprintf(&out, "33");
  if (!strcmp(instr, "CALLVALUE")) asprintf(&out, "34");
  if (!strcmp(instr, "CALLDATALOAD")) asprintf(&out, "35");
  if (!strcmp(instr, "CALLDATASIZE")) asprintf(&out, "36");
  if (!strcmp(instr, "CALLDATACOPY")) asprintf(&out, "37");
  if (!strcmp(instr, "CODESIZE")) asprintf(&out, "38");
  if (!strcmp(instr, "CODECOPY")) asprintf(&out, "39");
  if (!strcmp(instr, "GASPRICE")) asprintf(&out, "3a");
  if (!strcmp(instr, "EXTCODESIZE")) asprintf(&out, "3b");
  if (!strcmp(instr, "EXTCODECOPY")) asprintf(&out, "3c");
  if (!strcmp(instr, "RETURNDATASIZE")) asprintf(&out, "3d");
  if (!strcmp(instr, "RETURNDATACOPY")) asprintf(&out, "3e");

  if (!strcmp(instr, "BLOCKHASH")) asprintf(&out, "40");
  if (!strcmp(instr, "COINBASE")) asprintf(&out, "41");
  if (!strcmp(instr, "TIMESTAMP")) asprintf(&out, "42");
  if (!strcmp(instr, "NUMBER")) asprintf(&out, "43");
  if (!strcmp(instr, "DIFFICULTY")) asprintf(&out, "44");
  if (!strcmp(instr, "GASLIMIT")) asprintf(&out, "45");

  if (!strcmp(instr, "POP")) asprintf(&out, "50");
  if (!strcmp(instr, "MLOAD")) asprintf(&out, "51");
  if (!strcmp(instr, "MSTORE")) asprintf(&out, "52");
  if (!strcmp(instr, "MSTORE8")) asprintf(&out, "53");
  if (!strcmp(instr, "SLOAD")) asprintf(&out, "54");
  if (!strcmp(instr, "SSTORE")) asprintf(&out, "55");
  if (!strcmp(instr, "JUMP")) asprintf(&out, "56");
  if (!strcmp(instr, "JUMPI")) asprintf(&out, "57");
  if (!strcmp(instr, "PC")) asprintf(&out, "58");
  if (!strcmp(instr, "MSIZE")) asprintf(&out, "59");
  if (!strcmp(instr, "GAS")) asprintf(&out, "5a");
  if (!strcmp(instr, "JUMPDEST")) asprintf(&out, "5b");
  //0x60-0x7f PUSH1-PUSH32
  //0x80-0x8f DUP1-DUP16
  //0x90-0x9f SWAP1-SWAP16
  if (!strcmp(instr, "LOG0")) asprintf(&out, "a0");
  if (!strcmp(instr, "LOG1")) asprintf(&out, "a1");
  if (!strcmp(instr, "LOG2")) asprintf(&out, "a2");
  if (!strcmp(instr, "LOG3")) asprintf(&out, "a3");
/* disconinued:
  if (!strcmp(instr, "SLOADBYTES")) asprintf(&out, "e1");
  if (!strcmp(instr, "SSTOREBYTES")) asprintf(&out, "e2");
  if (!strcmp(instr, "SSIZE")) asprintf(&out, "e3");
*/
  if (!strcmp(instr, "CREATE")) asprintf(&out, "f0");
  if (!strcmp(instr, "CALL")) asprintf(&out, "f1");
  if (!strcmp(instr, "CALLCODE")) asprintf(&out, "f2");
  if (!strcmp(instr, "RETURN")) asprintf(&out, "f3");
  if (!strcmp(instr, "DELEGATECALL")) asprintf(&out, "f4");
  if (!strcmp(instr, "CALLBLACKBOX")) asprintf(&out, "f5");

  if (!strcmp(instr, "STATICCALL")) asprintf(&out, "fa");
  if (!strcmp(instr, "INVALID")) asprintf(&out, "fe");
  if (!strcmp(instr, "REVERT")) asprintf(&out, "fd");
  if (!strcmp(instr, "SUICIDE") || !strcmp(instr, "SELFDESTRUCT")) asprintf(&out, "ff");
  return out;
}


void add_callname_to_file(const char *str) {
  FILE *file = fopen(EMASM_CALLNAME_FILE,"a");
  if (file==NULL) {
      printf("WARNING: Can't open %s used for disassembling\n", EMASM_CALLNAME_FILE);
  }
  char *line = NEW_LINE;
  fseek(file, 0, SEEK_END);
  fputs(str, file);
  fputs("\n", file);
  fclose(file);
}


size_t parse_store_sequence_to_arr(const char *operand, const char *prev_operand, bool sstore, char*** arr) {
  if (!operand) return 0;
  if (operand[0] != '0' || (operand[1] != 'x' && operand[1] != 'X')) return 0;
  char *mstore_sstore;
  if (sstore) {
         mstore_sstore = "SSTORE";
  } else mstore_sstore = "MSTORE";
  asprintf(*arr, "%s", mstore_sstore);
  char *line = operand + 2;
  size_t len = strlen(line);
  char *hex = NEW_WORD;
  char *offset = strdup(prev_operand);
  char *new_offset;
  size_t count = 0;
  while (len > 64) {
    memcpy(hex, line, 64);
    line += 64;
    len -= 64;

    new_offset = hex_add(offset, "0x20");
    asprintf(*arr + 1 + count, "PUSH 0x%s", hex);
    asprintf(*arr + 2 + count, "PUSH %s", new_offset);
    asprintf(*arr + 3 + count, "%s", mstore_sstore);
    FREE(offset);
    offset = new_offset;

    count +=3;
  }
  FREE(hex);
  hex = strdup("0000000000000000000000000000000000000000000000000000000000000000");
  // memset(hex, '0', 64);
  memcpy(hex, line, strlen(line));

  new_offset = hex_add(offset, "0x20");
  asprintf(*arr + 1 + count, "PUSH 0x%s", hex);
  asprintf(*arr + 2 + count, "PUSH %s", new_offset);
  asprintf(*arr + 3 + count, "%s", mstore_sstore);
  *(*arr + 4 + count) = 0;

  FREE(offset);
  FREE(new_offset);
  FREE(hex);
  FREE(offset);
  return strlen(operand)/2 - 1;
}


char* emasm_compile(const char* in) {
  size_t ii,i;

  // Form lines_arr
  FILE *file = fopen(in,"r");
  if (file==NULL) {
      printf("Unable to open file to compile, disassembling the hex string:\n%s\n\n", in);
      printf("%s\n", emasm_disasm(in));
      return NULL;
  }
  char **lines_arr = NEW_HUGE_ARR;
  char *line = NEW_LINE;
  fseek(file, 0, SEEK_SET);
  ii = 0;
  while (fgets(line, 4096, file)) {
    line[strlen(line)-1] = 0;
    if (line[0]) {
      lines_arr[ii] = strdup(line);
      ++ii;
    }
  }
  fclose(file);
  lines_arr[ii] = 0;

  // preprocessor (CONDITIONNOT, CONDITIONYES, CONDITIONEND)
  // count conditions
  char **conditionnot_arr = NEW_ARR;
  char **conditionyes_arr = NEW_ARR;
  size_t conditionnot_count = 0;
  size_t conditionyes_count = 0;
  conditionnot_arr[0] = 0;
  conditionyes_arr[0] = 0;
  char *instr = NEW_WORD;
  char *operand = NEW_LINE;
  ii=0;
  while (lines_arr[ii]) {
    instr[0]=0;
    operand[0]=0;
    parse_asm_line(lines_arr[ii], &instr, &operand);
    // printf("instr %s,operand %s\n", instr, operand);
    if (!strcmp(instr, "CONDITIONNOT")) {
      if (string_index_in_arr(operand, conditionyes_arr) != -1) {
        printf("ERROR: [CONDITIONNOT %s] must go before [CONDITIONYES %s]\n", operand, operand);
        exit(1);
      }
      if (-1 == string_index_in_arr(operand, conditionnot_arr)) {
        asprintf(&conditionnot_arr[conditionnot_count], "%s", operand);
        ++conditionnot_count;
        conditionnot_arr[conditionnot_count] = 0;
      }
    }
    else if (!strcmp(instr, "CONDITIONYES")) {
      if (-1 == string_index_in_arr(operand, conditionyes_arr)) {
        asprintf(&conditionyes_arr[conditionyes_count], "%s", operand);
        ++conditionyes_count;
        conditionyes_arr[conditionyes_count] = 0;
      }
    }
    ++ii;
  }
  conditionnot_arr[conditionnot_count] = 0;
  conditionyes_arr[conditionyes_count] = 0;


  // preprocessor replaces
  char *prev_instr = NEW_WORD;
  char *prev_operand = NEW_LINE;
  ii=0;
  while (lines_arr[ii]) {
    // printf("%zu  \n", ii);
    // printf("%zu    %s\n", ii, lines_arr[ii]);
    strcpy(prev_instr, instr);
    strcpy(prev_operand, operand);
    parse_asm_line(lines_arr[ii], &instr, &operand);
    if (comp(instr, "CONDITIONNOT")) {
      if (string_index_in_arr(operand, conditionyes_arr) != -1) {
        FREE(lines_arr[ii]);
        asprintf(&lines_arr[ii], "JUMPI %syes", operand);
      } else {
        FREE(lines_arr[ii]);
        asprintf(&lines_arr[ii], "JUMPI %send", operand);
      }
    }
    else if (comp(instr, "CONDITIONYES")) {
      char **arr = NEW_ARR;
      if (string_index_in_arr(operand, conditionnot_arr) != -1) {
        FREE(lines_arr[ii]);
        asprintf(&lines_arr[ii], "JUMP %send", operand);
        asprintf(&arr[0], "JUMPDEST %syes", operand);
        arr[1] = 0;
        arr_insert_elements_at_position(arr, ii+1, &lines_arr);
      } else {
        FREE(lines_arr[ii]);
        asprintf(&lines_arr[ii], "JUMPI %syes", operand);
        asprintf(&arr[0], "JUMP %send", operand);
        asprintf(&arr[1], "JUMPDEST %syes", operand);
        arr[2] = 0;
        arr_insert_elements_at_position(arr, ii+1, &lines_arr);
      }
      FREE(arr);
    }
    else if (comp(instr, "CONDITIONEND")) {
      if ((-1 == string_index_in_arr(operand, conditionnot_arr)) &&
          (-1 == string_index_in_arr(operand, conditionyes_arr))) {
        printf("ERROR: [CONDITIONNOT %s] or [CONDITIONYES %s] must go before [CONDITIONEND %s]\n", operand, operand, operand);
        exit(1);
      } else {
        FREE(lines_arr[ii]);
        asprintf(&lines_arr[ii], "JUMPDEST %send", operand);
      }
    }

    if ((comp(instr, "MSTORESEQ") && comp(prev_instr, "PUSH") && ii)
    ||  (comp(instr, "SSTORESEQ") && comp(prev_instr, "PUSH") && ii)) {
      bool sstore;
      if (comp(instr, "MSTORESEQ")) {
        sstore = false;
      } else sstore = true;

      char **arr = NEW_ARR;
      size_t len = parse_store_sequence_to_arr(operand, prev_operand, sstore, &arr);
      FREE(lines_arr[ii-1]);

      asprintf(&lines_arr[ii-1], "PUSH %s", hex_from_digit(len));  //length
      FREE(lines_arr[ii]);
      asprintf(&lines_arr[ii], "PUSH %s", prev_operand);
      if (arr) arr_insert_elements_at_position(arr, ii+1, &lines_arr);
//      printf("XXXXX\n" );
      // print_arr("ww", arr);
      ii += arr_count(arr);
      FREE(arr);
      continue;
    }

    ++ii;
  }
print_arr("lines", lines_arr);

  FREE(instr);
  FREE(operand);
  FREE_ARR(conditionnot_arr);
  FREE_ARR(conditionyes_arr);
  FREE(prev_instr);
  FREE(prev_operand);
  REALLOC_ARR(lines_arr);


  // print_arr("conditionnot_arr", conditionnot_arr);
  // print_arr("conditionyes_arr", conditionyes_arr);




  //temp vars
  bool fallback_dispatcher_used = false;
  char *fallback_operand = NEW_LINE;

  char **init_seq_arr;
  char **seq_arr = NEW_HUGE_ARR;
  asprintf(seq_arr, "LOADER");    //init_seq_arr[0] reserved for loader
  size_t seq_count = 1;

  struct tag *init_tags_arr = NEW_BIG_ARR; //starts from index 1
  size_t init_tags_count = 0;
  size_t *init_jumps_arr = NEW_BIG_ARR;

  struct tag *tags_arr = NEW_BIG_ARR;     //starts from index 1
  size_t tags_count = 0;
  size_t *jumps_arr = NEW_BIG_ARR;   //[1, 4, 2, ...]

  struct tag *func_arr = NEW_ARR;
  size_t func_count = 1;


  /* form tags_arr */
  bool init_part = true;
  size_t refjump_tag_no = 0;
  ii=0;
  while (lines_arr[ii]) {
    char *instr = NEW_WORD;
    char *operand = NEW_LINE;
    parse_asm_line(lines_arr[ii], &instr, &operand);
    REALLOC_STR(instr);
    REALLOC_STR(operand);
    if (!strcmp(instr, "INIT")) {
      init_part = false;
    }
    if (!strcmp(instr, "FALLBACK")) {
      if (fallback_dispatcher_used) {
        printf("ERROR: FALLBACK DISPATCHER CAN ONLY BE USED ONCE\n");
        exit(1);
      }
      fallback_dispatcher_used = true;
    }

    if (!strcmp(instr, "JUMPDEST") || !strcmp(instr, "FALLBACK")
    || !strcmp(instr, "REFJUMP") || !strcmp(instr, "REFJUMPI")) {
      if (operand == 0) {
        printf("ERROR: %s OPERAND ABSENT\n", instr);
        exit(1);
      }
      char *op;
      if (!strcmp(instr, "REFJUMP") || !strcmp(instr, "REFJUMPI")) {
        ++refjump_tag_no;
        asprintf(&op, "%s%zu", EMASM_REFJUMP_TAG, refjump_tag_no);
      } else op = strdup(operand);
      if (init_part) {
        ++init_tags_count;
        strcpy(init_tags_arr[init_tags_count].name, op);
      } else {
        ++tags_count;
        strcpy(tags_arr[tags_count].name, op);
      }
      FREE(op);
    }
    ++ii;
  }
  //null ending for tags_arr
  init_tags_arr[init_tags_count + 1] = (const struct tag){0};
  tags_arr[tags_count + 1] = (const struct tag){0};
  REALLOC_TAG_ARR(init_tags_arr);
  REALLOC_TAG_ARR(tags_arr);


  /* form seq_arr */
  char *wr = NEW_LINE;
  char *wr_p = wr;
  bool loader_exists = false;
  ii=0;
  while (lines_arr[ii]) {
    char *instr = NEW_WORD;
    char *operand = NEW_LINE;
    parse_asm_line(lines_arr[ii], &instr, &operand);
    REALLOC_STR(instr);
    REALLOC_STR(operand);
    if (!strcmp(instr, "INIT")) {
      if (loader_exists) {
        printf("ERROR: MORE THAN ONE INIT KEYWORD\n");
        exit(1);
      }
      loader_exists = true;
      //finalize seq_arr
      seq_arr[seq_count] = strdup(wr);
      ++seq_count;
      wr[0] = 0;
      wr_p = wr;
      seq_arr[seq_count] = 0;

      init_seq_arr = seq_arr;
      seq_count = 1;
      seq_arr = NEW_HUGE_ARR;
      asprintf(seq_arr, "LOADER");    //seq_arr[0] reserved for loader
    }
    if (!strcmp(instr, "FALLBACK")) {
      //form func_arr
      size_t i;
      for (i = 1; tags_arr[i].name[0]; ++i) {
        if (string_is_function_name(tags_arr[i].name)) {
          strcpy(func_arr[func_count].name, tags_arr[i].name);
          ++func_count;
          //add to CALLNAME_FILE
          if (!callname_exists(tags_arr[i].name) && memcmp(tags_arr[i].name, "UNKNOWN_FUNC(", 13)) {
            add_callname_to_file(tags_arr[i].name);
          }
        }
      }
      REALLOC_TAG_ARR(func_arr);

      strcpy(fallback_operand, operand);
      sprintf(wr_p, "60e060020a60003504");
      wr_p += 18;
      for (i = 1; i < func_count; ++i) {
        char hash[65];
        if (!memcmp(func_arr[i].name, "UNKNOWN_FUNC(", 13)) {
          memcpy(hash, func_arr[i].name+13, 8);
        } else {
          strcpy(hash, keccak_hash(func_arr[i].name));
          hash[8] = 0;
        }
        if (i < func_count - 1) {
          sprintf(wr_p, "80");
          wr_p += 2;
        }
        sprintf(wr_p, "63%s14", hash);
        wr_p += 12;

        seq_arr[seq_count] = strdup(wr);
        ++seq_count;
        wr[0] = 0;
        wr_p = wr;

        seq_arr[seq_count] = strdup("57");
        ++seq_count;
        wr[0] = 0;
        wr_p = wr;
      }
      seq_arr[seq_count] = strdup("5b");
      ++seq_count;
      wr[0] = 0;
      wr_p = wr;
    }  /* FALLBACK */


    if (!strcmp(instr, "JUMP") || !strcmp(instr, "JUMPI") || !strcmp(instr, "JUMPDEST")
    || !strcmp(instr, "REFJUMP") || !strcmp(instr, "REFJUMPI")) {
      if (wr[0]) {
        seq_arr[seq_count] = strdup(wr);
        ++seq_count;
        wr[0] = 0;
        wr_p = wr;
      }

      if (!strcmp(instr, "JUMP") || !strcmp(instr, "JUMPI") || !strcmp(instr, "JUMPDEST")) {
        char *str = hex_from_instr(instr, operand);
        seq_arr[seq_count] = str;
        ++seq_count;
      }
      else if (!strcmp(instr, "REFJUMP")) {
      //  sprintf(str, "6XXX6000526YYY56");  //+5b if not present
        asprintf(seq_arr + seq_count, "600052");
        ++seq_count;
        asprintf(seq_arr + seq_count, "56");
        ++seq_count;
        asprintf(seq_arr + seq_count, "5b");
        ++seq_count;
      }
      else if (!strcmp(instr, "REFJUMPI")) {
        asprintf(seq_arr + seq_count, "600052");
        ++seq_count;
        asprintf(seq_arr + seq_count, "57");
        ++seq_count;
        asprintf(seq_arr + seq_count, "5b");
        ++seq_count;
      }

      // push xx
      // push 0x00
      // MSTORE
      // jump yy
      // jumpdest
    }
    else {    // not JUMP, JUMPI, REFJUMP etc
      char *hex = hex_from_instr(instr, operand);
      strcpy(wr_p, hex);
      wr_p += strlen(hex);
    }
    ++ii;
  }

  if (!loader_exists) {
    printf("ERROR: INIT KEYWORD NOT PRESENT\n");
    exit(1);
  }
  // if (!loader_exists && !testing_disasm) {
  //   printf("ERROR: INIT KEYWORD NOT PRESENT\n");
  //   exit(1);
  // }

  //finalize seq_arr
  seq_arr[seq_count] = strdup(wr);
  ++seq_count;
  seq_arr[seq_count] = 0;
  FREE(wr);
  REALLOC_ARR(init_seq_arr);
  REALLOC_ARR(seq_arr);



  size_t jumps_arr_count = 0;       //zero means none
  size_t init_jumps_arr_count = 0;

  /* form jumps_arr */
  init_part = true;
  size_t jumps_before_fallback = 0;
  ii=0;
  while (lines_arr[ii]) {
    char *instr = NEW_WORD;
    char *operand = NEW_LINE;
    parse_asm_line(lines_arr[ii], &instr, &operand);
    REALLOC_STR(instr);
    REALLOC_STR(operand);
    if (!strcmp(instr, "INIT")) {
      init_part = false;
    }
    if (!strcmp(instr, "FALLBACK")) {
      jumps_before_fallback = jumps_arr_count;
//      printf("jumps_before_fallback %zu\n", jumps_before_fallback);
    }
    if (!strcmp(instr, "JUMP") || !strcmp(instr, "JUMPI")
    || !strcmp(instr, "REFJUMP") || !strcmp(instr, "REFJUMPI")) {
      if (init_part) {
        init_jumps_arr[init_jumps_arr_count] = string_index_in_tag_arr(operand, init_tags_arr);
        ++init_jumps_arr_count;
        init_jumps_arr[init_jumps_arr_count] = 0;
      } else {  //not init_part
        jumps_arr[jumps_arr_count] = string_index_in_tag_arr(operand, tags_arr);
        ++jumps_arr_count;
        jumps_arr[jumps_arr_count] = 0;
      }
    }
    ++ii;
  }  /* form jumps_arr */


  // printf("\n\n******************************LOG:");
  // printf("\ninit_seq_arr:\n");
  // for (i = 1; *(init_seq_arr + i) != 0; ++i) {
  //   printf("[%02zu] %s\n", i, init_seq_arr[i]);}
  // printf("\nseq_arr:\n");
  // for (i = 1; *(seq_arr + i) != 0; ++i) {
  //   printf("[%02zu] %s\n", i, seq_arr[i]);}
  // printf("\ninit_tags_arr:\n");
  // for (i = 1; *(init_tags_arr + i)->name != 0; ++i) {
  //   printf("[%02zu] %s\n", i, init_tags_arr[i].name);}
  // printf("\ntags_arr:\n");
  // for (i = 1; *(tags_arr + i)->name != 0; ++i) {
  //   printf("[%02zu] %s\n", i, tags_arr[i].name);}
  // printf("\njumps_arr:\n");
  // for (i = 0; jumps_arr[i] != 0; ++i) {
  //   printf("[%zu] = %zu\n", i, jumps_arr[i]);}
  // printf("*******************************END\n\n");



  if (fallback_dispatcher_used) {
    //insert fallback dispatcher functions to jumps_arr + jumps_before_fallback
    size_t *old_jumps_arr = jumps_arr;
    jumps_arr = NEW_ARR;
    size_t i;
    for (i = 0; i < jumps_before_fallback; ++i) {
      jumps_arr[i] = old_jumps_arr[i];
    }
    size_t y = 0;
    for (i = 1; i < func_count; ++i) {
      jumps_arr[y + jumps_before_fallback] = string_index_in_tag_arr(func_arr[i].name, tags_arr);
      ++y;
      ++jumps_arr_count;
    }
    jumps_arr[y + jumps_before_fallback] = 0;
    for (i = jumps_before_fallback; jumps_arr[i]; ++i) {
      jumps_arr[i+y] = old_jumps_arr[i];
    }
  }



  // printf("\n\n******************************LOG:");
  // printf("\ninit_seq_arr:\n");
  // for (i = 1; *(init_seq_arr + i) != 0; ++i) {
  //   printf("[%02zu] %s\n", i, init_seq_arr[i]);}
  // printf("\nseq_arr:\n");
  // for (i = 1; *(seq_arr + i) != 0; ++i) {
  //   printf("[%02zu] %s\n", i, seq_arr[i]);}
  // printf("\ninit_tags_arr:\n");
  // for (i = 1; *(init_tags_arr + i)->name != 0; ++i) {
  //   printf("[%02zu] %s\n", i, init_tags_arr[i].name);}
  // printf("\ntags_arr:\n");
  // for (i = 1; *(tags_arr + i)->name != 0; ++i) {
  //   printf("[%02zu] %s\n", i, tags_arr[i].name);}
  // printf("\njumps_arr:\n");
  // for (i = 0; jumps_arr[i] != 0; ++i) {
  //   printf("[%zu] = %zu\n", i, jumps_arr[i]);}
  // printf("*******************************END\n\n");
  //
  //
  //
  // printf("\n\n::::::::::::::VALIDATE INIT SEQ::::::::::::::");
  do {
    update_tags_offset(init_seq_arr, &init_tags_arr);
    write_jumps_seq(init_tags_arr, init_jumps_arr, &init_seq_arr);
  } while (test_jumpdest(init_seq_arr, init_tags_arr));

  // printf("\n\n******************************LOG:");
  // printf("\ninit_seq_arr:\n");
  // for (i = 1; *(init_seq_arr + i) != 0; ++i) {
  //   printf("[%02zu] %s\n", i, init_seq_arr[i]);}
  // printf("\nseq_arr:\n");
  // for (i = 1; *(seq_arr + i) != 0; ++i) {
  //   printf("[%02zu] %s\n", i, seq_arr[i]);}
  // printf("\ninit_tags_arr:\n");
  // for (i = 1; *(init_tags_arr + i)->name != 0; ++i) {
  //   printf("[%02zu] %s\n", i, init_tags_arr[i].name);}
  // printf("\ntags_arr:\n");
  // for (i = 1; *(tags_arr + i)->name != 0; ++i) {
  //   printf("[%02zu] %s\n", i, tags_arr[i].name);}
  // printf("\njumps_arr:\n");
  // for (i = 0; jumps_arr[i] != 0; ++i) {
  //   printf("[%zu] = %zu\n", i, jumps_arr[i]);}
  // printf("*******************************END\n\n");
  //
  //
  // printf("\n\n::::::::::::::VALIDATE SEQ::::::::::::::");
  do {
    update_tags_offset(seq_arr, &tags_arr);
    write_jumps_seq(tags_arr, jumps_arr, &seq_arr);
  } while (test_jumpdest(seq_arr, tags_arr));

  // printf("\n\n******************************LOG:");
  // printf("\ninit_seq_arr:\n");
  // for (i = 1; *(init_seq_arr + i) != 0; ++i) {
  //   printf("[%02zu] %s\n", i, init_seq_arr[i]);}
  // printf("\nseq_arr:\n");
  // for (i = 1; *(seq_arr + i) != 0; ++i) {
  //   printf("[%02zu] %s\n", i, seq_arr[i]);}
  // printf("\ninit_tags_arr:\n");
  // for (i = 1; *(init_tags_arr + i)->name != 0; ++i) {
  //   printf("[%02zu] %s\n", i, init_tags_arr[i].name);}
  // printf("\ntags_arr:\n");
  // for (i = 1; *(tags_arr + i)->name != 0; ++i) {
  //   printf("[%02zu] 0x%04lx   %s\n", i, tags_arr[i].offset, tags_arr[i].name);}
  // printf("\njumps_arr:\n");
  // for (i = 0; jumps_arr[i] != 0; ++i) {
  //   printf("[%zu] = %zu\n", i, jumps_arr[i]);}
  // printf("*******************************END\n\n");



  char *out = add_loader(&init_seq_arr, &seq_arr);
  print("Compiled to %zu bytes.\n", strlen(out)/2);
  print("init + output:  0x%s\n\n", out);

  FREE(line);
  FREE_ARR(lines_arr);
  FREE(fallback_operand);
  FREE_ARR(init_seq_arr);
  FREE_ARR(seq_arr);
  init_tags_arr[1] = (const struct tag){0};
  tags_arr[1] = (const struct tag){0};
  func_arr[1] = (const struct tag){0};
  REALLOC_TAG_ARR(init_tags_arr);
  REALLOC_TAG_ARR(tags_arr);
  REALLOC_TAG_ARR(func_arr);
  FREE(jumps_arr);
  return out;
}
