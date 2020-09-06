#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h> 
#include "bm.c"
#include "term.c"
#include "util.c"

#define DEF_SIZE 5000
#define MAX_MATCHES_FILE 99999
#define MAX_FILES 100
#define INPUT_MODE 1
#define BROWSE_FILES_MODE 2
#define BROWSE_MATCHES_MODE 3
#define KEY_DEL 127
#define KEY_ENTER 13
#define ESC 27
#define ESC_SEQUENCE 91
#define LEFT_ARROW 68
#define RIGHT_ARROW 67
#define UP_ARROW 65
#define DOWN_ARROW 66
#define UP_PAG 53
#define DOWN_PAG 54

typedef struct {
  wchar_t *file;
  int file_len;
  wchar_t *name;
  int matches_n;
  int matches[MAX_MATCHES_FILE];
  int *nl;
  int nl_n;
} file_scan;

file_scan files[MAX_FILES];
wchar_t needle[MAX_NEEDLE_SIZE];
int files_n = 0;
int need_update = 0;
int need_repaint = 1;
int needle_len = 0;
int cursor_file_row = 0;
int cursor_match_row = 0;
int match_list_y_offset = 0;
int mode = INPUT_MODE;
int term_size_x = 0;
int term_size_y = 0;

int read_file (char *name, file_scan *fs) {
  fs->nl = (int *)malloc(sizeof(int) * DEF_SIZE);
  fs->nl_n = 0;
  fs->matches_n = 0;
  fs->file = (wchar_t *)malloc(sizeof(wchar_t) * DEF_SIZE);
  fs->file_len = 0;
  fs->name = (wchar_t *)malloc(sizeof(wchar_t) * PATH_MAX);
  mbstowcs(fs->name, name, PATH_MAX);

  FILE *fp = fopen(name, "r");
  int nl_size = DEF_SIZE;
  int buf_size = DEF_SIZE;
  int i = 0;
  wint_t temp  = 0;

  if (fp == NULL) return -1;
  while ((temp = getwc(fp)) != WEOF) {

    fs->file[i] = (wchar_t)temp;

    if (fs->file[i] == (wchar_t)10) {
      fs->nl[fs->nl_n] = i;
      fs->nl_n++;
      if (fs->nl_n == nl_size-1) {
        nl_size += DEF_SIZE;
        fs->nl = (int *)realloc(fs->nl, nl_size * sizeof(int));
        if (fs->nl == NULL) return -1;
      }
    }
    i++;
    fs->file_len++;
    if (i == buf_size-2) {
      buf_size += DEF_SIZE;
      fs->file = (wchar_t *)realloc(fs->file, buf_size * sizeof(wchar_t));
      if (fs->file == NULL) return -1;
    }
  }
  fs->file_len = i-1;
  fs->file[i] = L'\0';
  fclose(fp);
  return 1;
}

int read_dir_files() {

  char current_path[PATH_MAX];
  FILE *curr_file;
  DIR *current_dir;
  struct dirent *dirp;

  getcwd(current_path, PATH_MAX);
  current_dir = opendir(current_path);
  while ((dirp = readdir(current_dir)) != NULL) {
    if (!strcmp(dirp->d_name, "..") || !strcmp(dirp->d_name, ".")) continue;
    if (read_file(dirp->d_name, &(files[files_n])) < 0) return -1;
    files_n++;
  }
  return 0;
}

void search () {

  for (int i = 0; i < files_n; i++) {
    file_scan *file = &(files[i]);
    file->matches_n = 0;
    int j = 0;
    int offset = 0;
    int match = -1;
    do {
      match = bm_search(file->file, file->file_len, &offset, needle);
      if (match >= 0) {
        file->matches[j] = match;
        file->matches_n++;
        j++;
      }
    } while ((match != -1) && j < MAX_MATCHES_FILE);
  }
}

void empty_str (wchar_t *str, int len) {
  wmemset(str, L' ', len);
  str[len-1] = L'\0';
}
void format_out_line (wchar_t *src, wchar_t *out_buffer, int out_size) {
  for (int i = 0; i < out_size -1 && (src[i] != L'\n' && src[i] != L'\0'); i++) {
    out_buffer[i] = src[i];
  }
}

void display_ui(int x, int y) {

  wchar_t substr[x+1];
  empty_str(substr, x+1);

  format_out_line(L"search: ", substr, x+1);
  format_out_line(needle, substr+9, x-8);
  printf("\033[48;5;0m\033[38;5;255m");
  printf("%ls", substr);

  for (int i = 0, line = 0; line < y-1; i++, line++) {
    if (i >= files_n) {
      empty_str(substr, x+1);
      printf("\033[39;49m");
      printf("%ls", substr);
      continue;
    }

    if (i == cursor_file_row) printf("\033[48;5;183m\033[38;5;0m");
    else printf("\033[48;5;159m\033[38;5;0m");

    empty_str(substr, x+1);

    format_out_line(L"File: ", substr, x+1);
    format_out_line(files[i].name, substr+7, x-6);
    printf("%ls", substr);
    printf("\033[48;5;0m");
    if (mode == BROWSE_MATCHES_MODE && i == cursor_file_row) {
      if (cursor_match_row > (match_list_y_offset + (y - (i+3)))) match_list_y_offset++; 
      else if (cursor_match_row < match_list_y_offset) match_list_y_offset--;
      for (int j = match_list_y_offset; j < files[i].matches_n && line < y-2; j++, line++) {
        if (j == cursor_match_row) printf("\033[48;5;0m\033[38;5;9m");
        else printf("\033[48;5;0m\033[38;5;255m");

        int match_index = files[i].matches[j];
        int match_line_index = 0;
        int match_line = get_closest_lesser_n(files[i].nl, files[i].nl_n, match_index, &match_line_index);
        match_line_index = match_line < 1 ? 1 : match_line_index+2;

        empty_str(substr, x+1);
        swprintf(substr, x,L"Line %10d:  ", match_line_index);
        format_out_line(files[i].file+match_line+1, substr+18, x-17);
        printf("%.*ls", x+1,substr);
      }
    }
  }
}

