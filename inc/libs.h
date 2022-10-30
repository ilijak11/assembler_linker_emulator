#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

#define MAXLINE 100
#define MAXSYM 100
#define MAXSEC 10
#define MAXRELOC 100
#define MAXSECTIONSIZE 1024
#define INSTRNUM 26

int open_files(FILE** in, FILE** out, char** in_file_name, char** out_file_name, int argc, char** argv);
int get_next_line(FILE* in, char* line);
int assemble(FILE* in, FILE* out, char* infile);
char* dupstr(char* str);

int parser_test(FILE* in);

enum scope{L, G};
enum defined{DEFINED, UNDEFINED};
enum used{USED, UNUSED};
enum type{SECT, NOTYPE};
enum patch_type{I_ARG_APS16, I_ARG_PC16, WDIR};
enum reloc_type{APS16, PC16, APS16D, UND};

typedef struct fwdlink{
  enum patch_type type;
  int sect;
  int patch;
  struct fwdlink* nlink;
} Fwd_refs;

typedef struct sym{
  char* sym;
  int sect;
  int sym_tab_entry;
  int offs_val;
  enum type type;
  enum scope scope;
  enum defined defined;
  enum used used;
  Fwd_refs* flink;
}Sym_tab_entry;

typedef struct rel{
  int sec;
  int offs;
  enum reloc_type type;
  int sym_tab_entry;
  int addend;
  struct rel* next;
} Reloc;

typedef struct sec{
  //char* name;
  int sym_tab_entry;
  int offs;
  char* data;
  int size;
  enum used used;
  Reloc* reloc;
}Sec_tab_entry;


