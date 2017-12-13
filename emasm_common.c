/**
* Copyright (c) 2017
* Mikhail Baynov m.baynov@gmail.com
* Distributed under the GNU GPL v2
*/


char* hex_from_digit(size_t num) {
  char *out;
  if (num <= 0xff) {
    asprintf(&out, "0x%02zx", num);
  } else if (num <= 0xffff) {
    asprintf(&out, "0x%04zx", num);
  } else if (num <= 0xffffff) {
    asprintf(&out, "0x%06zx", num);
  } else asprintf(&out, "0x%08zx", num);
  return out;
}

int comp(const char *p1, const char *p2) {
  if ((p1 && !p2) || (!p1 && p2)) return 0;
  else if (!p1 && !p2) return 1;
  if (!strcmp(p1, p2)) return 1;
  else return 0;
}

int case_comp(const char *p1, const char *p2) {
  if ((p1 && !p2) || (!p1 && p2)) return 0;
  else if (!p1 && !p2) return 1;
  if (!strcasecmp(p1, p2)) return 1;
  else return 0;
}

int string_ends_with(const char *str, const char *tail) {
  size_t len = strlen(str);
  if (strlen(tail) > len) return 0;
  if (comp(&str[len-strlen(tail)],tail)) return 1;
  return 0;
}

size_t atoi_from_string(const char *str) {
  if (!str[0]) return -1;
  int out = 0;
  for (int i = 0; str[i]; ++i) {
    if (str[i] < '0' || str[i] > '9') return -1;
    out = out*10 + str[i] - '0';
  }
  return out;
}

int is_hex(const char* str) {
  if (strlen(str) < 3 || str[0] != '0' || strlen(str) > 66) {
     return 0;
  }
  if (str[1] != 'x' && str[1] != 'X') {
     return 0;
  }
  size_t i;
  for (i = 2; i < strlen(str); ++i) {
     char current = str[i];
     if ((current >= '0' && current <= '9') ||
         (current >= 'A' && current <= 'F') ||
         (current >= 'a' && current <= 'f')) {
        continue;
     }
     return 0;
  }
  return 1;
}
char hexchar_from_byte(char in) {
  if (in >= 0x10) in = in % 0x10;
  if (in <= 9) return in + '0';
  if (in <= 0x0f) return in + 'a' - 10;
  return 0;
}
char byte_from_hexchar(const char in) {
  if ((in >= '0') && (in <= '9')) return in - '0';
  if ((in >= 'a') && (in <= 'f')) return in - 'a' + 10;
  if ((in >= 'A') && (in <= 'F')) return in - 'A' + 10;
  return 0;
}
void hex2bin(const char *in, uint8_t** out, size_t* out_len) {
  if (!in) return;
  *out = (uint8_t*)malloc(strlen(in) / 2);
  size_t i;
  for (i = 0; i < strlen(in) / 2; ++i) {
     const char hi = byte_from_hexchar(in[i * 2]);
     const char lo = byte_from_hexchar(in[i * 2 + 1]);
     (*out)[i] = hi * 16 + lo;
  }
  if (out_len) {
     *out_len = strlen(in) / 2;
  }
  if (in[0] == '0' && (in[1] == 'x' || in[1] == 'X')) {
    ++(*out);
    --(*out_len);
  }
}



char* hex_append_leading_zeroes_to_size(const char *str, size_t size) {
  if (size <= strlen(str)) return str;
  char *out = (char*)malloc(size+1);
  memset(out, '0', size);
  out[size] = 0;
  strcpy(out + size - strlen(str), str);
  return out;
}