char make_ctrl (wchar_t key) {
  return key & 0x1f;
}

void digest_input() {
  reset_cursor(); //??????

  wchar_t key = L'\0';

  char tmp[4] = {0,0,0,0};
  char mb_len = 1;
  read(STDIN_FILENO, &tmp, 1);
  if (tmp[0] == -1) return;
  if (tmp[0] & 126) {
      if (tmp[0] & 64) {
        mb_len++;
        read(STDIN_FILENO, &tmp[1], 1);
        if (tmp[0] & 32) {
          mb_len++;
          read(STDIN_FILENO, &tmp[2], 1);
          if (tmp[0] & 16) {
            mb_len++;
            read(STDIN_FILENO, &tmp[3], 1);
          }
        }
      }
  }

  mbtowc(&key, tmp, mb_len);

  if (key == ESC) {
    char sequence = '\0';

    if (!read(STDIN_FILENO, &sequence, 1)) { mode = INPUT_MODE; return;}
    if (sequence == ESC_SEQUENCE) {
      need_repaint = 1;
      key = getwchar();
      if (mode == BROWSE_FILES_MODE){
        if (key == UP_ARROW && cursor_file_row - 1 >= 0) cursor_file_row--;
        else if (key == DOWN_ARROW && cursor_file_row + 1 < files_n) cursor_file_row++;
        else if (key == RIGHT_ARROW) mode = BROWSE_MATCHES_MODE;
        else if (key == UP_PAG) {
          int delta_row = cursor_file_row - term_size_y;
          cursor_file_row = delta_row >= 0 ? delta_row : 0;
        }
        else if (key == DOWN_PAG) {
          int delta_row = cursor_file_row + term_size_y;
          cursor_file_row = delta_row < files_n ? delta_row : files_n-1;
        }

        cursor_match_row = 0;
        match_list_y_offset = 0;
      }
      else if (mode == BROWSE_MATCHES_MODE) {
        if (key == UP_ARROW && cursor_match_row - 1 >= 0) cursor_match_row--;
        else if (key == DOWN_ARROW && cursor_match_row + 1 < files[cursor_file_row].matches_n) cursor_match_row++;
        else if (key == LEFT_ARROW) mode = BROWSE_FILES_MODE;
        else if (key == UP_PAG) {
          int delta_row = cursor_match_row - (term_size_y - (cursor_file_row + 1));
          cursor_match_row = delta_row >= 0 ? delta_row : 0;
          match_list_y_offset = cursor_match_row;
        }
        else if (key == DOWN_PAG) {
          int delta_row = cursor_match_row + (term_size_y - (cursor_file_row + 1));

          if (delta_row <= files[cursor_file_row].matches_n) {
            cursor_match_row = delta_row;

            if (cursor_match_row < (files[cursor_file_row].matches_n - (term_size_y - (cursor_file_row + 1)))){
              match_list_y_offset = cursor_match_row;
            }
            else {
              match_list_y_offset = files[cursor_file_row].matches_n-1 - (term_size_y - (cursor_file_row + 1));
            }
          }
          else {
            cursor_match_row = files[cursor_file_row].matches_n-1;
          }
        }
      }
    }
  }
  else if (key == KEY_ENTER) {
    if(mode == INPUT_MODE && need_update) {
      search();
      need_update = 0;
    }
    mode = BROWSE_FILES_MODE;
    
  }
  else if (iswprint(key) && (mode == INPUT_MODE) && needle_len <  MAX_NEEDLE_SIZE) {
    need_update = 1;
    need_repaint = 1;
    needle[needle_len] = key;
    needle_len++;
  }
  else if (key == (wchar_t)127 && (mode  == INPUT_MODE) && needle_len > 0) {
    need_update = 1;
    need_repaint = 1;
    needle_len--;
    needle[needle_len] = L'\0';
  }
  else if (key == KEY_ENTER) mode = BROWSE_FILES_MODE;

  if (key == make_ctrl('c')) {
    exit(0);
  }

}

int main () {

  enter_raw_term();
  read_dir_files();
  needle[0] = L'\0';

  while(1) {
    need_repaint = get_win_size(&term_size_x, &term_size_y);
    digest_input(); 
    if (need_repaint) display_ui(term_size_x, term_size_y);
    need_repaint = 0;
    fflush(stdout);
  }

  return 0;
}

