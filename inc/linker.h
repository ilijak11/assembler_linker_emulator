#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHEX "-hex"
#define CRELOCATABLE "-relocatable"
#define CPLACE "-place="
#define CNAME "-o"

#define MAXSYM 300
#define MAXSEC 100
#define MAXSECSIZE 1024

enum type{HEX, RELOCATABLE, NOTYPE};
enum sym_type{SYM, SECT};
enum defined{DEF, UNDEF};
enum scope{L, G};
enum reloc_type{APS16, APS16D, PC16, UND};

typedef struct file_list{
  FILE* file;
  char* file_name;
  struct file_list* next;
} File_node;

typedef struct place_list{
  char* sect;
  int offset;
  struct place_list* next;
} Place_node;

typedef struct command{
  enum type type;
  char* out_file_name;
  Place_node* place_list;
  File_node* file_list;
} Parsed_command;

typedef struct sym{
  char* sym;
  int sect;
  int size;
  int offs_val;
  enum sym_type type;
  enum scope scope;
  enum defined defined;
  char* file;
  int file_no;
} Symbol;

typedef struct rel{
  char* sym;
  int offs;
  int addend;
  enum reloc_type type;
  int resolved;
  struct rel* next;
} Reloc;

typedef struct sect{
  char* name;
  int offs;
  char* data;
  int size;
  char* file;
  int file_no;
  int mapped; //offset mapping
  int outputted; //outputted to file
  Reloc* relocs;
} Section;

char* dupstr(char* str);
Parsed_command* parse_comand_line_arguments(int argc, char* argv[]);
void print_parsed_command(Parsed_command* pc);
void delete_object(Parsed_command* pc);
int link(Parsed_command* pc);