/*
 *  The scanner definition for COOL.
 *  The lexical units of Cool are integers, type identifiers, object identifiers, 
 *  special notation, strings, keywords, and white space
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;
#define APPEND_STR(c) {\
  if (string_buf_ptr - string_buf >= MAX_STR_CONST) { \
    cool_yylval.error_msg = stringtable.add_string("strlen is too long")->get_string(); \
    BEGIN(INITIAL); \
    return ERROR; \
  } else { \
    *string_buf_ptr++ = (char)c; \
  } \
}

int symbol_index = 0;
int comment_count = 0;
extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */

%}

/*
 * Define names for regular expressions here.
 */
%x COMMENT STR

/* keywords */
CLASS [Cc][Ll][Aa][Ss][Ss]
ELSE  [Ee][Ll][Ss][Ee]
FI    [Ff][Ii]
IF [Ii][Ff]
IN [Ii][Nn]
INHERITS [Ii][Nn][Hh][Ee][Rr][Ii][Tt][Ss]
LET [Ll][Ee][Tt]
LOOP [Ll][Oo][Oo][Pp]
POOL [Pp][Oo][Oo][Ll]
THEN [Tt][Hh][Ee][Nn]
WHILE [Ww][Hh][Ii][Ll][Ee]
CASE [Cc][Aa][Ss][Ee]
ESAC [Ee][Ss][Aa][Cc]
OF [Oo][Ff]
NEW [Nn][Ee][Ww]
ISVOID [Ii][Ss][Vv][Oo][Ii][Dd]

/* constant */
INT_CONST [0-9]+
CHAR_CONST '[.\n]'
BOOL_CONST (t[Rr][Uu][Ee])|(f[Aa][Ll][Ss][Ee])

/* whitespace */
WS [ \n\t\r\f\v]+

/* id */
TYPEID [A-Z][0-9A-Za-z_]*
OBJECTID [a-z][0-9A-Za-z_]*

/* multi-char op */
DARROW "=>"
ASSIGN "<-"
NOT "not"
LE "<="

/* LET_STMT */
ERROR [^ \n\t\r\f\v]+
%%

 /*
  *  Nested comments
  */
<INITIAL>"*)" {
    cool_yylval.error_msg = stringtable.add_string("extro close comment quote")->get_string();
    return ERROR;
}
<COMMENT><<EOF>> {
    cool_yylval.error_msg = stringtable.add_string("unterminated quote")->get_string();
    BEGIN(INITIAL);
    return ERROR;
}
<COMMENT,INITIAL>"(*"		{
   BEGIN(COMMENT);
   comment_count++;
}

<COMMENT>[^*(\n]*	   /* eat anything that's not a '*' or '(' */
<COMMENT>"*"+[^*)\n]*   /* eat up '*'s not followed by '/'s */
<COMMENT>"("+[^*\n]*	   /* eat up '('s not followed by '*'s */
<COMMENT>\n		   ++curr_lineno;
<COMMENT>"*"+")"	   {
  comment_count--;
  if (comment_count == 0) {
      BEGIN(INITIAL);
  }
}

--.* { }

{INT_CONST} {
    cool_yylval.symbol = new IntEntry(yytext, yyleng, symbol_index++);
    return INT_CONST;    
}
{CHAR_CONST} {
    cool_yylval.symbol = new IntEntry(&yytext[1], 1, symbol_index++);
    return INT_CONST;    
}

{WS} { 
    unsigned int i = 0;

    for (i = 0; i < yyleng; i++) {
        if (yytext[i] == '\n') {
            curr_lineno++;
        }
    }
}

 /*
  *  The multiple-character operators.
  */

{DARROW}		{ return (DARROW); }
{LE}		{ return (LE); }
{NOT}		{ return (NOT); }
{ASSIGN}		{ return (ASSIGN); }

 /*
{ERROR} { 
  cool_yylval.error_msg = stringtable.add_string(yytext)->get_string();
  return (ERROR); 
}
*/

 /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */
{CLASS} { return CLASS; }
{ELSE} { return ELSE; }
{FI} { return FI; }
{IF} { return IF; }
{IN} { return IN; }
{INHERITS} { return INHERITS; }
{LET} { return LET; }
{LOOP} { return LOOP; }
{POOL} { return POOL; }
{THEN} { return THEN; }
{WHILE} { return WHILE; }
{CASE} { return CASE; }
{ESAC} { return ESAC; }
{OF} { return OF; }
{NEW} { return NEW; }
{ISVOID} { return ISVOID; }
[+/\-*=<.~,;:{}()@\[\]] { return yytext[0]; }

{BOOL_CONST} {
    if (yytext[0] == 't') {
        cool_yylval.boolean = 1;
    } else {
        cool_yylval.boolean = 0;
    }
    return BOOL_CONST;    
}

{TYPEID} {
    cool_yylval.symbol = new IdEntry(yytext, yyleng, symbol_index++);
    return TYPEID;
}

{OBJECTID} {
    cool_yylval.symbol = new IdEntry(yytext, yyleng, symbol_index++);
    return OBJECTID;
}

 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */
\"	   {
  string_buf_ptr = string_buf; BEGIN(STR);
}

<STR>\"	  { /* saw closing quote - all done */
    BEGIN(INITIAL);
    APPEND_STR('\0');
    /* return string constant token type and
    * value to parser
    */
    cool_yylval.symbol = new StringEntry(string_buf, string_buf_ptr - string_buf, symbol_index++);
    return STR_CONST;    
  }

<STR>\n	  {
    /* error - unterminated string constant */
    /* generate error message */
    curr_lineno++;
    cool_yylval.error_msg = stringtable.add_string("unterminated string constant")->get_string();
    BEGIN(INITIAL);
    return ERROR;
  }

<STR>\\[0-7]{1,3} {
    /* octal escape sequence */
    int result;

    (void) sscanf( yytext + 1, "%o", &result );

    if ( result > 0xff ) {
        /* error, constant is out-of-bounds */
        cool_yylval.error_msg = stringtable.add_string("constant is out-of-bounds")->get_string();
        BEGIN(INITIAL);
        return ERROR;
    }
    APPEND_STR(result);
}

<STR>\\[0-9]+ {
  /* generate error - bad escape sequence; something
  * like '\48' or '\0777777'
  */
    cool_yylval.error_msg = stringtable.add_string("bad escape sequence")->get_string();
    BEGIN(INITIAL);
    return ERROR;
}

<STR>\\n  { 
  APPEND_STR('\n');
}

<STR>\\t  APPEND_STR('\t');
<STR>\\r  APPEND_STR('\r');
<STR>\\b  APPEND_STR('\b');
<STR>\\f  APPEND_STR('\f');

<STR>\\(.|\n)  {
    if (yytext[1] == '\n') {
        curr_lineno++;
    }
    APPEND_STR(yytext[1]);
}

<STR>[^\\\n\"]+	  {
  char *yptr = yytext;
  while ( *yptr ) {
      APPEND_STR(*yptr++);
  }
}

%%
