%{

%}

%define api.pure true

%code top {
	/* XOPEN for strdup */
	#define _XOPEN_SOURCE 600
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>

	#ifndef YYNOMEM
	#define YYNOMEM goto yyexhaustedlab
	#endif
}

%code requires{

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

}

%union{
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
}

%parse-param {struct line **data}


%param {void *scanner}

%code provides{
  void free_line(struct line*);
  void free_instr(struct instruction*);
  void free_dir(struct directive*);
  void free_lab(struct label*);
  void free_data_operand(struct data_operand*);
  void free_addr_operand(struct addr_operand*);
  void free_sym_list(struct symbol_list_node*);
  void free_mix_list(struct mix_list_node*);
}

%code {
	int yyerror(void *foo, char const *msg, const void *s);
	int yylex(void *lval, const void *s);
}


%token DIR_GLOB DIR_EXT DIR_SECT DIR_WORD DIR_SKIP DIR_ASCII DIR_EQU DIR_END
%token I_HALT I_INT I_IRET I_CALL I_RET I_JMP I_JEQ I_JNE I_JGT I_PUSH I_POP I_XCHG I_ADD I_SUB I_MUL I_DIV I_CMP I_NOT I_AND I_OR I_XOR I_TEST I_SHL I_SHR I_LDR I_STR
%token<regnum> R0 R1 R2 R3 R4 R5 R6 R7 R8
%token<string> STRING
%token<sym> SYMBOL
%token<lit> HEXLIT DECLIT
%token DOLL
%token PERC
%token LSQRB
%token RSQRB
%token PLUS
%token STAR
%token COMMENT
%token COMMA
%token COLON
%token MINUS
%token DIVIDE

%type<data_op> data_operand  
%type<addr_op> addr_operand
%type<line> line
%type<lab> label
%type<regnum> reg
%type<sym_list> symbol_list
%type<dir> directive
%type<instr> instruction
%type<lit> expr
%type<mix_list> mix_list

%destructor { free($$); } <sym>
%destructor { free($$); } <string>
%destructor { free_line($$); } <line>
%destructor { free_instr($$); } <instr>
%destructor { free_dir($$); } <dir>
%destructor { free_lab($$); } <lab>
%destructor { free_data_operand($$); } <data_op>
%destructor { free_addr_operand($$); } <addr_op>
%destructor { free_sym_list($$); } <sym_list>



%% 

