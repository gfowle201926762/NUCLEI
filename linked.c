#include "lisp.h"
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>

#define LISPIMPL "Linked"
#define UNDEFINED 0
#define BASE 10
#define LISTSTRLEN 1000
#define LISTLISP 1000

struct lisp {
   struct lisp* car;
   struct lisp* cdr;
   atomtype atom;
};



void test_linked(void);
void lisp_rec_copy(const lisp* l, lisp* p);
void atom_tostring(const lisp* l, char* str, int* i);
void rec_tostring(const lisp* l, char* str, int* i);
int power(int c, int exp);
void abs_tostring(char* str, int* i, int abs, int prev_count);
void pre_car(char* str, int* i);
void post_car(char* str, int* i);
int digit_reader(const char* str, int* i);
bool bracket_calc(const char* str, int index);
void read_string(const char* str, int* i, lisp** l);
void save_cdr(lisp** l, lisp* ll[], int* i, int* count, const char* str);
bool string_parser(const char* str, int len);
void get_cdr(lisp** l, lisp* ll[], int* i, int* count, const char* str);
void test_times(lisp* l, atomtype* accum);
void test_atms(lisp* l, atomtype* accum);



lisp* lisp_atom(const atomtype a)
{
   lisp* p = calloc(1, sizeof(lisp));
   if (p == NULL){
      on_error("[ERROR] Could not calloc memory.");
   }
   p->atom = a;
   return p;
}


lisp* lisp_cons(const lisp* l1,  const lisp* l2)
{
   lisp* p = lisp_atom(UNDEFINED);
   p->car = (lisp*)l1;
   p->cdr = (lisp*)l2;
   return p;
}


lisp* lisp_car(const lisp* l)
{
   if (l == NULL){
      return (lisp*)NULL;
   }
   return l->car;
}


lisp* lisp_cdr(const lisp* l)
{
   if (l == NULL){
      return (lisp*)NULL;
   }
   return l->cdr;
}


atomtype lisp_getval(const lisp* l)
{
   if (l == NULL){
      return 0;
   }
   return l->atom;
}

// Takes an original lisp structure l, and a new lisp structure p.
// Goes through the original structure recursively, using lisp_atom to calloc new lisp structures.
void lisp_rec_copy(const lisp* l, lisp* p)
{
   if (l->cdr != NULL){
      lisp* p_cdr = lisp_atom(l->cdr->atom);
      p->cdr = p_cdr;
      lisp_rec_copy(l->cdr, p->cdr);
   }
   if (l->car != NULL){
      lisp* p_car = lisp_atom(l->car->atom);
      p->car = p_car;
      lisp_rec_copy(l->car, p->car);
   }
}


lisp* lisp_copy(const lisp* l)
{
   if (l == NULL){
      return (lisp*)NULL;
   }

   // p is the head of the new list.
   lisp* p = lisp_atom(l->atom);

   // use recursion to copy the list.
   lisp_rec_copy(l, p);
   return p;
}


int lisp_length(const lisp* l)
{
   if (l == NULL){
      return 0;
   }
   if (lisp_isatomic(l)){
      return 0;
   }
   int count = 1;
   lisp* l_cdr = l->cdr;
   while (l_cdr != NULL){
      count += 1;
      l_cdr = l_cdr->cdr;
   }
   return count;
}


bool lisp_isatomic(const lisp* l)
{
   if (l == NULL){
      return false;
   }
   if (l->cdr == NULL && l->car == NULL){
      return true;
   }
   return false;
}

// a power function because there is no -lm in the makefile provided.
int power(int c, int exp)
{
   int m = 1;
   for (int i = 0; i < exp; i++){
      m *= BASE;
   }
   return c * m;
}

// recursively find the next digit of an atom.
void abs_tostring(char* str, int* i, int abs, int prev_count)
{
   int digit = abs;
   int count = 0;
   while (digit >= BASE){
      digit /= BASE;
      count += 1;
   }
   for (int zeros = 0; zeros < prev_count - (count + 1); zeros++){
      str[*i] = '0';
      *i += 1;
   }
   str[*i] = (char)((int)'0' + digit);
   *i += 1;
   if (count > 0){
      abs -= power(digit, count);
      abs_tostring(str, i, abs, count);
   }
}


void atom_tostring(const lisp* l, char* str, int* i)
{
   if (*i - 1 >= 0 && str[*i - 1] == '('){
      *i -= 1;
   }
   int abs = l->atom;
   if (l->atom < 0){
      abs = -(l->atom);
      str[*i] = '-';
      *i += 1;
   }
   abs_tostring(str, i, abs, 0);
   str[*i] = ' ';
   *i += 1;
}

// format the string before a new car has been looked at.
void pre_car(char* str, int* i)
{
   if (*i - 1 >= 0 && str[*i - 1] != '('){
      str[*i] = ' ';
      *i += 1;
   }
   str[*i] = '(';
   *i += 1;
}

