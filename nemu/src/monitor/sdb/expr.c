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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <memory/paddr.h>



enum {
  TK_NOTYPE = 256, TK_EQ, TK_NEQ, TK_LE, TK_GE, TK_AND, TK_OR, TK_NUM, TK_HEX, TK_DEREF,TK_NEG, TK_REG
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // +
  {"-", '-'},           // -
  {"\\*", '*'},         // *
  {"/", '/'},           // /
  {"\\(", '('},         // (
  {"\\)", ')'},         // )
  {"==", TK_EQ},        // ==
  {"!=", TK_NEQ},       // != 
  {"<=", TK_LE},        // <=
  {">=", TK_GE},        // >=
  {"<", '<'},           // <
  {">", '>'},           // > 
  {"&&", TK_AND},       // &&
  {"\\|\\|", TK_OR},    // |
  {"!", '!'},           // !
  {"0[xX][0-9a-fA-F]+", TK_HEX},//十六进制数
  {"[0-9]+", TK_NUM},   // 十进制数
  {"\\$?[a-z]{2,3}", TK_REG} // 匹配 eax, ebx, ecx, edx 等

};

#define NR_REGEX ARRLEN(rules)
#define MAX_TOKENS 256
static regex_t re[NR_REGEX] = {};

word_t eval(int p, int q);
bool check_parentheses(int p, int q);
int find_op(int p, int q);
int priority(int type);
word_t isa_reg_str2val(const char *s, bool *success);
bool is_operator(int type);


/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[MAX_TOKENS] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;


//把解析结果存在char e中
static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {

    for (i = 0; i < NR_REGEX; i ++) {
      
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;
      
        switch (rules[i].token_type) {
          case TK_NOTYPE: break;
          
          default:
            if (nr_token >= MAX_TOKENS) {
              printf("Error: Too many tokens\n");
              return false;
            }
            tokens[nr_token].type = rules[i].token_type;
            int len = substr_len;
            if (len >= sizeof(tokens[nr_token].str)) {
              len = sizeof(tokens[nr_token].str) - 1;
            }

            strncpy(tokens[nr_token].str, substr_start, len);
            tokens[nr_token].str[len] = '\0';
            
            nr_token++;
            break;
        }

        break;
      }
    }


    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == '*') {
      if (i == 0 || is_operator(tokens[i-1].type)) {
        tokens[i].type = TK_DEREF;
      }
    }

    if (tokens[i].type == '-') {
      if (i == 0 || is_operator(tokens[i-1].type)) {
       tokens[i].type = TK_NEG;
     }
    }
  }

  return true;
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  *success = true;
  return eval(0, nr_token - 1);

}

word_t eval(int p, int q) {
  if (p > q) {
    /* Bad expression */
    assert(0);
  }
  else if (p == q) {
    if(tokens[p].type == TK_NUM || tokens[p].type == TK_HEX){
      return strtoul(tokens[p].str, NULL, 0);
    }
    bool success = true;
    word_t val = isa_reg_str2val(tokens[p].str, &success);
    assert(success);
    return val;
  }
  if (check_parentheses(p, q)) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  int op = find_op(p, q);
  assert(op != -1);
  if(tokens[op].type == TK_NEG){
    return -eval(op + 1, q);
  } 
  if(tokens[op].type == TK_DEREF){
    word_t addr = eval(op + 1,q);
    return paddr_read(addr,4);
  } 
  word_t val1 = eval(p, op - 1);
  word_t val2 = eval(op + 1, q);
  switch (tokens[op].type) {
    case '+': return val1 + val2;
    case '-': return val1 - val2;
    case '*': return val1 * val2;
    case '/': 
    if(val2 == 0){
      printf("Error: Division by zero");
      return 0;
    }
    return val1 / val2;
    case TK_EQ: return val1 == val2;
    case TK_NEQ: return val1 != val2;
    case TK_AND: return val1 && val2;
    case TK_OR: return val1 || val2;
    case TK_LE: return val1 <= val2;
    case TK_GE: return val1 >= val2;
    case '<': return val1 < val2;
    case '>': return val1 > val2;
    case '!': return !eval(op + 1, q);
    case TK_REG:     
      bool success = true;
      word_t val = isa_reg_str2val(tokens[p].str, &success);
      assert(success);
      return val;
    default: assert(0);
  }
}


int find_op(int p, int q) {
  int level = 0;
  int op = -1;
  int min_pri = 100;

  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') level++;
    else if (tokens[i].type == ')') level--;

    else if (level == 0) {

      if (tokens[i].type == TK_NEG || tokens[i].type == TK_DEREF || tokens[i].type == '!')
        continue; 

      int pri = priority(tokens[i].type);
      if (pri <= min_pri) {
        min_pri = pri;
        op = i;
      }
    }
  }

  return op;
}


bool check_parentheses(int p, int q) {
  if (tokens[p].type != '(' || tokens[q].type != ')') {
    return false;
  }

  int level = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') level++;
    else if (tokens[i].type == ')') level--;

    if (level == 0 && i < q) return false;
  }

  return level == 0;
}

int priority(int type) {
  switch (type) {
    case TK_OR: return 1;
    case TK_AND: return 2;
    case TK_EQ: 
    case TK_NEQ: return 3;
    case '+':
    case '-': return 4;
    case '*':
    case '/': return 5;
    case TK_NEG:
    case TK_DEREF:
    case '!': return 6;
    default: return 100;
  }
}

bool is_operator(int type) {
  switch(type) {
    case '+': case '-': case '*': case '/':
    case TK_EQ: case TK_NEQ:
    case TK_AND: case TK_OR:
    case TK_LE: case TK_GE:
    case '<': case '>':
    case '!':
      return true;
    default:
      return false;
  }
}
