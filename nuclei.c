#include "nuclei.h"


int main(int argc, char* argv[])
{
   test();

   if (argc != 2){
      on_error("Incorrect usage.");
   }
   FILE* fp = fopen(argv[1], "r");
   if (fp == NULL){
      on_error("Could not open file.");
   }
   dinterp dint = parse;
   #ifdef INTERP
   dint = interp;
   #endif
   driver(fp, dint);
   fclose(fp);
   return 0;
}


void driver(FILE* fp, dinterp dint)
{
   parser* prsr = init_prsr();
   interpret* intrp = init_intrp();
   file2str(fp, prsr);
   bool status = is_prog(prsr, intrp);
   if (dint == parse){
      if (status){
         printf("Parsed OK\n");
      }
      else{
         fprintf(stderr, "[ERROR] %s\n", prsr->trc);
         exit(EXIT_FAILURE);
      }
   }
   free_struct(prsr, intrp);
}


parser* init_prsr(void)
{
   parser* prsr = calloc(1, sizeof(parser));
   if (prsr == NULL){
      on_error("Cannot calloc memory");
   }
   return prsr;
}


interpret* init_intrp(void)
{
   interpret* intrp = calloc(1, sizeof(interpret));
   if (intrp == NULL){
      on_error("Cannot calloc memory");
   }
   for (int i = 0; i < VARLEN; i++){
      var* v = calloc(1, sizeof(var));
      if (v == NULL){
         on_error("Cannot calloc memory");
      }
      intrp->vars[i] = v;
   }
   #ifdef INTERP
   init_funcs(intrp);
   #endif
   return intrp;
}

#ifdef INTERP
void init_funcs(interpret* intrp)
{
   intrp->func_ptr[car] = intrp_car;
   intrp->func_ptr[cdr] = intrp_cdr;
   intrp->func_ptr[cons] = intrp_cons;
   intrp->func_ptr[print] = intrp_print;
   intrp->func_ptr[set] = intrp_set;
   intrp->func_ptr[plus] = intrp_plus;
   intrp->func_ptr[length] = intrp_length;
   intrp->func_ptr[equal] = intrp_equal;
   intrp->func_ptr[less] = intrp_less;
   intrp->func_ptr[greater] = intrp_greater;
}
#endif


void file2str(FILE* fp, parser* prsr)
{
   int i = 0;
   char c;
   while ((c = (char)fgetc(fp)) != EOF){
      if (i >= STRLEN){
         on_error("File too big to parse.");
      }
      prsr->str[i] = c;
      i += 1;
   }
}


bool is_prog(parser* prsr, interpret* intrp)
{
   skip_chars(prsr);
   if (prsr->str[prsr->i] == B_OPEN){
      skip_space(prsr, NEXT);
      return is_instrcts(prsr, intrp);
   }
   estrcpy(prsr->trc, "Expected '(' to start <PROG>");
   return false;
}


bool is_instrcts(parser* prsr, interpret* intrp)
{
   if (prsr->str[prsr->i] == B_CLOSE){
      skip_space(prsr, NEXT);
      return true;
   }
   if (is_instrct(prsr, intrp)){
      return is_instrcts(prsr, intrp);
   }
   estrcpy(prsr->trc, "Expected ')' to end <INSTRCTS>");
   return false;
}


bool is_instrct(parser* prsr, interpret* intrp)
{
   if (prsr->str[prsr->i] == B_OPEN){
      skip_space(prsr, NEXT);
      if (is_func(prsr, intrp)){
         if(prsr->str[prsr->i] == B_CLOSE){
            skip_space(prsr, NEXT);
            return true;
         }
         estrcpy(prsr->trc, "Expected ')' to contain a <FUNC>");
      }
   }
   estrcpy(prsr->trc, "Expected '(' to contain a <FUNC>");
   return false;
}


void skip_scan(parser* prsr, interpret* intrp)
{
   skip_space(prsr, ZERO);
   sscanf(prsr->str + prsr->i, "%[A-Z]s", prsr->beg);
   prsr->i += strlen(prsr->beg);
   skip_space(prsr, ZERO);
   intrp->set_last[intrp->i] = false;
}


bool is_func(parser* prsr, interpret* intrp)
{
   skip_scan(prsr, intrp);
   int saved_i = prsr->i;
   if (is_retfunc(prsr, intrp)){
      return true;
   }
   prsr->i = saved_i;
   if (is_iofunc(prsr, intrp)){
      return true;
   }
   prsr->i = saved_i;
   if (is_loop_if(prsr, intrp)){
      return true;
   }
   prsr->i = saved_i;
   estrcpy(prsr->trc, "Expected function name");
   return false;
}


#ifdef INTERP

void sum(lisp* l, atomtype* accum)
{
   *accum = *accum + lisp_getval(l);
}


void intrp_car(interpret* intrp)
{
   lisp* l = lisp_fromstring(intrp->args_b[intrp->i]);
   lisp_tostring(lisp_car(l), intrp->out);
   lisp_free(&l);
}


void intrp_cdr(interpret* intrp)
{
   lisp* l = lisp_fromstring(intrp->args_b[intrp->i]);
   lisp_tostring(lisp_cdr(l), intrp->out);
   lisp_free(&l);
}


void intrp_cons(interpret* intrp)
{
   lisp* la = lisp_fromstring(intrp->args_a[intrp->i]);
   lisp* lb = lisp_fromstring(intrp->args_b[intrp->i]);

   lisp* test = lisp_cons(la, lb);
   lisp_tostring(test, intrp->out);

   lisp_free(&test);
}


void intrp_atoms(interpret* intrp)
{
   intrp->atom_a = 0;
   intrp->atom_b = 0;
   lisp* la = lisp_fromstring(intrp->args_a[intrp->i]);
   lisp* lb = lisp_fromstring(intrp->args_b[intrp->i]);
   lisp_reduce(sum, la, &intrp->atom_a);
   lisp_reduce(sum, lb, &intrp->atom_b);
   lisp_free(&la);
   lisp_free(&lb);
}


void intrp_plus(interpret* intrp)
{
   intrp_atoms(intrp);
   int sum = intrp->atom_a + intrp->atom_b;
   snprintf(intrp->out, TRCLEN, "%d", sum);
}


void intrp_length(interpret* intrp)
{
   lisp* l = lisp_fromstring(intrp->args_b[intrp->i]);
   snprintf(intrp->out, TRCLEN, "%d", lisp_length(l));
   lisp_free(&l);
}


void increment_ig(interpret* intrp)
{
   if (intrp->ig + 1 < NESTLEN){
      intrp->ig += 1;
      intrp->ignore[intrp->ig] = false;
      intrp_atoms(intrp);
      strcpy(intrp->out, "1");
   }
   else{
      on_error("[ERROR] Nesting too deep.");
   }
}


void execute_boolfunc(interpret* intrp)
{
   if (intrp->loopif && intrp->i == 0){
      intrp->ignore[intrp->ig] = true;
   }
   strcpy(intrp->out, "0");
}


void intrp_less(interpret* intrp)
{
   increment_ig(intrp);
   if (intrp->atom_a >= intrp->atom_b){
      execute_boolfunc(intrp);
   }
}


void intrp_greater(interpret* intrp)
{
   increment_ig(intrp);
   if (intrp->atom_a <= intrp->atom_b){
      execute_boolfunc(intrp);
   }
}


void intrp_equal(interpret* intrp)
{
   increment_ig(intrp);
   if (intrp->atom_a != intrp->atom_b){
      execute_boolfunc(intrp);
   }
}


void intrp_set(interpret* intrp)
{
   char tempstr[TRCLEN] = {'\0'};
   strcpy(tempstr, intrp->args_b[intrp->i]);
   strcpy(intrp->vars[intrp->var_i]->varstr, tempstr);
}


void intrp_print(interpret* intrp)
{
   if (!intrp->print_str){
      lisp* l = lisp_fromstring(intrp->args_b[intrp->i]);
      lisp_tostring(l, intrp->out);
      lisp_free(&l);
   }
   int comb = strlen(intrp->istr) + strlen(intrp->out);
   if (comb + 1 < STRLEN && intrp->testing){
      strcat(intrp->istr, intrp->out);
      strcat(intrp->istr, "\n");
   }
   if (!intrp->testing){
      printf("%s\n", intrp->out);
   }
}
#endif


// decrement the stack counter, executing all the functions until
// it reaches a function in the stack which is missing an argument
void execute(interpret* intrp)
{
   while (intrp->i > 0){
      intrp->i -= 1;
      int i = intrp->i;

      intrp->print_str = false;
      intrp->func_ptr[intrp->stack[i]](intrp);
      if (i > 0 && intrp->set_last[i - 1]){
         strcpy(intrp->args_b[i - 1], intrp->out);
      }
      if (i > 0 && !intrp->set_last[i - 1]){
         strcpy(intrp->args_a[i - 1], intrp->out);
      }
      if (i > 0 && !intrp->set_last[i - 1] && intrp->doub[i - 1]){
         intrp->set_last[i - 1] = true;
         return;
      }
   }
}


void intrp_list(char str[], interpret* intrp)
{
   // set the last argument of the current function
   if (intrp->set_last[intrp->i - 1]){
      strcpy(intrp->args_b[intrp->i - 1], str);
      intrp->set_last[intrp->i - 1] = false;
   }
   // set the first argument of the current function
   else if (intrp->doub[intrp->i - 1]){
      strcpy(intrp->args_a[intrp->i - 1], str);
      intrp->set_last[intrp->i - 1] = true;
   }
   // execute functions which have full sets of arguments
   if (intrp->set_last[intrp->i - 1] == false){
      #ifdef INTERP
      execute(intrp);
      #endif
   }
}


void intrp_var(char var, interpret* intrp)
{
   int i = intrp->i;
   bool set_last = intrp->set_last[i - 1];

   if (intrp->stack[i - 1] == set && !set_last){
      intrp->var_i = (int)(var - 'A');
      intrp->set_last[i - 1] = true;
   }
   else{
      char tempstr[TRCLEN] = {'\0'};
      strcpy(tempstr, intrp->vars[(int)(var - 'A')]->varstr);
      intrp_list(tempstr, intrp);
   }
}


bool is_var(parser* prsr, interpret* intrp)
{
   if (isupper(prsr->str[prsr->i])){
      bool upper = isupper(prsr->str[prsr->i + 1]);
      if (prsr->i + 1 < STRLEN && (!upper)){
         intrp_var(prsr->str[prsr->i], intrp);
         skip_space(prsr, NEXT);
         return true;
      }
   }
   return false;
}


void intrp_litstr(interpret* intrp, char litstr, char lit[])
{
   if (litstr == S_QUOTE){
      intrp_list(lit, intrp);
   }
   else{
      #ifdef INTERP
      intrp->i -= 1;
      intrp->print_str = true;
      strcpy(intrp->out, lit);
      intrp_print(intrp);
      #endif
   }
}


bool is_litstr(parser* prsr, interpret* intrp, char litstr)
{
   char lit[TRCLEN] = {'\0'};
   int p = 0;
   if (prsr->str[prsr->i] == litstr){
      prsr->i += 1;
      while (prsr->str[prsr->i] != '\0'){
         if (prsr->str[prsr->i] == litstr){
            skip_space(prsr, NEXT);
            intrp_litstr(intrp, litstr, lit);
            return true;
         }
         lit[p] = prsr->str[prsr->i];
         prsr->i += 1;
         p += 1;
      }
   }
   return false;
}


bool is_nil(parser* prsr, interpret* intrp)
{
   char nil_str[FNCLEN] = {'\0'};
   sscanf(prsr->str + prsr->i, "%[A-Z]s", nil_str);
   if (!strcmp(nil_str, NIL)){
      skip_space(prsr, strlen(nil_str));
      intrp_list("()", intrp);
      return true;
   }
   return false;
}


