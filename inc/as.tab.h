/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_YY_INC_AS_TAB_H_INCLUDED
# define YY_YY_INC_AS_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 19 "misc/as.y"


    enum addressing_type{
        IMMED_LIT, IMMED_SYM,  MEMDIR_LIT, MEMDIR_SYM, PCREL, REGDIR, REGINDIR, REGINDIRPOM_LIT, REGINDIRPOM_SYM 
    };

    enum line_type{
        INSTR, LAB_INSTR, DIR, LAB_DIR, COMM, LAB
    };

    enum dir_type{
        GLOBAL, EXTERN, SECTION, WORD, SKIP, ASCII, EQU, END
    };

    enum instr_type{
        HALT = 0, INT, IRET, CALL, RET, JMP, JEQ, JNE, JGT, PUSH, POP, XCHG, ADD, SUB, MUL, DIV, CMP, NOT, AND, OR, XOR, TEST, SHL, SHR, LDR, STR
    };

    enum mix_list_node_type{
        MLN_LIT, MLN_SYM
    };

    struct symbol_list_node{
        char* sym;
        struct symbol_list_node* next;
    };

    struct mix_list_node{
        enum mix_list_node_type type;
        union{
          char* sym;
          int lit;
        } data;
        struct mix_list_node* next;
    };

    struct label{
        char* sym;
    };

    struct data_operand{
        union{
          char* sym;
          int lit;
          int reg;
        } operand;
        union{
          char* sym;
          int lit;
        } offs;
        enum addressing_type type;
    };

    struct addr_operand{
        union{
          char* sym;
          int lit;
          int reg;   
        } operand;
         union{
          char* sym;
          int lit;
        } offs;
        enum addressing_type type;
    };

    struct directive{
        enum dir_type type;
        union{
          int lit;
          struct symbol_list_node* sym_list;
          struct mix_list_node* mix_list;
          char* sym;
          char* string;
        } param;
        int exprval;
    };

    struct instruction{
        enum instr_type type;
        union{
          int reg;
          struct addr_operand* addr_op;
        }op1;
        union{
          int reg;
          struct data_operand* data_op;
          struct addr_operand* addr_op;
        }op2;
    };

    struct line{
        enum line_type type;
        union{
          struct directive* dir;
          struct instruction* instr;
        }content;
        struct label* lab;
    };


#line 150 "./inc/as.tab.h"

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    DIR_GLOB = 258,
    DIR_EXT = 259,
    DIR_SECT = 260,
    DIR_WORD = 261,
    DIR_SKIP = 262,
    DIR_ASCII = 263,
    DIR_EQU = 264,
    DIR_END = 265,
    I_HALT = 266,
    I_INT = 267,
    I_IRET = 268,
    I_CALL = 269,
    I_RET = 270,
    I_JMP = 271,
    I_JEQ = 272,
    I_JNE = 273,
    I_JGT = 274,
    I_PUSH = 275,
    I_POP = 276,
    I_XCHG = 277,
    I_ADD = 278,
    I_SUB = 279,
    I_MUL = 280,
    I_DIV = 281,
    I_CMP = 282,
    I_NOT = 283,
    I_AND = 284,
    I_OR = 285,
    I_XOR = 286,
    I_TEST = 287,
    I_SHL = 288,
    I_SHR = 289,
    I_LDR = 290,
    I_STR = 291,
    R0 = 292,
    R1 = 293,
    R2 = 294,
    R3 = 295,
    R4 = 296,
    R5 = 297,
    R6 = 298,
    R7 = 299,
    R8 = 300,
    STRING = 301,
    SYMBOL = 302,
    HEXLIT = 303,
    DECLIT = 304,
    DOLL = 305,
    PERC = 306,
    LSQRB = 307,
    RSQRB = 308,
    PLUS = 309,
    STAR = 310,
    COMMENT = 311,
    COMMA = 312,
    COLON = 313,
    MINUS = 314,
    DIVIDE = 315
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 121 "misc/as.y"

  char* sym;
  char* string;
  int regnum;
  int lit;
  struct addr_operand* addr_op;
  struct data_operand* data_op;
  struct line* line;
  struct label* lab; 
  struct symbol_list_node* sym_list;
  struct directive* dir;
  struct instruction* instr;
  struct mix_list_node* mix_list;

#line 237 "./inc/as.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (struct line **data, void *scanner);
/* "%code provides" blocks.  */
#line 141 "misc/as.y"

  void free_line(struct line*);
  void free_instr(struct instruction*);
  void free_dir(struct directive*);
  void free_lab(struct label*);
  void free_data_operand(struct data_operand*);
  void free_addr_operand(struct addr_operand*);
  void free_sym_list(struct symbol_list_node*);
  void free_mix_list(struct mix_list_node*);

#line 260 "./inc/as.tab.h"

#endif /* !YY_YY_INC_AS_TAB_H_INCLUDED  */
