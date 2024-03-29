#ifndef SEMANT_H_
#define SEMANT_H_

#include <assert.h>
#include <iostream>  
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"

#define TRUE 1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

// This is a structure that may be used to contain the semantic
// information such as the inheritance graph.  You may use it or not as
// you like: it is only here to provide a container for the supplied
// methods.

class ClassTable {
private:
  int semant_errors;
  void install_basic_classes();
  ostream& error_stream;
  Classes classlist;

public:
  ClassTable(Classes);
  Class_ get_class_by_symbol(Symbol s);
  int errors() { return semant_errors; }
  bool add_class_to_classlist(Class_ c);
  ostream& semant_error();
  ostream& semant_error(Class_ c);
  ostream& semant_error(Symbol filename, tree_node *t);
};

class VarSymbolType;
typedef VarSymbolType *VarSymbolTypeP;

class VarSymbolType {
public:
  Symbol name;
  Symbol type;
  VarSymbolType(Symbol name, Symbol type) {
    this->name = name;
    this->type = type;
  }
};

class FuncSymbolType;
typedef FuncSymbolType *FuncSymbolTypeP;
class FuncSymbolType {

};


extern SymbolTable<Symbol, VarSymbolType> *vartable;
extern ClassTable *classtable;
//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
extern Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;

extern char log_buf[256];
extern class__class *cur_class;
extern int cur_line;
extern ClassTable *classtable;

inline void semant_error_log(char *msg)
{
   cerr << "[semant error " << cur_class->get_filename()->get_string() << ":" << cur_line << "] " << msg << endl;
   if (classtable != NULL) {
      classtable->semant_error();
   }
}

#endif