bool is_brac_retfunc(parser* prsr, interpret* intrp)
{
   if (prsr->str[prsr->i] == B_OPEN){
      prsr->i += 1;
      skip_scan(prsr, intrp);
      if (is_retfunc(prsr, intrp)){
         if(prsr->str[prsr->i] == B_CLOSE){
            skip_space(prsr, NEXT);
            return true;
         }
      }
   }
   return false;
}


bool is_list(parser* prsr, interpret* intrp)
{
   int saved_i = prsr->i;
   if (is_var(prsr, intrp) || is_nil(prsr, intrp)){
      return true;
   }
   prsr->i = saved_i;
   if (is_litstr(prsr, intrp, S_QUOTE)){
      return true;
   }
   prsr->i = saved_i;
   if (is_brac_retfunc(prsr, intrp)){
      return true;
   }
   prsr->i = saved_i;
   return false;
}


bool is_double_list(parser* prsr, interpret* intrp)
{
   intrp->doub[intrp->i - 1] = true;
   if (is_list(prsr, intrp)){
      if (is_list(prsr, intrp)){
         return true;
      }
   }
   estrcpy(prsr->trc, "Expected <LIST> <LIST>");
   return false;
}


void push_stack(interpret* intrp, func fnc)
{
   intrp->stack[intrp->i] = fnc;
   intrp->i += 1;
}


bool is_boolfunc(parser* prsr, interpret* intrp)
{
   if (!strcmp(prsr->beg, LESS)){
      push_stack(intrp, less);
      return is_double_list(prsr, intrp);
   }
   if (!strcmp(prsr->beg, GREATER)){
      push_stack(intrp, greater);
      return is_double_list(prsr, intrp);
   }
   if (!strcmp(prsr->beg, EQUAL)){
      push_stack(intrp, equal);
      return is_double_list(prsr, intrp);
   }
   return false;
}


bool is_trc_list(parser* prsr, interpret* intrp)
{
   intrp->doub[intrp->i - 1] = false;
   intrp->set_last[intrp->i - 1] = true;

   if (is_list(prsr, intrp)){
      return true;
   }
   estrcpy(prsr->trc, "Expected <LIST>");
   return false;
}


bool is_listfunc(parser* prsr, interpret* intrp)
{
   if (!strcmp(prsr->beg, CAR)){
      push_stack(intrp, car);
      return is_trc_list(prsr, intrp);
   }
   if (!strcmp(prsr->beg, CDR)){
      push_stack(intrp, cdr);
      return is_trc_list(prsr, intrp);
   }
   if (!strcmp(prsr->beg, CONS)){
      push_stack(intrp, cons);
      return is_double_list(prsr, intrp);
   }
   return false;
}


bool is_intfunc(parser* prsr, interpret* intrp)
{
   if (!strcmp(prsr->beg, LENGTH)){
      push_stack(intrp, length);
      return is_trc_list(prsr, intrp);
   }
   if (!strcmp(prsr->beg, PLUS)){
      push_stack(intrp, plus);
      return is_double_list(prsr, intrp);
   }
   return false;
}


bool is_retfunc(parser* prsr, interpret* intrp)
{
   int saved_i = prsr->i;
   if (is_listfunc(prsr, intrp)){
      return true;
   }
   prsr->i = saved_i;
   if (is_intfunc(prsr, intrp)){
      return true;
   }
   prsr->i = saved_i;
   if (is_boolfunc(prsr, intrp)){
      return true;
   }
   prsr->i = saved_i;
   return false;
}


bool is_set(parser* prsr, interpret* intrp)
{
   intrp->doub[intrp->i - 1] = true;

   if (is_var(prsr, intrp)){
      if (is_list(prsr, intrp)){
         return true;
      }
   }
   estrcpy(prsr->trc, "Expected <VAR> <LIST>");
   return false;
}


bool is_print(parser* prsr, interpret* intrp)
{
   intrp->doub[intrp->i - 1] = false;
   intrp->set_last[intrp->i - 1] = true;

   int saved_i = prsr->i;
   if (is_list(prsr, intrp)){
      return true;
   }
   prsr->i = saved_i;
   if (is_litstr(prsr, intrp, D_QUOTE)){
      return true;
   }
   estrcpy(prsr->trc, "Expected <LIST> or <STRING>");
   return false;
}


bool is_iofunc(parser* prsr, interpret* intrp)
{
   if (!strcmp(prsr->beg, SET)){
      push_stack(intrp, set);
      return is_set(prsr, intrp);
   }
   if (!strcmp(prsr->beg, PRINT)){
      push_stack(intrp, print);
      return is_print(prsr, intrp);
   }
   return false;
}


void do_skip(parser* prsr, interpret* intrp, int save)
{
   int len = (int)strlen(prsr->str);
   if (intrp->ignore[save] && prsr->i < len){
      skip_space(prsr, ZERO);
      int bracs = 1;
      while (bracs > 0 && prsr->i < len){
         prsr->i += 1;
         if (prsr->str[prsr->i] == B_OPEN){
            bracs += 1;
         }
         if (prsr->str[prsr->i] == B_CLOSE){
            bracs -= 1;
         }
      }
      skip_space(prsr, NEXT);
   }
   intrp->ignore[save] = 1 - intrp->ignore[save];
}


bool inner_bracbool(parser* prsr, interpret* intrp)
{
   intrp->loopif = false;
   skip_space(prsr, ZERO);
   if (prsr->str[prsr->i] == B_CLOSE){
      skip_space(prsr, NEXT);
      return true;
   }
   estrcpy(prsr->trc, "Expected ')' to contain a <BOOLFUNC>");
   return false;
}


bool is_brac_boolfunc(parser* prsr, interpret* intrp)
{
   if (prsr->str[prsr->i] == B_OPEN){
      prsr->i += 1;
      skip_scan(prsr, intrp);
      intrp->loopif = true;
      if (is_boolfunc(prsr, intrp)){
         if (inner_bracbool(prsr, intrp)){
            #ifdef INTERP
            int save_i = intrp->ig;
            do_skip(prsr, intrp, save_i);
            #endif
            return true;
         }
      }
      estrcpy(prsr->trc, "Expected <BOOLFUNC> name");
   }
   estrcpy(prsr->trc, "Expected '(' to contain a <BOOLFUNC>");
   return false;
}


void inner_while(parser* prsr, interpret* intrp, int save)
{
   if (prsr->str[prsr->i] == B_OPEN){
      skip_space(prsr, NEXT);
      if (is_instrcts(prsr, intrp)){
         intrp->ret = true;
         #ifdef INTERP
         prsr->i = save;
         #else
         save = prsr->i;
         #endif
      }
   }
   if (!intrp->ret){
      estrcpy(prsr->trc, "Expected '(' to contain <INSTRCTS>");
   }
}


bool skip_while(parser* prsr, interpret* intrp, int save)
{
   #ifndef INTERP
   inner_while(prsr, intrp, save);
   #else
   if (intrp->ignore[intrp->ig]){
      inner_while(prsr, intrp, save);
      intrp->ig -= 1;
   }
   else{
      intrp->ig -= 1;
      return true;
   }
   #endif
   return false;
}


bool is_while(parser* prsr, interpret* intrp)
{
   intrp->ret = false;
   int save = prsr->i;
   while (save == prsr->i){
      if (is_brac_boolfunc(prsr, intrp)){
         if (skip_while(prsr, intrp, save)){
            return true;
         }
      }
      if (!intrp->ret){
         return false;
      }
   }
   return intrp->ret;
}


bool inner_if(parser* prsr, interpret* intrp, int save)
{
   if (prsr->str[prsr->i] == B_OPEN){
      skip_space(prsr, NEXT);
      if (is_instrcts(prsr, intrp)){
         #ifdef INTERP
         do_skip(prsr, intrp, save);
         #else
         save = 0;
         #endif
         return true;
      }
   }
   return false;
}


bool is_if(parser* prsr, interpret* intrp)
{
   if (is_brac_boolfunc(prsr, intrp)){
      int save = intrp->ig;
      #ifdef INTERP
      if (inner_if(prsr, intrp, save)){
         intrp->ig -= 1;
         return true;
      }
      #else
      if (inner_if(prsr, intrp, save)){
         if (inner_if(prsr, intrp, save)){
            return true;
         }
      }
      #endif
      estrcpy(prsr->trc, "Expected '(' to contain <INSTRCTS>");
   }
   return false;
}


bool is_loop_if(parser* prsr, interpret* intrp)
{
   if (!strcmp(prsr->beg, WHILE)){
      return is_while(prsr, intrp);
   }
   if (!strcmp(prsr->beg, IF)){
      return is_if(prsr, intrp);
   }
   return false;
}


void skip_space(parser* prsr, int increment)
{
   int len = (int)strlen(prsr->str);
   if (prsr->i + increment <= len){
      prsr->i += increment;
   }
   while(prsr->i < len && isspace(prsr->str[prsr->i])){
      prsr->i += 1;
   }
}


void skip_chars(parser* prsr)
{
   int len = (int)strlen(prsr->str);
   while(prsr->i < len && prsr->str[prsr->i] != B_OPEN){
      prsr->i += 1;
   }
}


void estrcpy(char trc[], const char msg[])
{
   if (trc[0] == '\0'){
      strcpy(trc, msg);
   }
}


void free_struct(parser* prsr, interpret* intrp)
{
   for (int i = 0; i < VARLEN; i++){
      free(intrp->vars[i]);
   }
   free(prsr);
   free(intrp);
}




void clear_parser(parser* prsr)
{
   int a = 0;
   while (prsr->trc[a] != '\0'){
      prsr->trc[a] = '\0';
      a += 1;
   }
   prsr->i = 0;
}


void clear_strings(interpret* intrp)
{
   for (int a = 0; a < STRLEN; a++){
      intrp->istr[a] = '\0';
   }
   for (int a = 0; a < TRCLEN; a++){
      intrp->out[a] = '\0';
   }
   for (int a = 0; a < NESTLEN; a++){
      int i = 0;
      while (intrp->args_a[a][i] != 0){
         intrp->args_a[a][i] = '\0';
         i += 1;
      }
      i = 0;
      while (intrp->args_b[a][i] != 0){
         intrp->args_b[a][i] = '\0';
         i += 1;
      }
   }
}


void clear_nests(interpret* intrp)
{
   for (int a = 0; a < NESTLEN; a++){
      intrp->set_last[a] = false;
   }
   for (int a = 0; a < NESTLEN; a++){
      intrp->doub[a] = false;
   }
   for (int a = 0; a < NESTLEN; a++){
      intrp->stack[a] = false;
   }
   for (int a = 0; a < NESTLEN; a++){
      intrp->ignore[a] = false;
   }
}


void clear_intrp(interpret* intrp)
{
   clear_strings(intrp);
   clear_nests(intrp);
   intrp->var_i = 0;
   intrp->i = 0;
   intrp->ig = 0;
   intrp->loopif = false;
   intrp->testing = true;
   intrp->print_str = false;
   intrp->ret = false;
   intrp->atom_a = 0;
   intrp->atom_b = 0;
   for (int a = 0; a < VARLEN; a++){
      int x = 0;
      while (intrp->vars[a]->varstr[x] != '\0'){
         intrp->vars[a]->varstr[x] = '\0';
      }
   }
}

// reset the parser and interpret structures for testing
void clear(parser* prsr, interpret* intrp)
{
   clear_parser(prsr);
   clear_intrp(intrp);
}


