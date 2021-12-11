//
// See copyright.h for copyright notice and limitation of liability
// and disclaimer of warranty provisions.
//
#include "copyright.h"

#include "cool.h"
#include "tree.h"
#include "cool-tree.h"
#include "utilities.h"
#include "symtab.h"
#include "semant.h"
#include <list>

static char log_buf[256];
class__class *cur_class;
int cur_line = 1;

static inline void scope_enter()
{
   vartable->enterscope();
}

static inline void scope_exits()
{
   vartable->exitscope();
}

static void semant_error(char *msg)
{
   cerr << "[semant error " << cur_class->get_filename()->get_string() << ":" << cur_line << "] " << msg << endl;
}

// double pointer checking
void inherent_checking()
{
   Class_ c1 = cur_class;
   Class_ c2 = classtable->get_class_by_symbol(cur_class->get_class_parent());
   if (c2 == NULL) {
      sprintf(log_buf, "can't get %s's parent!\n",
                        c1->get_class_name()->get_string());
      semant_error(log_buf);
      fatal_error("inherent_checking error!");
   }

   while(c1 != NULL && c2 != NULL) {
      if (c1 == c2) {
         sprintf(log_buf, "cyclic for class %s!\n",
                           c1->get_class_name()->get_string());
         semant_error(log_buf);
         fatal_error("inherent_checking error!");
      }
      c1 = classtable->get_class_by_symbol(c1->get_class_parent());
      if (c1 != NULL) {
         c1 = classtable->get_class_by_symbol(c1->get_class_parent());
      }
      c2 = classtable->get_class_by_symbol(c2->get_class_parent());
   }
}

static void add_ancestor_features()
{
   Class_ tmp = classtable->get_class_by_symbol(cur_class->get_class_parent());
   while (tmp != NULL) {
      Features features = tmp->get_class_features();
      for(int i = features->first(); features->more(i); i = features->next(i)) {
         Feature f = features->nth(i);
         attr_class *attr = dynamic_cast<attr_class*>(f);
         if (attr) {
            vartable->addid(attr->name, new VarSymbolType(attr->name, attr->type_decl));
         }
      }
      tmp = classtable->get_class_by_symbol(tmp->get_class_parent());
   }
   
}

// TODO need to do the parameter types validation
static method_class *get_method_declare(Symbol cs, Symbol method_name)
{
   if (classtable == 0) {
      fatal_error("classtable has not been initialized!\n");
   }

   Class_ c = classtable->get_class_by_symbol(cs);

   if (c == NULL) {
      sprintf(log_buf, "can't find class %s declaration in symbol table!\n", cs->get_string());
      semant_error(log_buf);
      return NULL;
   }
   Features features = c->get_class_features();
   for(int i = features->first(); features->more(i); i = features->next(i)) {
      Feature f = features->nth(i);
      method_class *m = dynamic_cast<method_class*>(f);
      if (m && m->get_method_name() == method_name) {
         return m;
      }
   }

   if (c->get_class_name() == Object) return NULL;

   return get_method_declare(c->get_class_parent(), method_name);
}

static bool is_valid_class(Symbol cs)
{
   if (classtable == 0) {
      fatal_error("classtable has not been initialized!\n");
   }

   if (cs == SELF_TYPE) {
      return true;
   }

   if (cs == NULL) {
      semant_error("is_valid_class: Symbol is NULL");
      return false;
   }

   if (classtable->get_class_by_symbol(cs) == NULL) {
      sprintf(log_buf, "is_valid_class:class %s can't find class in class table", cs->get_string());
      semant_error(log_buf);
   }

   return true;
}

