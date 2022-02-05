#ifndef __CHEAT_H__
#define __CHEAT_H__

#include "psxcommon.h"

typedef struct cheat_line_ {
  u32 code1;
  u16 code2;
} cheat_line_t;

typedef struct cheat_entry_ {
  int num_lines;
  int lines_cap;
  cheat_line_t *lines;
  char name[37];
  int user_enabled:2; // 0: disabled  1: enabled  2: M code
  int cont_enabled:1; // 0: disabled  1: enabled
} cheat_entry_t;

typedef struct cheat_ {
  int num_entries;
  int entries_cap;
  cheat_entry_t *entries;
} cheat_t;

void cheat_load(void);
void cheat_unload(void);
void cheat_set_run_per_sec(int r);
const cheat_t *cheat_get(void);
void cheat_apply(void);
void cheat_toggle(int idx);

#endif