void test(void){


   //----------------------------------------------//
   //-------- TESTING INITIALISE FUNCTIONS --------//
   //----------------------------------------------//

   // INIT_PRSR
   parser* prsr = init_prsr();
   prsr->i = 2;
   assert(prsr->i == 2);
   strcpy(prsr->str, "this is a string");
   assert(!strcmp(prsr->str, "this is a string"));

   // INIT_INTRP
   interpret* intrp = init_intrp();
   strcpy(intrp->vars[0]->varstr, "hello");
   assert(!strcmp(intrp->vars[0]->varstr, "hello"));
   strcpy(intrp->istr, "abcde");
   assert(!strcmp(intrp->istr, "abcde"));
   intrp->i = 8;
   assert(intrp->i == 8);

   // INIT_FUNCS
   #ifdef INTERP
   assert(intrp->func_ptr[car] == intrp_car);
   assert(intrp->func_ptr[cdr] == intrp_cdr);
   assert(intrp->func_ptr[cons] == intrp_cons);
   assert(intrp->func_ptr[print] == intrp_print);
   assert(intrp->func_ptr[set] == intrp_set);
   assert(intrp->func_ptr[plus] == intrp_plus);
   assert(intrp->func_ptr[length] == intrp_length);
   assert(intrp->func_ptr[equal] == intrp_equal);
   assert(intrp->func_ptr[less] == intrp_less);
   assert(intrp->func_ptr[greater] == intrp_greater);
   #endif


   //----------------------------------------------//
   //------ TESTING CLEAR AND FREE FUNCTIONS ------//
   //----------------------------------------------//

   assert(prsr->i == 2);
   assert(!strcmp(intrp->vars[0]->varstr, "hello"));
   assert(!strcmp(intrp->istr, "abcde"));
   assert(intrp->i == 8);
   intrp->var_i = 5;
   intrp->set_last[8] = true;
   intrp->doub[15] = true;
   intrp->loopif = true;
   #ifdef INTERP
   intrp->stack[2] = car;
   #endif
   strcpy(intrp->args_a[10], "hello");
   strcpy(intrp->args_b[11], "whatever hmm");
   strcpy(intrp->out, "hello");
   intrp->atom_a = 90;
   intrp->atom_b = 80;
   intrp->testing = false;
   intrp->print_str = true;
   intrp->ret = true;
   intrp->ignore[5] = true;
   intrp->ig = 8;

   clear(prsr, intrp);

   assert(prsr->i == 0);
   assert(strlen(intrp->vars[0]->varstr) == 0);
   assert(strlen(intrp->istr) == 0);
   assert(intrp->i == 0);
   assert(intrp->var_i == 0);
   assert(intrp->set_last[8] == false);
   assert(intrp->doub[15] == false);
   assert(intrp->loopif == false);
   assert(intrp->stack[2] == false);
   assert(strlen(intrp->args_a[10]) == 0);
   assert(strlen(intrp->args_a[11]) == 0);
   assert(strlen(intrp->out) == 0);
   assert(intrp->atom_a == false);
   assert(intrp->atom_b == false);
   assert(intrp->testing == true);
   assert(intrp->print_str == false);
   assert(intrp->ret == false);
   assert(intrp->ignore[5] == false);
   assert(intrp->ig == 0);




   //---------------------------------------------//
   //-------- TESTING SCRUBBING FUNCTIONS --------//
   //---------------------------------------------//

   //-------// SKIP_SPACE //-------//

   strcpy(prsr->str, "this   is  a   string  with  spaces");
   prsr->i = 4;
   skip_space(prsr, ZERO);
   assert(prsr->i == 7);

   prsr->i = 4;
   skip_space(prsr, NEXT);
   assert(prsr->i == 7);

   prsr->i = 0;
   skip_space(prsr, ZERO);
   assert(prsr->i == 0);

   prsr->i = 0;
   skip_space(prsr, NEXT);
   assert(prsr->i == 1);

   //-------// SKIP_CHARS //-------//

   strcpy(prsr->str, "asdf sdf  rag  (FUNC");
   prsr->i = 0;
   skip_chars(prsr);
   assert(prsr->i == 15);

   prsr->i = 16;
   skip_chars(prsr);
   assert(prsr->i == 20);


   //-------// SKIP_SCAN //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "(FUNC R ");
   prsr->i = 1;
   skip_scan(prsr, intrp);
   assert(!strcmp(prsr->beg, "FUNC"));
   assert(!intrp->set_last[intrp->i]);
   assert(prsr->i == 6);

   clear(prsr, intrp);
   strcpy(prsr->str, "(FUNCsdEF R ");
   prsr->i = 1;
   skip_scan(prsr, intrp);
   assert(!strcmp(prsr->beg, "FUNC"));
   assert(!intrp->set_last[intrp->i]);
   assert(prsr->i == 5);


   //--------------------------------------------//
   //------- TESTING PARSE BASE FUNCTIONS -------//
   //--------------------------------------------//

   //-------// IS_VAR //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "(FUNC R ");
   intrp->i = 1;

   assert(!is_var(prsr, intrp));
   assert(intrp->i == 1);
   assert(prsr->i == 0);

   prsr->i = 1;
   assert(!is_var(prsr, intrp));
   assert(intrp->i == 1);
   assert(prsr->i == 1);

   prsr->i = 2;
   assert(!is_var(prsr, intrp));
   assert(intrp->i == 1);
   assert(prsr->i == 2);

   prsr->i = 5;
   assert(!is_var(prsr, intrp));
   assert(intrp->i == 1);
   assert(prsr->i == 5);

   prsr->i = 6;
   assert(is_var(prsr, intrp));
   assert(prsr->i == 8);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif

   intrp->i = 1;

   prsr->i = 7;
   assert(!is_var(prsr, intrp));
   assert(intrp->i == 1);
   assert(prsr->i == 7);

   clear(prsr, intrp);
   strcpy(prsr->str, "(FUNC P NIL random");
   intrp->i = 1;

   prsr->i = 6;
   assert(is_var(prsr, intrp));
   assert(prsr->i == 8);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "(FUNC p NIL random");
   intrp->i = 1;

   prsr->i = 6;
   assert(!is_var(prsr, intrp));
   assert(prsr->i == 6);


   //-------// IS_NIL //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "(FUNC R NIL ()A ");
   intrp->i = 1;

   assert(!is_nil(prsr, intrp));
   assert(intrp->i == 1);
   assert(prsr->i == 0);

   prsr->i = 1;
   assert(!is_nil(prsr, intrp));
   assert(intrp->i == 1);
   assert(prsr->i == 1);

   prsr->i = 3;
   assert(!is_nil(prsr, intrp));
   assert(intrp->i == 1);
   assert(prsr->i == 3);

   prsr->i = 6;
   assert(!is_nil(prsr, intrp));
   assert(intrp->i == 1);
   assert(prsr->i == 6);

   prsr->i = 7;
   assert(!is_nil(prsr, intrp));
   assert(intrp->i == 1);
   assert(prsr->i == 7);

   prsr->i = 8;
   assert(is_nil(prsr, intrp));
   assert(prsr->i == 12);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif

   intrp->i = 1;

   prsr->i = 9;
   assert(!is_nil(prsr, intrp));
   assert(intrp->i == 1);
   assert(prsr->i == 9);

   prsr->i = 10;
   assert(!is_nil(prsr, intrp));
   assert(intrp->i == 1);
   assert(prsr->i == 10);

   prsr->i = 11;
   assert(!is_nil(prsr, intrp));
   assert(intrp->i == 1);
   assert(prsr->i == 11);

   prsr->i = 12;
   assert(!is_nil(prsr, intrp));
   assert(intrp->i == 1);
   assert(prsr->i == 12);

   prsr->i = 15;
   assert(!is_nil(prsr, intrp));
   assert(intrp->i == 1);
   assert(prsr->i == 15);


   //-------// IS_LITSTR //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "af sda 'R NIL ()A' more stuff ");
   intrp->i = 1;

   assert(!is_litstr(prsr, intrp, S_QUOTE));
   assert(intrp->i == 1);
   assert(prsr->i == 0);

   prsr->i = 6;
   assert(!is_litstr(prsr, intrp, S_QUOTE));
   assert(intrp->i == 1);
   assert(prsr->i == 6);

   prsr->i = 0;
   assert(!is_litstr(prsr, intrp, D_QUOTE));
   assert(intrp->i == 1);
   assert(prsr->i == 0);

   prsr->i = 6;
   assert(!is_litstr(prsr, intrp, D_QUOTE));
   assert(intrp->i == 1);
   assert(prsr->i == 6);

   prsr->i = 7;
   assert(is_litstr(prsr, intrp, S_QUOTE));
   assert(prsr->i == 19);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif

   intrp->i = 1;
   prsr->i = 8;
   assert(!is_litstr(prsr, intrp, S_QUOTE));
   assert(intrp->i == 1);
   assert(prsr->i == 8);

   prsr->i = 17;
   assert(!is_litstr(prsr, intrp, S_QUOTE));
   assert(intrp->i == 1);
   assert(prsr->i == 30);

   prsr->i = 18;
   assert(!is_litstr(prsr, intrp, S_QUOTE));
   assert(intrp->i == 1);
   assert(prsr->i == 18);

   clear(prsr, intrp);
   strcpy(prsr->str, "af sda 'R NIL ()A' 'more'   stuff ");
   intrp->i = 1;

   prsr->i = 7;
   assert(!is_litstr(prsr, intrp, D_QUOTE));
   assert(prsr->i == 7);
   assert(intrp->i == 1);

   prsr->i = 7;
   assert(is_litstr(prsr, intrp, S_QUOTE));
   assert(prsr->i == 19);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif

   intrp->i = 1;

   prsr->i = 19;
   assert(!is_litstr(prsr, intrp, D_QUOTE));
   assert(prsr->i == 19);
   assert(intrp->i == 1);

   prsr->i = 19;
   assert(is_litstr(prsr, intrp, S_QUOTE));
   assert(prsr->i == 28);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "af sda \"R NIL ()A\" 'more'   stuff ");
   intrp->i = 1;

   prsr->i = 7;
   assert(!is_litstr(prsr, intrp, S_QUOTE));
   assert(prsr->i == 7);
   assert(intrp->i == 1);

   prsr->i = 7;
   assert(is_litstr(prsr, intrp, D_QUOTE));
   assert(prsr->i == 19);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif



   //--------------------------------------------//
   //------- TESTING PARSE LIST FUNCTIONS -------//
   //--------------------------------------------//


   //-------// ESTRCPY //-------//

   clear(prsr, intrp);
   estrcpy(prsr->trc, "hello this is a message");
   assert(!strcmp(prsr->trc, "hello this is a message"));

   estrcpy(prsr->trc, "attempted overwrite");
   assert(!strcmp(prsr->trc, "hello this is a message"));


   //-------// IS_LIST //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "af S 'R NIL ()A' \"more\"  Z (LENGTH A) NIL");
   intrp->i = 1;

   // fail
   prsr->i = 0;
   assert(!is_list(prsr, intrp));
   assert(prsr->i == 0);
   assert(intrp->i == 1);

   // var
   prsr->i = 3;
   assert(is_list(prsr, intrp));
   assert(prsr->i == 5);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif
   intrp->i = 1;

   // literal
   prsr->i = 5;
   assert(is_list(prsr, intrp));
   assert(prsr->i == 17);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif
   intrp->i = 1;

   // fail
   prsr->i = 17;
   assert(!is_list(prsr, intrp));
   assert(prsr->i == 17);
   assert(intrp->i == 1);

   // bracketed retfunc
   prsr->i = 27;
   assert(is_list(prsr, intrp));
   assert(prsr->i == 38);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 2);
   assert(intrp->stack[1] == length);
   #endif
   intrp->i = 1;

   // nil
   prsr->i = 38;
   assert(is_list(prsr, intrp));
   assert(prsr->i == 41);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif


   //-------// IS_TRC_LIST //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "af S 'R NIL ()A' \"more\"  Z (LENGTH A) NIL");
   intrp->i = 1;

   // fail
   prsr->i = 0;
   assert(!is_trc_list(prsr, intrp));
   assert(prsr->i == 0);
   assert(!strcmp(prsr->trc, "Expected <LIST>"));
   assert(intrp->i == 1);
   assert(!intrp->doub[0]);
   assert(intrp->set_last[0]);

   // var
   clear_parser(prsr);
   prsr->i = 3;
   assert(is_trc_list(prsr, intrp));
   assert(prsr->i == 5);
   assert(!strcmp(prsr->trc, ""));
   assert(!intrp->doub[0]);
   // set last is false because it goes to intrp_list function.
   assert(!intrp->set_last[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif
   intrp->i = 1;

   // literal
   clear_parser(prsr);
   prsr->i = 5;
   assert(is_trc_list(prsr, intrp));
   assert(prsr->i == 17);
   assert(!strcmp(prsr->trc, ""));
   assert(!intrp->doub[0]);
   assert(!intrp->set_last[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif
   intrp->i = 1;

   // fail
   clear_parser(prsr);
   prsr->i = 17;
   assert(!is_trc_list(prsr, intrp));
   assert(prsr->i == 17);
   assert(!strcmp(prsr->trc, "Expected <LIST>"));
   assert(!intrp->doub[0]);
   assert(intrp->set_last[0]);
   assert(intrp->i == 1);

   // bracketed retfunc
   clear_parser(prsr);
   prsr->i = 27;
   assert(is_trc_list(prsr, intrp));
   assert(prsr->i == 38);
   assert(!strcmp(prsr->trc, ""));
   assert(!intrp->doub[0]);
   // set last remains true at index 0 because there is a push
   assert(intrp->set_last[0]);
   assert(!intrp->set_last[1]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 2);
   assert(intrp->stack[1] == length);
   #endif
   intrp->i = 1;

   // nil
   clear_parser(prsr);
   prsr->i = 38;
   assert(is_trc_list(prsr, intrp));
   assert(prsr->i == 41);
   assert(!strcmp(prsr->trc, ""));
   assert(!intrp->doub[0]);
   assert(!intrp->set_last[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif



   //-------// IS_DOUBLE_LIST //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage A B garbage A garbage");
   intrp->i = 1;

   // fail nothing
   prsr->i = 0;
   assert(!is_double_list(prsr, intrp));
   assert(prsr->i == 0);
   assert(intrp->i == 1);
   assert(intrp->doub[0]);

   // fail single var
   clear_intrp(intrp);
   intrp->i = 1;
   prsr->i = 20;
   assert(!is_double_list(prsr, intrp));
   assert(prsr->i == 22);
   assert(intrp->i == 1);
   assert(intrp->doub[0]);

   // double var
   clear_intrp(intrp);
   intrp->i = 1;
   prsr->i = 8;
   assert(is_double_list(prsr, intrp));
   assert(prsr->i == 12);
   assert(intrp->doub[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif


   clear(prsr, intrp);
   strcpy(prsr->str, "garbage '1 2' '()' garbage '2' garbage");
   intrp->i = 1;

   // fail single literal
   prsr->i = 27;
   assert(!is_double_list(prsr, intrp));
   assert(prsr->i == 31);
   assert(intrp->i == 1);
   assert(intrp->doub[0]);

   // double literal
   clear_intrp(intrp);
   intrp->i = 1;
   prsr->i = 8;
   assert(is_double_list(prsr, intrp));
   assert(prsr->i == 19);
   assert(intrp->doub[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif


   clear(prsr, intrp);
   strcpy(prsr->str, "garbage NIL NIL garbage NIL garbage");
   intrp->i = 1;

   // fail single NIL
   prsr->i = 24;
   assert(!is_double_list(prsr, intrp));
   assert(prsr->i == 28);
   assert(intrp->i == 1);
   assert(intrp->doub[0]);

   // double NIL
   clear_intrp(intrp);
   intrp->i = 1;
   prsr->i = 8;
   assert(is_double_list(prsr, intrp));
   assert(prsr->i == 16);
   assert(intrp->doub[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif


   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (LENGTH A) (CAR B) garbage (CDR C) garbage");
   intrp->i = 1;

   // fail single bracketed retfunc
   prsr->i = 35;
   assert(!is_double_list(prsr, intrp));
   assert(prsr->i == 43);
   #ifdef INTERP
   assert(intrp->i == 1);
   #else
   assert(intrp->i == 2);
   #endif
   assert(intrp->doub[0]);

   // double bracketed retfunc
   clear_intrp(intrp);
   intrp->i = 1;
   prsr->i = 8;
   assert(is_double_list(prsr, intrp));
   assert(prsr->i == 27);
   assert(intrp->doub[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 3);
   #endif


   clear(prsr, intrp);
   strcpy(prsr->str, "garbage B 'lit' garbage");
   intrp->i = 1;

   // var, lit
   prsr->i = 8;
   assert(is_double_list(prsr, intrp));
   assert(prsr->i == 16);
   assert(intrp->doub[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage B NIL garbage");
   intrp->i = 1;

   // var, nil
   prsr->i = 8;
   assert(is_double_list(prsr, intrp));
   assert(prsr->i == 14);
   assert(intrp->doub[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage B (CAR A) garbage");
   intrp->i = 1;

   // var, bracketed retfunc
   prsr->i = 8;
   assert(is_double_list(prsr, intrp));
   assert(prsr->i == 18);
   assert(intrp->doub[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 2);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage 'lit' N garbage");
   intrp->i = 1;

   // lit, var
   prsr->i = 8;
   assert(is_double_list(prsr, intrp));
   assert(prsr->i == 16);
   assert(intrp->doub[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage 'lit' NIL garbage");
   intrp->i = 1;

   // lit, nil
   prsr->i = 8;
   assert(is_double_list(prsr, intrp));
   assert(prsr->i == 18);
   assert(intrp->doub[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage 'lit' (CAR A) garbage");
   intrp->i = 1;

   // lit, bracketed retfunc
   prsr->i = 8;
   assert(is_double_list(prsr, intrp));
   assert(prsr->i == 22);
   assert(intrp->doub[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 2);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage NIL Z garbage");
   intrp->i = 1;

   // nil, var
   prsr->i = 8;
   assert(is_double_list(prsr, intrp));
   assert(prsr->i == 14);
   assert(intrp->doub[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage NIL 'lit' garbage");
   intrp->i = 1;

   // nil, lit
   prsr->i = 8;
   assert(is_double_list(prsr, intrp));
   assert(prsr->i == 18);
   assert(intrp->doub[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage NIL (CAR A) garbage");
   intrp->i = 1;

   // nil, bracketed retfunc
   prsr->i = 8;
   assert(is_double_list(prsr, intrp));
   assert(prsr->i == 20);
   assert(intrp->doub[0]);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 2);
   #endif


   //--------------------------------------------//
   //------ TESTING IS_RETFUNC DESCENDANTS ------//
   //--------------------------------------------//


   //-------// IS_BOOLFUNC //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage A (PLUS '5' '2')");
   strcpy(prsr->beg, "LESS");
   prsr->i = 8;
   intrp->i = 1;
   assert(is_boolfunc(prsr, intrp));
   assert(intrp->stack[1] == less);
   assert(prsr->i == 24);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 3);   // ??
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage NIL (CAR '5')");
   strcpy(prsr->beg, "GREATER");
   prsr->i = 8;
   intrp->i = 1;
   assert(is_boolfunc(prsr, intrp));
   assert(intrp->stack[1] == greater);
   assert(prsr->i == 21);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 3);   // ??
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (CDR '8')  (CAR '5') hmm");
   strcpy(prsr->beg, "EQUAL");
   prsr->i = 8;
   intrp->i = 1;
   assert(is_boolfunc(prsr, intrp));
   assert(intrp->stack[1] == equal);
   assert(prsr->i == 29);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 4);   // ??
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (CDR '8')  (CAR '5') hmm");
   strcpy(prsr->beg, "EQUALL");
   prsr->i = 8;
   intrp->i = 1;
   assert(!is_boolfunc(prsr, intrp));
   assert(prsr->i == 8);
   assert(intrp->i == 1);


   //-------// IS_INTFUNC //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage NIL NIL hmm");
   strcpy(prsr->beg, "PLUS");
   prsr->i = 8;
   intrp->i = 1;
   assert(is_intfunc(prsr, intrp));
   assert(intrp->stack[1] == plus);
   assert(prsr->i == 16);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 2);      // ??????
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage A hmm");
   strcpy(prsr->beg, "PLUS");
   prsr->i = 8;
   intrp->i = 1;
   assert(!is_intfunc(prsr, intrp));
   assert(intrp->stack[1] == plus);
   assert(prsr->i == 10);
   assert(intrp->i == 2);               // ???????????

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage '1 3' hmm");
   strcpy(prsr->beg, "LENGTH");
   prsr->i = 8;
   intrp->i = 1;
   assert(is_intfunc(prsr, intrp));
   assert(intrp->stack[1] == length);
   assert(prsr->i == 14);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 2);      // ??????
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage A hmm");
   strcpy(prsr->beg, "PLUSS");
   prsr->i = 8;
   intrp->i = 1;
   assert(!is_intfunc(prsr, intrp));
   assert(prsr->i == 8);
   assert(intrp->i == 1);


   //-------// IS_LISTFUNC //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage A hmm");
   strcpy(prsr->beg, "CAR");
   prsr->i = 8;
   intrp->i = 1;
   assert(is_listfunc(prsr, intrp));
   assert(prsr->i == 10);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 2);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage A hmm");
   strcpy(prsr->beg, "CDR");
   prsr->i = 8;
   intrp->i = 1;
   assert(is_listfunc(prsr, intrp));
   assert(prsr->i == 10);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 2);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage A B hmm");
   strcpy(prsr->beg, "CONS");
   prsr->i = 8;
   intrp->i = 1;
   assert(is_listfunc(prsr, intrp));
   assert(prsr->i == 12);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 2);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage A B hmm");
   strcpy(prsr->beg, "CONSS");
   prsr->i = 8;
   intrp->i = 1;
   assert(!is_listfunc(prsr, intrp));
   assert(prsr->i == 8);
   assert(intrp->i == 1);


   //-------// IS_RETFUNC //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage A B hmm");
   strcpy(prsr->beg, "CONS");
   prsr->i = 8;
   intrp->i = 1;
   assert(is_retfunc(prsr, intrp));
   assert(prsr->i == 12);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 2);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage '1 3' hmm");
   strcpy(prsr->beg, "LENGTH");
   prsr->i = 8;
   intrp->i = 1;
   assert(is_retfunc(prsr, intrp));
   assert(intrp->stack[1] == length);
   assert(prsr->i == 14);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 2);      // ??????
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage NIL (CAR '5')");
   strcpy(prsr->beg, "GREATER");
   prsr->i = 8;
   intrp->i = 1;
   assert(is_retfunc(prsr, intrp));
   assert(intrp->stack[1] == greater);
   assert(prsr->i == 21);
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 3);   // ??
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage A B hmm");
   strcpy(prsr->beg, "CONSS");
   prsr->i = 8;
   intrp->i = 1;
   assert(!is_retfunc(prsr, intrp));
   assert(prsr->i == 8);
   assert(intrp->i == 1);


   //-------------------------------------------//
   //------ TESTING IS_IOFUNC DESCENDANTS ------//
   //-------------------------------------------//


   //-------// IS_SET //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "A B garbage");
   intrp->stack[0] = set;
   intrp->i = 1;
   assert(is_set(prsr, intrp));
   assert(prsr->i == 4);
   assert(!strcmp(prsr->trc, ""));
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "'lit' B garbage");
   intrp->stack[0] = set;
   intrp->i = 1;
   assert(!is_set(prsr, intrp));
   assert(prsr->i == 0);
   assert(intrp->i == 1);
   assert(!strcmp(prsr->trc, "Expected <VAR> <LIST>"));

   clear(prsr, intrp);
   strcpy(prsr->str, "A garbage");
   intrp->stack[0] = set;
   intrp->i = 1;
   assert(!is_set(prsr, intrp));
   assert(prsr->i == 2);
   assert(intrp->i == 1);
   assert(!strcmp(prsr->trc, "Expected <VAR> <LIST>"));


   //-------// IS_PRINT //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "A  garbage");
   intrp->stack[0] = print;
   intrp->i = 1;
   assert(is_print(prsr, intrp));
   assert(prsr->i == 3);
   assert(!strcmp(prsr->trc, ""));
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "\"string\"  garbage");
   intrp->stack[0] = print;
   intrp->i = 1;
   assert(is_print(prsr, intrp));
   assert(prsr->i == 10);
   assert(!strcmp(prsr->trc, ""));
   #ifdef INTERP
   assert(intrp->i == 0);
   #else
   assert(intrp->i == 1);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage");
   intrp->stack[0] = print;
   intrp->i = 1;
   assert(!is_print(prsr, intrp));
   assert(prsr->i == 0);
   assert(intrp->i == 1);
   assert(!strcmp(prsr->trc, "Expected <LIST> or <STRING>"));


   //-------// IS_IOFUNC //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "A B garbage");
   strcpy(prsr->beg, "SET");
   assert(is_iofunc(prsr, intrp));
   assert(!strcmp(prsr->trc, ""));
   assert(intrp->stack[0] == set);
   assert(prsr->i == 4);

   clear(prsr, intrp);
   strcpy(prsr->str, "A garbage");
   strcpy(prsr->beg, "SET");
   assert(!is_iofunc(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <VAR> <LIST>"));
   assert(intrp->stack[0] == set);
   assert(prsr->i == 2);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage");
   strcpy(prsr->beg, "SET");
   assert(!is_iofunc(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <VAR> <LIST>"));
   assert(intrp->stack[0] == set);
   assert(prsr->i == 0);

   clear(prsr, intrp);
   strcpy(prsr->str, "A garbage");
   strcpy(prsr->beg, "PRINT");
   assert(is_iofunc(prsr, intrp));
   assert(!strcmp(prsr->trc, ""));
   assert(intrp->stack[0] == print);
   assert(prsr->i == 2);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage");
   strcpy(prsr->beg, "PRINT");
   assert(!is_iofunc(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <LIST> or <STRING>"));
   assert(intrp->stack[0] == print);
   assert(prsr->i == 0);


   //-------------------------------------------//
   //------- TESTING BRACKETED FUNCTIONS -------//
   //-------------------------------------------//

   //-------// IS_BRAC_RETFUNC //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (CONS A '1 2') something");
   prsr->i = 0;
   assert(!is_brac_retfunc(prsr, intrp));
   assert(prsr->i == 0);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (CONS A '1 2') something");
   prsr->i = 8;
   assert(is_brac_retfunc(prsr, intrp));
   assert(prsr->i == 23);


   //-------// INNER_BRACBOOL //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage  )  garbage");
   intrp->loopif = true;
   prsr->i = 7;
   assert(inner_bracbool(prsr, intrp));
   assert(prsr->i == 12);
   assert(!intrp->loopif);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage  )  garbage");
   intrp->loopif = true;
   prsr->i = 6;
   assert(!inner_bracbool(prsr, intrp));
   assert(prsr->i == 6);
   assert(!intrp->loopif);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage  garbage");
   intrp->loopif = true;
   prsr->i = 7;
   assert(!inner_bracbool(prsr, intrp));
   assert(prsr->i == 9);
   assert(!intrp->loopif);


   //-------// IS_BRAC_BOOLFUNC //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (EQUAL '1' '1') garbage");
   prsr->i = 8;
   assert(is_brac_boolfunc(prsr, intrp));
   assert(!strcmp(prsr->trc, ""));
   assert(prsr->i == 24);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage EQUAL '1' '1') garbage");
   prsr->i = 8;
   assert(!is_brac_boolfunc(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected '(' to contain a <BOOLFUNC>"));
   assert(prsr->i == 8);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (GARBAGE '1' '1') garbage");
   prsr->i = 8;
   assert(!is_brac_boolfunc(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <BOOLFUNC> name"));
   assert(prsr->i == 17);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (LESS garbage '1') garbage");
   prsr->i = 8;
   assert(!is_brac_boolfunc(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <LIST> <LIST>"));
   assert(prsr->i == 14);



   //-------------------------------------------//
   //-------- TESTING IS_IF DESCENDANTS --------//
   //-------------------------------------------//


   //--------// DO_SKIP //-------//

   int save;

   #ifdef INTERP

   // save is needed instead of intrp->ig to prevent overwriting

   clear(prsr, intrp);
   save = 2;
   prsr->i = 8;
   strcpy(prsr->str, "garbage (garbage) garbage");
   do_skip(prsr, intrp, save);
   assert(prsr->i == 8);

   clear(prsr, intrp);
   save = 2;
   prsr->i = 8;
   intrp->ignore[save] = true;
   strcpy(prsr->str, "garbage (garbage) garbage");
   do_skip(prsr, intrp, save);
   assert(prsr->i == 18);

   #endif


   //-------// INNER_IF //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage ((CAR A)) something");
   prsr->i = 8;
   save = 1;
   assert(inner_if(prsr, intrp, save));
   assert(!strcmp(prsr->trc, ""));
   assert(prsr->i == 18);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage ((PRINT A)(PRINT B)) something");
   prsr->i = 8;
   save = 1;
   assert(inner_if(prsr, intrp, save));
   assert(!strcmp(prsr->trc, ""));
   assert(prsr->i == 29);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (CAR A)) something");
   prsr->i = 8;
   save = 1;
   assert(!inner_if(prsr, intrp, save));
   assert(!strcmp(prsr->trc, "Expected '(' to contain a <FUNC>"));
   assert(prsr->i == 9);


   //-------// IS_IF //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (NO '1' '1') ((PRINT '5')) ((PRINT '2'))) h");
   prsr->i = 8;
   assert(!is_if(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <BOOLFUNC> name"));
   assert(prsr->i == 12);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (EQUAL '1' '1') (PRINT '5')) ((PRINT '2'))) h");
   prsr->i = 8;
   assert(!is_if(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected '(' to contain a <FUNC>"));
   assert(prsr->i == 25);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (EQUAL '1' '1') ((PRINT '5' ((PRINT '2'))) h");
   prsr->i = 8;
   assert(!is_if(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected ')' to contain a <FUNC>"));
   assert(prsr->i == 36);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (EQUAL '1' '1') ((PRINT '5')) (PRINT '2'))) h");
   prsr->i = 8;
   #ifdef INTERP
   assert(is_if(prsr, intrp));
   assert(!strcmp(prsr->trc, ""));
   assert(prsr->i == 49);
   #else
   assert(!is_if(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected '(' to contain a <FUNC>"));
   assert(prsr->i == 39);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (EQUAL '1' '1') ((GARBAGE '5')) ((PRINT '2'))) h");
   prsr->i = 8;
   assert(!is_if(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected function name"));
   assert(prsr->i == 34);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (EQUAL '2' '1') ((GARBAGE '5')) ((PRINT '2'))) h");
   prsr->i = 8;
   #ifdef INTERP
   assert(is_if(prsr, intrp));
   assert(!strcmp(prsr->trc, ""));
   assert(prsr->i == 53);
   #else
   assert(!is_if(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected function name"));
   assert(prsr->i == 34);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (EQUAL '1' '1') ((PRINT '5')) ((PRINT '2'))) h");
   prsr->i = 8;
   assert(is_if(prsr, intrp));
   assert(!strcmp(prsr->trc, ""));
   assert(prsr->i == 51);


   //----------------------------------------------//
   //-------- TESTING IS_WHILE DESCENDANTS --------//
   //----------------------------------------------//

   //-------// INNER_WHILE //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage ((PRINT A)) end");
   prsr->i = 8;
   save = 1;
   inner_while(prsr, intrp, save);
   assert(intrp->ret);
   assert(!strcmp(prsr->trc, ""));
   #ifdef INTERP
   assert(prsr->i == 1);
   #else
   assert(prsr->i == 20);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (  (PRINT A)) end");
   prsr->i = 8;
   save = 1;
   inner_while(prsr, intrp, save);
   assert(intrp->ret);
   assert(!strcmp(prsr->trc, ""));
   #ifdef INTERP
   assert(prsr->i == 1);
   #else
   assert(prsr->i == 22);
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage PRINT A)) end");
   prsr->i = 8;
   save = 1;
   inner_while(prsr, intrp, save);
   assert(!intrp->ret);
   assert(!strcmp(prsr->trc, "Expected '(' to contain <INSTRCTS>"));
   assert(prsr->i == 8);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (PRINT A)) end");
   prsr->i = 8;
   save = 1;
   inner_while(prsr, intrp, save);
   assert(!intrp->ret);
   assert(!strcmp(prsr->trc, "Expected '(' to contain a <FUNC>"));
   assert(prsr->i == 9);

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage ((PRINT garbage)) end");
   prsr->i = 8;
   save = 1;
   inner_while(prsr, intrp, save);
   assert(!intrp->ret);
   assert(!strcmp(prsr->trc, "Expected <LIST> or <STRING>"));
   assert(prsr->i == 16);


   //-------// SKIP_WHILE //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage ((PRINT A)) end");
   prsr->i = 8;
   save = 1;
   intrp->ig = 1;
   intrp->ignore[intrp->ig] = false;
   #ifdef INTERP
   assert(skip_while(prsr, intrp, save));
   assert(!intrp->ret);
   assert(prsr->i == 8);
   assert(intrp->ig == 0);
   #else
   assert(!skip_while(prsr, intrp, save));
   assert(intrp->ret);
   assert(prsr->i == 20);
   assert(intrp->ig == 1);
   #endif
   assert(!strcmp(prsr->trc, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage ((PRINT A)) end");
   prsr->i = 8;
   save = 1;
   intrp->ig = 1;
   intrp->ignore[intrp->ig] = true;
   #ifdef INTERP
   assert(!skip_while(prsr, intrp, save));
   assert(intrp->ret);
   assert(prsr->i == 1);
   assert(intrp->ig == 0);
   #else
   assert(!skip_while(prsr, intrp, save));
   assert(intrp->ret);
   assert(prsr->i == 20);
   assert(intrp->ig == 1);
   #endif
   assert(!strcmp(prsr->trc, ""));


   //-------// IS_WHILE //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (LESS '1' '1') ((PRINT B))) end");
   prsr->i = 8;
   intrp->ig = 1;
   intrp->ignore[intrp->ig] = false;
   assert(is_while(prsr, intrp));
   assert(prsr->i == 34);
   assert(!strcmp(prsr->trc, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (LESS '1' '1' ((PRINT B))) end");
   prsr->i = 8;
   assert(!is_while(prsr, intrp));
   assert(prsr->i == 22);
   assert(!strcmp(prsr->trc, "Expected ')' to contain a <BOOLFUNC>"));



   //----------------------------------------------//
   //-------- TESTING HIGH LEVEL FUNCTIONS --------//
   //----------------------------------------------//

   //-------// IS_LOOP_IF //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (LESS '1' '1') ((PRINT B))) end");
   strcpy(prsr->beg, "WHILE");
   prsr->i = 8;
   assert(is_loop_if(prsr, intrp));
   assert(prsr->i == 34);
   assert(!strcmp(prsr->trc, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (LESS '1' '1') ((PRINT B))((PRINT A))) end");
   strcpy(prsr->beg, "IF");
   prsr->i = 8;
   assert(is_loop_if(prsr, intrp));
   assert(prsr->i == 45);
   assert(!strcmp(prsr->trc, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (LESS '1' '1') ((PRINT B))) end");
   strcpy(prsr->beg, "IF");
   prsr->i = 8;
   assert(!is_loop_if(prsr, intrp));
   assert(prsr->i == 34);
   assert(!strcmp(prsr->trc, "Expected '(' to contain <INSTRCTS>"));

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (LESS '1' '1') ((PRINT B))((PRINT A))) end");
   strcpy(prsr->beg, "WHILE");
   prsr->i = 8;
   // it passes this test because the closing bracket is
   // detected in the higher level is_func function
   assert(is_loop_if(prsr, intrp));
   assert(prsr->i == 34);
   assert(!strcmp(prsr->trc, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (LESS '1' '1') ((PRINT B))) end");
   strcpy(prsr->beg, "WHILEE");
   prsr->i = 8;
   assert(!is_loop_if(prsr, intrp));
   assert(prsr->i == 8);
   // "expected function name" traceback is appended in a higher level function.
   assert(!strcmp(prsr->trc, ""));


   //-------// IS_FUNC //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage PRINT A end");
   prsr->i = 8;
   assert(is_func(prsr, intrp));
   assert(prsr->i == 16);
   assert(!strcmp(prsr->trc, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage LESS '1' '1') ((PRINT B)) end");
   prsr->i = 8;
   assert(is_func(prsr, intrp));
   assert(prsr->i == 20);
   assert(!strcmp(prsr->trc, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage LESSS '1' '1') ((PRINT B)) end");
   prsr->i = 8;
   assert(!is_func(prsr, intrp));
   assert(prsr->i == 14);
   assert(!strcmp(prsr->trc, "Expected function name"));


   //-------// IS_INSTRCT //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (PRINT A) end");
   prsr->i = 8;
   assert(is_instrct(prsr, intrp));
   assert(prsr->i == 18);
   assert(!strcmp(prsr->trc, ""));
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "()\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (PRINT (CONS (CAR '(5)') NIL)) end");
   prsr->i = 8;
   assert(is_instrct(prsr, intrp));
   assert(prsr->i == 39);
   assert(!strcmp(prsr->trc, ""));
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(5)\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage PRINT (CONS (CAR A) NIL)) end");
   prsr->i = 8;
   assert(!is_instrct(prsr, intrp));
   assert(prsr->i == 8);
   assert(!strcmp(prsr->trc, "Expected '(' to contain a <FUNC>"));
   #ifdef INTERP
   assert(!strcmp(intrp->istr, ""));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (PRINT (CONS (CAR '(3)') NIL) end");
   prsr->i = 8;
   assert(!is_instrct(prsr, intrp));
   assert(prsr->i == 38);
   assert(!strcmp(prsr->trc, "Expected ')' to contain a <FUNC>"));
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(3)\n"));
   #endif


   //-------// IS_INSTRCTS //-------//

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (PRINT A)) end");
   prsr->i = 8;
   assert(is_instrcts(prsr, intrp));
   assert(prsr->i == 19);
   assert(!strcmp(prsr->trc, ""));
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "()\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (IF (LESS '1' '1')((PRINT A))((PRINT '5')))) end");
   prsr->i = 8;
   assert(is_instrcts(prsr, intrp));
   assert(prsr->i == 53);
   assert(!strcmp(prsr->trc, ""));
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "5\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (PRINT A) end");
   prsr->i = 8;
   assert(!is_instrcts(prsr, intrp));
   assert(prsr->i == 18);
   assert(!strcmp(prsr->trc, "Expected '(' to contain a <FUNC>"));
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "()\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "garbage (PRINT A) (PRINT B)) end");
   prsr->i = 8;
   assert(is_instrcts(prsr, intrp));
   assert(prsr->i == 29);
   assert(!strcmp(prsr->trc, ""));
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "()\n()\n"));
   #endif


   //----------------------------------------//
   //---- TESTING INTERPRETING FUNCTIONS ----//
   //----------------------------------------//

   #ifdef INTERP

   //-------// INTRP_VAR //-------//

   clear(prsr, intrp);
   intrp->i = 1;
   intrp->set_last[0] = false;
   intrp->stack[0] = set;
   intrp_var('B', intrp);
   assert(intrp->var_i == 1);
   assert(intrp->set_last[0]);

   clear(prsr, intrp);
   intrp->i = 1;
   intrp->set_last[0] = true;
   intrp->stack[0] = print;
   strcpy(intrp->vars[1]->varstr, "(1 2)");
   intrp_var('B', intrp);
   assert(!strcmp(intrp->istr, "(1 2)\n"));


   //-------// INTRP_LITSTR //-------//

   clear(prsr, intrp);
   intrp->i = 1;
   intrp->set_last[0] = true;
   intrp->stack[0] = print;
   strcpy(intrp->vars[1]->varstr, "(1 2)");
   intrp_litstr(intrp, S_QUOTE, intrp->vars[1]->varstr);
   assert(intrp->i == 0);
   assert(!strcmp(intrp->istr, "(1 2)\n"));

   clear(prsr, intrp);
   intrp->i = 1;
   intrp->set_last[0] = true;
   intrp->stack[0] = print;
   strcpy(intrp->vars[1]->varstr, "hello");
   intrp_litstr(intrp, D_QUOTE, intrp->vars[1]->varstr);
   assert(intrp->print_str);
   assert(intrp->i == 0);
   assert(!strcmp(intrp->istr, "hello\n"));


   //-------// INTRP_LIST //-------//

   // set last argument of current function
   // this also falls through to execution
   clear(prsr, intrp);
   intrp->i = 1;
   intrp->stack[0] = print;
   intrp->doub[0] = false;
   intrp->set_last[0] = true;
   intrp_list("(1 2 3 4)", intrp);
   assert(!strcmp(intrp->args_b[0], "(1 2 3 4)"));
   assert(!intrp->set_last[0]);
   assert(!strcmp(intrp->istr, "(1 2 3 4)\n"));

   // set first argument of current function
   clear(prsr, intrp);
   intrp->i = 1;
   intrp->stack[0] = set;
   intrp->doub[0] = true;
   intrp->set_last[0] = false;
   intrp_list("(1 2 3)", intrp);
   assert(!strcmp(intrp->args_a[0], "(1 2 3)"));
   assert(intrp->set_last[0]);


   //-------// EXECUTE //-------//

   // two functions: cons then a print
   clear(prsr, intrp);
   intrp->i = 2;
   intrp->stack[0] = print;
   intrp->set_last[0] = true;
   intrp->doub[0] = false;
   intrp->stack[1] = cons;
   intrp->doub[1] = true;
   intrp->set_last[1] = false;
   strcpy(intrp->args_a[1], "50");
   strcpy(intrp->args_b[1], "()");
   execute(intrp);
   assert(intrp->i == 0);
   assert(!strcmp(intrp->args_b[0], "(50)"));
   assert(!strcmp(intrp->istr, "(50)\n"));

   // three functions saved, but only execute 1
   // because there are missing arguments for the first 2.
   clear(prsr, intrp);
   intrp->i = 3;
   intrp->stack[0] = print;
   intrp->set_last[0] = true;
   intrp->doub[0] = false;

   intrp->stack[1] = cons;
   intrp->doub[1] = true;
   intrp->set_last[1] = false;

   intrp->stack[2] = car;
   intrp->doub[2] = false;
   intrp->set_last[2] = true;

   strcpy(intrp->args_b[2], "(70)");
   execute(intrp);
   assert(intrp->i == 2);
   assert(!strcmp(intrp->istr, ""));
   assert(!strcmp(intrp->args_a[1], "70"));
   assert(intrp->set_last[1]);



   //-------// PUSH_STACK //-------//

   clear(prsr, intrp);
   push_stack(intrp, length);
   assert(intrp->i == 1);
   assert(intrp->stack[0] == length);


   //-------// SUM //-------//

   // See linked.c testing with lisp_reduce


   //-------// INTRP_CAR //-------//

   clear(prsr, intrp);
   intrp->i = 2;
   strcpy(intrp->args_b[intrp->i], "(1 (2))");
   intrp_car(intrp);
   assert(!strcmp(intrp->out, "1"));


   //-------// INTRP_CDR //-------//

   clear(prsr, intrp);
   intrp->i = 21;
   strcpy(intrp->args_b[intrp->i], "(1 (2))");
   intrp_cdr(intrp);
   assert(!strcmp(intrp->out, "((2))"));


   //-------// INTRP_CONS //-------//

   clear(prsr, intrp);
   intrp->i = 21;
   strcpy(intrp->args_b[intrp->i], "(1 (2))");
   strcpy(intrp->args_a[intrp->i], "5");
   intrp_cons(intrp);
   assert(!strcmp(intrp->out, "(5 1 (2))"));


   //-------// INTRP_ATOMS //-------//

   clear(prsr, intrp);
   intrp->i = 2;
   strcpy(intrp->args_a[intrp->i], "(1 ((2) 4))");
   strcpy(intrp->args_b[intrp->i], "5");
   intrp_atoms(intrp);
   assert(intrp->atom_a == 7);
   assert(intrp->atom_b == 5);


   //-------// INTRP_PLUS //-------//

   clear(prsr, intrp);
   intrp->i = 2;
   strcpy(intrp->args_a[intrp->i], "(-7 0)");
   strcpy(intrp->args_b[intrp->i], "((4))");
   intrp_plus(intrp);
   assert(!strcmp(intrp->out, "-3"));


   //-------// INTRP_LENGTH //-------//

   clear(prsr, intrp);
   intrp->i = 0;
   strcpy(intrp->args_b[intrp->i], "(-7 0 ((3) 4 5))");
   intrp_length(intrp);
   assert(!strcmp(intrp->out, "3"));


   //-------// INCREMENT_IG //-------//

   clear(prsr, intrp);
   increment_ig(intrp);
   assert(intrp->ig == 1);
   assert(!intrp->ignore[1]);
   assert(!strcmp(intrp->out, "1"));


   //-------// EXECUTE_BOOLFUNC //-------//

   clear(prsr, intrp);
   intrp->loopif = false;
   execute_boolfunc(intrp);
   assert(!strcmp(intrp->out, "0"));
   assert(!intrp->ignore[0]);

   clear(prsr, intrp);
   intrp->loopif = true;
   intrp->i = 0;
   execute_boolfunc(intrp);
   assert(!strcmp(intrp->out, "0"));
   assert(intrp->ignore[0]);


   //-------// INTRP_EQUAL //-------//

   clear(prsr, intrp);
   intrp->i = 2;
   strcpy(intrp->args_a[intrp->i], "(-7 11)");
   strcpy(intrp->args_b[intrp->i], "((4))");
   intrp_equal(intrp);
   assert(!strcmp(intrp->out, "1"));

   clear(prsr, intrp);
   intrp->i = 2;
   strcpy(intrp->args_a[intrp->i], "(-7 11)");
   strcpy(intrp->args_b[intrp->i], "((1))");
   intrp_equal(intrp);
   assert(!strcmp(intrp->out, "0"));


   //-------// INTRP_LESS //-------//

   clear(prsr, intrp);
   intrp->i = 2;
   strcpy(intrp->args_a[intrp->i], "(-7 11)");
   strcpy(intrp->args_b[intrp->i], "((4))");
   intrp_greater(intrp);
   assert(!strcmp(intrp->out, "0"));

   clear(prsr, intrp);
   intrp->i = 2;
   strcpy(intrp->args_a[intrp->i], "(-7 11)");
   strcpy(intrp->args_b[intrp->i], "((1))");
   intrp_greater(intrp);
   assert(!strcmp(intrp->out, "1"));


   //-------// INTRP_GREATER //-------//

   clear(prsr, intrp);
   intrp->i = 2;
   strcpy(intrp->args_a[intrp->i], "(-7 11)");
   strcpy(intrp->args_b[intrp->i], "((40))");
   intrp_less(intrp);
   assert(!strcmp(intrp->out, "1"));

   clear(prsr, intrp);
   intrp->i = 2;
   strcpy(intrp->args_a[intrp->i], "(-7 11)");
   strcpy(intrp->args_b[intrp->i], "((-11))");
   intrp_less(intrp);
   assert(!strcmp(intrp->out, "0"));


   //-------// INTRP_SET //-------//

   clear(prsr, intrp);
   intrp->var_i = 3;
   strcpy(intrp->args_b[intrp->i], "(1 2 3)");
   intrp_set(intrp);
   assert(!strcmp(intrp->vars[3]->varstr, "(1 2 3)"));


   //-------// INTRP_PRINT //-------//

   clear(prsr, intrp);
   intrp->print_str = false;
   strcpy(intrp->args_b[intrp->i], "(1 2 3)");
   intrp_print(intrp);
   assert(!strcmp(intrp->out, "(1 2 3)"));
   assert(!strcmp(intrp->istr, "(1 2 3)\n"));

   clear(prsr, intrp);
   intrp->print_str = true;
   strcpy(intrp->out, "hello there");
   intrp_print(intrp);
   assert(!strcmp(intrp->istr, "hello there\n"));

   // During non-testing, intrp->out is printed directly to terminal.
   // This is the only situation during interpreting
   // where the program prints directly to the screen.

   #endif



   //----------------------------------------//
   //------- TESTING IS_PROG FUNCTION -------//
   //----------------------------------------//

   // FALSE IS_PROG EXAMPLES

   // incorrect opening statement
   clear(prsr, intrp);
   strcpy(prsr->str, "");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected '(' to start <PROG>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "\0");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected '(' to start <PROG>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "WHILE");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected '(' to start <PROG>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "sdfdsad");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected '(' to start <PROG>"));
   assert(!strcmp(intrp->istr, ""));

   // testing tracebacks
   clear(prsr, intrp);
   strcpy(prsr->str, "(SET A '-505'))");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected '(' to contain a <FUNC>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "(WHILE (EQUAL A B) ((CONS A NIL))))");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected '(' to contain a <FUNC>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((CAR A");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected ')' to contain a <FUNC>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((CAR A)");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected '(' to contain a <FUNC>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <VAR> <LIST>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((PRINT");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <LIST> or <STRING>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((CONS");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <LIST> <LIST>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((GREATER nonsense");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <LIST> <LIST>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((PLUS nonsense");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <LIST> <LIST>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((LENGTH nonsense");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <LIST>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((CAR  ");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <LIST>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((CDR (garbage A B)  ");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <LIST>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((GARBAGE");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected function name"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF ");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected '(' to contain a <BOOLFUNC>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <BOOLFUNC> name"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (LESS");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <LIST> <LIST>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (LESS A B");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected ')' to contain a <BOOLFUNC>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (LESS A B)");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected '(' to contain <INSTRCTS>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (LESS A B) (");
   assert(!is_prog(prsr, intrp));
   #ifndef INTERP
   assert(!strcmp(prsr->trc, "Expected '(' to contain a <FUNC>"));
   #else
   assert(!strcmp(prsr->trc, "Expected '(' to contain <INSTRCTS>"));
   #endif
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (LESS '1' '2') (");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected '(' to contain a <FUNC>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (LESS A B) ((PRINT A))");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected '(' to contain <INSTRCTS>"));
   assert(!strcmp(intrp->istr, "")); // because A and B are both 0


   clear(prsr, intrp);
   strcpy(prsr->str, "((WHILE (LESS A B) ");
   assert(!is_prog(prsr, intrp));
   #ifndef INTERP
   assert(!strcmp(prsr->trc, "Expected '(' to contain <INSTRCTS>"));
   #else
   assert(!strcmp(prsr->trc, "Expected ')' to contain a <FUNC>"));
   #endif
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((WHILE (LESS A B) ((PRINT A))");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected ')' to contain a <FUNC>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A (SET B '5')))");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <VAR> <LIST>"));
   assert(!strcmp(intrp->istr, ""));

   // incorrect nested statements with traceback
   clear(prsr, intrp);
   strcpy(prsr->str, "((WHILE (GREATER '1' '1') ((IF (EQUAL Y Z) ((SET A (CONS B (CONS C NIL))))((WHILE(LESSS I J) ((PRINT \"ok\")))) )) ))");
   #ifndef INTERP
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <BOOLFUNC> name"));
   assert(!strcmp(intrp->istr, ""));
   #else
   assert(is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, ""));
   assert(!strcmp(intrp->istr, ""));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((WHILE (GREATER '1' '1') ((IF (EQUAL Y Z) ((SET A (CONS B (CONS C NIL)))) ((WHILE (LESS I J) ((PRINT ()))))))))");
   #ifndef INTERP
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <LIST> or <STRING>"));
   assert(!strcmp(intrp->istr, ""));
   #else
   assert(is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, ""));
   assert(!strcmp(intrp->istr, ""));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "(\n(\nSET\n A\n (\nIF\n (\nEQUAL\n '1'\n '1'\n)\n (\n(\nLENGTH\n B\n)\n)\n)\n)\n)");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <VAR> <LIST>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A IF (LESS '1' '1')((PRINT A))((PRINT B))))");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <VAR> <LIST>"));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "\n(\n(\nSET\n A\n (\nIF\n (\nLESS \n'1'\n '1'\n)\n(\n(\nPRINT\r A\r)\r)\r(\n(\rPRINT\n B\n)\n)\n)\n)");
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected <VAR> <LIST>"));
   assert(!strcmp(intrp->istr, ""));


   // TRUE IS_PROG EXAMPLES
   clear(prsr, intrp);
   strcpy(prsr->str, "((WHILE (GREATER '1' '1') ((PRINT \"hello\")) ))");
   assert(is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, ""));
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A '-505'))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((WHILE (GREATER A B) ((CONS A NIL))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "((WHILE (GREATER '1' '1') ((IF (EQUAL Y Z) ((SET A (CONS B (CONS C NIL)))) ((WHILE (LESS I J) ((PRINT \"ok\"))))))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   assert(!strcmp(intrp->istr, ""));

   clear(prsr, intrp);
   strcpy(prsr->str, "\n(\n(\nWHILE \n(\nLESS \nA\n B\n)\n (\n(\nCONS \nA\n NIL\n)\n)\n)\n)\n");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   assert(!strcmp(intrp->istr, ""));


   // INTERPRETING EXAMPLES:
   clear(prsr, intrp);
   strcpy(prsr->str, "((PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "()\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((PRINT NIL))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "()\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET C NIL)(PRINT C))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "()\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((PRINT (CONS '5' NIL)))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(5)\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET C (CONS '5' NIL)) (PRINT C))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(5)\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((PRINT '82'))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "82\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((PRINT '(82)'))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(82)\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((PRINT '(8   2  ( -4040) )'))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(8 2 (-4040))\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((PRINT \"hello world!\"))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "hello world!\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A '-505')(PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "-505\n"));
   #endif

   // IF STATEMENTS
   // using equal
   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '1' '1')  ((PRINT \"equal!\")) ((PRINT \"not equal\"))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "equal!\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '2' '1')  ((PRINT \"equal!\")) ((PRINT \"not equal!\"))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "not equal!\n"));
   #endif

   // using less
   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (LESS '2' '1')  ((PRINT \"true\")) ((PRINT \"false\"))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "false\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (LESS '1' '1')  ((PRINT \"true\")) ((PRINT \"false\"))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "false\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (LESS '1' '2')  ((PRINT \"true\")) ((PRINT \"false\"))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "true\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (LESS '2' '1') ((SET A '5')) ((SET A '2'))) (PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "2\n"));
   #endif

   // using greater
   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (GREATER '2' '1')  ((PRINT \"true\")) ((PRINT \"false\"))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "true\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (GREATER '1' '2') ((PRINT \"true\")) ((PRINT \"false\"))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "false\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (GREATER '1' '1') ((PRINT \"true\")) ((PRINT \"false\"))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "false\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (GREATER '2' '1') ((SET A '5')) ((SET A '2'))) (PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "5\n"));
   #endif

   // DEEP NESTING
   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A (CONS '15' NIL))(PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(15)\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET J (CONS '15' (CONS '18' NIL)))(PRINT J))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(15 18)\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET F (CONS '15' (CONS '18' (CONS '40' NIL))))(PRINT F))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(15 18 40)\n"));
   #endif


   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A (CONS (CONS '17' NIL) NIL))(PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "((17))\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A (CONS (CONS (CONS '8080' NIL) NIL) NIL))(PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(((8080)))\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((PRINT (CONS (CONS (CONS '-70' NIL) NIL) NIL)))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(((-70)))\n"));
   #endif


//   X ---> X
//   |      |
//   V      V
//   9      X --> X --> X
//          |     |     |
//          V     V     V
//          80    77    101


   clear(prsr, intrp);
   strcpy(prsr->str, "((SET D (CONS '9' (CONS (CONS '80' (CONS '77' (CONS '101' NIL))) NIL))) (PRINT D))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(9 (80 77 101))\n"));
   #endif


//   X -------> X ---> X
//   |          |      |
//   V          V      V
//   X --> X    9      X --> X --> X
//   |     |           |     |     |
//   V     V           V     V     V
//   320   -5          80    77    101

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET Y (CONS (CONS '320' (CONS '-5' NIL)) (CONS '90' (CONS (CONS '801' (CONS '707' (CONS '1991' NIL))) NIL)))) (PRINT Y))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "((320 -5) 90 (801 707 1991))\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((PRINT (CONS (CONS '1' (CONS '-1' NIL)) (CONS '97' (CONS (CONS '82' (CONS '-80' (CONS '1891' NIL))) NIL)))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "((1 -1) 97 (82 -80 1891))\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET B '5') (SET C NIL) (SET D (CONS B C)) (PRINT D))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(5)\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET B (CONS '1' (CONS '2' (CONS '3' NIL)))) (SET C (CONS '-60' NIL)) (SET D (CONS B C)) (PRINT D))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "((1 2 3) -60)\n"));
   #endif


   // INT FUNCS
   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A (PLUS '3' '4')) (PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "7\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A (CONS '7' (CONS '19' NIL))) (PRINT (LENGTH A)))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "2\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A (CONS '7' (CONS (CONS '3' NIL) NIL))) (PRINT (LENGTH A)))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "2\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((PRINT (LENGTH (CONS '17' (CONS '4' (CONS (PLUS '-2' '-5') NIL))))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "3\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A (PLUS '5' '-2')) (SET B (PLUS A '10')) (PRINT B))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "13\n"));
   #endif

   // NESTED IFS
   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '3' (PLUS '2' '1')) ((SET A '15')) ((SET A '150'))) (PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "15\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '3' (PLUS '2' '1')) ((IF (EQUAL '1' '0') ((SET A '15')) ((SET A '90')))) ((SET A '150'))) (PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "90\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '3' (PLUS '2' '1'))   ((SET A '15'))   ((IF (EQUAL '1' '1') ((SET A '400')) ((SET A '500')))))   (PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "15\n"));
   #endif


   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '3' (PLUS '2' '1'))   ((IF (LESS '1' '2') ((SET A '81')) ((SET A '55'))))   ((IF (EQUAL '1' '1') ((SET A '400')) ((SET A '500')))))   (PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "81\n"));
   #endif


   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '3' (PLUS '2' '1'))   ((IF (LESS '3' '2') ((SET A '81')) ((SET A '55'))))   ((IF (EQUAL '1' '1') ((SET A '400')) ((SET A '500')))))   (PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "55\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (LESS '3' (PLUS '2' '1'))   ((IF (LESS '1' '2') ((SET A '81')) ((SET A '55'))))   ((IF (EQUAL '1' '1') ((SET A '400')) ((SET A '500')))))   (PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "400\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (LESS '3' (PLUS '2' '1'))   ((IF (LESS '1' '2') ((SET A '81')) ((SET A '55'))))   ((IF (EQUAL '1' '2') ((SET A '400')) ((SET A '500')))))   (PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "500\n"));
   #endif

   // TRIPLE NESTED IFS
   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '1' '1')((IF (EQUAL '1' '1')((IF (EQUAL '1' '1')((PRINT '1'))((PRINT '2'))))((IF (EQUAL '1' '1')((PRINT '3'))((PRINT '4'))))))((IF (EQUAL '1' '1')((IF (EQUAL '1' '1')((PRINT '5'))((PRINT '6'))))((IF (EQUAL '1' '1')((PRINT '7'))((PRINT '8'))))))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "1\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '1' '1')((IF (EQUAL '1' '1')((IF (EQUAL '1' '5')((PRINT '1'))((PRINT '2'))))((IF (EQUAL '1' '1')((PRINT '3'))((PRINT '4'))))))((IF (EQUAL '1' '1')((IF (EQUAL '1' '1')((PRINT '5'))((PRINT '6'))))((IF (EQUAL '1' '1')((PRINT '7'))((PRINT '8'))))))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "2\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '1' '1')((IF (EQUAL '1' '3')((IF (EQUAL '1' '1')((PRINT '1'))((PRINT '2'))))((IF (EQUAL '1' '1')((PRINT '3'))((PRINT '4'))))))((IF (EQUAL '1' '1')((IF (EQUAL '1' '1')((PRINT '5'))((PRINT '6'))))((IF (EQUAL '1' '1')((PRINT '7'))((PRINT '8'))))))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "3\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '1' '1')((IF (EQUAL '3' '1')((IF (EQUAL '1' '1')((PRINT '1'))((PRINT '2'))))((IF (EQUAL '3' '1')((PRINT '3'))((PRINT '4'))))))((IF (EQUAL '1' '1')((IF (EQUAL '1' '1')((PRINT '5'))((PRINT '6'))))((IF (EQUAL '1' '1')((PRINT '7'))((PRINT '8'))))))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "4\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '8' '1')((IF (EQUAL '1' '1')((IF (EQUAL '1' '1')((PRINT '1'))((PRINT '2'))))((IF (EQUAL '1' '1')((PRINT '3'))((PRINT '4'))))))((IF (EQUAL '1' '1')((IF (EQUAL '1' '1')((PRINT '5'))((PRINT '6'))))((IF (EQUAL '1' '1')((PRINT '7'))((PRINT '8'))))))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "5\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '2' '1')((IF (EQUAL '1' '1')((IF (EQUAL '1' '1')((PRINT '1'))((PRINT '2'))))((IF (EQUAL '1' '1')((PRINT '3'))((PRINT '4'))))))((IF (EQUAL '1' '1')((IF (EQUAL '2' '1')((PRINT '5'))((PRINT '6'))))((IF (EQUAL '1' '1')((PRINT '7'))((PRINT '8'))))))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "6\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '4' '1')((IF (EQUAL '1' '1')((IF (EQUAL '1' '1')((PRINT '1'))((PRINT '2'))))((IF (EQUAL '1' '1')((PRINT '3'))((PRINT '4'))))))((IF (EQUAL '1' '4')((IF (EQUAL '1' '1')((PRINT '5'))((PRINT '6'))))((IF (EQUAL '1' '1')((PRINT '7'))((PRINT '8'))))))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "7\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '1' '0')((IF (EQUAL '1' '1')((IF (EQUAL '1' '1')((PRINT '1'))((PRINT '2'))))((IF (EQUAL '1' '1')((PRINT '3'))((PRINT '4'))))))((IF (EQUAL '1' '3')((IF (EQUAL '1' '1')((PRINT '5'))((PRINT '6'))))((IF (EQUAL '1' '90')((PRINT '7'))((PRINT '8'))))))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "8\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "(\
      (IF (EQUAL (LENGTH A) '0')   \n(\n(\nPRINT \n\"A HAS ZERO LENGTH\")) ((PRINT \"FAILURE\")))\
   )");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "A HAS ZERO LENGTH\n"));
   #endif

   // printset
   clear(prsr, intrp);
   strcpy(prsr->str, "(\
      (IF\n (\nEQUAL\n (LENGTH NIL) '0')   ((PRINT \"NZ\")) ((PRINT \"FAILURE\")))\
      (IF (EQUAL (LENGTH '(1)'\n)\n '0') ((PRINT \"FAILURE\")) ((PRINT \"(1)!Z\")))\
      (IF (EQUAL (LENGTH A) '0')   \n(\n(\nPRINT \n\"A HAS ZERO LENGTH\")) ((PRINT \"FAILURE\")))\
      (SET L\n (\nCONS \n'2' NIL)\n)\
      (PRINT L)\
      (IF (EQUAL (CAR L) '2') ((PRINT \"L1 has Car 2\"))((PRINT \"FAILURE\")))\
      (SET M (CONS '3' (CONS '4' (CONS '5' NIL))))\
      (PRINT M)\
      (SET N (CONS '1' L))\
      (PRINT N)\
      (SET P (CONS N M))\
      (PRINT \nP)\
   )");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "NZ\n(1)!Z\nA HAS ZERO LENGTH\n(2)\nL1 has Car 2\n(3 4 5)\n(1 2)\n((1 2) 3 4 5)\n"));
   #endif


   // while
   clear(prsr, intrp);
   strcpy(prsr->str, "(\n(\nWHILE \n(\nLESS \n'1' \n'1'\n) \n(\n(\nPRINT\n \"hello!\"))))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, ""));
   #endif


   clear(prsr, intrp);
   strcpy(prsr->str, "(\
      (SET A '5')\
      (WHILE (LESS '1' A)\
         (\
            (PRINT A)\
            (SET A (PLUS A '-1'))\
         )\
      )\
   )");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "5\n4\n3\n2\n"));
   #endif


   // first 20 fib numbers
   clear(prsr, intrp);
   strcpy(prsr->str, "(\
      (SET L '(1 0)')\
      (SET C '2')\
      (WHILE (LESS C '20') (\
         (SET N (PLUS (CAR L)  (CAR (CDR L))))\
         (SET M (CONS N L))\
         (SET L M)\
         (SET B (PLUS '1' C))\
         (SET C B)\
      ))\
      (PRINT M)\
   )");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(4181 2584 1597 987 610 377 233 144 89 55 34 21 13 8 5 3 2 1 1 0)\n"));
   #endif

   // nested while
   clear(prsr, intrp);
   strcpy(prsr->str, "(\
      (SET A '4')\
      (WHILE (LESS '1' A)(\
         (SET B '5')\
         (WHILE (LESS '1' B)(\
            (PRINT B)\
            (SET B (PLUS B '-1'))\
         ))\
         (SET A (PLUS A '-1'))\
      ))\
   )");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "5\n4\n3\n2\n5\n4\n3\n2\n5\n4\n3\n2\n"));
   #endif


   // nested while and if
   clear(prsr, intrp);
   strcpy(prsr->str, "(\
      (SET A '4')\
      (\nWHILE \n(\nLESS \n'1'\n A\n)\n(\
         (\nIF\n (\nEQUAL \nA \n'3'\n)(\
            (\nSET \nB \n'5')\
            (\nWHILE \n(\nLESS \n'1'\n B\n)\n(\
               (\nPRINT \nB\n)\
               (\nSET \nB \n(\nPLUS\n B \n'-1'\n)\n)\
            )\n)\n)\
            (\n(\nSET \nC \n'60'\n)\n(\nPRINT \nC\n)\n)\
         )\
         (\nSET \nA\n (\nPLUS\n A \n'-1'\n)\n)\
      ))\
   )");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "60\n5\n4\n3\n2\n60\n"));
   #endif


   // nested boolfuncs
   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A '(5 4 (3 2))')(SET B '8')\
   (PRINT (CAR (CAR (CDR (CDR A)))))\
   (PRINT (CAR A))\
   (PRINT (CAR (CDR A)))\
   (PRINT (GREATER (CAR (CAR (CDR (CDR A)))) (CAR A)))\
   (PRINT (LESS (CAR (CDR A)) (CAR A)))\
   (PRINT \"while evaluation:\")\
   (PRINT (LESS (GREATER (CAR (CAR (CDR (CDR A)))) (CAR A)) (LESS (CAR (CDR A)) (CAR A))))\
   (SET F '1')\
   (WHILE (GREATER F '0') ((PRINT \"looping!\") (SET F (PLUS F '-1'))))\
   (WHILE (LESS (GREATER (CAR (CAR (CDR (CDR A)))) B) (LESS (CAR (CDR A)) (CAR A))) ((PRINT B)(SET B (PLUS B '-1')))))) ");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "3\n5\n4\n0\n1\nwhile evaluation:\n1\nlooping!\n8\n7\n6\n5\n4\n3\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A (CONS '3' NIL)) (SET B A) (SET F (CONS (CONS '5' NIL) NIL)) (PRINT A) (PRINT B) (PRINT F))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(3)\n(3)\n((5))\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '1' '1') ((SET A (GREATER (LESS (EQUAL '1' '2') (LESS '1' '2'))(GREATER '5' '6'))))((PRINT \"failure\")))(PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "1\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '1' '1') ((SET A (GREATER (LESS (EQUAL '1' '2') (LESS '1' '2'))(GREATER '5' '1'))))((PRINT \"failure\")))(PRINT A))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "0\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A '(5 3)')(SET G (LENGTH A))(PRINT A)(PRINT G))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(5 3)\n2\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET B '(5 4 3)')(SET A (LENGTH '(5 3)'))(SET G (GREATER (LENGTH B) '3'))(PRINT B)(PRINT A)(PRINT G))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(5 4 3)\n2\n0\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A '(5 6 (3 4) (6 5) 9)') (SET B (CAR A)) (SET C (CAR (CDR A))) (PRINT A) (PRINT B) (PRINT C))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(5 6 (3 4) (6 5) 9)\n5\n6\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((SET A '(5 6 (3 4) (7 10) 9)') (SET B (CONS (CAR (CDR A)) NIL)) (PRINT A) (PRINT B) (PRINT (CONS (CAR (CAR (CDR (CDR A)))) B)))");
   assert(is_prog(prsr, intrp));
   assert(strlen(prsr->trc)==0);
   #ifdef INTERP
   assert(!strcmp(intrp->istr, "(5 6 (3 4) (7 10) 9)\n(6)\n(3 6)\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '1' '1') ((PRINT \"YES\"))((GARBAGE))))");
   #ifndef INTERP
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected function name"));
   #else
   assert(is_prog(prsr, intrp));
   assert(!strcmp(intrp->istr, "YES\n"));
   #endif

   clear(prsr, intrp);
   strcpy(prsr->str, "((IF (EQUAL '1' '1') ((PRINT \"YES\"))((GARBAGE)))(PRINT \"skip successful\"))");
   #ifndef INTERP
   assert(!is_prog(prsr, intrp));
   assert(!strcmp(prsr->trc, "Expected function name"));
   #else
   is_prog(prsr, intrp);
   assert(!strcmp(intrp->istr, "YES\nskip successful\n"));
   #endif

   free_struct(prsr, intrp);
}
