#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include <stdint.h>
#include <stdbool.h>

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  char expr[256];
  uint32_t old_value;
} WP;

void init_wp_pool();
WP* new_wp();
void free_wp(WP *wp);

/* 关键：声明外部变量 */
extern WP *head;

#endif
