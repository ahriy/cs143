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

static char log_buf[256];
class__class *cur_class;

static inline void scope_enter()
{
   vartable->enterscope();
   functable->enterscope();
}

static inline void scope_exits()
{
   vartable->exitscope();
   functable->exitscope();
}

void warn_error(char *msg)
{
   cerr << "[warn] " << msg << endl;
}

#define SEMANT_ERROR(msg) \
      cerr << msg << endl;
      // classtable->semant_error(cur_class); \
      

static method_class *get_method_declare(Symbol cs, Symbol method_name)
{
   if (classtable == 0) {
      fatal_error("classtable has not been initialized!\n");
   }

   Class_ c = classtable->get_class_by_symbol(cs);

   if (c == NULL) {
      sprintf(log_buf, "can't find class %s declaration in symbol table!\n", cs->get_string());
      warn_error(log_buf);
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

static bool class_is_comfort(Symbol cs1, Symbol cs2)
{
   if (classtable == 0) {
      fatal_error("classtable has not been initialized!\n");
   }

   if (cs1 == NULL) {
      SEMANT_ERROR("class_is_comfort: class1 is no type");
      return false;
   }

   if (cs2 == NULL) {
      SEMANT_ERROR("class_is_comfort: class2 is no type");
      return false;
   }

   if (cs1 == SELF_TYPE) {
      cs1 = cur_class->get_class_name();
   }

   if (cs2 == SELF_TYPE) {
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

   if (c1->get_class_parent() == cs2) {
      return true;
   } else {
      return class_is_comfort(c1->get_class_parent(), cs2);
   }
} 

//
void program_class::annotate_with_types()
{
   for(int i = classes->first(); classes->more(i); i = classes->next(i))
     classes->nth(i)->annotate_with_types();
}

void class__class::annotate_with_types()
{
   scope_enter();
   vartable->addid(self, new VarSymbolType(self, SELF_TYPE));
   cur_class = this;
   
   for(int i = features->first(); features->more(i); i = features->next(i)) {
      features->nth(i)->annotate_with_types();
   }
   scope_exits();
}

void method_class::annotate_with_types()
{
   scope_enter();
   for(int i = formals->first(); formals->more(i); i = formals->next(i))
     formals->nth(i)->annotate_with_types();
   expr->annotate_with_types();
   if (!class_is_comfort(expr->type, return_type)) {
      SEMANT_ERROR("method_class is not comfort");
   }
   scope_exits();
}

void attr_class::annotate_with_types()
{
   VarSymbolType *v = new VarSymbolType(name, type_decl);
   vartable->addid(name, v);
   init->annotate_with_types();
   if (!class_is_comfort(init->type, type_decl)) {
      SEMANT_ERROR("attr_class is not comfort");
   }
}

//
// formal_class::annotate_with_types dumps the name and type declaration
// of a formal parameter.
//
void formal_class::annotate_with_types()
{
   VarSymbolType *v = new VarSymbolType(name, type_decl);
   vartable->addid(name, v);
}

//
// branch_class::annotate_with_types dumps the name, type declaration,
// and body of any case branch.
//
void branch_class::annotate_with_types()
{
   expr->annotate_with_types();
   //TODO
}

//
// assign_class::annotate_with_types prints "assign" and then (indented)
// the variable being assigned, the expression, and finally the type
// of the result.  Note the call to dump_type (see above) at the
// end of the method.
//
void assign_class::annotate_with_types()
{
   expr->annotate_with_types();
   this->type = expr->type;
   VarSymbolType *v = vartable->lookup(name);
   if (v == NULL) {
      warn_error("variable not declared before use in assign_class\n");
   } else {
      if (!class_is_comfort(expr->type, v->type)) {
         SEMANT_ERROR("assign_class is not comfort");
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
   expr->annotate_with_types();
   for(int i = actual->first(); actual->more(i); i = actual->next(i))
     actual->nth(i)->annotate_with_types();
   //TODO
}

//
//   dispatch_class::annotate_with_types is similar to 
//   static_dispatch_class::annotate_with_types 
//
void dispatch_class::annotate_with_types()
{
   method_class *m = NULL;
   expr->annotate_with_types();
   for(int i = actual->first(); actual->more(i); i = actual->next(i))
     actual->nth(i)->annotate_with_types();

   if (expr->get_type() == SELF_TYPE) {
      m = get_method_declare(cur_class->get_class_name(), name);
   } else {
      m = get_method_declare(expr->get_type(), name);
   }
   
   if (m == NULL) {
      sprintf(log_buf, "method %s:%s not declared before use in dispatch_class in %s:%d",
               expr->get_type()->get_string(), name->get_string(), cur_class->get_filename()->get_string(), 
               this->get_line_number());
      warn_error(log_buf);
      return;
   }
   type = m->get_return_type();
}

//
// cond_class::annotate_with_types dumps each of the three expressions
// in the conditional and then the type of the entire expression.
//
void cond_class::annotate_with_types()
{
   pred->annotate_with_types();
   then_exp->annotate_with_types();
   else_exp->annotate_with_types();
   if (pred->type != Bool) {
      SEMANT_ERROR("pred is not Bool");
   }
   //TODO
}

//
// loop_class::annotate_with_types dumps the predicate and then the
// body of the loop, and finally the type of the entire expression.
//
void loop_class::annotate_with_types()
{
   pred->annotate_with_types();
   body->annotate_with_types();
   if (pred->type != Bool) {
      SEMANT_ERROR("pred is not Bool");
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
   expr->annotate_with_types();
   for(int i = cases->first(); cases->more(i); i = cases->next(i))
     cases->nth(i)->annotate_with_types();
   //TODO
}

//
//  The rest of the cases for Expression are very straightforward
//  and introduce nothing that isn't already in the code discussed
//  above.
//
void block_class::annotate_with_types()
{
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
   scope_enter();
   VarSymbolType *v = new VarSymbolType(identifier, type_decl);
   vartable->addid(identifier, v);
   init->annotate_with_types();
   body->annotate_with_types();
   scope_exits();
}

void plus_class::annotate_with_types()
{
   e1->annotate_with_types();
   e2->annotate_with_types();
   if (e1->type != Int) {
      SEMANT_ERROR("e1->type is not Int");
      
   }
   if (e2->type != Int) {
      SEMANT_ERROR("e2->type is not Int");
   }
   type = Int;
}

void sub_class::annotate_with_types()
{
   e1->annotate_with_types();
   e2->annotate_with_types();
   if (e1->type != Int) {
      SEMANT_ERROR("e1->type is not Int");
      
   }
   if (e2->type != Int) {
      SEMANT_ERROR("e2->type is not Int");
   }
   type = Int;
}

void mul_class::annotate_with_types()
{
   e1->annotate_with_types();
   e2->annotate_with_types();
   if (e1->type != Int) {
      SEMANT_ERROR("e1->type is not Int");
      
   }
   if (e2->type != Int) {
      SEMANT_ERROR("e2->type is not Int");
   }
   type = Int;
}

void divide_class::annotate_with_types()
{
   e1->annotate_with_types();
   e2->annotate_with_types();
   if (e1->type != Int) {
      SEMANT_ERROR("e1->type is not Int");
      
   }
   if (e2->type != Int) {
      SEMANT_ERROR("e2->type is not Int");
   }
   type = Int;
}

void neg_class::annotate_with_types()
{
   e1->annotate_with_types();
   type = Bool;
}

void lt_class::annotate_with_types()
{
   e1->annotate_with_types();
   e2->annotate_with_types();
   type = Bool;
}


void eq_class::annotate_with_types()
{
   e1->annotate_with_types();
   e2->annotate_with_types();
   type = Bool;
}

void leq_class::annotate_with_types()
{
   e1->annotate_with_types();
   e2->annotate_with_types();
   type = Bool;
}

void comp_class::annotate_with_types()
{
   e1->annotate_with_types();
   type = Int;
}

void int_const_class::annotate_with_types()
{
   type = Int;
}

void bool_const_class::annotate_with_types()
{
   type = Bool;
}

void string_const_class::annotate_with_types()
{
   type = Str;
}

void new__class::annotate_with_types()
{
   if (type_name == SELF_TYPE) {
      type = cur_class->get_class_name();
   }
   type = type_name;
}

void isvoid_class::annotate_with_types()
{
   e1->annotate_with_types();
}

void no_expr_class::annotate_with_types()
{
}

void object_class::annotate_with_types()
{
   VarSymbolType *v = vartable->lookup(name);
   if (v == NULL) {
      sprintf(log_buf, "variable %s not declared before use in object_class", name->get_string());
      warn_error(log_buf);
      this->type = Object;

   } else {
      this->type = v->type;
   }
}