line:
    COMMENT{
      struct line* line = (struct line*)malloc(sizeof(struct line));
      if(!line) YYNOMEM;
      *line = (struct line){
        .type = COMM,
        .lab = 0
      };
      *data = $$ = line; return 0;
    }
  | directive{
      struct line* line = (struct line*)malloc(sizeof(struct line));
      if(!line) YYNOMEM;
      *line = (struct line){
        .type = DIR,
        .content.dir = $1,
        .lab = 0
      };
      *data = $$ = line; return 0;
    }
  | instruction{
      struct line* line = (struct line*)malloc(sizeof(struct line));
      if(!line) YYNOMEM;
      *line = (struct line){
        .type = INSTR,
        .content.instr = $1,
        .lab = 0
      };
      *data = $$ = line; return 0;
    }
  | label instruction{
      struct line* line = (struct line*)malloc(sizeof(struct line));
      if(!line) YYNOMEM;
      *line = (struct line){
        .type = LAB_INSTR,
        .content.instr = $2,
        .lab = $1
      };
      *data = $$ = line; return 0;
  }
  | label directive{
      struct line* line =(struct line*)malloc(sizeof(struct line));
      if(!line) YYNOMEM;
      *line = (struct line){
        .type = LAB_DIR,
        .content.dir = $2,
        .lab = $1
      };
      *data = $$ = line; return 0;
  }
  | directive COMMENT{
      struct line* line = (struct line*)malloc(sizeof(struct line));
      if(!line) YYNOMEM;
      *line = (struct line){
        .type = DIR,
        .content.dir = $1,
        .lab = 0
      };
      *data = $$ = line; return 0;
  }
  | instruction COMMENT{
      struct line* line = (struct line*)malloc(sizeof(struct line));
      if(!line) YYNOMEM;
      *line = (struct line){
        .type = INSTR,
        .content.instr = $1,
        .lab = 0
      };
      *data = $$ = line; return 0;
  }
  | label directive COMMENT{
      struct line* line =(struct line*)malloc(sizeof(struct line));
      if(!line) YYNOMEM;
      *line = (struct line){
        .type = LAB_DIR,
        .content.dir = $2,
        .lab = $1
      };
      *data = $$ = line; return 0;
  }
  | label instruction COMMENT{
      struct line* line =(struct line*)malloc(sizeof(struct line));
      if(!line) YYNOMEM;
      *line = (struct line){
        .type = LAB_INSTR,
        .content.instr = $2,
        .lab = $1
      };
      *data = $$ = line; return 0;
  }
  | label{
      struct line* line =(struct line*)malloc(sizeof(struct line));
      if(!line) YYNOMEM;
      *line = (struct line){
        .type = LAB,
        .lab = $1
      };
      *data = $$ = line; return 0;
  }
  | label COMMENT{
      struct line* line =(struct line*)malloc(sizeof(struct line));
      if(!line) YYNOMEM;
      *line = (struct line){
        .type = LAB,
        .lab = $1
      };
      *data = $$ = line; return 0;
  }

instruction:
    I_HALT{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = HALT
      };
      $$ = instr;
    }
  | I_INT reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = INT,
        .op1.reg = $2
      };
      $$ = instr;
  }
  | I_IRET{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = IRET
      };
      $$ = instr;
  }
  | I_CALL addr_operand{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = CALL,
        .op1.addr_op = $2
      };
      $$ = instr;
  }
  | I_RET{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = RET
      };
      $$ = instr;
  }
  | I_JMP addr_operand{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = JMP,
        .op1.addr_op = $2
      };
      $$ = instr;
  }
  | I_JEQ addr_operand{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = JEQ,
        .op1.addr_op = $2
      };
      $$ = instr;
  }
  | I_JNE addr_operand{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = JNE,
        .op1.addr_op = $2
      };
      $$ = instr;
  }
  | I_JGT addr_operand{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = JGT,
        .op1.addr_op = $2
      };
      $$ = instr;
  }
  | I_PUSH reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = PUSH,
        .op1.reg = $2
      };
      $$ = instr;
  }
  | I_POP reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = POP,
        .op1.reg = $2
      };
      $$ = instr;
  }
  | I_XCHG reg COMMA reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = XCHG,
        .op1.reg = $2,
        .op2.reg = $4
      };
      $$ = instr;
  }
  | I_ADD reg COMMA reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = ADD,
        .op1.reg = $2,
        .op2.reg = $4
      };
      $$ = instr;
  }
  | I_SUB reg COMMA reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = SUB,
        .op1.reg = $2,
        .op2.reg = $4
      };
      $$ = instr;
  }
  | I_MUL reg COMMA reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = MUL,
        .op1.reg = $2,
        .op2.reg = $4
      };
      $$ = instr;
  }
  | I_DIV reg COMMA reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = DIV,
        .op1.reg = $2,
        .op2.reg = $4
      };
      $$ = instr;
  }
  | I_CMP reg COMMA reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = CMP,
        .op1.reg = $2,
        .op2.reg = $4
      };
      $$ = instr;
  }
  | I_NOT reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = NOT,
        .op1.reg = $2
      };
      $$ = instr;
  }
  | I_AND reg COMMA reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = AND,
        .op1.reg = $2,
        .op2.reg = $4
      };
      $$ = instr;
  }
  | I_OR reg COMMA reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = OR,
        .op1.reg = $2,
        .op2.reg = $4
      };
      $$ = instr;
  }
  | I_XOR reg COMMA reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = XOR,
        .op1.reg = $2,
        .op2.reg = $4
      };
      $$ = instr;
  }
  | I_TEST reg COMMA reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = TEST,
        .op1.reg = $2,
        .op2.reg = $4
      };
      $$ = instr;
  }
  | I_SHL reg COMMA reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = SHL,
        .op1.reg = $2,
        .op2.reg = $4
      };
      $$ = instr;
  }
  | I_SHR reg COMMA reg{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = SHR,
        .op1.reg = $2,
        .op2.reg = $4
      };
      $$ = instr;
  }
  | I_LDR reg COMMA data_operand{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = LDR,
        .op1.reg = $2,
        .op2.data_op = $4
      };
      $$ = instr;
  }
  | I_STR reg COMMA data_operand{
      struct instruction* instr = (struct instruction*)malloc(sizeof(struct instruction));
      if(!instr) YYNOMEM;
      *instr = (struct instruction){
        .type = STR,
        .op1.reg = $2,
        .op2.data_op = $4
      };
      $$ = instr;
  }

