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

static method_class *get_method_declare(Symbol cs, Symbol method_name)
{
   Class_ c = classtable->get_class_by_symbol(cs);
   Features features = c->get_class_features();
   for(int i = features->first(); features->more(i); i = features->next(i)) {
      Feature f = features->nth(i);
      method_class *m = dynamic_cast<method_class*>(f);
      if (m && m->get_method_name() == method_name) {
         return m;
      }
   }

   return get_method_declare(c->get_class_parent(), method_name);
}

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

void warn_error(char *msg, tree_node* t)
{
   cerr << "[warn] " << cur_class->get_filename() << " at line " << t->get_line_number() << ": " << msg << endl;
}

static bool assert_type_sound(Symbol type_child, Symbol type_parent) 
{
   return true;
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
   assert_type_sound(expr->type, return_type);
   scope_exits();
}

void attr_class::annotate_with_types()
{
   VarSymbolType *v = new VarSymbolType(name, type_decl);
   vartable->addid(name, v);
   init->annotate_with_types();
   assert_type_sound(init->type, type_decl);
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
      warn_error("variable not declared before use in assign_class\n", this);
   } else {
      assert_type_sound(expr->type, v->type);
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
}

//
//   dispatch_class::annotate_with_types is similar to 
//   static_dispatch_class::annotate_with_types 
//
void dispatch_class::annotate_with_types()
{
   expr->annotate_with_types();
   for(int i = actual->first(); actual->more(i); i = actual->next(i))
     actual->nth(i)->annotate_with_types();
   method_class *m = get_method_declare(cur_class->get_class_name(), name);
   if (m == NULL) {
      sprintf(log_buf, "method %s not declared before use in dispatch_class", name->get_string());
      warn_error(log_buf, this);
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
}

//
// loop_class::annotate_with_types dumps the predicate and then the
// body of the loop, and finally the type of the entire expression.
//
void loop_class::annotate_with_types()
{
   pred->annotate_with_types();
   body->annotate_with_types();
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
}

void sub_class::annotate_with_types()
{
   e1->annotate_with_types();
   e2->annotate_with_types();
}

void mul_class::annotate_with_types()
{
   e1->annotate_with_types();
   e2->annotate_with_types();
}

void divide_class::annotate_with_types()
{
   e1->annotate_with_types();
   e2->annotate_with_types();
}

void neg_class::annotate_with_types()
{
   e1->annotate_with_types();
}

void lt_class::annotate_with_types()
{
   e1->annotate_with_types();
   e2->annotate_with_types();
}


void eq_class::annotate_with_types()
{
   e1->annotate_with_types();
   e2->annotate_with_types();
}

void leq_class::annotate_with_types()
{
   e1->annotate_with_types();
   e2->annotate_with_types();
}

void comp_class::annotate_with_types()
{
   e1->annotate_with_types();
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
      warn_error(log_buf, this);
      this->type = Object;

   } else {
      this->type = v->type;
   }
}

