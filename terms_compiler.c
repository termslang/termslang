/**
* Copyright (c) 2017
* Mikhail Baynov m.baynov@gmail.com
* Distributed under the GNU GPL v2
*/
int string_is_jumptag(const char *str) {
  if (!str) return 0;
  size_t ii=0;
  while (str[ii]) {
    if (!((str[ii] >= '0' && str[ii] <= '9') || str[ii] == '_' || str[ii] == '.' || str[ii] == ' ')) {
      return 0;
    }
    ++ii;
  }
  return 1;
}


void update_match_lines(size_t match_number, const char** constants, const char** constants_bodies,
  char*** variables, char*** variable_arrays_arr, char*** match_lines) {

  size_t variables_count = arr_count(*variables);
  size_t variable_arrays_count = arr_count(*variable_arrays_arr);


  size_t i = 0;
  while (*(*match_lines + i)) {
    char *instr = NEW_WORD;
    char *operand = NEW_LINE;
    parse_asm_line(*(*match_lines + i), &instr, &operand);

    if (!(strcmp(instr, "JUMPDEST") && strcmp(instr, "JUMP") && strcmp(instr, "JUMPI")
      && strcmp(instr, "REFJUMP") && strcmp(instr, "REFJUMPI"))) {
        FREE(*(*match_lines + i));
      if (string_is_function_name_or_fallback(operand) || !operand[0]) {
        //TODO remove hardcoded fallback tag "()"
        asprintf((*match_lines + i), "%s %s", instr, operand);
      }
      else if (string_is_jumptag(operand)) {
        asprintf((*match_lines + i), "%s TAG%s", instr, operand);
      }
      else if(operand[0] == '\"') {
        char *str = NEW_LINE;
        size_t str_letter_count = 0;
        size_t ii=0;
        while (operand[ii]) {
          if (operand[ii] == ' ' || operand[ii] == '\"') {
            str[str_letter_count] = '_';
          } else str[str_letter_count] = operand[ii];
          ++str_letter_count;
          ++ii;
        }
        asprintf((*match_lines + i), "%s TAG%sPROCEDURE", instr, str);
      }
      else {
        asprintf((*match_lines + i), "%s TAG%s_MATCH_NUMBER%zu", instr, operand, match_number);
      }
      operand = *(*match_lines + i);
    }
    if (!operand) continue;


    /// Parse PUSH and PUSH* in macros
    if ((!strcmp(instr, "PUSH*") || !strcmp(instr, "PUSH"))) {
      char *var_name_str = NEW_WORD;
      char *var_index_str = NEW_WORD;
      parse_macro_argument(operand, &var_name_str, &var_index_str);
      REALLOC_STR(var_name_str);
      REALLOC_STR(var_index_str);
      size_t var_index = string_index_in_arr(var_name_str, *variables);
      size_t const_index = string_index_in_arr(var_name_str, constants);


      // Array
      if (var_name_str[0] && var_index_str[0]) {
        size_t variable_array_index = string_index_in_arr(var_name_str, *variable_arrays_arr);

        // PUSH ARR#1
        size_t var_index = atoi_from_string(var_index_str);
        if (var_index != -1) {
          size_t memory_offset = var_index * 32 + variable_array_index * TERMS_VARIABLE_ARRAYS_OFFSET;
          sprintf(*(*match_lines + i), "PUSH %s", hex_from_digit(memory_offset));
        }
        var_index = string_index_in_arr(var_index_str, *variables);
        if (var_index != -1 || atoi_from_string(var_index_str)) {
          if (var_index != -1) {
            // PUSH ARR#INDEX
            FREE(*(*match_lines + i));
            asprintf((*match_lines + i), "PUSH %s", hex_from_digit(var_index * 32));
            char **arr = NEW_ARR;
            asprintf(&arr[0], "MLOAD");
            asprintf(&arr[1], "PUSH 0x20");
            asprintf(&arr[2], "MUL");
            asprintf(&arr[3], "PUSH %s", hex_from_digit((variable_array_index * TERMS_VARIABLE_ARRAYS_OFFSET)));
            asprintf(&arr[4], "ADD");
            arr[5] = 0;
            size_t insert_count = 5;
            arr_insert_elements_at_position(arr, i + 1, match_lines);
            i += insert_count;  //items_to_insert
          }
        }
      }
      else if (!var_index_str[0] && is_hex(var_name_str)) {
        // Hex
        FREE(*(*match_lines + i));
        asprintf((*match_lines + i), "PUSH %s", var_name_str);
      }
      else if (!var_index_str[0] && atoi_from_string(var_name_str) != -1) {
        // Digit
        FREE(*(*match_lines + i));
        asprintf((*match_lines + i), "PUSH %s", hex_from_digit(atoi_from_string(var_name_str)));
      }
      else if (!var_index_str[0] && const_index != -1) {
        // Constant
        FREE(*(*match_lines + i));
        asprintf((*match_lines + i), "%s", constants_bodies[const_index]);
      }
      else {
        // Variable

        size_t var_index = string_index_in_arr(var_name_str, *variables);
        if (var_index != -1) {
          FREE(*(*match_lines + i));
          asprintf((*match_lines + i), "PUSH %s", hex_from_digit(var_index * 32));
        }
      }
      // Add MLOAD for PUSH*, not hex, not digit, not const
      if (!strcmp(instr, "PUSH*") && !is_hex(var_name_str) && -1 == atoi_from_string(var_name_str) && -1 == const_index) {
        // Insert variable MLOAD
        char **arr = NEW_ARR;
        arr[0] = strdup("MLOAD");
        arr[1] = 0;
        size_t insert_count = 1;
        arr_insert_elements_at_position(arr, i + 1, match_lines);
        i += insert_count;  //items_to_insert
      }
    }
  ++i;
  }
}