directive:
    DIR_GLOB symbol_list{
      struct directive* dir = (struct directive*)malloc(sizeof(struct directive));
      if(!dir) YYNOMEM;
      *dir = (struct directive){
        .type = GLOBAL,
        .param.sym_list = $2
      };
      $$ = dir;
    }
  | DIR_EXT symbol_list{
      struct directive* dir = (struct directive*)malloc(sizeof(struct directive));
      if(!dir) YYNOMEM;
      *dir = (struct directive){
        .type = EXTERN,
        .param.sym_list = $2
      };
      $$ = dir;
  }
  | DIR_SECT SYMBOL{
      struct directive* dir = (struct directive*)malloc(sizeof(struct directive));
      if(!dir) YYNOMEM;
      *dir = (struct directive){
        .type = SECTION,
        .param.sym = strdup($2)
      };
      if(!dir->param.sym){
        free(dir);
        YYNOMEM;
      }
      $$ = dir;
  }
  | DIR_WORD mix_list{
      struct directive* dir = (struct directive*)malloc(sizeof(struct directive));
      if(!dir) YYNOMEM;
      *dir = (struct directive){
        .type = WORD,
        .param.mix_list = $2
      };
      $$ = dir;
  }
  | DIR_SKIP HEXLIT{
      struct directive* dir = (struct directive*)malloc(sizeof(struct directive));
      if(!dir) YYNOMEM;
      *dir = (struct directive){
        .type = SKIP,
        .param.lit = $2
      };
      $$ = dir;
  }
  | DIR_SKIP DECLIT{
      struct directive* dir = (struct directive*)malloc(sizeof(struct directive));
      if(!dir) YYNOMEM;
      *dir = (struct directive){
        .type = SKIP,
        .param.lit = $2
      };
      $$ = dir;
  }
  | DIR_ASCII STRING{
      struct directive* dir = (struct directive*)malloc(sizeof(struct directive));
      if(!dir) YYNOMEM;
      *dir = (struct directive){
        .type = ASCII,
        .param.string = strdup($2)
      };
      if(!dir->param.string){
        free(dir);
        YYNOMEM;
      }
      $$ = dir;
  }
  | DIR_EQU SYMBOL COMMA expr{
      struct directive* dir = (struct directive*)malloc(sizeof(struct directive));
      if(!dir) YYNOMEM;
      *dir = (struct directive){
        .type = EQU,
        .param.sym = strdup($2),
        .exprval = $4
      };
      $$ = dir;
  }
  | DIR_END{
      struct directive* dir = (struct directive*)malloc(sizeof(struct directive));
      if(!dir) YYNOMEM;
      *dir = (struct directive){
        .type = END,
      };
      $$ = dir;
  }

