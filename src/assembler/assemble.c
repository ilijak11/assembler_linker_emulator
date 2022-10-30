#include "../../inc/libs.h"

int location_ctr = 0;
int curr_sect_ptr = -1;

int sym_tab_size = 1;
int sec_tab_size = 1;
//int rel_tab_size = 0;

int assembling = 0;

Sym_tab_entry sym_tab[MAXSYM];
Sec_tab_entry sec_tab[MAXSEC];
//Reloc_tab_entry reloc_tab[MAXRELOC];


int init_as(){
  for(int i = 0; i<MAXSYM; i++){
    sym_tab[i].sym = 0;
    sym_tab[i].sect = -1;
    sym_tab[i].offs_val = -1;
    sym_tab[i].type = NOTYPE;
    sym_tab[i].scope = L;
    sym_tab[i].defined = UNDEFINED;
    sym_tab[i].used = UNUSED;
    sym_tab[i].flink = 0;
  }
  for(int i = 0; i<MAXSEC; i++){
    //sec_tab[i].name = 0;
    sec_tab[i].offs = -1;
    sec_tab[i].data = 0;
    sec_tab[i].size = -1;
    sec_tab[i].sym_tab_entry = -1;
    sec_tab[i].used = UNUSED;
    sec_tab[i].reloc = 0;
  }
  //init *ABS* section (section 0) (sym 0)
  char* name = (char*)malloc(6*sizeof(char));
  if(!name) return -1;
  strcpy(name, "*ABS*");
  //sec_tab[0].name = name;
  sym_tab[0].sym = name;
  sym_tab[0].sect = 0;
  sym_tab[0].offs_val = 0;
  sym_tab[0].type = SECT;
  sym_tab[0].scope = L;
  sym_tab[0].defined = DEFINED;
  sym_tab[0].used = USED;
  sym_tab[0].flink = 0;
  //
  sec_tab[0].sym_tab_entry = 0;
  sec_tab[0].offs = 0;
  sec_tab[0].data = 0;
  sec_tab[0].size = 0;
  sec_tab[0].used = USED;
  sec_tab[0].reloc = 0;
  return 0;
}

void free_fwd_links(Fwd_refs* flink){
  Fwd_refs* head = flink;
  Fwd_refs* old = head;
  while(head){
    old = head;
    head = head->nlink;
    free(old);
  }
}

void free_relocs(Reloc* reloc){
  Reloc* head = reloc;
  Reloc* old = head;
  while(head){
    old = head;
    head = head->next;
    free(old);
  }
}

void destroy_as(){
  for(int i = 0; i<MAXSYM; i++){
    if(sym_tab[i].used == USED){
      if(sym_tab[i].sym) free(sym_tab[i].sym);
      //printf("symbol: %s destroyed\n", sym_tab[i].sym);
      free_fwd_links(sym_tab[i].flink);
    }
  }
  for(int i = 0; i<MAXSEC; i++){
    if(sec_tab[i].used == USED){
      //if(sec_tab[i].name) free(sec_tab[i].name);
      if(sec_tab[i].data) free(sec_tab[i].data);
      //printf("sect: %s destroyed\n", sec_tab[i].name);
      free_relocs(sec_tab[i].reloc);
    }
  }
}

char* dupstr(char* str){
  int len = sizeof(str) + 1;
  char* dupstr = (char*)malloc(len*sizeof(char));
  if(!dupstr) return 0;
  strcpy(dupstr, str);
  return dupstr;
}

int sym_exists(char* sym){
  int len = strlen(sym) + 1;
   Sym_tab_entry* entry;
  for(int i = 0; i<sym_tab_size; i++){
    entry = &sym_tab[i];
    //if(entry->type == SECT) continue;
    if(strncmp(sym, entry->sym, len) == 0){
      return i;
    }
  }
  return -1;
}

/*int sec_exists(char* sec){
  int len = strlen(sec) + 1;
   Sym_tab_entry* entry;
  for(int i = 0; i<sym_tab_size; i++){
    entry = &sym_tab[i];
   // if(entry->type == NOTYPE) continue;
    if(strncmp(sec, entry->sym, len) == 0){
      return i;
    }
  }
  return -1;
}
*/

void print_sec_relocs(Sec_tab_entry* entry){
  if(!entry || !entry->reloc) return;
  printf("reloc table:\n");
  printf("______________________________________________\n");
  printf("%8s\t%6s\t%10s\t%6s\n", "offset", "type", "symbol", "addend");
  printf("----------------------------------------------\n");
  Reloc* head = entry->reloc;
  while(head){
    printf("0x%06x\t", head->offs);
    switch(head->type){
      case APS16: printf("%6s\t", "APS16"); break;
      case PC16: printf("%6s\t", "PC16"); break;
      case APS16D: printf("%6s\t", "APS16D"); break;
      case UND: printf("%6s\t", "UND"); break;
    }
    printf("%10s\t",sym_tab[head->sym_tab_entry].sym);
    printf("%6d\n", head->addend);
    head = head->next;
  }
  printf("----------------------------------------------\n");
  printf("\n\n");
}

void print_sym_tab_entry(int ptr){
  //printf("entry: %d\n", ptr);
  Sym_tab_entry* entry = &sym_tab[ptr];
  if(entry->sym) printf("%3d %20s\t", ptr, entry->sym);
  else printf("%3d %20s\t", ptr, "null");
  if(entry->sect != -1)  printf("%10s\t", sym_tab[sec_tab[entry->sect].sym_tab_entry].sym);
  else  printf("%10s\t", "null");
  printf("0x%08x\t", entry->offs_val);
  if(entry->type == SECT) printf("%6s\t", "SECT");
  else printf("%6s\t", "NOTYPE");
  char c;
  if(entry->scope == L) c = 'l';
  //else if(entry->scope == E) c = 'e';
  else c = 'g';
  printf("%5c\t", c);
  if(entry->defined == DEFINED)  printf("%5s\t", "true");
  else  printf("%5s\t", "false");
  if(entry->used == USED) printf("%5s\n", "true");
  else printf("%5s\n", "false");
}

void print_sec_tab_entry(int ptr){
  Sec_tab_entry* entry = &sec_tab[ptr];
  if(entry->sym_tab_entry != -1) printf("%3d %20s\t", ptr, sym_tab[entry->sym_tab_entry].sym);
  else printf("%3d %20s\t", ptr, "null");
  printf("0x%08x\t", entry->offs);
  printf("%4d\t", entry->size);
  if(entry->used == USED) printf("%5s\n", "true");
  else printf("%5s\n", "false");
  if(ptr != 0){
    printf("memory content:\n");
    int rows = (entry->size / 8) + 1;
    int ctr = 0;
    for(int i = 0; i < rows; i++){
      printf("0x%04x\t", ctr);
      for(int j = 0; j<8; j++) printf("0x%02x ", (unsigned char)entry->data[ctr++]);
      printf("\n");
    }
    //printf("-------------\n");
  }
  print_sec_relocs(entry);
}

