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

void warn_error(char *msg, tree_node* t)
{
   cerr << "[warn] at line " << t->get_line_number() << " with " << msg;
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

//
// Prints the components of a class, including all of the features.
// Note that printing the Features is another use of an iterator.
//
void class__class::annotate_with_types()
{
   vartable->enterscope();
   functable->enterscope();
   vartable->addid(self, new VarSymbolType(self, SELF_TYPE));
   vartable->addid(cur_class, new VarSymbolType(cur_class, name));
   for(int i = features->first(); features->more(i); i = features->next(i))
     features->nth(i)->annotate_with_types();
   vartable->exitscope();
   functable->exitscope();
}

void method_class::annotate_with_types()
{
   vartable->enterscope();
   functable->enterscope();
   for(int i = formals->first(); formals->more(i); i = formals->next(i))
     formals->nth(i)->annotate_with_types();
   expr->annotate_with_types();
   vartable->exitscope();
   functable->exitscope();
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
   init->annotate_with_types();
   body->annotate_with_types();
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
      VarSymbolType *v = vartable->lookup(cur_class);
      if (v == NULL) {
         fatal_error("unexpected semantic error, cur_class is not initialized!\n");
      }
      type = v->type;
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
      warn_error("variable not declared before use in object_class\n", this);
      this->type = Object;

   } else {
      this->type = v->type;
   }
}