addr_operand:
    HEXLIT{
      struct addr_operand *a = (struct addr_operand*)malloc(sizeof(struct addr_operand));
      if(!a) YYNOMEM;
      *a = (struct addr_operand){
          .operand.lit = $1,
          .type = IMMED_LIT
      };
      $$ = a;
    }
  | DECLIT{
      struct addr_operand *a = (struct addr_operand*)malloc(sizeof(struct addr_operand));
      if(!a) YYNOMEM;
      *a = (struct addr_operand){
          .operand.lit = $1,
          .type = IMMED_LIT
      };
      $$ = a;
  }
  | SYMBOL{
      struct addr_operand *a = (struct addr_operand*)malloc(sizeof(struct addr_operand));
      if(!a) YYNOMEM;
      *a = (struct addr_operand){
          .operand.sym = strdup($1),
          .type = IMMED_SYM
      };
      if(!a->operand.sym){
        free(a);
        YYNOMEM;
      }
      $$ = a;
  }
  | PERC SYMBOL{
      struct addr_operand *a = (struct addr_operand*)malloc(sizeof(struct addr_operand));
      if(!a) YYNOMEM;
      *a = (struct addr_operand){
          .operand.sym = strdup($2),
          .type = PCREL
      };
      if(!a->operand.sym){
        free(a);
        YYNOMEM;
      }
      $$ = a;
  }
  | STAR HEXLIT{
      struct addr_operand *a = (struct addr_operand*)malloc(sizeof(struct addr_operand));
      if(!a) YYNOMEM;
      *a = (struct addr_operand){
          .operand.lit = $2,
          .type = MEMDIR_LIT
      };
      $$ = a;
  }
  | STAR DECLIT{
      struct addr_operand *a = (struct addr_operand*)malloc(sizeof(struct addr_operand));
      if(!a) YYNOMEM;
      *a = (struct addr_operand){
          .operand.lit = $2,
          .type = MEMDIR_LIT
      };
      $$ = a;
  }
  | STAR SYMBOL{
      struct addr_operand *a = (struct addr_operand*)malloc(sizeof(struct addr_operand));
      if(!a) YYNOMEM;
      *a = (struct addr_operand){
          .operand.sym = strdup($2),
          .type = MEMDIR_SYM
      };
      if(!a->operand.sym){
        free(a);
        YYNOMEM;
      }
      $$ = a;
  }
  | STAR reg{
      struct addr_operand *a = (struct addr_operand*)malloc(sizeof(struct addr_operand));
      if(!a) YYNOMEM;
      *a = (struct addr_operand){
          .operand.reg = $2,
          .type = REGDIR
      };
      $$ = a;
  }
  | STAR LSQRB reg RSQRB{
      struct addr_operand *a = (struct addr_operand*)malloc(sizeof(struct addr_operand));
      if(!a) YYNOMEM;
      *a = (struct addr_operand){
          .operand.reg = $3,
          .type = REGINDIR
      };
      $$ = a;
  }
  | STAR LSQRB reg PLUS HEXLIT RSQRB{
      struct addr_operand *a = (struct addr_operand*)malloc(sizeof(struct addr_operand));
      if(!a) YYNOMEM;
      *a = (struct addr_operand){
          .operand.reg = $3,
          .offs.lit = $5,
          .type = REGINDIRPOM_LIT
      };
      $$ = a;
  }
  | STAR LSQRB reg PLUS DECLIT RSQRB{
      struct addr_operand *a = (struct addr_operand*)malloc(sizeof(struct addr_operand));
      if(!a) YYNOMEM;
      *a = (struct addr_operand){
          .operand.reg = $3,
          .offs.lit = $5,
          .type = REGINDIRPOM_LIT
      };
      $$ = a;
  }
  | STAR LSQRB reg PLUS SYMBOL RSQRB{
      struct addr_operand *a = (struct addr_operand*)malloc(sizeof(struct addr_operand));
      if(!a) YYNOMEM;
      *a = (struct addr_operand){
          .operand.reg = $3,
          .offs.sym = strdup($5),
          .type = REGINDIRPOM_SYM
      };
      if(!a->offs.sym){
        free(a);
        YYNOMEM;
      }
      $$ = a;
  }