void print_sym_tab(){
  printf("symbol table:\n");
  printf("_____________________________________________________________________________________________\n");
  printf("%3s %20s\t%10s\t%8s\t%6s\t%5s\t%5s\t%5s\n", "num", "sym", "sect", "offs", "type", "scope", "def", "used");
  printf("---------------------------------------------------------------------------------------------\n");
  Sym_tab_entry* entry;
  for(int i = 0; i<sym_tab_size; i++){
    entry = &sym_tab[i];
    if(entry->type == SECT) print_sym_tab_entry(i);
  }
  for(int i = 0; i<sym_tab_size; i++){
    entry = &sym_tab[i];
    if(entry->type == NOTYPE) print_sym_tab_entry(i);
  }
  printf("---------------------------------------------------------------------------------------------\n");
}

void print_sec_tab(){
  printf("section table:\n");
  printf("_____________________________________________________________\n");
  printf("%3s %20s\t%8s\t%4s\t%5s\n", "num", "name", "offs", "size", "used");
  printf("-------------------------------------------------------------\n");
  for(int i = 0; i<sec_tab_size; i++){
    print_sec_tab_entry(i);
  }
  printf("-------------------------------------------------------------\n");
}

int add_new_sym(char* sym, int offs, int sec, enum type type,  enum scope scope, enum defined defined){
  if(sym_tab_size == MAXSYM) {
    printf("error: adding new symbol failed - max number of symbols reached\n");
    return -1;
  }
  Sym_tab_entry* new_entry = &sym_tab[sym_tab_size];
  new_entry->sym = dupstr(sym);
  if(!new_entry->sym) {
    printf("error: adding new symbol failed - symbol name allocation failed\n");
    return -1;
  }
  new_entry->sect = sec;
  new_entry->sym_tab_entry = sym_tab_size;
  new_entry->offs_val = offs;
  new_entry->scope = scope;
  new_entry->defined = defined;
  new_entry->type = type;
  new_entry->used = USED;
  sym_tab_size++;
  return 0;
}

int add_new_sec(int offs, int sym_tab_entry){
  if(sec_tab_size == MAXSEC) {
    printf("error: adding new section failed - max number of sections reached\n");
    return -1;
  }
  Sec_tab_entry* new_entry = &sec_tab[sec_tab_size];
  //new_entry->name = dupstr(name);
  //if(!new_entry->name) {
  // printf("error: adding new section failed - section name allocation failed\n");
  //  return -1;
  //}
  new_entry->offs = offs;
  new_entry->data = (char*)calloc(MAXSECTIONSIZE,sizeof(char));
  if(!new_entry->data) {
    printf("error: adding new section failed - section data memory allocation failed\n");
    return -1;
  }
  new_entry->size = 0;
  new_entry->used = USED;
  new_entry->sym_tab_entry = sym_tab_entry;
  sec_tab_size++;
  return 0;
}

int add_new_flink(Sym_tab_entry* entry, short addr, int sec, enum patch_type type){
  if(!entry){
    printf("error: add_new_flink - null symbol entry\n");
    return -1;
  }
  Fwd_refs* new_ref = (Fwd_refs*)malloc(sizeof(Fwd_refs));
  if(!new_ref){
    printf("error: adding new forward ref failed - allocating memory failed\n");
    return -1;
  }
  new_ref->sect = sec;
  new_ref->patch = addr;
  new_ref->type = type;
  new_ref->nlink = 0;
  Fwd_refs* tmp = entry->flink;
  if(!tmp){
    entry->flink = new_ref;
    return 0;
  }
  while(tmp->nlink) tmp = tmp->nlink;
  tmp->nlink = new_ref;
  return 0;
}

int add_new_reloc(Sec_tab_entry* entry, int sec, int offs, enum reloc_type type, int sym, int addend){
  if(!entry){
    printf("error: add_new_realoc - null section entry\n");
    return -1;
  }
  Reloc* new_reloc = (Reloc*)malloc(sizeof(Reloc));
  if(!new_reloc){
    printf("error: adding new reloc failed - allocating memory failed\n");
    return -1;
  }
  new_reloc->sec = sec;
  new_reloc->offs = offs;
  new_reloc->type = type;
  new_reloc->sym_tab_entry = sym;
  new_reloc->addend = addend;
  new_reloc->next = 0;
  Reloc* tmp = entry->reloc;
  if(!tmp){
    entry->reloc = new_reloc;
    return 0;
  }
  while(tmp->next) tmp = tmp->next;
  tmp->next = new_reloc;
  return 0;
}

void update_reloc(Sym_tab_entry* entry, enum scope oldscope){
  if(oldscope == G) return;
  for(int i = 1; i<sec_tab_size; i++){
    Sec_tab_entry* sec = &sec_tab[i];
    if(sec->reloc){
      Reloc* rel = sec->reloc;
      while(rel){
        if(rel->sym_tab_entry == sec_tab[entry->sect].sym_tab_entry && rel->addend == entry->offs_val){
          rel->sym_tab_entry = entry->sym_tab_entry;
          switch(rel->type){
            case APS16: rel->addend = 0; break;
            case APS16D:  rel->addend = 0; break;
            case PC16: rel->addend = -2; break;
          }
        }
        rel = rel->next;
      }
    }
  }
}

