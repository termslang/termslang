/**
* Copyright (c) 2017
* Mikhail Baynov m.baynov@gmail.com
* Distributed under the GNU GPL v2
*/


int toupper(int c) {
	if( c >= 'a' && c <= 'z') c = c +'A' - 'a';
	return c;
}

size_t string_index_in_arr(const char *str, const char **arr) {  //CASE INSENSITIVE
  if (!str) return -1;
	if (!arr) return -1;
	size_t i;
  for (i=0; arr[i]; ++i) {
    if (!strcasecmp(str, arr[i])) return i;
  }
  return -1;
}

size_t arr_count(const char **arr) {
  if (!arr) return 0;
  size_t count=0;
  while (arr[count]) ++count;
  return count;
}

int arr_comp(const char **arr1, const char **arr2) {
  if (!arr1 && !arr2) return 1;
  if (!arr1) return 0;
  if (!arr2) return 0;
  size_t len = arr_count(arr1);
  if (len != arr_count(arr2)) return 0;
  size_t i;
	for (i=0; i < len; ++i) {
    if (strcmp(arr1[i], arr2[i])) return 0;
  }
  return 1;
}

int arr_exists_in_arr(const char **arr, const char ***arr_arr) {
  size_t ii;
	for (ii=0; arr_arr[ii]; ++ii) {
    if (arr_comp(arr_arr[ii], arr)) return 1;
  }
  return 0;
}

void arr_reverse(char ***arr) {
	size_t ii = arr_count(*arr) - 1;
	char **tmp_arr = malloc((ii+1) * sizeof(char*));
	size_t yy = 0;
	while (ii+1) {
		tmp_arr[yy] = *(*arr + ii);
		++yy;
		--ii;
	}
	tmp_arr[yy] = 0;
	*arr = tmp_arr;
}


size_t arr_copy(char ***out_arr, const char **arr) {
  if (!arr) return 0;
  size_t count=0;
  while (arr[count]) {
		asprintf(*out_arr + count, "%s", arr[count]);
    ++count;
  }
  *(*out_arr + count) = NULL;
  return count;
}


void arr_insert_elements_at_position(char **arr, size_t position, char ***out_arr) {
  if (!arr || !*out_arr) return;
  char **after_insert_arr = NEW_BIG_ARR;
  char **tmp_arr = NEW_BIG_ARR;
  arr_copy(&after_insert_arr, *out_arr + position);
  char **dest1 = *out_arr + position;
  arr_copy(&tmp_arr, dest1);
  size_t len = arr_copy(&dest1, arr);
  char **dest2 = dest1 + len;
  arr_copy(&dest2, tmp_arr);
  FREE_ARR(after_insert_arr);
  FREE_ARR(tmp_arr);
}