static bool class_is_comfort(Symbol cs1, Symbol cs2)
{

   if (!is_valid_class(cs1) || !is_valid_class(cs2)) {
      return false;
   }

   if (cs1 == SELF_TYPE) {
      // TODO maybe not right
      cs1 = cur_class->get_class_name();
   }

   if (cs2 == SELF_TYPE) {
      // TODO maybe not right
      cs2 = cur_class->get_class_name();
   }

   Class_ c1 = classtable->get_class_by_symbol(cs1);
   Class_ c2 = classtable->get_class_by_symbol(cs2);
   if (cs1 == Object) {
      if (cs2 != Object) {
         return false;
      } else {
         return true;
      }
   }

   if (c1->get_class_parent() == cs2 || cs1 == cs2) {
      return true;
   } else {
      return class_is_comfort(c1->get_class_parent(), cs2);
   }
} 

Symbol type_join(Symbol cs1, Symbol cs2)
{

   if (!is_valid_class(cs1) || !is_valid_class(cs2)) {
      return Object;
   }

   if (cs1 == SELF_TYPE && cs2 == SELF_TYPE) {
      return SELF_TYPE;
   }

   if (cs1 == SELF_TYPE) {
      cs1 = cur_class->get_class_name();
   }

   if (cs2 == SELF_TYPE) {
      cs2 = cur_class->get_class_name();
   }

   Class_ c1 = classtable->get_class_by_symbol(cs1);
   Class_ c2 = classtable->get_class_by_symbol(cs2);
   std::list<Class_> c1_ancestor_path;
   std::list<Class_> c2_ancestor_path;

   Class_ tmp = c1;
   while(tmp != NULL) {
      c1_ancestor_path.push_back(tmp);
      tmp = classtable->get_class_by_symbol(tmp->get_class_parent());
   }

   tmp = c2;
   while(tmp != NULL) {
      c2_ancestor_path.push_back(tmp);
      tmp = classtable->get_class_by_symbol(tmp->get_class_parent());
   }

   std::list<Class_>::iterator iter1;
   std::list<Class_>::iterator iter2;
   for (iter1 = c1_ancestor_path.begin(); iter1 != c1_ancestor_path.end(); iter1++) {
      for (iter2 = c2_ancestor_path.begin(); iter2 != c2_ancestor_path.end(); iter2++) {
         if (*iter1 == *iter2 && *iter2 != NULL) {
            return (*iter2)->get_class_name();
         }
      }
   }

   return Object;
}

//
void program_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   for(int i = classes->first(); classes->more(i); i = classes->next(i))
     classes->nth(i)->annotate_with_types();
}

void class__class::annotate_with_types()
{
   cur_line = this->get_line_number();
   scope_enter();
   vartable->addid(self, new VarSymbolType(self, SELF_TYPE));
   cur_class = this;
   //inherent_checking();
   add_ancestor_features();
   
   for(int i = features->first(); features->more(i); i = features->next(i)) {
      Feature f = features->nth(i);
      attr_class *attr = dynamic_cast<attr_class*>(f);
      if (attr) {
         if (vartable->lookup(attr->name) != NULL) {
            sprintf(log_buf, "redef of %s", attr->name->get_string());
            semant_error(log_buf);
         }
         vartable->addid(attr->name, new VarSymbolType(attr->name, attr->type_decl));
      }
   }

   for(int i = features->first(); features->more(i); i = features->next(i)) {
      features->nth(i)->annotate_with_types();
   }
   scope_exits();
}

void method_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   scope_enter();
   for(int i = formals->first(); formals->more(i); i = formals->next(i))
     formals->nth(i)->annotate_with_types();
   expr->annotate_with_types();
   if (!class_is_comfort(expr->type, return_type)) {
      semant_error("class is not comfort");
   }
   scope_exits();
}

void attr_class::annotate_with_types()
{
   cur_line = this->get_line_number();

   // VarSymbolType *v = new VarSymbolType(name, type_decl);
   // vartable->addid(name, v);
   init->annotate_with_types();
   if (init->get_type() != NULL && !class_is_comfort(init->type, type_decl)) {
      sprintf(log_buf, "attr_class is not comfort for init->type %s and type_decl %s", init->get_type()->get_string(), type_decl->get_string());
      semant_error(log_buf);
   }
}