int resolve_flinks(Sym_tab_entry* entry){
  if(!entry->flink) return 0;
  Fwd_refs* head = entry->flink;
  Sec_tab_entry* sec;
  while(head){
    sec = &sec_tab[head->sect];
    switch(head->type){
      case I_ARG_APS16:{
        if(entry->defined == DEFINED && entry->sect == 0){ //equ
          short val = (short)entry->offs_val;
          char hi = (char)(val >> 8);
          char low = (char)val;
          sec->data[head->patch++] = hi;
          sec->data[head->patch++] = low;
          break;
        }
        else{
          switch(entry->scope){
            case L: if(add_new_reloc(sec, head->sect, head->patch, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
            case G: if(add_new_reloc(sec, head->sect, head->patch, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
            default: printf("error: bad symbol scope\n"); return -1;
          }
        }
        break;
      }
      case I_ARG_PC16:{
        if(head->sect == entry->sect || entry->defined == DEFINED && entry->sect == 0){
          short offs = entry->offs_val - head->patch - 2;
          char hi = (char)(offs >> 8);
          char low = (char)offs;
          sec->data[head->patch++] = hi;
          sec->data[head->patch++] = low;
          break;
        }
        else{
          switch(entry->scope){ 
            case L: if(add_new_reloc(sec, head->sect, head->patch, PC16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val - 2) != 0) return -1; break;
            case G: if(add_new_reloc(sec, head->sect, head->patch, PC16, entry->sym_tab_entry, -2) != 0) return -1; break;
            default: printf("error: bad symbol scope\n"); return -1;
          }
        }
        break;
      }
      case WDIR:{
        if(entry->defined == DEFINED && entry->sect == 0){
          short val = (short)entry->offs_val;
          char hi = (char)(val >> 8);
          char low = (char)val;
          sec->data[head->patch++] = low;
          sec->data[head->patch++] = hi;
          break;
        }
        else{
          switch(entry->scope){
            case L: if(add_new_reloc(sec, head->sect, head->patch, APS16D, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
            case G: if(add_new_reloc(sec, head->sect, head->patch, APS16D, entry->sym_tab_entry, 0) != 0) return -1; break;
            default: printf("error: bad symbol scope\n"); return -1;
          }
        }
        break;
      }
      default: printf("error: resolve flinks undef type\n"); return -1;
    }
    head = head->nlink;
  }
  free_fwd_links(entry->flink);
  entry->flink = 0;
  return 0;
}

int define_symbol(Sym_tab_entry* entry, int sect, int offs, enum scope scope, enum defined defined){
  entry->sect = sect;
  entry->offs_val = offs;
  entry->scope = scope;
  entry->defined = defined;
  return resolve_flinks(entry);
}

int handle_label(struct label* lab){
  int ptr = sym_exists(lab->sym);
  if(ptr == -1){
    return add_new_sym(lab->sym, location_ctr, curr_sect_ptr, NOTYPE, L, DEFINED);
  }
  Sym_tab_entry* entry = &sym_tab[ptr];
  if(entry->defined == DEFINED || (entry->defined == UNDEFINED && entry->sect == 0)){
    printf("symbol: %s already exists\n", lab->sym);
    return -1;
  }
  //UNDEF sec == -1 - nedefinisan simbol
  if(entry->defined == UNDEFINED && entry->sect == -1){
    return define_symbol(entry, curr_sect_ptr, location_ctr, entry->scope, DEFINED);
  }
  return 0;
}

/*
I instr desc
4 bita OC - 4 bita MOD
II regs desc
4 bita Rd - 4 bita Rs
...
*/

char instr_oc[INSTRNUM] = {
  [HALT] = 0x00,
  [INT] = 0x10,
  [IRET] = 0x20,
  [CALL] = 0x30,
  [RET] = 0x40,
  [JMP] = 0x50, // + MOD 0x00
  [JEQ] = 0x51, // + MOD 0x01
  [JNE] = 0x52, // + MOD 0x02
  [JGT] = 0x53, // + MOD 0x03
  [XCHG] = 0x60,
  [ADD] = 0x70, // + MOD 0x00
  [SUB] = 0x71, // + MOD 0x01
  [MUL] = 0x72, // + MOD 0x02
  [DIV] = 0x73, // + MOD 0x03
  [CMP] = 0x74, // + MOD 0x04
  [NOT] = 0x80, // + MOD 0x00
  [AND] = 0x81, // + MOD 0x01
  [OR] = 0x82, // + MOD 0x02
  [XOR] = 0x83, // + MOD 0x03
  [TEST] = 0x84, // + MOD 0x04
  [SHL] = 0x90, // + MOD 0x00
  [SHR] = 0x91, // + MOD 0x01
  [LDR] = 0xa0,
  [STR] = 0xb0
};

/*
III addr mode
4 bita UP - 4 bita AM
IV data high
V data low
0x00 - immed
0x01 - regdir
0x02 - regindir
0x03 - regindirpom 16 bit offs
0x04 - mem
0x05 - regdirpom 16 bit offs
*/

int handle_instruction(struct instruction* instr, struct label* lab){
  if(lab)
   if(handle_label(lab) != 0) return -1;
  Sec_tab_entry* sec = &sec_tab[curr_sect_ptr];
  switch(instr->type){
    case HALT:{
      sec->data[location_ctr++] = instr_oc[HALT];
      sec->size += 1;
      break;
    }
    case INT:{
      sec->data[location_ctr++] = instr_oc[INT];
      sec->data[location_ctr++] = ((char)(instr->op1.reg) << 4) | 0xf;
      sec->size += 2;
      break;
    }
    case IRET:{
      sec->data[location_ctr++] = instr_oc[IRET];
      sec->size += 1;
      break;
    }
    case CALL:{
      sec->data[location_ctr++] = instr_oc[CALL];
      struct addr_operand* op = instr->op1.addr_op;
      switch(op->type){
        case IMMED_LIT:{
          sec->data[location_ctr++] = 0xff; //reg
          sec->data[location_ctr++] = 0x00; // am
          short lit = (short)op->operand.lit;
          char hi = (char)(lit >> 8);
          char low = (char)lit;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case IMMED_SYM:{
          sec->data[location_ctr++] = 0xff; //reg
          sec->data[location_ctr++] = 0x00; // am
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case PCREL:{
          sec->data[location_ctr++] = 0xf7; //pc
          sec->data[location_ctr++] = 0x05; // regdir + offs
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){ //simbol postoji
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short offs = entry->offs_val - location_ctr - 2;
              char hi = (char)(offs >> 8);
              char low = (char)offs;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size += 5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){ // def simbol
              if(entry->sect == curr_sect_ptr){
                short offs = entry->offs_val - location_ctr - 2;
                char hi = (char)(offs >> 8);
                char low = (char)offs;
                sec->data[location_ctr++] = hi;
                sec->data[location_ctr++] = low;
                sec->size += 5;
                break;
              }
              else{
                switch(entry->scope){
                  case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val-2) != 0) return -1; break;
                  case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, entry->sym_tab_entry, -2) != 0) return -1; break;
                  default: printf("error: bad symbol scope\n"); return -1;
                }
                location_ctr += 2;
                sec->size += 5;
                break;
              }
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //ext
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, entry->sym_tab_entry, -2) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){ //nedefinisan simbol
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_PC16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{ //simbol ne postoji
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size -1], location_ctr, curr_sect_ptr, I_ARG_PC16) !=  0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case MEMDIR_LIT:{
          sec->data[location_ctr++] = 0xff;
          sec->data[location_ctr++] = 0x04;
          short lit = (short)op->operand.lit;
          char hi = (char)(lit >> 8);
          char low = (char)lit;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case MEMDIR_SYM:{
          sec->data[location_ctr++] = 0xff;
          sec->data[location_ctr++] = 0x04;
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case REGDIR:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x01;
          sec->size += 3;
          break;
        }
        case REGINDIR:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x02;
          sec->size += 3;
          break;
        }
        case REGINDIRPOM_LIT:{
          char reg = (char)op->operand.reg; // odabrani reg
          short offs = (short)op->offs.lit; // offs
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x03;
          char hi = (char)(offs >> 8);
          char low = (char)offs;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case REGINDIRPOM_SYM:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x03;
          char* sym = op->offs.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
      }
      break;
    }
    case RET:{
      sec->data[location_ctr++] = instr_oc[RET];
      sec->size += 1;
      break;
    }
    case JMP:{
      sec->data[location_ctr++] = instr_oc[JMP];
      struct addr_operand* op = instr->op1.addr_op;
      switch(op->type){
        case IMMED_LIT:{
          sec->data[location_ctr++] = 0xff; //reg
          sec->data[location_ctr++] = 0x00; // am
          short lit = (short)op->operand.lit;
          char hi = (char)(lit >> 8);
          char low = (char)lit;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case IMMED_SYM:{
          sec->data[location_ctr++] = 0xff; //reg
          sec->data[location_ctr++] = 0x00; // am
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case PCREL:{
          sec->data[location_ctr++] = 0xf7; //pc
          sec->data[location_ctr++] = 0x05; // regdir + offs
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){ //simbol postoji
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short offs = entry->offs_val - location_ctr - 2;
              char hi = (char)(offs >> 8);
              char low = (char)offs;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size += 5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){ // def simbol
              if(entry->sect == curr_sect_ptr){
                short offs = entry->offs_val - location_ctr - 2;
                char hi = (char)(offs >> 8);
                char low = (char)offs;
                sec->data[location_ctr++] = hi;
                sec->data[location_ctr++] = low;
                sec->size += 5;
                break;
              }
              else{
                switch(entry->scope){
                  case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val-2) != 0) return -1; break;
                  case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, entry->sym_tab_entry, -2) != 0) return -1; break;
                  default: printf("error: bad symbol scope\n"); return -1;
                }
                location_ctr += 2;
                sec->size += 5;
                break;
              }
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //ext
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, entry->sym_tab_entry, -2) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){ //nedefinisan simbol
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_PC16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{ //simbol ne postoji
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size -1], location_ctr, curr_sect_ptr, I_ARG_PC16) !=  0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case MEMDIR_LIT:{
          sec->data[location_ctr++] = 0xff;
          sec->data[location_ctr++] = 0x04;
          short lit = (short)op->operand.lit;
          char hi = (char)(lit >> 8);
          char low = (char)lit;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case MEMDIR_SYM:{
          sec->data[location_ctr++] = 0xff;
          sec->data[location_ctr++] = 0x04;
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case REGDIR:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x01;
          sec->size += 3;
          break;
        }
        case REGINDIR:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x02;
          sec->size += 3;
          break;
        }
        case REGINDIRPOM_LIT:{
          char reg = (char)op->operand.reg; // odabrani reg
          short offs = (short)op->offs.lit; // offs
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x03;
          char hi = (char)(offs >> 8);
          char low = (char)offs;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case REGINDIRPOM_SYM:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x03;
          char* sym = op->offs.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
      }
      break;
    }
    case JEQ:{
      sec->data[location_ctr++] = instr_oc[JEQ];
      struct addr_operand* op = instr->op1.addr_op;
      switch(op->type){
        case IMMED_LIT:{
          sec->data[location_ctr++] = 0xff; //reg
          sec->data[location_ctr++] = 0x00; // am
          short lit = (short)op->operand.lit;
          char hi = (char)(lit >> 8);
          char low = (char)lit;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case IMMED_SYM:{
          sec->data[location_ctr++] = 0xff; //reg
          sec->data[location_ctr++] = 0x00; // am
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case PCREL:{
          sec->data[location_ctr++] = 0xf7; //pc
          sec->data[location_ctr++] = 0x05; // regdir + offs
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){ //simbol postoji
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short offs = entry->offs_val - location_ctr - 2;
              char hi = (char)(offs >> 8);
              char low = (char)offs;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size += 5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){ // def simbol
              if(entry->sect == curr_sect_ptr){
                short offs = entry->offs_val - location_ctr - 2;
                char hi = (char)(offs >> 8);
                char low = (char)offs;
                sec->data[location_ctr++] = hi;
                sec->data[location_ctr++] = low;
                sec->size += 5;
                break;
              }
              else{
                switch(entry->scope){
                  case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val-2) != 0) return -1; break;
                  case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, entry->sym_tab_entry, -2) != 0) return -1; break;
                  default: printf("error: bad symbol scope\n"); return -1;
                }
                location_ctr += 2;
                sec->size += 5;
                break;
              }
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //ext
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, entry->sym_tab_entry, -2) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){ //nedefinisan simbol
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_PC16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{ //simbol ne postoji
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size -1], location_ctr, curr_sect_ptr, I_ARG_PC16) !=  0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case MEMDIR_LIT:{
          sec->data[location_ctr++] = 0xff;
          sec->data[location_ctr++] = 0x04;
          short lit = (short)op->operand.lit;
          char hi = (char)(lit >> 8);
          char low = (char)lit;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case MEMDIR_SYM:{
          sec->data[location_ctr++] = 0xff;
          sec->data[location_ctr++] = 0x04;
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case REGDIR:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x01;
          sec->size += 3;
          break;
        }
        case REGINDIR:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x02;
          sec->size += 3;
          break;
        }
        case REGINDIRPOM_LIT:{
          char reg = (char)op->operand.reg; // odabrani reg
          short offs = (short)op->offs.lit; // offs
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x03;
          char hi = (char)(offs >> 8);
          char low = (char)offs;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case REGINDIRPOM_SYM:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x03;
          char* sym = op->offs.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
      }
      break;
    }
    case JNE:{
      sec->data[location_ctr++] = instr_oc[JNE];
      struct addr_operand* op = instr->op1.addr_op;
      switch(op->type){
        case IMMED_LIT:{
          sec->data[location_ctr++] = 0xff; //reg
          sec->data[location_ctr++] = 0x00; // am
          short lit = (short)op->operand.lit;
          char hi = (char)(lit >> 8);
          char low = (char)lit;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case IMMED_SYM:{
          sec->data[location_ctr++] = 0xff; //reg
          sec->data[location_ctr++] = 0x00; // am
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case PCREL:{
          sec->data[location_ctr++] = 0xf7; //pc
          sec->data[location_ctr++] = 0x05; // regdir + offs
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){ //simbol postoji
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short offs = entry->offs_val - location_ctr - 2;
              char hi = (char)(offs >> 8);
              char low = (char)offs;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size += 5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){ // def simbol
              if(entry->sect == curr_sect_ptr){
                short offs = entry->offs_val - location_ctr - 2;
                char hi = (char)(offs >> 8);
                char low = (char)offs;
                sec->data[location_ctr++] = hi;
                sec->data[location_ctr++] = low;
                sec->size += 5;
                break;
              }
              else{
                switch(entry->scope){
                  case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val-2) != 0) return -1; break;
                  case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, entry->sym_tab_entry, -2) != 0) return -1; break;
                  default: printf("error: bad symbol scope\n"); return -1;
                }
                location_ctr += 2;
                sec->size += 5;
                break;
              }
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //ext
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, entry->sym_tab_entry, -2) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){ //nedefinisan simbol
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_PC16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{ //simbol ne postoji
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size -1], location_ctr, curr_sect_ptr, I_ARG_PC16) !=  0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case MEMDIR_LIT:{
          sec->data[location_ctr++] = 0xff;
          sec->data[location_ctr++] = 0x04;
          short lit = (short)op->operand.lit;
          char hi = (char)(lit >> 8);
          char low = (char)lit;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case MEMDIR_SYM:{
          sec->data[location_ctr++] = 0xff;
          sec->data[location_ctr++] = 0x04;
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case REGDIR:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x01;
          sec->size += 3;
          break;
        }
        case REGINDIR:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x02;
          sec->size += 3;
          break;
        }
        case REGINDIRPOM_LIT:{
          char reg = (char)op->operand.reg; // odabrani reg
          short offs = (short)op->offs.lit; // offs
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x03;
          char hi = (char)(offs >> 8);
          char low = (char)offs;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case REGINDIRPOM_SYM:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x03;
          char* sym = op->offs.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
      }
      break;
    }
    case JGT: {
      sec->data[location_ctr++] = instr_oc[JGT];
      struct addr_operand* op = instr->op1.addr_op;
      switch(op->type){
        case IMMED_LIT:{
          sec->data[location_ctr++] = 0xff; //reg
          sec->data[location_ctr++] = 0x00; // am
          short lit = (short)op->operand.lit;
          char hi = (char)(lit >> 8);
          char low = (char)lit;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case IMMED_SYM:{
          sec->data[location_ctr++] = 0xff; //reg
          sec->data[location_ctr++] = 0x00; // am
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case PCREL:{
          sec->data[location_ctr++] = 0xf7; //pc
          sec->data[location_ctr++] = 0x05; // regdir + offs
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){ //simbol postoji
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short offs = entry->offs_val - location_ctr - 2;
              char hi = (char)(offs >> 8);
              char low = (char)offs;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size += 5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){ // def simbol
              if(entry->sect == curr_sect_ptr){
                short offs = entry->offs_val - location_ctr - 2;
                char hi = (char)(offs >> 8);
                char low = (char)offs;
                sec->data[location_ctr++] = hi;
                sec->data[location_ctr++] = low;
                sec->size += 5;
                break;
              }
              else{
                switch(entry->scope){
                  case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val-2) != 0) return -1; break;
                  case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, entry->sym_tab_entry, -2) != 0) return -1; break;
                  default: printf("error: bad symbol scope\n"); return -1;
                }
                location_ctr += 2;
                sec->size += 5;
                break;
              }
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //ext
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, entry->sym_tab_entry, -2) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){ //nedefinisan simbol
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_PC16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{ //simbol ne postoji
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size -1], location_ctr, curr_sect_ptr, I_ARG_PC16) !=  0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case MEMDIR_LIT:{
          sec->data[location_ctr++] = 0xff;
          sec->data[location_ctr++] = 0x04;
          short lit = (short)op->operand.lit;
          char hi = (char)(lit >> 8);
          char low = (char)lit;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case MEMDIR_SYM:{
          sec->data[location_ctr++] = 0xff;
          sec->data[location_ctr++] = 0x04;
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case REGDIR:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x01;
          sec->size += 3;
          break;
        }
        case REGINDIR:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x02;
          sec->size += 3;
          break;
        }
        case REGINDIRPOM_LIT:{
          char reg = (char)op->operand.reg; // odabrani reg
          short offs = (short)op->offs.lit; // offs
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x03;
          char hi = (char)(offs >> 8);
          char low = (char)offs;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case REGINDIRPOM_SYM:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)(0xf0 | reg);
          sec->data[location_ctr++] = 0x03;
          char* sym = op->offs.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
      }
      break;
    }
    case PUSH:{
      sec->data[location_ctr++] = instr_oc[STR];
      sec->data[location_ctr++] = (char)(((char)instr->op1.reg << 4) | 0x06); //r6 sp
      sec->data[location_ctr++] = 0x12; // regind sp <= sp - 2; mem16[sp] <= regD; 
      sec->size += 3;
      break;
    }
    case POP:{
      sec->data[location_ctr++] = instr_oc[LDR];
      sec->data[location_ctr++] = (char)(((char)instr->op1.reg << 4) | 0x06); //r6 sp
      sec->data[location_ctr++] = 0x42; //regind regD <= mem16[sp]; sp <= sp + 2;
      sec->size += 3;
      break;
    }
    case XCHG:{
      sec->data[location_ctr++] = instr_oc[XCHG];
      sec->data[location_ctr++] = ((char)instr->op1.reg << 4) | (char)instr->op2.reg;
      sec->size += 2;
      break;
    }
    case ADD:{
      sec->data[location_ctr++] = instr_oc[ADD];
      sec->data[location_ctr++] = ((char)instr->op1.reg << 4) | (char)instr->op2.reg;
      sec->size += 2;
      break;
    }
    case SUB:{
      sec->data[location_ctr++] = instr_oc[SUB];
      sec->data[location_ctr++] = ((char)instr->op1.reg << 4) | (char)instr->op2.reg;
      sec->size += 2;
      break;
    }
    case MUL:{
      sec->data[location_ctr++] = instr_oc[MUL];
      sec->data[location_ctr++] = ((char)instr->op1.reg << 4) | (char)instr->op2.reg;
      sec->size += 2;
      break;
    }
    case DIV:{
      sec->data[location_ctr++] = instr_oc[DIV];
      sec->data[location_ctr++] = ((char)instr->op1.reg << 4) | (char)instr->op2.reg;
      sec->size += 2;
      break;
    }
    case CMP:{
      sec->data[location_ctr++] = instr_oc[CMP];
      sec->data[location_ctr++] = ((char)instr->op1.reg << 4) | (char)instr->op2.reg;
      sec->size += 2;
      break;
    }
    case NOT: {
      sec->data[location_ctr++] = instr_oc[NOT];
      sec->data[location_ctr++] = ((char)instr->op1.reg << 4) | 0xf;
      sec->size += 2;
      break;
    }
    case AND:{
      sec->data[location_ctr++] = instr_oc[AND];
      sec->data[location_ctr++] = ((char)instr->op1.reg << 4) | (char)instr->op2.reg;
      sec->size += 2;
      break;
    }
    case OR:{
      sec->data[location_ctr++] = instr_oc[OR];
      sec->data[location_ctr++] = ((char)instr->op1.reg << 4) | (char)instr->op2.reg;
      sec->size += 2;
      break;
    }
    case XOR:{
      sec->data[location_ctr++] = instr_oc[XOR];
      sec->data[location_ctr++] = ((char)instr->op1.reg << 4) | (char)instr->op2.reg;
      sec->size += 2;
      break;
    }
    case TEST:{
      sec->data[location_ctr++] = instr_oc[TEST];
      sec->data[location_ctr++] = ((char)instr->op1.reg << 4) | (char)instr->op2.reg;
      sec->size += 2;
      break;
    }
    case SHL:{
      sec->data[location_ctr++] = instr_oc[SHL];
      sec->data[location_ctr++] = ((char)instr->op1.reg << 4) | (char)instr->op2.reg;
      sec->size += 2;
      break;
    }
    case SHR:{
      sec->data[location_ctr++] = instr_oc[SHR];
      sec->data[location_ctr++] = ((char)instr->op1.reg << 4) | (char)instr->op2.reg;
      sec->size += 2;
      break;
    }
    case LDR:{
      sec->data[location_ctr++] = instr_oc[LDR];
      struct data_operand* op = instr->op2.data_op;
      char Rd = instr->op1.reg;
      switch(op->type){
        case IMMED_LIT:{
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | 0xf); //reg
          sec->data[location_ctr++] = 0x00; // am
          short lit = (short)op->operand.lit;
          char hi = (char)(lit >> 8);
          char low = (char)lit;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case IMMED_SYM:{
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | 0xf); //reg
          sec->data[location_ctr++] = 0x00; // am
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case PCREL:{
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | 0x7); //pc
          sec->data[location_ctr++] = 0x03; // regindir + offs
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){ //simbol postoji
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short offs = entry->offs_val - location_ctr - 2;
              char hi = (char)(offs >> 8);
              char low = (char)offs;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size += 5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){ // def simbol
              if(entry->sect == curr_sect_ptr){
                short offs = entry->offs_val - location_ctr - 2;
                char hi = (char)(offs >> 8);
                char low = (char)offs;
                sec->data[location_ctr++] = hi;
                sec->data[location_ctr++] = low;
                sec->size += 5;
                break;
              }
              else{
                switch(entry->scope){
                  case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val-2) != 0) return -1; break;
                  case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, entry->sym_tab_entry, -2) != 0) return -1; break;
                  default: printf("error: bad symbol scope\n"); return -1;
                }
                location_ctr += 2;
                sec->size += 5;
                break;
              }
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //ext
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, entry->sym_tab_entry, -2) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){ //nedefinisan simbol
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_PC16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{ //simbol ne postoji
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size -1], location_ctr, curr_sect_ptr, I_ARG_PC16) !=  0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case MEMDIR_LIT:{
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | 0xf);
          sec->data[location_ctr++] = 0x04;
          short lit = (short)op->operand.lit;
          char hi = (char)(lit >> 8);
          char low = (char)lit;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case MEMDIR_SYM:{
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | 0xf);
          sec->data[location_ctr++] = 0x04;
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case REGDIR:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | reg);
          sec->data[location_ctr++] = 0x01;
          sec->size += 3;
          break;
        }
        case REGINDIR:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | reg);
          sec->data[location_ctr++] = 0x02;
          sec->size += 3;
          break;
        }
        case REGINDIRPOM_LIT:{
          char reg = (char)op->operand.reg; // odabrani reg
          short offs = (short)op->offs.lit; // offs
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | reg);
          sec->data[location_ctr++] = 0x03;
          char hi = (char)(offs >> 8);
          char low = (char)offs;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case REGINDIRPOM_SYM:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | reg);
          sec->data[location_ctr++] = 0x03;
          char* sym = op->offs.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
      }
      break;
    }
    case STR:{
      sec->data[location_ctr++] = instr_oc[STR];
      struct data_operand* op = instr->op2.data_op;
      char Rd = instr->op1.reg;
      switch(op->type){
        case IMMED_LIT:{
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | 0xf); //reg
          sec->data[location_ctr++] = 0x00; // am
          short lit = (short)op->operand.lit;
          char hi = (char)(lit >> 8);
          char low = (char)lit;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case IMMED_SYM:{
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | 0xf); //reg
          sec->data[location_ctr++] = 0x00; // am
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case PCREL:{
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | 0x7); //pc
          sec->data[location_ctr++] = 0x03; // regindir + offs
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){ //simbol postoji
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short offs = entry->offs_val - location_ctr - 2;
              char hi = (char)(offs >> 8);
              char low = (char)offs;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size += 5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){ // def simbol
              if(entry->sect == curr_sect_ptr){
                short offs = entry->offs_val - location_ctr - 2;
                char hi = (char)(offs >> 8);
                char low = (char)offs;
                sec->data[location_ctr++] = hi;
                sec->data[location_ctr++] = low;
                sec->size += 5;
                break;
              }
              else{
                switch(entry->scope){
                  case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val-2) != 0) return -1; break;
                  case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, entry->sym_tab_entry, -2) != 0) return -1; break;
                  default: printf("error: bad symbol scope\n"); return -1;
                }
                location_ctr += 2;
                sec->size += 5;
                break;
              }
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //ext
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, PC16, entry->sym_tab_entry, -2) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){ //nedefinisan simbol
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_PC16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{ //simbol ne postoji
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size -1], location_ctr, curr_sect_ptr, I_ARG_PC16) !=  0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case MEMDIR_LIT:{
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | 0xf);
          sec->data[location_ctr++] = 0x04;
          short lit = (short)op->operand.lit;
          char hi = (char)(lit >> 8);
          char low = (char)lit;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case MEMDIR_SYM:{
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | 0xf);
          sec->data[location_ctr++] = 0x04;
          char* sym = op->operand.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
        case REGDIR:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | reg);
          sec->data[location_ctr++] = 0x01;
          sec->size += 3;
          break;
        }
        case REGINDIR:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | reg);
          sec->data[location_ctr++] = 0x02;
          sec->size += 3;
          break;
        }
        case REGINDIRPOM_LIT:{
          char reg = (char)op->operand.reg; // odabrani reg
          short offs = (short)op->offs.lit; // offs
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | reg);
          sec->data[location_ctr++] = 0x03;
          char hi = (char)(offs >> 8);
          char low = (char)offs;
          sec->data[location_ctr++] = hi;
          sec->data[location_ctr++] = low;
          sec->size += 5;
          break;
        }
        case REGINDIRPOM_SYM:{
          char reg = (char)op->operand.reg; // odabrani reg
          sec->data[location_ctr++] = (char)((char)(Rd << 4) | reg);
          sec->data[location_ctr++] = 0x03;
          char* sym = op->offs.sym;
          int ptr = sym_exists(sym);
          if(ptr != -1){
            Sym_tab_entry* entry = &sym_tab[ptr];
            if(entry->defined == DEFINED && entry->sect == 0){ // equ
              short val = (short)entry->offs_val;
              char hi = (char)(val >> 8);
              char low = (char)val;
              sec->data[location_ctr++] = hi;
              sec->data[location_ctr++] = low;
              sec->size+=5;
              break;
            }
            else if(entry->defined == DEFINED && entry->sect > 0){
              switch(entry->scope){
                case L: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                case G: if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1; break;
                default: printf("error: bad symbol scope\n"); return -1;
              }
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == 0){ //extern
              if(add_new_reloc(sec, curr_sect_ptr, location_ctr, APS16, entry->sym_tab_entry, 0) != 0) return -1;
              location_ctr += 2;
              sec->size+=5;
              break;
            }
            else if(entry->defined == UNDEFINED && entry->sect == -1){
              if(add_new_flink(entry, location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
              location_ctr += 2;
              sec->size += 5;
              break;
            }
            else{
              printf("error: illegal symbol state\n");
              return -1;
            }
          }
          else{
            if(add_new_sym(sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
            if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, I_ARG_APS16) != 0) return -1;
            location_ctr += 2;
            sec->size += 5;
            break;
          }
        }
      }
      break;
    }
    default:  printf("fatal error: method for this instruction does not exist\n"); return -1;
  }
  return 0;
}

