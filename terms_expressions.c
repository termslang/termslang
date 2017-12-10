/**
* Copyright (c) 2017
* Mikhail Baynov m.baynov@gmail.com
* Distributed under the GNU GPL v2
*/


char* expression_from_if_sentence(const char *sentence, bool* expression_negative, char** then, char** ifelse, bool* has_else) {
  if (!sentence) return 0;
  if (strlen(sentence) < 4) return 0;
  size_t ii=0;
  *has_else = false;
  while (sentence[ii] == ' ') ++ii;
  if (sentence[ii] == 'i' || sentence[ii] == 'I') ++ii; else return 0;
  if (sentence[ii] == 'f') ++ii; else return 0;
  while (sentence[ii] == ' ') ++ii;
  if (sentence[ii] == 'n' && sentence[ii+1] == 'o' && sentence[ii+2] == 't') {
    *expression_negative = true;
    ii += 3;
    while (sentence[ii] == ' ') ++ii;
  }
  else {
    *expression_negative = false;
  }
  size_t expr_start = ii;
  char *out = NEW_LINE;
  while (sentence[ii] && sentence[ii] != ',') {
    out[ii - expr_start] = sentence[ii];
    ++ii;
  }
  out[ii - expr_start]=0;
  REALLOC_STR(out);

  if (sentence[ii] == ',') {
    ++ii;
    size_t then_count=0;
    while (sentence[ii] && sentence[ii] != ',') {
      *(*then + then_count) = sentence[ii];
      ++ii;
      ++then_count;
    }
    *(*then + then_count) = 0;
  }


  if (sentence[ii] == ',') {
    ++ii;
    while (sentence[ii] == ' ') ++ii;
    if (sentence[ii] == 'e' && sentence[ii+1] == 'l' && sentence[ii+2] == 's' && sentence[ii+3] == 'e') {
      *has_else = true;
      ii += 4;
      while (sentence[ii] == ' ') ++ii;
    }
    size_t ifelse_count=0;
    while (sentence[ii]) {
      *(*ifelse + ifelse_count) = sentence[ii];
      ++ii;
      ++ifelse_count;
    }
    *(*ifelse + ifelse_count) = 0;
  }


  // printf("expression_from_if_sentence %s\n", sentence);
  // printf("out %s\n", out);
  // printf("expression_negative %s\n", *expression_negative ? "true":"false");
  // printf("then <%s>\n", *then);
  // printf("ifelse <%s>\n", *ifelse);
  // printf("ifelse[0] <%hhd>\n", **ifelse);
  // printf("has_else %s\n", *has_else ? "true":"false");
  // getchar();
  return out;
}


size_t words_from_expression(const char *line, char ***words) {
 printf("words_from_expression: {%s}\n", line);
  if (!line || *words == 0) return 0;
  char *letters = NEW_LINE;
  size_t words_count = 0;
  size_t letters_count = 0;
  size_t ii=0;
  while (line[ii] == ' ') ++ii;
  while (line[ii]) {
    if  (line[ii] == ' ') {
      if (letters_count) {
        *(*words + words_count) = strdup(letters);
        ++words_count;
        letters[0] = 0;
        letters_count = 0;
      }
    }
    else {
      letters[letters_count] = line[ii];
      ++letters_count;
      letters[letters_count] = 0;
    }
    ++ii;
  }
  if (letters_count) {
    letters[letters_count] = 0;
    *(*words + words_count) = strdup(letters);
    ++words_count;
    letters[0] = 0;
    letters_count = 0;
  }
  *(*words + words_count) = 0;
  FREE(letters);


  // APPEND MULTIPLE WORDS VARIABLES
  size_t yy=0;
  ii=0;
  char *var_str;
  while (*(*words + ii)) {
    *(*words + yy) = *(*words + ii);
    if (is_variable(*(*words + yy))) {
      while (is_variable(*(*words + ii+1))) {
        char *var_str;
        asprintf(&var_str, "%s %s", *(*words + yy), *(*words + ii+1));
        FREE(*(*words + yy));
        FREE(*(*words + ii+1));
        *(*words + yy) = var_str;
        ++ii;
      }
    }
    ++ii;
    ++yy;
  }
  *(*words + yy) = 0;
  return ii;
}


