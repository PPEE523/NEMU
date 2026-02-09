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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <memory/paddr.h>
#include "watchpoint.h"
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

extern WP wp_pool[NR_WP];


/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}


//以下是cmd指令
static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return 0;
}

static int cmd_si(char *args) {
    int N = 1;  // 默认执行 1 条指令
    if (args != NULL) {
        if (sscanf(args, "%d", &N) != 1 || N <= 0) {
            printf("Invalid number: %s\n", args);
            return 0;
        }
    }
    cpu_exec(N);
    return 0;
}

static int cmd_info(char *args){
  char ch;
  if(args != NULL){
    if(sscanf(args, "%c", &ch) != 1){
      printf("Usage: info r/w \n");
      return 0;
    }
  }

  if(ch == 'r'){
    isa_reg_display();
    return 0;
  }
  if(ch == 'w'){
    WP *wp = head;
    if(!wp){
      printf("No watchpoints set.\n");
      return 0;
    }

    printf("Num\tExpression\tValue\n");
    while(wp){
      printf("%d\t%s\t%u\n", wp->NO, wp->expr, wp->old_value);
      wp = wp->next;
    }
    return 0;
  }
  printf("Usage: info r/w \n");
  return 0;
}

static int cmd_x(char *args){
  int N;
  paddr_t EXPR;
  if (args == NULL || sscanf(args, "%d %x", &N, &EXPR) != 2) {
      printf("Usage: x N EXPR\n");
      return 0;
  }
  for (int i = 0; i < N; i++) {
    if (i % 4 == 0) { 
      printf("0x%08x: ", EXPR);
    }

    uint32_t data = paddr_read(EXPR, 4);
        printf("0x%08x ", data);
        EXPR += 4;
        if (i % 4 == 3) printf("\n");
    }

  if (N % 4 != 0) printf("\n");
  return 0;
}

static int cmd_p(char *args) {
  if (args == NULL) {
    printf("Usage: p EXPR\n");
    return 0;
  }

  bool success = true;
  word_t result = expr(args, &success);

  if (success) {
    printf("%u\n", result);
  } else {
    printf("Bad expression\n");
  }

  return 0;
}

int cmd_w(char *args) {
  if (args == NULL) {
    printf("Usage: w EXPR\n");
    return 0;
  }

  bool success = true;
  word_t val = expr(args, &success);

  if (!success) {
    printf("Bad expression.\n");
    return 0;
  }

  WP *wp = new_wp();

  strncpy(wp->expr, args, sizeof(wp->expr) - 1);
  wp->expr[sizeof(wp->expr) - 1] = '\0';

  wp->old_value = val;

  printf("Watchpoint %d: %s = %u\n", wp->NO, wp->expr, wp->old_value);

  return 0;
}


static int cmd_d(char *args){
  if (args == NULL) {
    printf("Usage: d N\n");
    return 0;
  }

  int num;
  if (sscanf(args, "%d", &num) != 1) {
    printf("Invalid watchpoint number.\n");
    return 0;
  }

  WP *wp = head;
  while (wp) {
    if (wp->NO == num) {
      free_wp(wp);
      printf("Watchpoint %d deleted.\n", num);
      return 0;
    }
    wp = wp->next;
  }

  printf("Watchpoint %d not found.\n", num);
  return 0;
}


// static int cmd_Surprise(){
//   printf("Haha,I've not been finished!\n");
//   return 0;
// }

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si","Excute N instructions",cmd_si},
  { "info","Print program status:r for register;w for watchpoint",cmd_info},   //未实现
  { "x","Scan Memory",cmd_x},  //未实现
  { "p","Expression evaluation",cmd_p}, //未实现
  { "w","Set watchpoint",cmd_w},  //未实现
  { "d","Delete watchpoint",cmd_d}, //未实现
  // { "Surprise","Surprise m*****f*****",cmd_Surprise},
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1; //参数解析
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0 || nemu_state.state == NEMU_QUIT) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