int handle_directive(struct directive* dir, struct label* lab){
  if(lab) 
    if(handle_label(lab) != 0) return -1;
  switch(dir->type){
    case GLOBAL: {
      struct symbol_list_node* head = dir->param.sym_list;
      while(head){
        int ptr = sym_exists(head->sym);
        if(ptr != -1){
          Sym_tab_entry* entry = &sym_tab[ptr];
          enum scope oldscope = entry->scope;
          entry->scope = G;
          update_reloc(entry, oldscope);
        }
        else{
          if(add_new_sym(head->sym, -1, -1, NOTYPE, G, UNDEFINED) != 0) return -1;
        }
        head = head->next;
      }
      return 0;
    }
    case EXTERN: {
      struct symbol_list_node* head = dir->param.sym_list;
      while(head){
        int ptr = sym_exists(head->sym);
        if(ptr != -1){
          Sym_tab_entry* entry = &sym_tab[ptr];
          if(entry->defined == DEFINED || (entry->defined == UNDEFINED && entry->sect == 0)){
            printf("error: symbol: %s already exists\n", head->sym);
            return -1;
          }
          else if(entry->defined == UNDEFINED && entry->sect == -1){
            if(define_symbol(entry, 0, 0, G, UNDEFINED) != 0) return -1;
          }
        }
        else{
          if(add_new_sym(head->sym, 0, 0, NOTYPE, G, UNDEFINED) != 0) return -1;
        }
        head = head->next;
      }
      return 0;
    }
    case SECTION: {
      char* section_name = dir->param.sym;
      int ptr = sym_exists(section_name);
      if(ptr != -1){
        Sym_tab_entry* entry = &sym_tab[ptr];
        if(entry->defined == DEFINED || (entry->defined == UNDEFINED && entry->sect == 0)){
          printf("error: symbol or section: %s already exists\n", section_name);
          return -1;
        }
        else{
          curr_sect_ptr = sec_tab_size;
          //int curr_sym_ptr = sym_tab_size;
          location_ctr = 0;
          if(add_new_sec(0, ptr) != 0) return -1;
          if(define_symbol(entry, curr_sect_ptr, 0, L, DEFINED) != 0) return -1;
          entry->type = SECT;
          return 0;
        }
      }
      else{
        curr_sect_ptr = sec_tab_size;
        //int curr_sym_ptr = sym_tab_size;
        location_ctr = 0;
        if(add_new_sec(0, sym_tab_size) != 0) return -1;
        if(add_new_sym(section_name, 0, curr_sect_ptr, SECT, L, DEFINED) != 0) return -1;
        return 0;
      }
    }
    case WORD: {
      struct mix_list_node* head = dir->param.mix_list;
      Sec_tab_entry* curr_sec = &sec_tab[curr_sect_ptr];
      while(head){
        switch(head->type){
          case MLN_LIT: {
            if(curr_sec->size + 2 > MAXSECTIONSIZE){
              printf("error: unable to write %#x to memory - max section data size reached\n", head->data.lit);
              return -1;
            }
            short data = (short)head->data.lit;
            char low = (char)data;
            char hi = (char)(data >> 8);
            //little-endian
            curr_sec->data[location_ctr++] = low;
            curr_sec->data[location_ctr++] = hi;
            curr_sec->size+=2;
            break;
          }
          case MLN_SYM: {
            if(curr_sec->size + 2 > MAXSECTIONSIZE){
              printf("error: unable to write %#x to memory - max section data size reached\n", head->data.lit);
              return -1;
            }
            int ptr = sym_exists(head->data.sym);
            if(ptr != -1){
              Sym_tab_entry* entry = &sym_tab[ptr];
              /*if(entry->defined == DEFINED || (entry->defined == UNDEFINED && entry->sect == 0)){
                printf("error: symbol: %s already exists\n", entry->sym);
                return -1;
              }
              if(entry->defined == UNDEFINED && entry->sect == -1){
                define_symbol(entry, curr_sect_ptr, location_ctr);
                curr_sec->data[location_ctr++] = 0;
                curr_sec->data[location_ctr++] = 0;
                curr_sec->size+=2;
                break;
              }
              // UNDEF sec > 0 - lokalni jos nedefinisan
              // todo
              */
             if(entry->defined == DEFINED && entry->sect == 0 ){ // equ
                short val = (short)entry->offs_val;
                char hi = (char)(val >> 8);
                char low = (char)val;
                curr_sec->data[location_ctr++] = low;
                curr_sec->data[location_ctr++] = hi;
                curr_sec->size+=2;
                break;
              }
              else if(entry->defined == DEFINED && entry->sect > 0){ 
                switch(entry->scope){
                  case L: if(add_new_reloc(curr_sec, curr_sect_ptr, location_ctr, APS16D, sec_tab[entry->sect].sym_tab_entry, entry->offs_val) != 0) return -1; break;
                  case G: if(add_new_reloc(curr_sec, curr_sect_ptr, location_ctr, APS16D, entry->sym_tab_entry, 0) != 0) return -1; break;
                  default: printf("error: bad symbol scope\n"); return -1;
                } 
                location_ctr += 2;
                curr_sec->size += 2;
                break;
              }
              else if(entry->defined == UNDEFINED && entry->sect == 0){ // glob
                if(add_new_reloc(curr_sec, curr_sect_ptr, location_ctr, APS16D, entry->sym_tab_entry, 0) != 0) return -1;
                location_ctr += 2;
                curr_sec->size+=2;
              }
              else if(entry->defined == UNDEFINED && entry->sect == -1){ // lokalni jos nedefinisan
                if(add_new_flink(entry, location_ctr, curr_sect_ptr, WDIR) != 0) return -1;
                location_ctr+=2;
                curr_sec->size+=2;
                break;
              }
              else{
                printf("error: illegal symbol state\n");
                return -1;
              }
            }
            else{
              if(add_new_sym(head->data.sym, -1, -1, NOTYPE, L, UNDEFINED) != 0) return -1;
              if(add_new_flink(&sym_tab[sym_tab_size - 1], location_ctr, curr_sect_ptr, WDIR) != 0) return -1;
              location_ctr+=2;
              curr_sec->size+=2;
              break;
            }
          }
        }
        head = head->next;
      }
      return 0;
    }
    case SKIP: {
      unsigned int val = (unsigned int)dir->param.lit;
      Sec_tab_entry* curr_sec = &sec_tab[curr_sect_ptr];
      for(int i = 0; i<val; i++) curr_sec->data[location_ctr++] = 0;
      curr_sec->size += val;
      return 0;
    }
    case ASCII: {
      char* str = dir->param.string;
      Sec_tab_entry* curr_sect = &sec_tab[curr_sect_ptr];
      int len = strlen(str);
      for(int i = 1; i<len-1; i++) curr_sect->data[location_ctr++] = str[i];
      //null byte
      curr_sect->data[location_ctr++] = '\0';
      curr_sect->size += len - 1;
      return 0;
    }
    case EQU: {
      char* sym = dir->param.sym;
      int val = dir->exprval;
      int ptr = sym_exists(sym);
      if(ptr != -1){
        Sym_tab_entry* entry = &sym_tab[ptr];
        if(entry->defined == DEFINED || (entry->defined == UNDEFINED && entry->sect == 0)){
          printf("error: symbol: %s already exists\n", entry->sym);
          return -1;
        }
        else if(entry->defined == UNDEFINED && entry->sect == -1){
          return define_symbol(entry, 0, val, L, DEFINED);
        }
      }
      else{
        return add_new_sym(sym, val, 0, NOTYPE, L, DEFINED);
      }
    }
    case END: {
      //printf("assembling stopped\n");
      assembling = 0;
      return 0;
    }
    default: printf("fatal error: method for this directive does not exist\n"); return -1;
  }
  return 0;
}

