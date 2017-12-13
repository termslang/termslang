/**
* Copyright (c) 2017
* Mikhail Baynov m.baynov@gmail.com
* Distributed under the GNU GPL v2
*/


void get_function_name_and_parameters(const char *str, char **name, char ***parameters) {
    size_t stage = 0;
    size_t off = 0;
    size_t x=0;
    **parameters = NEW_WORD;
    while (str[x]) {
      if (str[x] == ')') {
        *(*name + x) = 0;
        *(*parameters + stage) = 0;
        return;
      }
      if ((str[x]) == '(' || (str[x]) == ',') {
        ++stage;
        *(*parameters + stage) = NEW_WORD;
        ++x;
        off = x;
        continue;
      }
      if (stage == 0) {
        *(*name + x) = str[x];
      }
      else {
        *(*(*parameters + stage-1) + x-off) = str[x];
      }
      ++x;
    }
    *(*name + x) = 0;
    *(*parameters + stage) = 0;
}


char* terms_abi_preprocessor(char ****sentences_arr, char ***event_strings_arr) {
  //TODO       printf("ERROR: Stop or return instruction missing in function {%s}\n", f.name);

  struct function {
    char name[256];
    bool constant;
    char **input_types;
    char **input_names;
    char output_type[256];
  };
  struct function *functions = NEW_ARR;
  size_t functions_count = 0;
  size_t ii;
  size_t i;
  for (ii=0; *(*sentences_arr + ii); ++ii) {
    for (i=0; *(*(*sentences_arr + ii) + i); ++i) {
      struct function f = {0};
      if (string_is_method_name(*(*(*sentences_arr + ii) + i))) {
        // printf("%s\n", *(*(*sentences_arr + ii) + i));
        // getchar();

        char *signature = NEW_LINE;
        char *event_name;
        char **argument_names = NEW_ARR;
        char **type_args = NEW_ARR;
        size_t *event_indexed1_unindexed2_arr = 0;
        parse_method_name(*(*(*sentences_arr + ii) + i), &signature, &event_name, &type_args, &argument_names, &event_indexed1_unindexed2_arr);
        REALLOC_ARR(argument_names);
        REALLOC_ARR(type_args);

        f.input_types = type_args;
        f.input_names = argument_names;
        strcpy(f.name, event_name);
        if (case_comp(*(*(*sentences_arr + ii)), "constant")) {
          f.constant = 1;
        }
        functions[functions_count] = f;
        ++functions_count;
      }
      else if (case_comp(*(*(*sentences_arr + ii)), "return")) {
        char *ret = *(*(*sentences_arr + ii) + 1);
        if (ret) {
          strcpy(functions[functions_count-1].output_type, to_lowercase(ret));
        }
      }

    }
  }

  char *text = NEW_BIG_ARR;
  char *text_p = text;
  text_p[0] = '[';
  ++text_p;
  for (ii=0; functions[ii].name[0]; ++ii) {
    if (ii>0) {
      text_p[0] = ',';
      ++text_p;
    }
    struct function f = functions[ii];
    strcpy(text_p, "{\"constant\":");
    text_p += strlen(text_p);
    if (f.constant) {
      strcpy(text_p, "true");
    } else {
      strcpy(text_p, "false");
    }
    text_p += strlen(text_p);
    strcpy(text_p, ",\"inputs\":[");
    text_p += strlen(text_p);
    size_t i;
    for (i=0; f.input_types[i]; ++i) {
      if (f.input_types[i][0]) {
        if (i>0) {
          text_p[0] = ',';
          ++text_p;
        }
        sprintf(text_p, "{\"name\":\"%s\",\"type\":\"%s\"}", f.input_names[i], f.input_types[i]);
        text_p += strlen(text_p);
      }
    }
    sprintf(text_p, "],\"name\":\"%s\",\"outputs\":[", f.name);
    text_p += strlen(text_p);
    if (f.output_type[0]) {
      sprintf(text_p, "{\"name\":\"output\",\"type\":\"%s\"}", f.output_type);
      text_p += strlen(text_p);
    }
    sprintf(text_p, "],\"payable\":false,\"type\":\"function\"}");
    text_p += strlen(text_p);
  }
  if (strlen(text) < 3) return 0;
  sprintf(text_p, ",{\"inputs\":[],\"payable\":false,\"type\":\"constructor\"},{\"payable\":true,\"type\":\"fallback\"}");
  text_p += strlen(text_p);


  ii=0;
  while (*(*event_strings_arr + ii)) {
    char *event_str = *(*event_strings_arr + ii);

    char *signature = NEW_LINE;
    char *event_name;
    char **argument_names = NEW_ARR;
    char **type_args = NEW_ARR;
    size_t *event_indexed1_unindexed2_arr = NEW_LINE;
    print("event_str [%s]\n", event_str);
    parse_method_name(event_str, &signature, &event_name, &type_args, &argument_names, &event_indexed1_unindexed2_arr);
    REALLOC_STR(signature);
    REALLOC_ARR(argument_names);
    REALLOC_ARR(type_args);
    //
    // printf("signature %s\n", signature);
    // print_arr("argument_names", argument_names);
    // print_arr("type_args", type_args);
    // printf("%s\n", event_name);
    // printf("hash %s\n", keccak_hash(signature));
    // for (i = 0; event_indexed1_unindexed2_arr[i]; i++) {
    //   printf("<<<< %zu\n", event_indexed1_unindexed2_arr[i]);
    // }
    // getchar();
    //

    text_p[0] = ',';
    ++text_p;
    sprintf(text_p, "{\"anonymous\":false,\"inputs\":[");
    text_p += strlen(text_p);

    size_t i=0;
    while (type_args[i]) {
      // printf("<%s>\n", type_args[i]);
      // getchar();
      char *indexed = (event_indexed1_unindexed2_arr[i] == 1) ? "true" : "false";
      sprintf(text_p, "{\"indexed\":%s,\"name\":\"%s\",\"type\":\"%s\"}", indexed, argument_names[i], type_args[i]);
      text_p += strlen(text_p);
      ++i;
      if (type_args[i]) {
        text_p[0] = ',';
        ++text_p;
      }
    }
    sprintf(text_p, "],\"name\":\"%s\",\"type\":\"event\"}", event_name);
    text_p += strlen(text_p);


    ++ii;
  }
  sprintf(text_p, "]");
  text_p += strlen(text_p);


  REALLOC_STR(text);
  return text;
}