data_operand:
    DOLL HEXLIT {
      struct data_operand *d = (struct data_operand*)malloc(sizeof(struct data_operand));
      if(!d) YYNOMEM;
      *d = (struct data_operand){
          .operand.lit = $2,
          .type = IMMED_LIT
      };
      $$ = d;
    }
  | DOLL DECLIT{
      struct data_operand *d = (struct data_operand*)malloc(sizeof(struct data_operand));
      if(!d) YYNOMEM;
      *d = (struct data_operand){
          .operand.lit = $2,
          .type = IMMED_LIT
      };
      $$ = d;
  }
  | DOLL SYMBOL{
      struct data_operand *d = (struct data_operand*)malloc(sizeof(struct data_operand));
      if(!d) YYNOMEM;
      *d = (struct data_operand){
          .operand.sym = strdup($2),
          .type = IMMED_SYM
      };
      if(!d->operand.sym){
        free(d);
        YYNOMEM;
      }
      $$ = d;
  }
  | DECLIT{
    struct data_operand *d = (struct data_operand*)malloc(sizeof(struct data_operand));
      if(!d) YYNOMEM;
      *d = (struct data_operand){
          .operand.lit = $1,
          .type = MEMDIR_LIT
      };
      $$ = d;
  }
  | HEXLIT{
      struct data_operand *d = (struct data_operand*)malloc(sizeof(struct data_operand));
      if(!d) YYNOMEM;
      *d = (struct data_operand){
          .operand.lit = $1,
          .type = MEMDIR_LIT
      };
      $$ = d;
  }
  | SYMBOL{
      struct data_operand *d = (struct data_operand*)malloc(sizeof(struct data_operand));
      if(!d) YYNOMEM;
      *d = (struct data_operand){
          .operand.sym = strdup($1),
          .type = MEMDIR_SYM
      };
      if(!d->operand.sym){
        free(d);
        YYNOMEM;
      }
      $$ = d;
  }
  | PERC SYMBOL{
      struct data_operand *d = (struct data_operand*)malloc(sizeof(struct data_operand));
      if(!d) YYNOMEM;
      *d = (struct data_operand){
          .operand.sym = strdup($2),
          .type = PCREL
      };
      if(!d->operand.sym){
        free(d);
        YYNOMEM;
      }
      $$ = d;
  }
  | reg{
      struct data_operand *d = (struct data_operand*)malloc(sizeof(struct data_operand));
      if(!d) YYNOMEM;
      *d = (struct data_operand){
          .operand.reg = $1,
          .type = REGDIR
      };
      $$ = d;
  }
  | LSQRB reg RSQRB{
      struct data_operand *d = (struct data_operand*)malloc(sizeof(struct data_operand));
      if(!d) YYNOMEM;
      *d = (struct data_operand){
          .operand.reg = $2,
          .type = REGINDIR
      };
      $$ = d;
  }
  | LSQRB reg PLUS HEXLIT RSQRB{
      struct data_operand *d = (struct data_operand*)malloc(sizeof(struct data_operand));
      if(!d) YYNOMEM;
      *d = (struct data_operand){
          .operand.reg = $2,
          .offs.lit = $4,
          .type = REGINDIRPOM_LIT
      };
      $$ = d;
  }
  | LSQRB reg PLUS DECLIT RSQRB{
      struct data_operand *d = (struct data_operand*)malloc(sizeof(struct data_operand));
      if(!d) YYNOMEM;
      *d = (struct data_operand){
          .operand.reg = $2,
          .offs.lit = $4,
          .type = REGINDIRPOM_LIT
      };
      $$ = d;
  }
  | LSQRB reg PLUS SYMBOL RSQRB{
      struct data_operand *d = (struct data_operand*)malloc(sizeof(struct data_operand));
      if(!d) YYNOMEM;
      *d = (struct data_operand){
          .operand.reg = $2,
          .offs.sym = strdup($4),
          .type = REGINDIRPOM_SYM
      };
      if(!d->offs.sym){
        free(d);
        YYNOMEM;
      }
      $$ = d;
  }