int process_parsed_line(struct line* parsed_line){
  switch(parsed_line->type){
    case COMM: return 0;
    case INSTR: return handle_instruction(parsed_line->content.instr, 0);
    case LAB_INSTR: return handle_instruction(parsed_line->content.instr, parsed_line->lab);
    case DIR: return handle_directive(parsed_line->content.dir, 0);
    case LAB_DIR: return handle_directive(parsed_line->content.dir, parsed_line->lab);
    case LAB: return handle_label(parsed_line->lab);
  }
}

int check_unresolved_symbols(){
  int unresolved_num = 0;
  int unresolved_ind_arr[MAXSYM];
  int unresolved_flg = 0;
  Sym_tab_entry* entry;
  for(int i = 0; i<sym_tab_size; i++){
    entry = &sym_tab[i];
    if(entry->defined == UNDEFINED && (entry->sect == -1 || entry->sect > 0)
        || entry->defined == DEFINED && entry->sect == -1){
      unresolved_flg = 1;
      unresolved_ind_arr[unresolved_num++] = i;
    }
  }
  if(unresolved_flg == 1){
    printf("error: unresolved symbols:\n");
    for(int i = 0; i<unresolved_num; i++){
      print_sym_tab_entry(unresolved_ind_arr[i]);
    }
    return -1;
  }
  return 0;
}

