#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#define MAX_NEEDLE_SIZE 100
#define CHARSET_SIZE 2097152

static wchar_t needle_cache[MAX_NEEDLE_SIZE];
static int *bad_char = NULL;
static int needle_backup_i[MAX_NEEDLE_SIZE];

static void gen_bad_char_t (wchar_t *needle) {
  if(bad_char == NULL) bad_char = malloc(2097152 * sizeof(wchar_t));
  
  for (int i = 0; i < CHARSET_SIZE; i++) {
    bad_char[i] = -1;
  }

  for (int i = 0; needle[i] != '\0'; i++) {
    bad_char[needle[i]] = i;
  }
}

static void gen_backup_i (wchar_t *needle) {
  needle_backup_i[0] = 0;
  for (int i = 1, k = 0; needle[i] != '\0';) {

    if (needle[i] == needle[k]) {
      needle_backup_i[i] = k;
      k++;
      i++;
    }
    else {
      if (k == 0){
        needle_backup_i[i] = k;
        i++;
      }
      else k = needle_backup_i[k-1];

    }
  }
}

int bm_search (wchar_t *file, int n, int *offset, wchar_t* needle) {
  int m = 0;
  if (needle[0] == L'\0' || file[0] == L'\0') {
    *offset = -1;
    return -1;
  }
  if (wcscmp(needle, needle_cache)) {
      gen_backup_i(needle);
      gen_bad_char_t(needle);
      wcscpy(needle_cache, needle);
  }

  while(needle[m] != L'\0')m++;
  for (int i = *offset; i+m < n;) {
    int k = m-1;
    int skip = 0;
    for(; file[i+k] == needle[k]; k--);

    if(k < 0){
      *offset = i+1;
      return i;
    }

    int bad_char_i = bad_char[file[i+k]];
    int backup_i = needle_backup_i[k];
    if (bad_char_i == -1) skip = backup_i;
    else skip = bad_char_i < backup_i ? k - bad_char_i : k - backup_i;
    i += (skip||1);
  }
  *offset = -1;
  return -1;
}