void parse_matches_final_arr(const char** constants, const char** constants_bodies, char**** matches_final_arr) {
  char **variables = NEW_ARR;
  variables[0] = strdup("TERMS_REFERENCE_RETURN_JUMPDEST");
  variables[1] = 0;
  size_t variables_count = 1;

  char **variable_arrays_arr = NEW_ARR;
  variable_arrays_arr[0] = strdup("TERMS_VARIABLE_ARRAY_RESERVED_FOR_VARIABLES");
  variable_arrays_arr[1] = 0;
  size_t variable_arrays_count = 1;

  char *var_name_str;
  for (size_t ii=0; *(*matches_final_arr + ii); ++ii) {
    char **sentence_arr = NEW_ARR;
    char **macro_sentence_arr = NEW_ARR;

    // printf("%s\n", *(*(*matches_final_arr + ii))+14);
    // printf("%s\n", *(*(*matches_final_arr + ii))+23);
    // getchar();

    // if (!memcmp(*(*(*matches_final_arr + ii)) + 14, "constant", 8)) {
    //   memcpy(*(*(*matches_final_arr + ii)) + 14, "jumpdest", 8);
    // }

    // printf("%s\n", *(*(*matches_final_arr + ii)));
    // getchar();
    if (!memcmp(*(*(*matches_final_arr + ii)) + 14, "jumpdest", 8) &&
         string_is_method_name(*(*(*matches_final_arr + ii)) + 23)) { // offset to <method> in "     ; jumpdest <method>"

      char *signature = NEW_LINE;
      char *event_name;
      char **argument_names = NEW_ARR;
      char **type_args = NEW_ARR;
      size_t *event_indexed1_unindexed2_arr = 0;
      parse_method_name(*(*(*matches_final_arr + ii)) + 23, &signature, &event_name, &type_args, &argument_names, &event_indexed1_unindexed2_arr);
      FREE_ARR(argument_names);
      FREE_ARR(type_args);
      sentence_arr[0] = strdup("jumpdest");
      // sentence_arr[0] = NEW_WORD;
      // memcpy(sentence_arr[0], *(*(*matches_final_arr + ii)) + 14, 8);
      sentence_arr[1] = signature;
      // printf("%s\n", *(*(*matches_final_arr + ii)));
      // getchar();
    } else {
      size_t sentence_count = words_from_sentence_line(*(*(*matches_final_arr + ii) + 0), &sentence_arr);
      if (sentence_count < 2) continue;
      sentence_arr += 1;       //;
    }

    size_t macro_sentence_count = words_from_sentence_line(*(*(*matches_final_arr + ii) + 1), &macro_sentence_arr);
    REALLOC_ARR(macro_sentence_arr);
    if (macro_sentence_count < 3) continue;
    macro_sentence_arr += 2; //; ##


    // Form arguments_arr
    char **arguments_arr = NEW_ARR;
    for (size_t x=0; macro_sentence_arr[x]; ++x) {
      char *operand = macro_sentence_arr[x];
      if (operand[0] == '^') {
        char *word = sentence_arr[x+1];

        // if (string_is_method_name(word)) {
        //   printf("%s    \n", word);
        //   getchar();
        //
        // }

        char *var_name_str = NEW_WORD;
        char *var_index_str = NEW_WORD;
        parse_macro_argument(word, &var_name_str, &var_index_str);
        REALLOC_STR(var_name_str);
        REALLOC_STR(var_index_str);

              // printf("<%s>\n", operand);
              // printf("<%s>\n", var_name_str);
              // printf("<%s>\n", var_index_str);
              // getchar();

        size_t var_index = string_index_in_arr(var_name_str, variables);
        size_t const_index = string_index_in_arr(var_name_str, constants);
        size_t variable_array_index = string_index_in_arr(var_name_str, variable_arrays_arr);

        /// Add new variable if not present
        if (is_variable(var_name_str)) {
          if (!var_index_str[0]) {
            //new variable
            if (var_index == -1 && const_index == -1) {
              *(variables + variables_count) = to_uppercase(var_name_str);
              var_index = variables_count;
              ++variables_count;
              *(variables + variables_count) = 0;
            }
          }
          else if (is_variable(var_index_str)) {
          //new variable found in subscript
            var_index = string_index_in_arr(var_index_str, variables);
            const_index = string_index_in_arr(var_index_str, constants);
            if (var_index == -1 && const_index == -1) {
              *(variables + variables_count) = to_uppercase(var_index_str);
              var_index = variables_count;
              ++variables_count;
              *(variables + variables_count) = 0;
            }
          }
          if (var_index_str[0]) {
          //new array
            if (variable_array_index == -1) {
              *(variable_arrays_arr + variable_arrays_count) = to_uppercase(var_name_str);
              variable_array_index = variable_arrays_count;
              ++variable_arrays_count;
              *(variable_arrays_arr + variable_arrays_count) = 0;
            }
          }
        }

        // Save argument to arguments_arr, needed to update macro operands from sentence
        size_t argument_index = atoi_from_string(operand + 1) - 1;
        if (-1 == argument_index || argument_index > 0xff)
          { printf("ERROR: Wrong ^ macro parameter\n"); exit(1);}
        arguments_arr[argument_index] = strdup(word);
        ++argument_index;
        arguments_arr[argument_index] = 0;
      }
    }  //for x
    *(variables + variables_count) = 0;
    *(variable_arrays_arr + variable_arrays_count) = 0;

    // Update macro operands from sentence
    size_t i = 0;
    while (*(*(*matches_final_arr + ii) + i)) {
      char *instr = NEW_WORD;
      char *operand = NEW_LINE;
      char *updated_operand;
      parse_asm_line(*(*(*matches_final_arr + ii) + i), &instr, &operand);
      if (operand) {
        if (operand[0] == '^') {
          size_t argument_index = atoi_from_string(operand + 1) - 1;
          if (argument_index != -1 && arguments_arr[argument_index]) {
            updated_operand = strdup(arguments_arr[argument_index]);
          }
          FREE(*(*(*matches_final_arr + ii) + i));

          // if (string_is_method_name(updated_operand)) {
          //   // printf("%s\n", updated_operand);
          //   // getchar();
          //   char *signature = NEW_LINE;
          //   char *event_name;
          //   char **argument_names = NEW_ARR;
          //   char **type_args = NEW_ARR;
          //   size_t *event_indexed1_unindexed2_arr = 0;
          //   parse_method_name(updated_operand, &signature, &event_name, &type_args, &argument_names, &event_indexed1_unindexed2_arr);
          //   FREE_ARR(argument_names);
          //   FREE_ARR(type_args);
          //   asprintf((*(*matches_final_arr + ii) + i), "%s %s", instr, signature);
          // }
          // else if (is_hex(updated_operand)) {
          if (string_is_function_name_or_fallback(updated_operand) || is_hex(updated_operand)) {
            asprintf((*(*matches_final_arr + ii) + i), "%s %s", instr, updated_operand);
          } else {
            asprintf((*(*matches_final_arr + ii) + i), "%s %s", instr, to_uppercase(updated_operand));
          }
        }
      }
      ++i;
    }
  }  //for ii
  REALLOC_ARR(variables);
  REALLOC_ARR(variable_arrays_arr);
  for (size_t ii=0; *(*matches_final_arr + ii); ++ii) {
    char **arr = *(*matches_final_arr + ii);
    update_match_lines(ii, constants, constants_bodies, &variables, &variable_arrays_arr, &arr);
  }
  // print_arr("VARS", variables);
  // print_arr("VAR_ARRS", variable_arrays_arr);
  // print_arr("constants", constants);
  // print_arr("constants_b", constants_bodies);
}