size_t infix_operator_precedence(const char* str) {
  if (!str) return 0;
  if (!strcmp(str, "and") || !strcmp(str, "or")) return 5;
  if (!strcmp(str, ">")  || !strcmp(str, "<"))  return 10;
  if (!strcmp(str, ">=") || !strcmp(str, "<=")) return 10;
  if (!strcmp(str, "==") || !strcmp(str, "!=")) return 10;
  if (!strcmp(str, "+") || !strcmp(str, "-")) return 15;
  if (!strcmp(str, "*") || !strcmp(str, "/")) return 20;
  if (!strcmp(str, "%")) return 20;
  if (!strcmp(str, "^")) return 25;
  return 0;
}
bool infix_operator_associativity_left(const char* str) {
  if (!str) return 0;
  if (!strcmp(str, "^")) return 0;
  return 1;
}

size_t expression_infix_to_rpn(const char **words, char ***expression_inserts) {
  printf("expression_infix_to_rpn\n");
  size_t ii,i;
  print_arr("input", words);


  // print_arr("WORDS", words);

  // Infix to reverse polish notation
  char **operators = NEW_ARR;
  char **output = NEW_ARR;
  size_t operators_count = 0;
  size_t output_count = 0;
  ii=0;
  while (words[ii]) {
    if (infix_operator_precedence(words[ii])) {    //if operator
      while (operators_count) {
        if ((infix_operator_associativity_left(words[ii]) && (infix_operator_precedence(words[ii]) <= infix_operator_precedence(operators[operators_count-1])))
        || (!infix_operator_associativity_left(words[ii]) && (infix_operator_precedence(words[ii]) < infix_operator_precedence(operators[operators_count-1])))) {
          --operators_count;
          output[output_count] = strdup(operators[operators_count]);
          operators[operators_count] = 0;
          ++output_count;
        } else break;
      }
      operators[operators_count] = words[ii];
      ++operators_count;
      operators[operators_count] = 0;
    }
    else if (!strcmp(words[ii], ")")) {
      operators[operators_count] = words[ii];
      ++operators_count;
      operators[operators_count] = 0;
    }
    else if (!strcmp(words[ii], "(")) {
      while (operators_count) {
        if (strcmp(operators[operators_count - 1], ")")) {
          --operators_count;
          output[output_count] = strdup(operators[operators_count]);
          operators[operators_count] = 0;
          ++output_count;
        }
        else { //if (!strcmp(operators[operators_count - 1], "("))
          --operators_count;
          // FREE(operators[operators_count]);
          // operators[operators_count] = 0;
          break;
        }
      }
    }
    else {      //if operand
      output[output_count] = strdup(words[ii]);
      ++output_count;
    }
    ++ii;
  }
  output[output_count] = 0;
  i = 0;
  while (operators_count) {
    --operators_count;
    output[output_count] = strdup(operators[operators_count]);
    operators[operators_count] = 0;
    ++output_count;
    output[output_count] = 0;
    ++i;
  }

  // printf("step       %zu\n", ii);
  // printf("processing \"%s\"\n", words[ii]);

  // print_arr("words", words);
  // print_arr("operators", operators);
  print_arr("output", output);
  // getchar();



  ii=0;
  while (output[ii]) {
    if (infix_operator_precedence(output[ii])) {
      asprintf((*expression_inserts + ii), "comp %s", output[ii]);
    } else asprintf((*expression_inserts + ii), "push %s", output[ii]);
    ++ii;
  }
  *(*expression_inserts + ii) = 0;
  FREE_ARR(output);
  return output_count;
}