label:
    SYMBOL COLON {
      struct label* l = (struct label*)malloc(sizeof(struct label));
      if(!l) YYNOMEM;
      *l = (struct label){
        .sym = strdup($1)
      };
      if(!l->sym){
        free(l);
        YYNOMEM;
      }
      $$ = l;
    }

symbol_list:
    SYMBOL{
        struct symbol_list_node* node = (struct symbol_list_node*)malloc(sizeof(struct symbol_list_node));
        if(!node) YYNOMEM;
        *node = (struct symbol_list_node){
          .sym = strdup($1),
          .next = 0
        };
        if(!node->sym){
          free(node);
          YYNOMEM;
        }
        $$ = node;
    }
  | SYMBOL COMMA symbol_list{
        struct symbol_list_node* node = (struct symbol_list_node*)malloc(sizeof(struct symbol_list_node));
        if(!node) YYNOMEM;
        *node = (struct symbol_list_node){
          .sym = strdup($1),
          .next = $3
        };
        if(!node->sym){
          free(node);
          YYNOMEM;
        }
        $$ = node;
    }

mix_list:
    HEXLIT{
      struct mix_list_node* node = (struct mix_list_node*)malloc(sizeof(struct mix_list_node));
      if(!node) YYNOMEM;
      *node = (struct mix_list_node){
        .type = MLN_LIT,
        .data.lit = $1,
        .next = 0
      };
      $$ = node;
    }
  | DECLIT{
      struct mix_list_node* node = (struct mix_list_node*)malloc(sizeof(struct mix_list_node));
      if(!node) YYNOMEM;
      *node = (struct mix_list_node){
        .type = MLN_LIT,
        .data.lit = $1,
        .next = 0
      };
      $$ = node;
  }
  | SYMBOL{
      struct mix_list_node* node = (struct mix_list_node*)malloc(sizeof(struct mix_list_node));
      if(!node) YYNOMEM;
      *node = (struct mix_list_node){
        .type = MLN_SYM,
        .data.sym = strdup($1),
        .next = 0
      };
      if(!node->data.sym){
        free(node);
        YYNOMEM;
      }
      $$ = node;
  }
  | HEXLIT COMMA mix_list{
      struct mix_list_node* node = (struct mix_list_node*)malloc(sizeof(struct mix_list_node));
      if(!node) YYNOMEM;
      *node = (struct mix_list_node){
        .type = MLN_LIT,
        .data.lit = $1,
        .next = $3
      };
      $$ = node;
  }
  | DECLIT COMMA mix_list{
       struct mix_list_node* node = (struct mix_list_node*)malloc(sizeof(struct mix_list_node));
      if(!node) YYNOMEM;
      *node = (struct mix_list_node){
        .type = MLN_LIT,
        .data.lit = $1,
        .next = $3
      };
      $$ = node;
  }
  | SYMBOL COMMA mix_list{
      struct mix_list_node* node = (struct mix_list_node*)malloc(sizeof(struct mix_list_node));
      if(!node) YYNOMEM;
      *node = (struct mix_list_node){
        .type = MLN_SYM,
        .data.sym = strdup($1),
        .next = $3
      };
      if(!node->data.sym){
        free(node);
        YYNOMEM;
      }
      $$ = node;
  }