void output_sym_to_elf(Sym_tab_entry* entry, int ptr, FILE* out){
  //printf("entry: %d\n", ptr);
  if(entry->sym) fprintf(out, "%3d %10s\t", ptr, entry->sym);
  else fprintf(out, "%3d %10s\t", ptr, "null");
  if(entry->sect != -1)  fprintf(out, "%d\t", entry->sect);
  else  printf("%d\t", -1);
  if(entry->type == SECT) fprintf(out, "%3d\t", sec_tab[entry->sect].size);
  else fprintf(out, "%3d\t", 0);
  fprintf(out, "%08x\t", entry->offs_val);
  if(entry->type == SECT) fprintf(out, "%d\t", 1);
  else fprintf(out, "%d\t", 0);
  char c;
  if(entry->scope == L) c = 'l';
  else c = 'g';
  fprintf(out, "%c\t", c);
  if(entry->defined == DEFINED)  fprintf(out, "%d\n", 1);
  else  fprintf(out, "%d\n", 0);
}

void output_sec_relocs(Sec_tab_entry* entry, FILE* out){
  if(!entry) return;
  Reloc* head = entry->reloc;
  fprintf(out, ".rela%s ", sym_tab[entry->sym_tab_entry].sym);
  if(!entry->reloc) fprintf(out, "%d\n", 0);
  else{
    int ctr = 0;
    Reloc* tmp = entry->reloc;
    while(tmp){
      ctr++;
      tmp = tmp->next;
    }
    fprintf(out, "%d\n", ctr);
  }
  fprintf(out, "%s\t%6s\t%10s\t%6s\n", "offset", "type", "symbol", "addend");
  while(head){
    fprintf(out, "%06x\t", head->offs);
    switch(head->type){
      case APS16: fprintf(out, "%d\t", 1); break;
      case PC16: fprintf(out, "%d\t", 2); break;
      case APS16D: fprintf(out, "%d\t", 3); break;
      case UND: fprintf(out, "%d\t", 4); break;
    }
    fprintf(out, "%10s\t",sym_tab[head->sym_tab_entry].sym);
    fprintf(out, "%6d\n", head->addend);
    head = head->next;
  }
}