char* hex_add(const char* in1, const char* in2) {
  if (!in1 || !in2) return 0;
  if (in1[0] != '0' || in2[0] != '0') return 0;
  if (in1[1] != 'x' || in2[1] != 'x') return 0;
  char *str1 = in1 + 2;
  char *str2 = in2 + 2;
  if (strlen(str1) > strlen(str2)) {
    str2 = hex_append_leading_zeroes_to_size(str2, strlen(str1));
  } else if (strlen(str1) < strlen(str2)) {
    str1 = hex_append_leading_zeroes_to_size(str1, strlen(str2));
  }
  size_t len = strlen(str1);
  char *out = malloc(len+3);
  memset(out, '0', len+2);
  out[len+2] = 0;
  out[1] = 'x';
  char *total = out+2;
  char rem = 0;
  size_t ii = len-1;
  while (ii+1) {
    char p = byte_from_hexchar(str1[ii]) + byte_from_hexchar(str2[ii]) + rem;
    if (p >= 0x10) {
      p = p % 0x10;
      rem = 1;
    } else {
      rem = 0;
    }
    total[ii] = hexchar_from_byte(p);
//    printf("%02x + %02x = %02x %hhd  \trem. %02x   out[ii] %c\n", byte_from_hexchar(str1[ii]), byte_from_hexchar(str2[ii]), p, p, rem, out[ii]);
    --ii;
  }
// printf("%s\n", str1);
// printf("%s\n", str2);
// printf("%s\n", total);
  return out;
}




char* to_uppercase(char *str) {
  if (!str) return 0;
  size_t len = strlen(str);
  char *out = malloc(len+1);
  size_t i=0;
  while (str[i]) {
    out[i] = str[i];
    if (str[i] >= 'a' && str[i] <= 'z') {
      out[i] += ('A' - 'a');
    }
    ++i;
  }
  out[len]=0;
  return out;
}

char* to_lowercase(char *str) {
  if (!str) return 0;
  size_t len = strlen(str);
  char *out = malloc(len+1);
  size_t i=0;
  while (str[i]) {
    out[i] = str[i];
    if (str[i] >= 'A' && str[i] <= 'Z') {
      out[i] -= ('A' - 'a');
    }
    ++i;
  }
  out[len]=0;
  return out;
}

int string_is_function_name(const char *str) {
  if (!str) return 0;
  for (size_t y = 0; y < strlen(str); ++y) {
    if (str[y] == '/' || str[y] == ';') {
      return 0;
    }
    if ((str[y] == '(') && (str[strlen(str)-1] == ')') && strlen(str) > 2) {
      return 1;
    }
  }
  return 0;
}

int string_is_function_name_or_fallback(const char *str) {
  if (!str) return 0;
  for (size_t y = 0; y < strlen(str); ++y) {
    if (str[y] == '/' || str[y] == ';') {
      return 0;
    }
    if ((str[y] == '(') && (str[strlen(str)-1] == ')')) {
      return 1;
    }
  }
  return 0;
}



int parse_asm_line(const char *line, char **instr, char **operand) {
  if (!line) return 0;
  if (!line[0]) return 0;
  // bool do_realloc = false;
  // if (!*instr) {
  //   *instr = NEW_WORD;
  //   do_realloc = true;
  // }
  // if (!*operand) {
  //   *operand = NEW_LINE;
  //   do_realloc = true;
  // }
  size_t instr_p;
  size_t operand_p;
  size_t i=0;
  while (line[i] != 0 && line[i] != '\n' && line[i] != ';') {
    if (i < strlen(line)-1) {
      if (line[i] == '/' && line[i+1] == '/') break;
    }
    //get instr
    instr_p = 0;
    while ( ((line[i] >= '0' && line[i] <= '9') || (line[i] == '*') || (line[i] == '=') ||   //'*' for terms compiler
             (line[i] >= 'A' && line[i] <= 'Z')) &&
              instr_p < 32 ) {
      *(*instr + instr_p) = line[i];
      ++instr_p;
      ++i;
    }
    *(*instr + instr_p) = '\0';
    if (instr_p) {
      //trim until operand
      while (line[i] == ' ' || line[i] == '\t') {
        ++i;
      }
      //get operand
      operand_p = 0;
      // while (line[i] && line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != ';') {
      while (line[i] && line[i] != '\t' && line[i] != '\n' && line[i] != ';') {
        if (i < strlen(line)-1) {
          if (line[i] == '/' && line[i+1] == '/') {
            break;
          }
        }
        *(*operand + operand_p) = line[i];
        ++i;
        ++operand_p;
      }
      while (*(*operand + operand_p) == ' ') {
        --operand_p;
      }
      *(*operand + operand_p) = '\0';
      break;
     }
     ++i;
  }
  // if (do_realloc) {
  //   REALLOC_STR(*instr);
  //   REALLOC_STR(*operand);
  // }
  return 0;
}