// format the string after a new car has been looked at.
void post_car(char* str, int* i)
{
   if (*i - 1 >= 0 && str[*i - 1] == ' '){
      *i -= 1;
   }
   else{
      str[*i] = ')';
      *i += 1;
   }
}

// recursively find the next atom in the lisp.
void rec_tostring(const lisp* l, char* str, int* i)
{
   if (l == NULL){
      return;
   }
   if (lisp_isatomic(l)){
      atom_tostring(l, str, i);
   }
   if (l->car != NULL){
      pre_car(str, i);
      rec_tostring(l->car, str, i);
      post_car(str, i);
   }
   if (l->cdr != NULL){
      rec_tostring(l->cdr, str, i);
   }
}


void lisp_tostring(const lisp* l, char* str)
{
   if (str == NULL){
      return;
   }
   int count = 0;
   if (!lisp_isatomic(l)){
      str[count] = '(';
      count += 1;
   }
   rec_tostring(l, str, &count);
   if (!lisp_isatomic(l)){
      str[count] = ')';
      count += 1;
   }
   else{
      count -= 1;
   }
   str[count] = '\0';
}


void lisp_free(lisp** l)
{
   if (l == NULL){
      return;
   }
   if (*l == NULL){
      return;
   }
   lisp* deref = *l;
   lisp_free(&deref->car);
   lisp_free(&deref->cdr);
   free(*l);
   *l = NULL;
}


/* ------------- Tougher Ones : Extensions ---------------*/

int digit_reader(const char* str, int* i)
{
   int number = 0;
   int count = 0;
   while (*i >= 0 && isdigit(str[*i])){
      number += power((str[*i] - '0'), count);
      *i -= 1;
      count += 1;
   }
   if (*i >= 0 && str[*i] == '-'){
      number = -number;
      *i -= 1;
   }
   return number;
}

// return true if the cdr to this would-be cons would NOT be NULL
bool bracket_calc(const char* str, int index)
{
   int len = (int)strlen(str);
   int brackets = 0;
   for (int i = index; i < len; i++){
      if (str[i] == '('){
         brackets += 1;
      }
      if (str[i] == ')'){
         brackets -= 1;
      }
      if (brackets == 0){
         if (i + 1 < len && str[i + 1] == ')'){
            return false;
         }
         if (i + 1 < len && str[i + 1] != ')'){
            return true;
         }
      }
   }
   return false;
}


void save_cdr(lisp** l, lisp* ll[], int* i, int* count, const char* str)
{
   if (*l != NULL){
      lisp* saved = lisp_copy(*l);
      lisp_free(l);
      if (*count >= LISTLISP){
         on_error("[ERROR] Too many lisps.");
      }
      ll[*count] = saved;
      *count += 1;
   }
   while (*i - 1 >= 0 && str[*i - 1] == ')'){
      *i -= 1;
   }
}


void get_cdr(lisp** l, lisp* ll[], int* i, int* count, const char* str)
{
   lisp* cdr = (lisp*)NULL;
   if (bracket_calc(str, *i)){
      if (*count - 1 >= LISTLISP){
         on_error("[ERROR] Too many lisps.");
      }
      cdr = ll[*count - 1];
      *count -= 1;
   }
   *l = lisp_cons(*l, cdr);
}

// i is the index of str; count is the index of saved cdrs in ll[].
void read_string(const char* str, int* i, lisp** l)
{
   lisp* ll[LISTLISP] = {(lisp*)NULL};
   int count = 1;
   while (*i >= 0){
      if (isdigit(str[*i])){
         atomtype atom = digit_reader(str, i);
         *l = lisp_cons(lisp_atom(atom), *l);
      }
      if (str[*i] == ')'){
         save_cdr(l, ll, i, &count, str);
      }
      if (*i > 0 && str[*i] == '('){
         get_cdr(l, ll, i, &count, str);
      }
      *i -= 1;
   }
}


bool string_parser(const char* str, int len)
{
   int brackets = 0;
   for (int i = 0; i < len; i++){
      if (str[i] == '('){
         brackets += 1;
      }
      if (str[i] == ')'){
         brackets -= 1;
      }
      if (brackets < 0){
         return false;
      }
   }
   if (brackets != 0){
      return false;
   }
   return true;
}



lisp* lisp_fromstring(const char* str)
{
   if (str == NULL){
      return (lisp*)NULL;
   }
   int len = (int)strlen(str);
   if (len == 0){
      return (lisp*)NULL;
   }
   if (string_parser(str, len) == false){
      on_error("[ERROR] Invalid input string.");
   }
   int i = len - 1;
   if (isdigit(str[i])){
      atomtype atom = digit_reader(str, &i);
      return lisp_atom(atom);
   }
   lisp* l = (lisp*)NULL;
   read_string(str, &i, &l);
   return l;
}


lisp* lisp_list(const int n, ...)
{
   lisp* parent = (lisp*)NULL;
   va_list args;
   va_start(args, n);

   if (n >= LISTLISP){
      on_error("[ERROR] Too many lisps.");
   }
   lisp* lisps[LISTLISP] = {(lisp*)NULL};
   for (int i = n - 1; i >= 0; i--){
      lisps[i] = va_arg(args, lisp*);
   }
   for (int i = 0; i < n; i++){
      lisp* l = lisps[i];
      parent = lisp_cons(l, parent);
   }
   return parent;
}