void output_sec_to_elf(Sec_tab_entry* entry, int ptr, FILE* out){
  if(entry->sym_tab_entry != -1) fprintf(out, "%s\t", sym_tab[entry->sym_tab_entry].sym);
  else fprintf(out, "%3d %s\t", ptr, "null");
  fprintf(out, "%d\n", entry->size);
  if(ptr != 0){
    int rows = (entry->size / 8) + 1;
    int ctr = 0;
    for(int i = 0; i < rows; i++){
      for(int j = 0; j<8; j++) fprintf(out, "%02x ", (unsigned char)entry->data[ctr++]);
      fprintf(out, "\n");
    }
    //printf("-------------\n");
  }
  //print_sec_relocs(entry);
  output_sec_relocs(entry, out);
}

void output_to_elf(FILE* out){
  //output sym tab
  fprintf(out, "#.symtab %d\n", sym_tab_size);
  fprintf(out, "%s %s\t%s\t%s\t%s\t%s\t%s\t%s\n", "num", "sym", "sect", "size", "offs", "type", "scope", "defined");
  Sym_tab_entry* entry;
  for(int i = 0; i<sym_tab_size; i++){
    entry = &sym_tab[i];
    if(entry->type == SECT) output_sym_to_elf(entry, i, out);
  }
  for(int i = 0; i<sym_tab_size; i++){
    entry = &sym_tab[i];
    if(entry->type == NOTYPE) output_sym_to_elf(entry, i, out);
  }
  //output sections
  fprintf(out, "#.sections %d\n", sec_tab_size-1);
  Sec_tab_entry* sec_entry;
  for(int i = 1; i<sec_tab_size; i++){
    sec_entry = &sec_tab[i];
    output_sec_to_elf(sec_entry, i, out);
  }
}