char* terms_compile(const char *source_filename, FILE *file, char ***errors_arr) {
  size_t ii,i;


  char *l = NEW_LINE;
  size_t all_lines_count = 0;
  char **all_lines = NEW_BIG_ARR;
  while (fgets(l, 4096, file)) {
    asprintf(&all_lines[all_lines_count], "%s", l);
    ++all_lines_count;
  }
  fclose(file);

  char *filename_ttp;
  asprintf(&filename_ttp, "%s.ttp", source_filename);
  FILE *file_ttp = fopen(filename_ttp, "r");
  if (file_ttp) {
    while (fgets(l, 4096, file)) {
      asprintf(&all_lines[all_lines_count], "%s", l);
      ++all_lines_count;
    }
    fclose(file_ttp);
  }
  FREE(filename_ttp);
  

  FREE(l);




  //form text, catch pre-contract defines
  char *line = NEW_LINE;
  char *text = NEW_HUGE_ARR;
  text[0]=0;
  size_t letter_count = 0;
  fseek(file, 0, SEEK_SET);
  bool inside_contract = false;
  bool inside_conditions = false;
  size_t events_count = 0;
  char **event_strings_arr = NEW_ARR;

  all_lines_count = 0;
  while (all_lines[all_lines_count]) {
    sprintf(line, "%s", all_lines[all_lines_count]);
    ++all_lines_count;

  // while (fgets(line, 4096, file)) {
    // print("LINE {%s}\n", line);
    // // print_arr("LINE", line_arr);
    // //
    // printf("LLLL [%s]\n", all_lines[all_lines_count]);
    // getchar();


    char **line_arr = NEW_ARR;
    size_t words_count = words_from_sentence_line(line, &line_arr);
    REALLOC_ARR(line_arr);

    // print("LINE %s", line);
    // print_arr("LINE", line_arr);
    // getchar();
    //
    if (!inside_contract) {
      if (line_arr[0]) {
        if (!memcmp(to_uppercase(line_arr[0]), "CONTRACT", 8)) {
          inside_contract = true;
          continue;
        }
// #ifdef VERSION_CUSTOM_REVISIONS
//         if (!strcasecmp(line_arr[0], "TERMS") && !strcasecmp(line_arr[1], "REVISION") && line_arr[2]) {
//           FREE(terms_revision);
//           terms_revision = to_lowercase(line_arr[2]);
//           print("Override default revision with terms revision %s\n", terms_revision);
// # ifdef VERSION_MASTER_SIGN
//           sign_terms_revision_file(terms_revision);
// # endif
//         }
// #endif
      }
      continue;
    }
    else {    // if inside_contract
      if (line_arr[0]) {
        if (!memcmp(to_uppercase(line_arr[0]), "CONDITIONS", 10)) {
          inside_conditions = true;
          strcpy(line, "CONDITIONS:\n");
        }
      }

//TODO REWRITE TO MAKE SEQ_ARR for abi
      if (inside_conditions) {

        char *method_is_constant = "jumpdest ";
        char *method_str = NEW_LINE;
        if (string_is_method_name(line)) {
          strcpy(method_str, line);
        } else if (strlen(line) > 9) {
          if (!memcmp(line, "constant", 8) && string_is_method_name(line + 9)) {
            strcpy(method_str, line + 9);
            method_is_constant = "constant ";
          }
          if (!memcmp(line, "jumpdest", 8) && string_is_method_name(line + 9)) {
            strcpy(method_str, line + 9);
            method_is_constant = "jumpdest ";
          }
        }
        if (method_str[0]) {
          char *signature = NEW_LINE;
          char *event_name;
          char **argument_names = NEW_ARR;
          char **type_args = NEW_ARR;
          size_t *event_indexed1_unindexed2_arr = 0;
          parse_method_name(method_str, &signature, &event_name, &type_args, &argument_names, &event_indexed1_unindexed2_arr);
          REALLOC_ARR(argument_names);
          REALLOC_ARR(type_args);

          // sprintf(line, "%s%s. ", method_is_constant, signature);
          method_str[strlen(method_str)-1] = '.';
          sprintf(line, "%s%s", method_is_constant, method_str);
          // printf("%s\n", line);
          // getchar();

          size_t y=0;
          char *line_p = line + strlen(line);
          while (argument_names[y]) {
            if (argument_names[y][0]) {
              // printf("type_args %s\n", type_args[y]);
              // getchar();
              if (!strcmp(type_args[y], "string")) {
                sprintf(line_p, "Let %s #0 get string argument %zu. ", to_uppercase(argument_names[y]), y+1);
              } else {
                sprintf(line_p, "Let %s get argument %zu. ", to_uppercase(argument_names[y]), y+1);
              }
              line_p += strlen(line_p);
            }
            ++y;
          }
          line_p[0] = '\n';
          ++line_p;
          line_p[0] = 0;
          ++line_p;

          // printf("signature %s\n", signature);
          // printf("))))) %s\n", line);
          // getchar();


          // print_arr("type_args", type_args);
          // print_arr("argument_names", argument_names);
          // printf("%s\n", event_name);
          // printf("count %zu\n", arr_count(argument_names));
          // getchar();


          FREE(signature);
      }

      } else {
        if (line_arr[0]) {

          // Register event
          if (!memcmp(to_uppercase(line), "EVENT ", 6)) {
            char *event_str = strdup(line + 6);
            event_str[strlen(event_str)-1] = 0;
            event_strings_arr[events_count] = event_str;

            char *signature = NEW_LINE;
            char *event_name;
            char **argument_names = NEW_ARR;
            char **type_args = NEW_ARR;
            size_t *event_indexed1_unindexed2_arr = 0;
            parse_method_name(event_str, &signature, &event_name, &type_args, &argument_names, &event_indexed1_unindexed2_arr);
            sprintf(line, "Write 0x%s to record %s EVENT.\n", keccak_hash(signature), to_uppercase(event_name));
            REALLOC_ARR(argument_names);
            REALLOC_ARR(type_args);
            FREE(signature);
            ++events_count;
          }
        }

      }

      // // Remove "constant" modifier
      // if (!memcmp(line, "constant ", 9)) {
      //   if (string_is_method_name(line+9)) {
      //     sprintf(text + strlen(text), "%s.", line+9);
      //     letter_count = strlen(text);
      //     continue;
      //   }
      // }

      // char **line_arr = NEW_ARR;
      // size_t words_count = words_from_sentence_line(line, &line_arr);
      // REALLOC_ARR(line_arr);
      // if (words_count == 1 || (words_count == 2 && strcmp(line_arr[0], "constant"))) {
      //   if (string_is_function_name_or_fallback(line_arr[words_count - 1])) {
      //     text[letter_count] = 0;
      //     sprintf(text + strlen(text), "%s.", line);
      //     letter_count = strlen(text);
      //     continue;
      //   }
      // }
    }
    size_t inside_braces = 0;
    i=0;
    while (line[i] != '\n' && !(line[i] == '/' && line[i+1] == '/')) {

      // Replace dots between digits with _
      if ((line[i] >= '0' && line[i] <= '9') && (line[i+1] == '.') && (line[i+2] >= '0' && line[i+2] <= '9')) {
        line[i+1] = '_';
        // if (!((line[i+3] >= '0' && line[i+3] <= '9') || line[i+3] == '.')) {  // not number
        //   line[i+3] = '.';
        // }
      }

      // Insert space before ','
      if (line[i] == '(') ++inside_braces;
      if (line[i] == ')') --inside_braces;
      if (!inside_braces) {
        if (line[i] == ',') {
          text[letter_count] = ' ';
          text[letter_count+1] = ',';
          letter_count += 2;
          ++i;
          continue;
        }
      }

      text[letter_count] = line[i];
      ++i;
      ++letter_count;
    }
    text[letter_count]='\n';
    ++letter_count;
  }
  strcpy(text + letter_count, "0.return.\n");
  letter_count = strlen(text);
  event_strings_arr[events_count] = 0;
  text[letter_count] = 0;
  REALLOC_STR(text);
print("TEXT:\n%s\n", text);
  FREE(all_lines);


  // Form sentence_lines_arr
  char *text_p = text;
  char **sentence_lines_arr = NEW_BIG_ARR;
  size_t sentence_lines_count = 0;
  size_t y=0;
  i=0;
  while (i < strlen(text)) {
    char *sentence_line = NEW_LINE;
    y=0;
    while (text_p[i] == '\n' || text_p[i] == '\t' || text_p[i] == ' ') ++i;
    while (text_p[i] != '.' && text_p[i] != ';' && text_p[i] != ':' && i < strlen(text)) {
      if (text_p[i] == '\n' || text_p[i] == '\t') {
        sentence_line[y] = ' ';
      } else {
        sentence_line[y] = text_p[i];
      }
      ++i;
      ++y;
    }
    if (y) {
      sentence_line[y] = 0;
      sentence_lines_arr[sentence_lines_count] = sentence_line;
      ++sentence_lines_count;
    }
    ++i;
  }
  sentence_lines_arr[sentence_lines_count] = 0;

//  print_arr("sentence_lines_arr", sentence_lines_arr);


  // Form sentences_arr
  size_t *case_depth = calloc(10000, 1);
  size_t case_level = 0;
  char ***sentences_arr = NEW_BIG_ARR;
  size_t sentences_count = 0;
  ii=0;
  while (sentence_lines_arr[ii]) {
    char **words_arr = NEW_ARR;

    //  1. => {jumpdest 1}
    if (!sentence_lines_arr[ii][0]) continue;
    if (string_is_jumptag(sentence_lines_arr[ii]) || string_is_method_name(sentence_lines_arr[ii])) {
      words_arr[0] = strdup("jumpdest");
      words_arr[1] = strdup(sentence_lines_arr[ii]);
      words_arr[2] = 0;
      REALLOC_ARR(words_arr);
      sentences_arr[sentences_count] = words_arr;
      ++sentences_count;
      ++ii;
      continue;
    }

    size_t words_count = words_from_sentence_line(sentence_lines_arr[ii], &words_arr);
    if (!words_count) {
      ++ii;
      continue;
    }

    // Replace TERMS_VARIABLE_SEQUENCE_MODIFIER with #0, "..." with 0x0...
    i=0;
    while (words_arr[i]) {
      if (string_ends_with(words_arr[i], TERMS_VARIABLE_SEQUENCE_MODIFIER)) {
        strcpy(words_arr[i] + strlen(words_arr[i])-strlen(TERMS_VARIABLE_SEQUENCE_MODIFIER), " #0");
      }
      if (words_arr[i+1]) {
        if (!strcmp(words_arr[i], "string")) {
          char *str = words_arr[i+1];
          if (str[0] == '\"' && str[strlen(str)-1] == '\"') {
            char *str_cut = strdup(words_arr[i+1]);
            str_cut[strlen(str_cut)-1] = 0;
            strcpy(words_arr[i+1], hex_from_string(str_cut + 1));
            FREE(str_cut);
          }
        }
      }
      ++i;
    }


    // Replace "if"/"if not" with expression inserts
    bool expression_negative;
    bool has_else;
    char *then = NEW_LINE;
    char *ifelse = NEW_LINE;
    char *expression = expression_from_if_sentence(sentence_lines_arr[ii], &expression_negative, &then, &ifelse, &has_else);
    if (expression) {
      FREE_ARR(words_arr);
      ++case_level;
      case_depth[case_level] += 1;

      // printf("input  [%s]", sentence_lines_arr[ii]);
      // getchar();

      char **expr_arr = NEW_ARR;
      words_from_expression(expression, &expr_arr);
      // print_arr("expr_arr", expr_arr);
      arr_reverse(&expr_arr);
      // print_arr("reverse expr_arr", expr_arr);


      char **expression_inserts = NEW_ARR;
      size_t expression_inserts_count = expression_infix_to_rpn(expr_arr, &expression_inserts);
      if (!expression_negative) {
        asprintf(&expression_inserts[expression_inserts_count], "comp not");
        ++expression_inserts_count;
      }
      asprintf(&expression_inserts[expression_inserts_count], "false %zu", 1000 * case_level + case_depth[case_level]);
      ++expression_inserts_count;

      // if (then[0]) {
      //   asprintf(&expression_inserts[expression_inserts_count], "%s", then);
      //   ++expression_inserts_count;
      //   asprintf(&expression_inserts[expression_inserts_count], "end %zu", 1000 * case_level + case_depth[case_level]);
      //   ++expression_inserts_count;
      //   --case_level;
      // }

      if (then[0]) {
        asprintf(&expression_inserts[expression_inserts_count], "%s", then);
        ++expression_inserts_count;
        if (ifelse[0]) {
          asprintf(&expression_inserts[expression_inserts_count], "else %zu", 1000 * case_level + case_depth[case_level]);
          ++expression_inserts_count;
          asprintf(&expression_inserts[expression_inserts_count], "%s", ifelse);
          ++expression_inserts_count;
        }

        if (!ifelse[0] && has_else) {
        }
        else {
          asprintf(&expression_inserts[expression_inserts_count], "end %zu", 1000 * case_level + case_depth[case_level]);
          ++expression_inserts_count;
          --case_level;
        }
      }



      // if (then[0]) {
      //   asprintf(&expression_inserts[expression_inserts_count], "%s", then);
      //   ++expression_inserts_count;
      // }
      // if (ifelse[0]) {
      //   asprintf(&expression_inserts[expression_inserts_count], "else %zu", 1000 * case_level + case_depth[case_level]);
      //   ++expression_inserts_count;
      //   asprintf(&expression_inserts[expression_inserts_count], "%s", ifelse);
      //   ++expression_inserts_count;
      // }
      // if (!then[0] || (has_else && !ifelse[0])) {
      //   asprintf(&expression_inserts[expression_inserts_count], "end %zu", 1000 * case_level + case_depth[case_level]);
      //   ++expression_inserts_count;
      //   --case_level;
      // }


      expression_inserts[expression_inserts_count] = 0;
      // print_arr("expression_inserts", expression_inserts);
      //  print_arr("expr_arr", expr_arr);
      //  getchar();

      i=0;
      while (expression_inserts[i]) {
        // printf("  expression_inserts[i]  %s\n", expression_inserts[i]);
        // printf("%zu\n", i);
        char **arr = NEW_ARR;
        words_from_sentence_line(expression_inserts[i], &arr);
        REALLOC_ARR(arr);
        // print_arr("W", arr);
        // getchar();

        sentences_arr[sentences_count] = arr;

        ++sentences_count;
        ++i;
      }
      FREE_ARR(expression_inserts);
      ++ii;
      continue;
    }
    else if (case_comp(words_arr[0], "else")) {
      FREE_ARR(words_arr);
      char **arr = NEW_ARR;
      arr[0] = strdup("else");
      asprintf(&arr[1], "%zu", 1000 * case_level + case_depth[case_level]);
//      printf("true %s\n", arr[1]);
      arr[2] = 0;
      REALLOC_ARR(arr);
      sentences_arr[sentences_count] = arr;
      ++sentences_count;
      ++ii;
      continue;
    }
    else if (case_comp(words_arr[0], "end")) {     // "end"
      FREE_ARR(words_arr);
      char **arr = NEW_ARR;
      arr[0] = strdup("end");
      asprintf(&arr[1], "%zu", 1000 * case_level + case_depth[case_level]);
      // printf("end %s\n", arr[1]);
      arr[2] = 0;
      REALLOC_ARR(arr);
      sentences_arr[sentences_count] = arr;
      ++sentences_count;
      ++ii;
      --case_level;
      continue;
    }

    REALLOC_ARR(words_arr);
    sentences_arr[sentences_count] = words_arr;
    ++sentences_count;
    ++ii;
  }
  FREE(text);
  FREE(line);
  FREE_ARR(sentence_lines_arr);
  FREE(case_depth);


  // for (size_t ii = 0; ii < sentences_count; ++ii) {
  //   print_arr("SENTENCES", sentences_arr[ii]);
  // }
  // getchar();

  // Get abi
  char *out_abi = terms_abi_preprocessor(&sentences_arr, &event_strings_arr);

  // Save abi to file
  char *filename_abi;
  asprintf(&filename_abi, "%s.abi", source_filename);
  FILE *out_file_abi=fopen(filename_abi,"w");
  if (out_file_abi==0) {
    printf("ERROR: Unable to open file for writing: %s\n", filename_abi);
    exit(1);
  }
  fseek(out_file_abi, 0, SEEK_SET);
  fputs(out_abi, out_file_abi);
  fclose(out_file_abi);
  FREE(filename_abi);


  // Replace constant with jumpdest
  for (ii=0; sentences_arr[ii]; ++ii) {
    for (i = 0; sentences_arr[ii][i]; ++i) {
      if (!strcmp(sentences_arr[ii][i], "constant")) {
        sentences_arr[ii][i] = strdup("jumpdest");
      } else REALLOC_STR(sentences_arr[ii][i]);
    }
  }
  REALLOC_STRPTR_ARR(sentences_arr);



  //form macros_head_arr
  //form macros_body_arr
  char **constants = NEW_ARR;
  char **constants_bodies = NEW_ARR;
  char ***macros_head_arr = NEW_BIG_ARR;
  char ***macros_body_arr = NEW_BIG_ARR;
  form_macros_arrays_for_revision("develop",
    &constants, &constants_bodies, &macros_head_arr, &macros_body_arr);
  REALLOC_ARR(constants);
  REALLOC_ARR(constants_bodies);
  REALLOC_STRPTR_ARR(macros_head_arr);
  REALLOC_STRPTR_ARR(macros_body_arr);


  //form matches_final_arr from sentences_arr
  char ***matches_final_arr = NEW_BIG_ARR;
  size_t matches_count = 0;
  size_t errors_count = 0;
  for (size_t ii = 0; sentences_arr[ii]; ++ii) {
    char **match = NEW_ARR;
    match[0] = NEW_LINE;
    match[1] = NEW_LINE;
    match[2] = NEW_LINE;
    match[3] = NEW_LINE;
    matches_final_arr[ii] = NEW_ARR;
    //source line as a comment
    char *comment_str = NEW_LINE;
    strcpy(comment_str, "            ;");
    for (size_t x = 0;  sentences_arr[ii][x] != 0; ++x) {
      strcat(comment_str, " ");
      strcat(comment_str, sentences_arr[ii][x]);
    }
    for (i = 0; macros_head_arr[i]; ++i) {
      //macro line as a comment
      char *comment_str1 = NEW_LINE;
      strcpy(comment_str1, "            ;");
      for (size_t x = 0;  macros_head_arr[i][x] != 0; ++x) {
        strcat(comment_str1, " ");
        strcat(comment_str1, macros_head_arr[i][x]);
      }
      if (sentence_matches_macro(sentences_arr[ii], macros_head_arr[i])) {
        match[0] = comment_str;
        match[1] = comment_str1;
        char **tmp = match + 2;
        arr_copy(&tmp, macros_body_arr[i]);
        matches_final_arr[matches_count] = match;
        ++matches_count;
        break;
      }
    }
    if (!match[2][0]) {
      *(*errors_arr + errors_count) = comment_str;
      ++errors_count;
    }
  }
  matches_final_arr[matches_count] = 0;


  // for (size_t ii = 0; ii < sentences_count; ++ii) {
  //   print_arr("SENTENCES", sentences_arr[ii]);
  // }
  for (size_t ii=0; sentences_arr[ii]; ++ii) {
    FREE_ARR(sentences_arr[ii]);
  }
  REALLOC_STRPTR_ARR(sentences_arr);




  // for (size_t ii = 0; matches_final_arr[ii][0]; ++ii) {
  //   print_arr("matches_final_arr", matches_final_arr[ii]);
  // }
  // getchar();

  parse_matches_final_arr(constants, constants_bodies, &matches_final_arr);

  // for (size_t ii=0; macros_head_arr[ii]; ++ii) {
  //   FREE_ARR(macros_head_arr[ii]);
  // }
  // FREE(macros_head_arr);

  for (size_t ii=0; macros_body_arr[ii]; ++ii) {
    FREE_ARR(macros_body_arr[ii]);
  }
  FREE(macros_body_arr);

  // printf("---------------------------------\n");
  // for (size_t ii = 0; macros_body_arr[ii] != 0; ++ii) {
  //   printf("BODIES[%zu]\n", ii);
  //   print_arr("BODY", macros_body_arr[ii]);
  // }
  // for (size_t ii = 0; macros_instr_arr[ii] != 0; ++ii) {
  //   printf("INSTR[%zu]\n", ii);
  //   print_arr("INSTR", macros_instr_arr[ii]);
  // }
  // for (size_t ii = 0; macros_operand_zarr[ii] != 0; ++ii) {
  //   printf("OPERAND[%zu]\n", ii);
  //   print_arr("INSTR", macros_operand_zarr[ii]);
  // }
  // for (size_t ii = 0; matches_final_arr[ii] != 0; ++ii) {
  //   printf("MATCHES[%zu]\n", ii);
  //   print_arr("MATCH", matches_final_arr[ii]);
  // }

//  printf("line %i\n", __LINE__);

  char *out = NEW_BIG_ARR;
  char *out_p = out;
  for (size_t ii = 0; matches_final_arr[ii] != 0; ++ii) {
    for (size_t y = 0; matches_final_arr[ii][y] != 0; ++y) {
      strcpy(out_p, matches_final_arr[ii][y]);
      out_p += strlen(matches_final_arr[ii][y]);
      out_p[0] = '\n';
      ++out_p;
      FREE(matches_final_arr[ii][y]);
    }
  }
  REALLOC_STRPTR_ARR(matches_final_arr);
  REALLOC_STR(out);

  char *filename;
  asprintf(&filename, "%s.asm", source_filename);
  FILE *out_file=fopen(filename,"w");
  if (out_file==0) {
    printf("ERROR: Unable to open file for writing: %s\n", filename);
    exit(1);
  }
  fseek(out_file, 0, SEEK_SET);
  fputs(out, out_file);
  fclose(out_file);

  // printf("line %i\n", __LINE__);


  // printf("OUT>>>>>>>>>>>>>>>>>>>>>>  \n%s\n", out);
  FREE(out);
  char *out_hex = emasm_compile(filename);

  // Save emasm output to file
  char *out_hex0x;
  asprintf(&out_hex0x, "0x%s", out_hex);
  char *filename_hex;
  asprintf(&filename_hex, "%s.hex", source_filename);
  FILE *out_file_hex=fopen(filename_hex,"w");
  if (out_file_hex==0) {
    printf("ERROR: Unable to open file for writing: %s\n", filename_hex);
    exit(1);
  }
  fseek(out_file_hex, 0, SEEK_SET);
  fputs(out_hex0x, out_file_hex);
  fclose(out_file_hex);
  FREE(filename_hex);
  FREE(out_hex0x);

  // printf("line %i\n", __LINE__);

  if (!out_hex || !out_abi) {
    printf("MEMORY FAIL\n");
    return NULL;
  }
  char *out_js;
  // asprintf(&out_js, "eth.defaultAccount = eth.coinbase;\nvar abi = %s;\nvar data = \'0x%s\';\nvar aContract = web3.eth.contract(abi);\nvar con = aContract.new({from: web3.eth.accounts[0], data: data, gas: '4700000'}, function (e, contract){console.log(e, contract); if (typeof contract.address !== 'undefined'){console.log('Contract mined! address: ' + contract.address + ' transactionHash: ' + contract.transactionHash);}})\n\n",
  asprintf(&out_js, "eth.defaultAccount = eth.coinbase;\nvar abi = %s;\nvar data = \'0x%s\';\nvar aContract = web3.eth.contract(abi);\nvar con = aContract.new({from: web3.eth.accounts[0], data: data, gas: '4700000'}, function (e, contract){console.log(e, contract); if (typeof contract.address !== 'undefined'){console.log('Contract mined! address: ' + contract.address + ' transactionHash: ' + contract.transactionHash); var events = con.allEvents(); events.watch(function(error, event){ if (!error) console.log(\"EVENT:\\n\", JSON.stringify(event));});}})\n\n", out_abi, out_hex);

  // event.stopWatching();

  // printf("line %i\n", __LINE__);

  if (out_js) {
    asprintf(&filename, "%s.js", source_filename);
    FILE *out_file=fopen(filename,"w");
    if (out_file==0) {
      printf("ERROR: Unable to open file for writing: %s\n", filename);
      exit(1);
    }
    fseek(out_file, 0, SEEK_SET);
    fputs(out_js, out_file);
    fclose(out_file);
    print("SUCCESS! Compiler output saved to %s\n", filename);


    printf("\n%s\n\n\n", out_js);


  }


  return out_hex;
}