//
// formal_class::annotate_with_types dumps the name and type declaration
// of a formal parameter.
//
void formal_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   VarSymbolType *v = new VarSymbolType(name, type_decl);
   vartable->addid(name, v);
}

//
// branch_class::annotate_with_types dumps the name, type declaration,
// and body of any case branch.
//
void branch_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   scope_enter();
   VarSymbolType *v = new VarSymbolType(name, type_decl);
   vartable->addid(name, v);
   expr->annotate_with_types();
   scope_exits();
}

//
// assign_class::annotate_with_types prints "assign" and then (indented)
// the variable being assigned, the expression, and finally the type
// of the result.  Note the call to dump_type (see above) at the
// end of the method.
//
void assign_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   expr->annotate_with_types();
   this->type = expr->type;
   VarSymbolType *v = vartable->lookup(name);
   if (v == NULL) {
      semant_error("variable not declared before use in assign_class\n");
   } else {
      if (!class_is_comfort(expr->type, v->type)) {
         sprintf(log_buf, "assign_class is not comfort for expr->type %s and v->type %s", expr->type->get_string(), v->type->get_string());
         semant_error(log_buf);
      }
   }
}

//
// static_dispatch_class::annotate_with_types prints the expression,
// static dispatch class, function name, and actual arguments
// of any static dispatch.  
//
void static_dispatch_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   expr->annotate_with_types();
   for(int i = actual->first(); actual->more(i); i = actual->next(i))
     actual->nth(i)->annotate_with_types();
   method_class *m = NULL;
   m = get_method_declare(this->type_name, name);
   if (m == NULL) {
      sprintf(log_buf, "method %s:%s not declared before use in dispatch_class",
               this->type_name->get_string(), name->get_string());
      semant_error(log_buf);
      this->type = Object;
      return;
   }
   this->type = m->get_return_type();
}

//
//   dispatch_class::annotate_with_types is similar to 
//   static_dispatch_class::annotate_with_types 
//
void dispatch_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   
   expr->annotate_with_types();
   for(int i = actual->first(); actual->more(i); i = actual->next(i))
     actual->nth(i)->annotate_with_types();

   method_class *m = NULL;
   if (expr->get_type() == SELF_TYPE) {
      m = get_method_declare(cur_class->get_class_name(), name);
   } else {
      m = get_method_declare(expr->get_type(), name);
   }
   
   if (m == NULL) {
      sprintf(log_buf, "method %s:%s not declared before use in dispatch_class",
               expr->get_type()->get_string(), name->get_string());
      semant_error(log_buf);
      this->type = Object;
      return;
   }
   if (expr->get_type() == SELF_TYPE && m->get_return_type() == SELF_TYPE) {
      this->type = SELF_TYPE;
   } else if (expr->get_type() != SELF_TYPE && m->get_return_type() == SELF_TYPE) {
      this->type = expr->get_type();
   } else {
      this->type = m->get_return_type();
   }
   // this->type = m->get_return_type();
}

//
// cond_class::annotate_with_types dumps each of the three expressions
// in the conditional and then the type of the entire expression.
//
void cond_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   pred->annotate_with_types();
   then_exp->annotate_with_types();
   else_exp->annotate_with_types();
   if (pred->type != Bool) {
      semant_error("pred is not Bool");
   }

   this->type = type_join(then_exp->get_type(), else_exp->get_type());
   //cerr << "cond noted " << this->type->get_string() << " " << cur_class->get_filename() << ":" << this->get_line_number() << " " << endl;
}

//
// loop_class::annotate_with_types dumps the predicate and then the
// body of the loop, and finally the type of the entire expression.
//
void loop_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   pred->annotate_with_types();
   body->annotate_with_types();
   if (pred->type != Bool) {
      semant_error("pred is not Bool");
   }
   type = Object;
}