reg:
    R0  {$$ = $1;}
  | R1  {$$ = $1;}
  | R2  {$$ = $1;}
  | R3  {$$ = $1;}
  | R4  {$$ = $1;}
  | R5  {$$ = $1;}
  | R6  {$$ = $1;}
  | R7  {$$ = $1;}
  | R8  {$$ = $1;}

expr:
    DECLIT  {$$ = $1;}
  | HEXLIT  {$$ = $1;}
  | expr PLUS DECLIT {$$ = $1 + $3;}
  | expr PLUS HEXLIT {$$ = $1 + $3;}
  | expr STAR DECLIT {$$ = $1 * $3;}
  | expr STAR HEXLIT {$$ = $1 * $3;}
  | expr MINUS DECLIT {$$ = $1 - $3;}
  | expr MINUS HEXLIT {$$ = $1 - $3;}
  | expr DIVIDE DECLIT {$$ = $1 / $3;}
  | expr DIVIDE HEXLIT {$$ = $1 / $3;}
  

%%

int yyerror(void* yylval, const char *msg, const void *s){
  (void)yylval;
  (void)s;
  printf("syntax error ");
  return 0;
}

void free_line(struct line* line){
  switch(line->type){
    case DIR: free_dir(line->content.dir); break;
    case INSTR: free_instr(line->content.instr); break;
    case LAB_INSTR: free_instr(line->content.instr); free_lab(line->lab); break;
    case LAB_DIR: free_dir(line->content.dir); free_lab(line->lab); break;
    case LAB: free_lab(line->lab); break;
    default: break;
  }
  free(line);
}

void free_instr(struct instruction* instr){
  switch(instr->type){
    case CALL: free_addr_operand(instr->op1.addr_op); break;
    case JMP: free_addr_operand(instr->op1.addr_op); break;
    case JEQ: free_addr_operand(instr->op1.addr_op); break;
    case JNE: free_addr_operand(instr->op1.addr_op); break;
    case JGT: free_addr_operand(instr->op1.addr_op); break;
    case LDR: free_data_operand(instr->op2.data_op); break;
    case STR: free_addr_operand(instr->op2.addr_op); break;
    default: break;
  }
  free(instr);
}

void free_dir(struct directive* dir){
  switch(dir->type){
    case GLOBAL: free_sym_list(dir->param.sym_list); break;
    case EXTERN: free_sym_list(dir->param.sym_list); break;
    case SECTION: free(dir->param.sym); break;
    case WORD: free_mix_list(dir->param.mix_list); break;
    case EQU: free(dir->param.sym); break;
    case ASCII: free(dir->param.string); break;
    default: break;
  }
  free(dir);
}

void free_sym_list(struct symbol_list_node* head){
  if(!head) return;
  struct symbol_list_node* old = head;
  while(!head){
    head = head->next;
    if(old->sym) free(old->sym);
    free(old);
    old = head;
  }
}

void free_mix_list(struct mix_list_node* head){
  if(!head) return;
  struct mix_list_node* old = head;
  while(!head){
    head = head->next;
    if(old->type == MLN_SYM){
      if(old->data.sym) free(old->data.sym);
    }
    free(old);
    old = head;
  }
}

void free_data_operand(struct data_operand* dop){
  switch(dop->type){
    case IMMED_SYM: free(dop->operand.sym); break;
    case MEMDIR_SYM: free(dop->operand.sym); break;
    case PCREL: free(dop->operand.sym); break;
    case REGINDIRPOM_SYM: free(dop->offs.sym); break;
    default: break;
  }
  free(dop);
}

void free_addr_operand(struct addr_operand* ao){
  switch(ao->type){
    case IMMED_SYM: free(ao->operand.sym); break;
    case MEMDIR_SYM: free(ao->operand.sym); break;
    case PCREL: free(ao->operand.sym); break;
    case REGINDIRPOM_SYM: free(ao->offs.sym); break;
    default: break; 
  }
  free(ao);
}

void free_lab(struct label* l){
  free(l->sym);
  free(l);
}
