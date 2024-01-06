#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "general.h"
#ifdef INTERP
#include "lisp.h"
#endif

// TOKENS
#define CAR "CAR"
#define CDR "CDR"
#define CONS "CONS"
#define SET "SET"
#define PRINT "PRINT"
#define LENGTH "LENGTH"
#define LESS "LESS"
#define GREATER "GREATER"
#define EQUAL "EQUAL"
#define PLUS "PLUS"
#define WHILE "WHILE"
#define IF "IF"
#define NIL "NIL"
#define B_OPEN '('
#define B_CLOSE ')'

// ARRAY LENGTHS
#define STRLEN 2000
#define TRCLEN 500
#define FNCLEN 20
#define VARLEN 26
#define NESTLEN 100
#define NOFUNCS 12

#define NEXT 1
#define ZERO 0
#define D_QUOTE '"'
#define S_QUOTE '\''


enum dinterp {parse, interp};
typedef enum dinterp dinterp;

enum func {car, cdr, cons, plus, length, less, greater, equal, set, print};
typedef enum func func;

struct var{
   char varstr[TRCLEN];
};
typedef struct var var;


struct interpret{
   char istr[STRLEN];
   var* vars[VARLEN];
   int var_i;
   bool set_last[NESTLEN];
   bool doub[NESTLEN];
   bool loopif;
   func stack[NESTLEN];
   char args_a[NESTLEN][TRCLEN];
   char args_b[NESTLEN][TRCLEN];
   int i;
   char out[TRCLEN];
   int atom_a;
   int atom_b;
   void (*func_ptr[NOFUNCS])(struct interpret* intrp);
   bool testing;
   bool print_str;
   bool ret;
   bool ignore[NESTLEN];
   int ig;
};
typedef struct interpret interpret;

struct parser{
   char str[STRLEN];
   int i;
   char trc[TRCLEN];
   char beg[FNCLEN];
};
typedef struct parser parser;



// functions necessary for testing
void test(void);
void clear(parser* prsr, interpret* intrp);
void clear_parser(parser* prsr);
void clear_intrp(interpret* intrp);
void clear_strings(interpret* intrp);
void clear_nests(interpret* intrp);

// parsing functions
parser* init_prsr(void);
bool is_prog(parser* prsr, interpret* intrp);
bool is_instrcts(parser* prsr, interpret* intrp);
bool is_instrct(parser* prsr, interpret* intrp);
bool is_func(parser* prsr, interpret* intrp);

bool is_retfunc(parser* prsr, interpret* intrp);
bool is_iofunc(parser* prsr, interpret* intrp);
bool is_loop_if(parser* prsr, interpret* intrp);

bool is_boolfunc(parser* prsr, interpret* intrp);
bool is_intfunc(parser* prsr, interpret* intrp);
bool is_listfunc(parser* prsr, interpret* intrp);

bool is_double_list(parser* prsr, interpret* intrp);
bool is_trc_list(parser* prsr, interpret* intrp);
bool is_list(parser* prsr, interpret* intrp);
bool is_var(parser* prsr, interpret* intrp);
bool is_nil(parser* prsr, interpret* intrp);
bool is_litstr(parser* prsr, interpret* intrp, char litstr);

bool is_brac_retfunc(parser* prsr, interpret* intrp);
bool is_brac_boolfunc(parser* prsr, interpret* intrp);
bool inner_bracbool(parser* prsr, interpret* intrp);

bool is_set(parser* prsr, interpret* intrp);
bool is_print(parser* prsr, interpret* intrp);

bool is_if(parser* prsr, interpret* intrp);
bool inner_if(parser* prsr, interpret* intrp, int save);
bool is_while(parser* prsr, interpret* intrp);
bool skip_while(parser* prsr, interpret* intrp, int save);
void inner_while(parser* prsr, interpret* intrp, int save);

void skip_space(parser* prsr, int increment);
void skip_chars(parser* prsr);
void skip_scan(parser* prsr, interpret* intrp);
void estrcpy(char trc[], const char msg[]);

//interpreting functions
interpret* init_intrp(void);
void push_stack(interpret* intrp, func fnc);
void execute(interpret* intrp);
void intrp_list(char str[], interpret* intrp);
void intrp_litstr(interpret* intrp, char litstr, char lit[]);
void intrp_var(char var, interpret* intrp);
void free_struct(parser* prsr, interpret* intrp);
void do_skip(parser* prsr, interpret* intrp, int save);

#ifdef INTERP
void init_funcs(interpret* intrp);
void sum(lisp* l, atomtype* accum);
void increment_ig(interpret* intrp);
void execute_boolfunc(interpret* intrp);
void intrp_atoms(interpret* intrp);
void intrp_car(interpret* intrp);
void intrp_cdr(interpret* intrp);
void intrp_cons(interpret* intrp);
void intrp_set(interpret* intrp);
void intrp_print(interpret* intrp);
void intrp_plus(interpret* intrp);
void intrp_length(interpret* intrp);
void intrp_equal(interpret* intrp);
void intrp_less(interpret* intrp);
void intrp_greater(interpret* intrp);
#endif

// file handling functions
void driver(FILE* fp, dinterp dint);
void file2str(FILE* fp, parser* prsr);