//
//  typcase_class::annotate_with_types dumps each branch of the
//  the Case_ one at a time.  The type of the entire expression
//  is dumped at the end.
//
void typcase_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   expr->annotate_with_types();
   for(int i = cases->first(); cases->more(i); i = cases->next(i)) {
      cases->nth(i)->annotate_with_types();
      branch_class* b = dynamic_cast<branch_class *>(cases->nth(i));
      if (b == NULL) {
         semant_error("bad branch class");
      } else {
         if (i == cases->first()) {
            this->type = b->expr->type;
         } else {
            this->type = type_join(this->type, b->expr->type);
         }
      }
   }
   // if (cur_line == 95) {
      
   //}
}

//
//  The rest of the cases for Expression are very straightforward
//  and introduce nothing that isn't already in the code discussed
//  above.
//
void block_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   for(int i = body->first(); body->more(i); i = body->next(i))
     body->nth(i)->annotate_with_types();
   if (body->len() != 0) {
      this->type = body->nth(body->len() - 1)->type;
   } else {
      this->type = Object;
   }
   
}

void let_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   scope_enter();
   VarSymbolType *v = new VarSymbolType(identifier, type_decl);
   vartable->addid(identifier, v);
   init->annotate_with_types();
   body->annotate_with_types();
   scope_exits();
   this->type = body->get_type();
}

static bool type_expect_check(Symbol actual, Symbol expect)
{
   if (actual != expect) {
      sprintf(log_buf, "e1->type is %s but expected is %s", 
                        actual? actual->get_string() : "NULL",
                        expect? expect->get_string() : "NULL");
      semant_error(log_buf);
      return false;
   }

   return true;
}

void plus_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   e1->annotate_with_types();
   e2->annotate_with_types();
   type_expect_check(e1->type, Int);
   type_expect_check(e2->type, Int);
   this->type = Int;
}

void sub_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   e1->annotate_with_types();
   e2->annotate_with_types();
   type_expect_check(e1->type, Int);
   type_expect_check(e2->type, Int);
   this->type = Int;
}

void mul_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   e1->annotate_with_types();
   e2->annotate_with_types();
   type_expect_check(e1->type, Int);
   type_expect_check(e2->type, Int);
   this->type = Int;
}

void divide_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   e1->annotate_with_types();
   e2->annotate_with_types();
   type_expect_check(e1->type, Int);
   type_expect_check(e2->type, Int);
   this->type = Int;
}

void neg_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   e1->annotate_with_types();
   this->type = Int;
}

void lt_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   e1->annotate_with_types();
   e2->annotate_with_types();
   this->type = Bool;
}


void eq_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   e1->annotate_with_types();
   e2->annotate_with_types();
   this->type = Bool;
}

void leq_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   e1->annotate_with_types();
   e2->annotate_with_types();
   this->type = Bool;
}

void comp_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   e1->annotate_with_types();
   this->type = Bool;
}

void int_const_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   this->type = Int;
}

void bool_const_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   this->type = Bool;
}

void string_const_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   this->type = Str;
}

void new__class::annotate_with_types()
{
   cur_line = this->get_line_number();
   if (type_name == SELF_TYPE) {
      type = cur_class->get_class_name();
   }
   this->type = type_name;
}

void isvoid_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   e1->annotate_with_types();
   this->type = Bool;
}

void no_expr_class::annotate_with_types()
{
   // cerr << cur_class->get_filename() << ":" << cur_line << " enter no expr" << endl;
}

void object_class::annotate_with_types()
{
   cur_line = this->get_line_number();
   VarSymbolType *v = vartable->lookup(name);
   if (v == NULL) {
      sprintf(log_buf, "variable %s not declared before use in object_class", name->get_string());
      semant_error(log_buf);
      this->type = Object;

   } else {
      this->type = v->type;
   }
}