void lisp_reduce(void (*func)(lisp* l, atomtype* n), lisp* l, atomtype* acc)
{
   if (l == NULL || func == NULL || acc == NULL){
      return;
   }
   if (lisp_isatomic(l)){
      func(l, acc);
   }
   if (l->car != NULL){
      lisp_reduce(func, l->car, acc);
   }
   if (l->cdr != NULL){
      lisp_reduce(func, l->cdr, acc);
   }
}


void test_linked(void)
{
   char str[LISTSTRLEN];

   // LISP_ATOM
   lisp* atom_test = lisp_atom(3);
   assert(atom_test->car == NULL && atom_test->cdr == NULL && atom_test->atom == 3);
   // free atom_test after lisp_isatomic testing.

   // LIPS_CONS
   lisp* atom = lisp_cons(NULL, NULL);
   assert(atom->car == NULL && atom->cdr == NULL);

   lisp* cons_car = lisp_atom(17);
   lisp* cons_cdr = lisp_atom(-50);
   lisp* cons = lisp_cons(cons_car, lisp_cons(cons_cdr, NULL));
   assert(cons->car->atom == 17 && cons->cdr->car->atom == -50);
   assert(cons->car == cons_car && cons->cdr->car == cons_cdr);

   // LISP_ISATOMIC
   assert(lisp_isatomic(atom_test));
   lisp_free(&atom_test);
   assert(!atom_test);
   assert(lisp_isatomic(NULL) == false);
   assert(lisp_isatomic(atom));
   lisp_free(&atom);
   assert(!atom);
   assert(lisp_isatomic(cons) == false);

   // LISP_TOSTRING
   lisp_tostring(cons, str);
   assert(strcmp(str, "(17 -50)")==0);

   // LISP_LENGTH
   assert(lisp_length(cons) == 2);
   assert(lisp_length(NULL) == 0);
   assert(lisp_length(cons_car) == 0);


   // LISP_CAR
   lisp* get_car = lisp_car(cons);
   assert(get_car->atom == 17 && get_car->car == NULL && get_car->cdr == NULL);


   // LISP_CDR
   lisp* a_cdr = lisp_cdr(cons);
   assert(a_cdr->cdr == NULL && a_cdr->car->atom == -50 && a_cdr->car->car == NULL && a_cdr->car->cdr == NULL);

   // LISP_GETVAL
   atomtype getval = lisp_getval(get_car);
   assert(getval == 17);
   getval = lisp_getval(a_cdr->car);
   assert(getval == -50);
   assert(lisp_getval(cons) == UNDEFINED);

   // LSIP_REC_COPY
   lisp* p = lisp_atom(UNDEFINED);
   lisp_rec_copy(cons, p);
   assert(cons->car->atom == 17 && cons->cdr->car->atom == -50);
   assert(p->car->atom == 17 && p->cdr->car->atom == -50);

   // check that it is a deep copy:
   cons->car->atom = 5;
   assert(p->car->atom == 17);
   lisp_free(&p);
   assert(!p);

   // LISP_COPY (this is tested for more complex examples further down)
   lisp* copy = lisp_copy(cons);
   assert(cons->car->atom == 5 && cons->cdr->car->atom == -50);
   assert(copy->car->atom == 5 && copy->cdr->car->atom == -50);
   assert(lisp_length(copy) == 2);

   // This should also free cons_car and cons_cdr; but check with valgrind
   lisp_free(&cons);
   assert(!cons);
   lisp_free(&copy);
   assert(!copy);


   /*
   A MORE COMPLEX EXAMPLE:

   (((2 3) 1) (4 5) -23 (19 (3 81)) 90)

   X|X -----------------> X|X -----------> X|X -----> X|X --------------> X|X
   |                      |                |          |                   |
   V                      V                V          V                   V
   X|X ----------> X|X    X|X --> X|X      -23        X|X --> X|X         90
   |               |      |       |                   |       |
   V               V      V       V                   V       V
   X|X --> X|X     1      4       5                   19      X|X --> X|X
   |       |                                                  |       |
   V       V                                                  V       V
   2       3                                                  3       81
   */

   // BUILDING THE LISP STRUCTURE
   lisp* l381 = lisp_cons(lisp_atom(3), lisp_cons(lisp_atom(81), NULL));
   lisp* l19 = lisp_cons(lisp_atom(19), lisp_cons(l381, NULL));
   lisp* lasttwo = lisp_cons(l19, lisp_cons(lisp_atom(90), NULL));
   lisp* lastthree = lisp_cons(lisp_atom(-23), lasttwo);

   lisp* l45 = lisp_cons(lisp_atom(4), lisp_cons(lisp_atom(5), NULL));
   lisp* lastfour = lisp_cons(l45, lastthree);

   lisp* l23 = lisp_cons(lisp_atom(2), lisp_cons(lisp_atom(3), NULL));
   lisp* l231 = lisp_cons(l23, lisp_cons(lisp_atom(1), NULL));

   // REMEMBER TO FREE l. This should also free all the other callocd memory. Check with valgrind.
   lisp* l = lisp_cons(l231, lastfour);

   // test the build was correct
   assert(l->car->car->car->atom == 2);
   assert(l->car->car->cdr->car->atom == 3);
   assert(l->car->cdr->car->atom == 1);
   assert(l->cdr->car->car->atom == 4);
   assert(l->cdr->car->cdr->car->atom == 5);
   assert(l->cdr->cdr->car->atom == -23);
   assert(l->cdr->cdr->cdr->car->car->atom == 19);
   assert(l->cdr->cdr->cdr->car->cdr->car->car->atom == 3);
   assert(l->cdr->cdr->cdr->car->cdr->car->cdr->car->atom == 81);
   assert(l->cdr->cdr->cdr->cdr->car->atom == 90);
   assert(lisp_length(l) == 5);

   // LISP_COPY testing for a complex lisp structure:
   copy = lisp_copy(l);
   assert(copy->car->car->car->atom == 2);
   assert(copy->car->car->cdr->car->atom == 3);
   assert(copy->car->cdr->car->atom == 1);
   assert(copy->cdr->car->car->atom == 4);
   assert(copy->cdr->car->cdr->car->atom == 5);
   assert(copy->cdr->cdr->car->atom == -23);
   assert(copy->cdr->cdr->cdr->car->car->atom == 19);
   assert(copy->cdr->cdr->cdr->car->cdr->car->car->atom == 3);
   assert(copy->cdr->cdr->cdr->car->cdr->car->cdr->car->atom == 81);
   assert(copy->cdr->cdr->cdr->cdr->car->atom == 90);
   assert(lisp_length(copy) == 5);

   // make sure the copy was a deep copy:
   copy->car->car->car->atom = 100002;
   copy->cdr->cdr->cdr->car->car->atom = 176;
   copy->cdr->cdr->cdr->cdr->car->atom = 95674;
   assert(copy->car->car->car->atom == 100002);
   assert(copy->cdr->cdr->cdr->car->car->atom == 176);
   assert(copy->cdr->cdr->cdr->cdr->car->atom == 95674);
   assert(l->car->car->car->atom == 2);
   assert(l->cdr->cdr->cdr->car->car->atom == 19);
   assert(l->cdr->cdr->cdr->cdr->car->atom == 90);


   // LISP_TOSTRING helper functions:

   // POWER
   // a negative value should never be passed to the power function, as either the exponent or the constant. The absolute (abs) value only gets passed.
   assert(power(2, 0) == 2);
   assert(power(2, 1) == 20);
   assert(power(2, 5) == 200000);

   // ABS_TOSTRING
   int i = 0;
   int abs = 56;
   int prev_count = 0;
   abs_tostring(str, &i, abs, prev_count);
   assert(str[0] == '5');
   assert(str[1] == '6');
   assert(i == 2);

   i = 5;
   abs = 4003;
   prev_count = 0;
   abs_tostring(str, &i, abs, prev_count);
   assert(str[5] == '4');
   assert(str[6] == '0');
   assert(str[7] == '0');
   assert(str[8] == '3');
   assert(i == 5 + 4);

   // ATOM_TOSTRING
   // only called if l is a pointer to an atom.
   i = 0;
   lisp* a = lisp_atom(-605);
   atom_tostring(a, str, &i);
   assert(str[0] == '-');
   assert(str[1] == '6');
   assert(str[2] == '0');
   assert(str[3] == '5');
   assert(str[4] == ' ');
   assert(i == 5);
   lisp_free(&a);
   assert(!a);

   i = 0;
   a = lisp_atom(40);
   atom_tostring(a, str, &i);
   assert(str[0] == '4');
   assert(str[1] == '0');
   assert(str[2] == ' ');
   assert(i == 3);
   lisp_free(&a);
   assert(!a);

   // REC_TOSTRING
   i = 0;
   // clear string to prevent it printing out extra stuff.
   for (int x = 0; x < LISTSTRLEN; x++){
      str[x] = '\0';
   }
   a = lisp_cons(lisp_atom(-50), lisp_cons(lisp_atom(100), NULL));
   rec_tostring(a, str, &i);
   assert(strcmp(str, "-50 100 ")==0);
   assert(i == 7);
   lisp_free(&a);
   assert(!a);

   // PRE_CAR
   str[2] = ' ';
   i = 3;
   pre_car(str, &i);
   assert(str[3] == ' ' && str[4] == '(' && i == 5);

   str[2] = '(';
   i = 3;
   pre_car(str, &i);
   assert(str[3] == '(' && i == 4);

   // POST_CAR
   str[2] = ' ';
   i = 3;
   post_car(str, &i);
   assert(str[2] == ' ' && i == 2);

   str[2] = '5';
   i = 3;
   post_car(str, &i);
   assert(str[3] == ')' && i == 4);


   // LISP_TOSTRING
   lisp_tostring(l, str);
   assert(strcmp(str, "(((2 3) 1) (4 5) -23 (19 (3 81)) 90)")==0);
   lisp_free(&l);
   assert(!l);

   // testing that 0s are correctly added to the string.
   lisp_tostring(copy, str);
   assert(strcmp(str, "(((100002 3) 1) (4 5) -23 (176 (3 81)) 95674)")==0);
   lisp_free(&copy);
   assert(!copy);

   lisp_tostring(NULL, str);
   assert(strcmp(str, "()")==0);

   lisp* non_list = lisp_atom(5);
   lisp_tostring(non_list, str);
   assert(strcmp(str, "5")==0);
   lisp_free(&non_list);

   lisp* single = lisp_cons(lisp_atom(5), NULL);
   lisp_tostring(single, str);
   assert(strcmp(str, "(5)")==0);
   lisp_free(&single);

   lisp* deep = lisp_cons(lisp_cons(lisp_cons(lisp_cons(lisp_atom(5), NULL), NULL), NULL), NULL);
   lisp_tostring(deep, str);
   assert(strcmp(str, "((((5))))")==0);
   lisp_free(&deep);

   /*
   AN EVEN MORE COMPLEX EXAMPLE

   ((((5 (-404 923 700 -2300) (18 51 -4))) (10 10 -8 4 77) (-1 1)) (((2 3) 1) (4 5) -23 (19 (3 81)) 90) -10 (300) ((-900 555) 8 90 60))

   X -------------------------------------------------> X --> X --> X
   |                                                    |     |     |
   V                                                    V     V     V
   X --> X --------------------------> X               -10    X     X ------> X --> X --> X
   |     |                             |                      |     |         |     |     |
   V     V                             V                      V     V         V     V     V
   X     X --> X --> X --> X --> X     X --> X               300    X --> X   8     90    60
   |     |     |     |     |     |     |     |                      |     |
   |     V     V     V     V     V     V     V                      V     V
   |     10   10    -8     4     77    -1    1                     -900  555
   V
   X ----> X ---------------------> X
   |       |                        |
   V       V                        V
   5       X --> X --> X --> X      X --> X --> x
           |     |     |     |      |     |     |
           V     V     V     V      V     V     V
           -404  923   700   -2300  18    51   -4
   */

   lisp* llast = lisp_cons(lisp_cons(lisp_atom(-900), lisp_cons(lisp_atom(555), NULL)), lisp_cons(lisp_atom(8), lisp_cons(lisp_atom(90), lisp_cons(lisp_atom(60), NULL))));
   lisp* l4 = lisp_cons(lisp_atom(300), NULL);
   lisp* l3 = lisp_atom(-10);
   lisp* l11 = lisp_cons(lisp_cons(lisp_atom(-1), lisp_cons(lisp_atom(1), NULL)), NULL);
   lisp* l77 = lisp_cons(lisp_atom(10), lisp_cons(lisp_atom(10), lisp_cons(lisp_atom(-8), lisp_cons(lisp_atom(4), lisp_cons(lisp_atom(77), NULL)))));
   lisp* l51 = lisp_cons(lisp_atom(18), lisp_cons(lisp_atom(51), lisp_cons(lisp_atom(-4), NULL)));
   lisp* l700 = lisp_cons(lisp_atom(-404), lisp_cons(lisp_atom(923), lisp_cons(lisp_atom(700), lisp_cons(lisp_atom(-2300), NULL))));
   lisp* lbottom = lisp_cons(lisp_cons(lisp_atom(5), lisp_cons(l700, lisp_cons(l51, NULL))), NULL);
   lisp* lfirst = lisp_cons(lbottom, lisp_cons(l77, l11));
   lisp* complex = lisp_cons(lfirst, lisp_cons(l3, lisp_cons(l4, lisp_cons(llast, NULL))));

   // testing the build was successful
   assert(complex->car->car->car->car->atom == 5);
   assert(complex->car->car->car->cdr->car->car->atom == -404);
   assert(complex->car->car->car->cdr->car->cdr->car->atom == 923);
   assert(complex->car->car->car->cdr->car->cdr->cdr->car->atom == 700);
   assert(complex->car->car->car->cdr->car->cdr->cdr->cdr->car->atom == -2300);
   assert(complex->car->car->car->cdr->cdr->car->car->atom == 18);
   assert(complex->car->car->car->cdr->cdr->car->cdr->car->atom == 51);
   assert(complex->car->car->car->cdr->cdr->car->cdr->cdr->car->atom == -4);
   assert(complex->car->cdr->car->car->atom == 10);
   assert(complex->car->cdr->car->cdr->car->atom == 10);
   assert(complex->car->cdr->car->cdr->cdr->car->atom == -8);
   assert(complex->car->cdr->car->cdr->cdr->cdr->car->atom == 4);
   assert(complex->car->cdr->car->cdr->cdr->cdr->cdr->car->atom == 77);
   assert(complex->car->cdr->cdr->car->car->atom == -1);
   assert(complex->car->cdr->cdr->car->cdr->car->atom == 1);
   assert(complex->cdr->car->atom == -10);
   assert(complex->cdr->cdr->car->car->atom == 300);
   assert(complex->cdr->cdr->cdr->car->car->car->atom == -900);
   assert(complex->cdr->cdr->cdr->car->car->cdr->car->atom == 555);
   assert(complex->cdr->cdr->cdr->car->cdr->car->atom == 8);
   assert(complex->cdr->cdr->cdr->car->cdr->cdr->car->atom == 90);
   assert(complex->cdr->cdr->cdr->car->cdr->cdr->cdr->car->atom == 60);
   assert(lisp_length(complex) == 4);

   // more LISP_TOSTRING and LISP_COPY testing
   lisp_tostring(complex, str);
   assert(strcmp(str, "((((5 (-404 923 700 -2300) (18 51 -4))) (10 10 -8 4 77) (-1 1)) -10 (300) ((-900 555) 8 90 60))")==0);

   lisp_tostring(NULL, str);
   assert(strcmp(str, "()")==0);

   copy = lisp_copy(complex);
   lisp_tostring(copy, str);
   assert(strcmp(str, "((((5 (-404 923 700 -2300) (18 51 -4))) (10 10 -8 4 77) (-1 1)) -10 (300) ((-900 555) 8 90 60))")==0);
   lisp_free(&copy);
   assert(!copy);

   lisp_free(&complex);
   assert(!complex);

   // LISP_FROMSTRING helper functions

   // STRING_PARSER
   assert(string_parser("(5 (7 8))", (int)strlen("(5 (7 8))")));
   assert(string_parser("()", (int)strlen("()")));
   assert(string_parser("", (int)strlen("")));
   assert(string_parser("((5 (7 8))", (int)strlen("(5 (7 8))")) == false);
   assert(string_parser(")(", (int)strlen(")(")) == false);


   // DIGIT_READER
   i = 6;
   char num[] = "-468070";
   assert(digit_reader(num, &i) == -468070);

   i = 1;
   char num1[] = "0";
   assert(digit_reader(num1, &i) == 0);


   // BRACKET_CALC
   // should be called when the index is on an open bracket.
   char bracket[] = "(5 7 (-90 55 (3 1)) 8)";
   i = 5;
   assert(bracket_calc(bracket, i));
   i = 13;
   assert(bracket_calc(bracket, i) == false);
   i = 0;
   assert(bracket_calc(bracket, i) == false);


   // READ_STRING
   i = (int)strlen(bracket) - 1;
   lisp* rs = (lisp*)NULL;
   read_string(bracket, &i, &rs);
   assert(lisp_length(rs) == 4);
   assert(rs->car->atom == 5 && rs->cdr->car->atom == 7);
   assert(rs->cdr->cdr->car->car->atom == -90);
   assert(rs->cdr->cdr->car->cdr->car->atom == 55);
   assert(rs->cdr->cdr->car->cdr->cdr->car->car->atom == 3);
   assert(rs->cdr->cdr->car->cdr->cdr->car->cdr->car->atom == 1);
   assert(rs->cdr->cdr->cdr->car->atom == 8);


   // SAVE_CDR
   lisp* ll[LISTLISP] = {(lisp*)NULL};
   int count = 1;
   i = 18;
   assert(rs);
   assert(!ll[1]);
   save_cdr(&rs, ll, &i, &count, bracket);

   // save_cdr frees the original and saves a pointer to a deep copy in the ll list.
   // therefore, rs has been saved as the cdr.
   assert(!rs);
   assert(ll[1]);
   assert(count == 2);


   // GET_CDR
   i = 5;
   lisp* grow_lisp = lisp_fromstring("(3 1)");
   get_cdr(&grow_lisp, ll, &i, &count, bracket);
   for (int x = 0; x < LISTSTRLEN; x++){
      str[x] = '\0';
   }
   lisp_tostring(grow_lisp, str);
   // rs has been appended as a cdr to (3 1) successfully.
   assert(strcmp(str, "((3 1) 5 7 (-90 55 (3 1)) 8)")==0);

   // and now free the saved cdr in the ll list.
   lisp_free(&ll[0]);
   assert(!ll[0]);

   // and free the grow lisp.
   // (possible to actually just do this and not free the cdr, as the cdr is a part of grow lisp now)
   lisp_free(&grow_lisp);
   assert(!grow_lisp);



   // LISP_FROMSTRING
   lisp* lfs;

   char str0[] = "5";
   lfs = lisp_fromstring(str0);
   assert(lfs);
   assert(lfs->atom == 5);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "5")==0);
   lisp_free(&lfs);
   assert(!lfs);

   char str1[] = "(1 2 3)";
   lfs = lisp_fromstring(str1);
   assert(lfs);
   assert(lisp_length(lfs) == 3);
   assert(lisp_car(lfs)->atom == 1);
   assert(lisp_car(lisp_cdr(lfs))->atom == 2);
   assert(lisp_car(lisp_cdr(lisp_cdr(lfs)))->atom == 3);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "(1 2 3)")==0);
   lisp_free(&lfs);
   assert(!lfs);

   char str2[] = "(1 (2 3))";
   lfs = lisp_fromstring(str2);
   assert(lfs);
   assert(lisp_length(lfs) == 2);
   assert(lisp_car(lfs)->atom == 1);
   assert(lisp_car(lisp_car(lisp_cdr(lfs)))->atom == 2);
   assert(lisp_car(lisp_cdr(lisp_car(lisp_cdr(lfs))))->atom == 3);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "(1 (2 3))")==0);
   lisp_free(&lfs);
   assert(!lfs);

   char str3[] = "((1 2) 3)";
   lfs = lisp_fromstring(str3);
   assert(lfs);
   lisp_tostring(lfs, str);
   assert(lisp_length(lfs) == 2);
   assert(lfs->car->car->atom == 1);
   assert(lfs->car->cdr->car->atom == 2);
   assert(lfs->cdr->car->atom == 3);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "((1 2) 3)")==0);
   lisp_free(&lfs);
   assert(!lfs);

   char str4[] = "((1 2) (3 4))";
   lfs = lisp_fromstring(str4);
   assert(lfs);
   assert(lisp_length(lfs) == 2);
   assert(lfs->car->car->atom == 1);
   assert(lfs->car->cdr->car->atom == 2);
   assert(lfs->cdr->car->car->atom == 3);
   assert(lfs->cdr->car->cdr->car->atom == 4);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "((1 2) (3 4))")==0);
   lisp_free(&lfs);
   assert(!lfs);

   char str5[] = "((1 2) (3 4) (5 6) (7 8))";
   lfs = lisp_fromstring(str5);
   assert(lfs);
   assert(lisp_length(lfs) == 4);
   assert(lfs->car->car->atom == 1);
   assert(lfs->car->cdr->car->atom == 2);
   assert(lfs->cdr->car->car->atom == 3);
   assert(lfs->cdr->car->cdr->car->atom == 4);
   assert(lfs->cdr->cdr->car->car->atom == 5);
   assert(lfs->cdr->cdr->car->cdr->car->atom == 6);
   assert(lfs->cdr->cdr->cdr->car->car->atom == 7);
   assert(lfs->cdr->cdr->cdr->car->cdr->car->atom == 8);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "((1 2) (3 4) (5 6) (7 8))")==0);
   lisp_free(&lfs);
   assert(!lfs);

   // no spaces (apart from digits)
   char str6[] = "((1(2 3))(4 5))";
   lfs = lisp_fromstring(str6);
   assert(lfs);
   assert(lisp_length(lfs) == 2);
   assert(lfs->car->car->atom == 1);
   assert(lfs->car->cdr->car->car->atom == 2);
   assert(lfs->car->cdr->car->cdr->car->atom == 3);
   assert(lfs->cdr->car->car->atom == 4);
   assert(lfs->cdr->car->cdr->car->atom == 5);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "((1 (2 3)) (4 5))")==0);
   lisp_free(&lfs);
   assert(!lfs);

   // super weird spaces and characters
   char str7[] = "(  (   1   s (     2     fgdh    3  )   4  ) ( 5  h 6    )    )";
   lfs = lisp_fromstring(str7);
   assert(lfs);
   assert(lisp_length(lfs) == 2);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "((1 (2 3) 4) (5 6))")==0);
   assert(lfs->car->car->atom == 1);
   assert(lfs->car->cdr->car->car->atom == 2);
   assert(lfs->car->cdr->car->cdr->car->atom == 3);
   assert(lfs->car->cdr->cdr->car->atom == 4);
   assert(lfs->cdr->car->car->atom == 5);
   assert(lfs->cdr->car->cdr->car->atom == 6);
   lisp_free(&lfs);
   assert(!lfs);

   char str8[] = "(((1 2) -3600300) (4 0) 6002 (7 (8000 9)) 1)";
   lfs = lisp_fromstring(str8);
   assert(lfs);
   assert(lisp_length(lfs) == 5);
   assert(lfs->car->car->car->atom == 1);
   assert(lfs->car->car->cdr->car->atom == 2);
   assert(lfs->car->cdr->car->atom == -3600300);
   assert(lfs->cdr->car->car->atom == 4);
   assert(lfs->cdr->car->cdr->car->atom == 0);
   assert(lfs->cdr->cdr->car->atom == 6002);
   assert(lfs->cdr->cdr->cdr->car->car->atom == 7);
   assert(lfs->cdr->cdr->cdr->car->cdr->car->car->atom == 8000);
   assert(lfs->cdr->cdr->cdr->car->cdr->car->cdr->car->atom == 9);
   assert(lfs->cdr->cdr->cdr->cdr->car->atom == 1);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "(((1 2) -3600300) (4 0) 6002 (7 (8000 9)) 1)")==0);
   lisp_free(&lfs);
   assert(!lfs);

   char str9[] = "(0)";
   lfs = lisp_fromstring(str9);
   assert(lfs);
   assert(lisp_length(lfs) == 1);
   assert(lfs->car->atom == 0);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "(0)")==0);
   lisp_free(&lfs);
   assert(!lfs);

   char str10[] = "(((9)))";
   lfs = lisp_fromstring(str10);
   assert(lfs);
   assert(lisp_length(lfs) == 1);
   assert(lfs->car->car->car->atom == 9);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "(((9)))")==0);
   lisp_free(&lfs);
   assert(!lfs);

   char str11[] = "()";
   lfs = lisp_fromstring(str11);
   assert(!lfs);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "()")==0);
   lisp_free(&lfs);
   assert(!lfs);

   char str12[] = "((((5 (-404 923 700 -2300) (18 51 -4))) (10 10 -8 4 77) (-1 1)) -10 (300) ((-900 555) 8 90 60))";
   lfs = lisp_fromstring(str12);
   assert(lfs);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "((((5 (-404 923 700 -2300) (18 51 -4))) (10 10 -8 4 77) (-1 1)) -10 (300) ((-900 555) 8 90 60))")==0);
   lisp_free(&lfs);
   assert(!lfs);

   char str13[] = "(((0)))";
   lfs = lisp_fromstring(str13);
   assert(lfs);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "(((0)))")==0);
   lisp_free(&lfs);
   assert(!lfs);

   char str14[] = "g";
   lfs = lisp_fromstring(str14);
   assert(!lfs);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "()")==0);
   lisp_free(&lfs);
   assert(!lfs);

   char str15[] = "";
   lfs = lisp_fromstring(str15);
   assert(!lfs);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "()")==0);
   lisp_free(&lfs);
   assert(!lfs);

   char str16[] = "-5";
   lfs = lisp_fromstring(str16);
   assert(lfs);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "-5")==0);
   lisp_free(&lfs);
   assert(!lfs);

   char str17[] = "(-5)";
   lfs = lisp_fromstring(str17);
   assert(lfs);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "(-5)")==0);
   lisp_free(&lfs);
   assert(!lfs);


   // LISP_LIST
   lfs = lisp_list(3, lisp_atom(1), lisp_atom(2), lisp_atom(3));
   assert(lfs);
   assert(lisp_length(lfs) == 3);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "(1 2 3)")==0);
   lisp_free(&lfs);
   assert(!lfs);

   lfs = lisp_list(2, lisp_fromstring("((1 2) 3)"), lisp_fromstring("(1 (2 3))"));
   assert(lfs);
   assert(lisp_length(lfs) == 2);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "(((1 2) 3) (1 (2 3)))")==0);
   lisp_free(&lfs);
   assert(!lfs);

   lfs = lisp_list(4, lisp_atom(5), lisp_fromstring("((1 2) ((3)))"), lisp_fromstring("(-50 808 (909))"), lisp_atom(6));
   assert(lfs);
   assert(lisp_length(lfs) == 4);
   lisp_tostring(lfs, str);
   assert(strcmp(str, "(5 ((1 2) ((3))) (-50 808 (909)) 6)")==0);
   lisp_free(&lfs);
   assert(!lfs);


   // LISP_REDUCE
   // how to reset value of a static variable without changing the function?

   lfs = lisp_fromstring("(((1 2) -7) (4 1) 602 (7 (80 9)) 1)");
   atomtype acc = 0;
   lisp_reduce(test_atms, lfs, &acc);
   assert(acc == 10);
   acc = 1;
   lisp_reduce(test_times, lfs, &acc);
   assert(acc == -169908480);
   lisp_free(&lfs);
   assert(!lfs);

   lfs = lisp_fromstring("(  ( 1    (     2  3  )   4  ) ( 5   6    ))");
   acc = 0;
   lisp_reduce(test_atms, lfs, &acc);
   assert(acc == 6);
   acc = 1;
   lisp_reduce(test_times, lfs, &acc);
   assert(acc == 720);
   lisp_free(&lfs);
   assert(!lfs);

   lfs = lisp_fromstring("(  ( 1    (2  3) 0) (5 6))");
   acc = 0;
   lisp_reduce(test_atms, lfs, &acc);
   assert(acc == 6);
   acc = 1;
   lisp_reduce(test_times, lfs, &acc);
   assert(acc == 0);
   lisp_free(&lfs);
   assert(!lfs);

   lfs = lisp_fromstring("3");
   acc = 0;
   lisp_reduce(test_atms, lfs, &acc);
   assert(acc == 1);
   acc = 1;
   lisp_reduce(test_times, lfs, &acc);
   assert(acc == 3);
   lisp_free(&lfs);
   assert(!lfs);


}



// Multiplies getval() of all atoms
void test_times(lisp* l, atomtype* accum)
{
   *accum = *accum * lisp_getval(l);
}

// To count number of atoms in list, including sub-lists
void test_atms(lisp* l, atomtype* accum)
{
   // Could just add one (since each node must be atomic),
   // but prevents unused warning for variable 'l'...
   *accum = *accum + lisp_isatomic(l);
}