int assemble(FILE* in, FILE* out, char* infile){
  struct line* parsed_line;
  int line = 1;
  char buff[MAXLINE];
  int len;
  if(init_as() != 0){
    printf("fatal  error: assembler initialization failed\n");
    return -1;
  }
  assembling = 1;
  while((len = get_next_line(in, buff)) != -1 && assembling == 1){
    //printf("len: %d, line: %d - %s\n",len, line, buff);
    if(len == -2){
      printf("fatal error: reading from file: %s failed\n", infile);
      destroy_as();
      return -1;
    }
    line++;
    if(len == 0) continue;
    if(parse_line(&parsed_line, buff) != 0){
      //syntax error
      printf("on line %d: %s\n",line-1, buff);
      destroy_as();
      return -1;
    }
    if(process_parsed_line(parsed_line) != 0){
      printf("error: processing line failed\n");
      printf("%s\n", buff);
      destroy_as();
      return -1;
    }
    free_line(parsed_line);
  }
  if(check_unresolved_symbols() == -1){
    destroy_as();
    return -2;
  }
  output_to_elf(out);
  //print_sym_tab();
  //print_sec_tab();
  destroy_as();
  return 0;
}

int parser_test(FILE* in){
  struct line* line_data;
  int line = 1;
  char buff[MAXLINE];
  int len;
  while((len = get_next_line(in, buff)) != -1){
    if(len == -2){
      printf("reading from file failed!\n");
      exit(1);
    }
    if(len == 0) continue;
    printf("%d %s len: %d\n",line, buff, len);
    int e = parse_line(&line_data, buff);
    if(e != 0){
      printf("on line %d\n", line);
      return -1;
    }
    print_line_data(line_data);
   // fprintf(out, "%d %s\n", line, buff);
    line++;
  }
  printf("END OF INFILE!\n");
  return 0;
}