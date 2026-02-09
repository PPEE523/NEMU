/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "watchpoint.h"
#include <assert.h>
#include <string.h>

WP wp_pool[NR_WP] = {};
WP *head = NULL;
WP *free_ = NULL;


void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

WP* new_wp() {
  if (free_ == NULL) {
    assert(0);   // 没有空闲监视点
  }

  WP *wp = free_;
  free_ = free_->next;

  wp->next = head; 
  head = wp;

  return wp;
}

void free_wp(WP *wp) {

  if (head == NULL) {
    assert(0);
  }

  if (head == wp) {
    head = head->next;
  } else {
    WP *prev = head;
    while (prev->next != NULL && prev->next != wp) {
      prev = prev->next;
    }

    if (prev->next == NULL) {
      assert(0);  // 没找到
    }

    prev->next = wp->next;
  }


  wp->next = free_;
  free_ = wp;
}
