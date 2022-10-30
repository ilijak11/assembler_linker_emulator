#include "../../inc/linker.h"

Symbol** sym_tab;
int sym_tab_size = 1;
Section** sec_tab;
int sec_tab_size = 0;

int init_linker(){
  sym_tab = (Symbol**)malloc(MAXSYM*sizeof(Symbol*));
  if(!sym_tab){
    printf("fatal error: no memory for empty symbol table\n");
    return -1;
  }
  for(int i = 0; i<MAXSYM; i++){
    sym_tab[i] = 0;
  }

  // *ABS* sekcija
  sym_tab[0] = (Symbol*)malloc(sizeof(Symbol));
  if(!sym_tab[0]){
    printf("fatal error: no memory symbol table\n");
    free(sym_tab);
    return -1;
  }
  Symbol* abs = sym_tab[0];
  abs->sym = dupstr("*ABS*");
  abs->sect = 0;
  abs->size = 0;
  abs->offs_val = 0;
  abs->type = SECT;
  abs->scope = G;
  abs->defined = DEF;
  abs->file = 0;
  abs->file_no = 0;

  sec_tab = (Section**)malloc(MAXSEC*sizeof(Section*));
  for(int i = 0; i<MAXSEC; i++){
    sec_tab[i] = 0;
  }

  return 0;
}

Section* init_sec(char* name, int offs, int size, char* file, int file_no){
  Section* new_sec = (Section*)malloc(sizeof(Section));
  if(!new_sec) return 0;
  new_sec->name = dupstr(name);
  new_sec->offs = offs;
  new_sec->data = (char*)calloc(MAXSECSIZE, sizeof(char));
  if(!new_sec->data){
    free(new_sec);
    return 0;
  }
  new_sec->size = size;
  new_sec->file = dupstr(file);
  new_sec->file_no = file_no;
  new_sec->mapped = 0;
  new_sec->outputted = 0;
  new_sec->relocs = 0;
  return new_sec;
}

void destroy_linker(){
  for(int i = 0; i<sym_tab_size; i++){
    if(sym_tab[i]){
      Symbol* entry = sym_tab[i];
      if(entry->sym) free(entry->sym);
      if(entry->file) free(entry->file);
      free(entry);
    }
  }
  free(sym_tab);

  for(int i = 0; i<sec_tab_size; i++){
    if(sec_tab[i]){
      Section* entry = sec_tab[i];
      if(entry->name) free(entry->name);
      if(entry->data) free(entry->data);
      if(entry->file) free(entry->file);

      if(entry->relocs){
        Reloc* head = entry->relocs;
        Reloc* old = 0;
        while(head){
          old = head;
          head = head->next;
          if(old->sym) free(old->sym);
          free(old);
        }
      }
      free(entry);
    }
  }
  free(sec_tab);
}

void print_sym_tab_entry(Symbol* entry){
  if(entry->sym) printf("%20s\t", entry->sym);
  else printf("%20s\t", "null");
  printf("%2d\t", entry->sect);
  printf("%3d\t", entry->size);
  printf("0x%08x\t", entry->offs_val);
  if(entry->type == SECT) printf("%6s\t", "SECT");
  else printf("%6s\t", "SYM");
  char c;
  if(entry->scope == L) c = 'l';
  //else if(entry->scope == E) c = 'e';
  else c = 'g';
  printf("%5c\t", c);
  if(entry->defined == DEF)  printf("%5s\t", "DEF");
  else  printf("%5s\t", "UNDEF");
  printf("file_no: %d\t", entry->file_no);
  if(entry->file) printf("%10s\n", entry->file);
  else printf("%10s\n", "linker");
}

void print_sym_table(){
  for(int i = 0; i<sym_tab_size; i++){
    if(sym_tab[i] && sym_tab[i]->type == SECT){
      print_sym_tab_entry(sym_tab[i]);
    }
  }
  for(int i = 0; i<sym_tab_size; i++){
    if(sym_tab[i] && sym_tab[i]->type == SYM){
      print_sym_tab_entry(sym_tab[i]);
    }
  }
  printf("\n\n");
}

void print_relocs(Section* entry){
  if(!entry || !entry->relocs) return;
  Reloc* head = entry->relocs;
  printf("reloc table:\n");
  printf("______________________________________________\n");
  printf("%8s\t%6s\t%10s\t%6s\n", "offset", "type", "symbol", "addend");
  printf("----------------------------------------------\n");
  while(head){
    printf("%06x\t", head->offs);
    switch(head->type){
      case APS16: printf("%6s\t", "APS16"); break;
      case PC16: printf("%6s\t", "PC16"); break;
      case APS16D: printf("%6s\t", "APS16D"); break;
      case UND: printf("%6s\t", "UND"); break;
    }
    printf("%10s\t",head->sym);
    printf("%6d\t", head->addend);
    printf("resolved: %d\n", head->resolved);
    head = head->next;
  }
  printf("----------------------------------------------\n");
  printf("\n\n");
}

void print_sec_table_entry(Section* entry){
  if(entry->name) printf("%20s\t", entry->name);
  else printf("%20s\t", "null");
  printf("0x%08x\t", entry->offs);
  printf("%4d\t", entry->size);
  printf("file_no: %d\t", entry->file_no);
  printf("mapped: %d\t", entry->mapped);
  printf("outputted: %d\t", entry->outputted);
  if(entry->file) printf("%10s\n", entry->file);
  else printf("%10s\n", "null");
  printf("memory content:\n");
  int rows = (entry->size / 8) + 1;
  int ctr = 0;
  for(int i = 0; i < rows; i++){
    printf("%04x\t", ctr);
    for(int j = 0; j<8; j++) printf("%02x ", (unsigned char)entry->data[ctr++]);
    printf("\n");
  }
  print_relocs(entry);
  //printf("-------------\n");
}

void print_sec_table(){
  for(int i = 0; i<sec_tab_size; i++){
    if(sec_tab[i]){
      print_sec_table_entry(sec_tab[i]);
    }
  }
}

Symbol* init_symbol(char* sym, int sect, int size, int offs, int type, char scope, int def, char* file_name, int file_no){
  Symbol* new_sym = (Symbol*)malloc(sizeof(Symbol));
  if(!new_sym){
    return 0;
  }
  new_sym->sym = dupstr(sym);
  new_sym->sect = sect;
  new_sym->size = size;
  new_sym->offs_val = offs;
  if(type == 1) new_sym->type = SECT;
  else new_sym->type = SYM;
  if(scope == 'l') new_sym->scope = L;
  else new_sym->scope = G;
  if(def == 1) new_sym->defined = DEF;
  else new_sym->defined = UNDEF;
  new_sym->file = strdup(file_name);
  new_sym->file_no = file_no;

  return new_sym;
}

int add_new_reloc(Section* sect, int offs, int type, char* sym, int addend){
  if(!sect) return -1;
  Reloc* new_reloc = (Reloc*)malloc(sizeof(Reloc));
  if(!new_reloc) return -1;
  new_reloc->offs = offs;
  switch (type){
    case 1:{
      new_reloc->type = APS16;
      break;
    }
    case 2:{
      new_reloc->type = PC16;
      break;  
    }
    case 3:{
      new_reloc->type = APS16D;
      break;
    }
    case 4:{
      new_reloc->type = UND;
      break;
    }
  }

  new_reloc->sym = dupstr(sym);
  new_reloc->addend = addend;
  new_reloc->next = 0;
  new_reloc->resolved = 0;

  if(!sect->relocs){
    sect->relocs = new_reloc;
  }
  else{
    Reloc* head = sect->relocs;
    while(head->next) head = head->next;
    head->next = new_reloc;
  }
  
}

int check_sec(Symbol* entry){
  int found = 0;
  for(int i = 0; i<sym_tab_size; i++){
    Symbol* existing = sym_tab[i];
    if(strcmp(entry->sym, existing->sym) == 0){
      if(existing->type == SYM){
        printf("fatal error: symbol: %s is defined as section in file: %s and as symbol in file: %s\n", existing->sym, entry->file, existing->file);
        return -1;
      }
    }
  }
  sym_tab[sym_tab_size++] = entry;
  return 0;
}

int check_sym(Symbol* entry){
  for(int i = 0; i<sym_tab_size; i++){
    Symbol* existing = sym_tab[i];
    if(strcmp(entry->sym, existing->sym) == 0){
      if(existing->type == SECT){
        printf("fatal error: symbol: %s is defined as section in file: %s and as symbol in file: %s\n", existing->sym, existing->file, entry->file);
        return -1;
      }
      else{
        if(entry->defined == DEF){
          //novi simbol je definisan
          if(existing->defined == DEF){
            printf("fatal error: symbol: %s is defined file: %s and file: %s\n", existing->sym, existing->file, entry->file);
            return -1;
          }
          else{
            //delete existing undefined
            if(existing->sym) free(existing->sym);
            if(existing->file) free(existing->file);
            free(existing);
            sym_tab[i] = entry;
            return 0;
          }
        }
        else{
          //novi simbol nije definisan, postoji istoimeni simbol
          return 0;
        }
      }
    }
  }
  //nepostoji simbol sa istim imenom
  sym_tab[sym_tab_size++] = entry;
  return 0;
}

int load_file(FILE* in, char* file_name, int file_no){
  int num_sym;
  int num_sec;
  int line = 1;
  if(fscanf(in, "%*s %d\n", &num_sym) == 0){
    printf("fatal error: error while reading line: %d\n", line);
    return -1;
  }
  line++;
  //printf("file: %s - numSym: %d\n", file_name, num_sym);
  fscanf(in, "%*s %*s %*s %*s %*s %*s %*s %*s\n");
  line++;
  fscanf(in, "%*d %*s %*d %*d %*s %*d %*c %*d\n");
  line++;
  for(int i = 1; i<num_sym; i++){
    char sym[20];
    int sect;
    int size;
    int offs;
    int type;
    char scope;
    int def;

    if(fscanf(in, "%*d %s %d %d %x %d %c %d\n", sym, &sect, &size, &offs, &type, &scope, &def) == 0){
      printf("fatal error: error while reading line: %d\n", line);
      return -1;
    }
    //printf("%s - %d\n", sym, offs);
    Symbol* new_sym = init_symbol(sym, sect, size, offs, type, scope, def, file_name, file_no);
    if(!new_sym){
      printf("fatal error: could not make new symbol - no memory\n");
      return -1;
    }

    if(new_sym->type == SYM && new_sym->scope == L) continue;

    if(file_no == 1){
      //prvi fajl
      sym_tab[sym_tab_size++] = new_sym;
    }
    else{
      if(new_sym->type == SYM){
        if(check_sym(new_sym) == -1) return -1;
      }
      else{
        if(check_sec(new_sym) == -1) return -1;
      }
    }
    
    line++;
  }
  if(fscanf(in, "%*s %d\n", &num_sec) == 0){
    printf("fatal error: error while reading line: %d\n", line);
    return -1;
  }
  line++;
  //printf("file: %s - numSec: %d\n", file_name, num_sec);
  for(int i = 0; i<num_sec; i++){
    char name[20];
    int size;
    unsigned int data;
    if(fscanf(in, "%s %d", name, &size) == 0){
      printf("fatal error: error while reading line: %d\n", line);
      return -1;
    }
    line++;
    Section* new_section = init_sec(name, 0, size, file_name, file_no);
    
    if(!new_section){
      printf("fatal error: could not make new section - no memory\n");
      return -1;
    }
    int rows = (size / 8) + 1;

    //read data
    for(int j = 0; j<rows; j++){
      /*if(fscanf(in, "%2x %2x %2x %2x %2x %2x %2x %2x\n", data, data+1, data+2, data+3, data+4, data+5, data+6, data+7) == 0){
        printf("fatal error: error while reading line: %d\n", line);
        return -1;
      }*/
      for(int k = 0; k<8; k++){
        if(fscanf(in, "%2x", &data) == 1){
          new_section->data[j*8 + k] = (unsigned char)data;
        }
        else{
          printf("fatal error: error while reading line: %d\n", line);
          return -1;
        }
      }
      line++;
    }
    //read relocs

    int num_reloc;

    if(fscanf(in, "%*s %d", &num_reloc) == 0){
      printf("fatal error: error while reading line: %d\n", line);
      return -1;
    }
    line++;
    fscanf(in, "%*s %*s %*s %*s");
    line++;
    for(int i = 0; i<num_reloc; i++){
      int offs;
      int type;
      char sym[20];
      int addend;

      if(fscanf(in, "%x %d %s %d", &offs, &type, sym, &addend) == 0){
        printf("fatal error: error while reading line: %d\n", line);
        return -1;
      }
      line++;
      if(add_new_reloc(new_section, offs, type, sym, addend) == -1){
        printf("fatal error: error while adding reloc - no memory: %d\n", line);
        return -1;
      }
    }
    sec_tab[sec_tab_size++] = new_section;
  }
  return 0;
}

int load_files(File_node* file_list){
  int file_no = 1;
  while(file_list){
    if(load_file(file_list->file, file_list->file_name, file_no) != 0){
      printf("fatal error: error occured while reading file: %s\n", file_list->file_name);
      return -1;
    }
    file_no++;
    file_list = file_list->next;
  }
}

void update_symbols_in_section(Symbol* section){
  if(!section) return;
  for(int i = 0; i<sym_tab_size; i++){
    Symbol* symbol = sym_tab[i];
    if(symbol->type == SYM && section->file_no == symbol->file_no && section->sect == symbol->sect){
      symbol->offs_val += section->offs_val;
    }
  }
  return;
}

void update_offset_in_sym_table(Section* section){
  if(!section) return;
  for(int i = 0; i<sym_tab_size; i++){
    Symbol* sec = sym_tab[i];
    if(strcmp(sec->sym, section->name) == 0 && sec->file_no == section->file_no && sec->type == SECT){
      sec->offs_val = section->offs;
      update_symbols_in_section(sec);
      return;
    }
  }
  return;
}

int map_sections(Place_node* place_list){
  int address = 0;
  if(place_list){
    while(place_list){
      if(address > place_list->offset){
        printf("fatal error: Could not map section: %s to address %x as the last section size too large\n", place_list->sect, place_list->offset);
        return -1;
      }
      //address = place_list->offset;
      //-place=<sect>@addr if sect not found contrinue
      //when first section found map to given addr, rest of sections with same name map after first 
      int found = 0;
      for(int i = 0; i<sec_tab_size; i++){
        Section* section = sec_tab[i];
        if(strcmp(section->name, place_list->sect) == 0 && section->mapped != 1){
          if(!found){
            address = place_list->offset;
            found = 1;
          }
          section->offs += address;
          section->mapped = 1;
          address += section->size;
          update_offset_in_sym_table(section);
        }
      }

      place_list = place_list->next;
    }
  }
  
  for(int i = 0; i<sec_tab_size; i++){
    Section* section = sec_tab[i];
    //if section already mapped continue
    if(section->mapped == 1) continue;
    section->offs = address;
    section->mapped = 1;
    address += section->size;
    update_offset_in_sym_table(section);
    //find all sections with same name
    for(int j = i+1; j<sec_tab_size; j++){
      Section* same_section = sec_tab[j];
      if(strcmp(section->name, same_section->name) == 0){
        same_section->offs += address;
        same_section->mapped = 1;
        address += same_section->size;
        update_offset_in_sym_table(same_section);
      }
    }
    //output section data to file
    //update section relocs
  }
  return 0;
}

int get_symbol_value(char* sym, int file_no){
  if(!sym) return -1;
  for(int i = 0; i<sym_tab_size; i++){
    Symbol* symbol = sym_tab[i];
    if(strcmp(sym, symbol->sym) == 0){
      if(symbol->type == SYM) return symbol->offs_val;
      else{
        if(symbol->file_no == file_no) return symbol->offs_val;
      }
    }
  }
  return -1;
}

int patch_section_relocs(Section* section){
  if(!section) return -1;
  Reloc* head = section->relocs;
  while(head){
    switch(head->type){
      case APS16:{
        //pathc in instruction
        //find symb value
        int value = get_symbol_value(head->sym, section->file_no);
        if(value == -1){
          printf("fatal error: could not get symbol value\n");
          return -1;
        }
        //calculate patch
        short patch = value + head->addend;
        char low = (char)patch;
        char hi = (char)(patch >> 8);
        //output to data
        int addr = head->offs;
        section->data[addr++] = hi;
        section->data[addr++] = low;
        break;
      }
      case APS16D:{
        //pathc in word directive
        int value = get_symbol_value(head->sym, section->file_no);
        if(value == -1){
          printf("fatal error: could not get symbol value\n");
          return -1;
        }
        //calculate patch
        short patch = value + head->addend;
        char low = (char)patch;
        char hi = (char)(patch >> 8);
        //output to data
        int addr = head->offs;
        section->data[addr++] = low;
        section->data[addr++] = hi;
        break;
      }
      case PC16:{
        //find symb value
        int value = get_symbol_value(head->sym, section->file_no);
        if(value == -1){
          printf("fatal error: could not get symbol value\n");
          return -1;
        }
        //calculate patch
        short patch = value - (section->offs + head->offs) + head->addend;
        char low = (char)patch;
        char hi = (char)(patch >> 8);
        //output to data
        int addr = head->offs;
        section->data[addr++] = hi;
        section->data[addr++] = low;
        break;
      }
      default:{
        return -1;
      }
    }
    head->resolved = 1;
    head = head->next;
  }
  return 0;
}

int patch_relocs(){
  for(int i = 0; i<sec_tab_size; i++){
    Section* sect = sec_tab[i];
    if(patch_section_relocs(sect) == -1){
      printf("fatal error: while resolving relocs for section %s\n", sect->name);
      return -1;
    }
  }
  return 0;
}

int check_unresolved_symbols(){
  int unres = 0;
  for(int i = 0; i<sym_tab_size; i++){
    Symbol* symbol = sym_tab[i];
    if(symbol->type == SYM && symbol->defined == UNDEF){
      printf("fatal error: Undefined symbol: %s in file: %s\n", symbol->sym, symbol->file);
      unres = -1;
    }
  }
  return unres;
}

/*
  int rows = (entry->size / 8) + 1;
  int ctr = 0;
  for(int i = 0; i < rows; i++){
    printf("%04x\t", ctr);
    for(int j = 0; j<8; j++) printf("%02x ", (unsigned char)entry->data[ctr++]);
    printf("\n");
  }
*/

int generate_mem_init_file(char* out_file_name){
  FILE* f = fopen(out_file_name, "w");
  if(!f){
    printf("fatal error: could not open output file: %s\n", out_file_name);
    return -1;
  }
  fprintf(f, "#file_name %s\n#data", out_file_name);
  int last_section_address = 0;
  int address = 0;
  // for(int i = 0; i<sec_tab_size; i++){
  //   Section* sect = sec_tab[i];
  //   if(sect->outputted) continue;
  //   //fprintf(f, "section: %s file: %s\n", sect->name, sect->file);
  //   last_section_address = address;
  //   address = sect->offs;


  //   //formating for -place option *****
  //   //
  //   //
  //   if(last_section_address == 0){
  //     //output first section
  //     int cnt = address % 8;
  //     if(cnt != 0){
  //       fprintf(f, "\n%04hx: ", address - cnt);
  //       for(int i = 0; i < cnt; i++) fprintf(f, "%02hhx ", 0);
  //     }
  //   }
  //   else{
  //     //not first section
  //     if(last_section_address != address){
  //       int cnt = (8 - (last_section_address % 8)) % 8;
  //       for(int i = 0; i < cnt; i++) fprintf(f, "%02hhx ", 0);
  //       cnt = address % 8;
  //       if(cnt != 0){
  //         fprintf(f, "\n%04hx: ", address - cnt);
  //         for(int i = 0; i < cnt; i++) fprintf(f, "%02hhx ", 0);
  //       }
  //     }
  //   }
  //   //
  //   //
  //   for(int i = 0; i < sect->size; i++){
  //     if(address % 8 == 0) fprintf(f, "\n%04hx: ", address);
  //     fprintf(f, "%02hhx ", sect->data[i]);
  //     address++;
  //   }
  //   sect->outputted = 1;
  //   //map sections with same name
  //   for(int j = i+1; j<sec_tab_size; j++){
  //     Section* same_section = sec_tab[j];
  //     if(strcmp(sect->name, same_section->name) == 0){
  //       address = same_section->offs;
  //       for(int i = 0; i < same_section->size; i++){
  //         if(address % 8 == 0) fprintf(f, "\n%04hx: ", address);
  //         fprintf(f, "%02hhx ", same_section->data[i]);
  //         address++;
  //       }
  //       same_section->outputted = 1;
  //     }
  //   }
  // }

  //test
  int outputted = 0;
  while(outputted < sec_tab_size){


    //find next section with min address that isnt outputted
    //printf("outputted: %d\n", outputted);
    Section* sect = 0;
    int min_address = 0;
    int j;
    for(j = 0; j<sec_tab_size; j++){
      if(sec_tab[j]->outputted == 0){
        sect = sec_tab[j];
        min_address = sect->offs;
        break;
      }
    }
    for(int i = j+1; i<sec_tab_size; i++){
      if(sec_tab[i]->outputted == 0 && sec_tab[i]->offs < min_address){
        sect = sec_tab[i];
        min_address = sect->offs;
      }
    }
    //printf("outputting: %s\n", sect->name);
    if(sect->outputted) continue;
    //fprintf(f, "section: %s file: %s\n", sect->name, sect->file);
    last_section_address = address;
    address = sect->offs;


    //formating for -place option *****
    //
    //
    if(last_section_address == 0){
      //output first section
      int cnt = address % 8;
      if(cnt != 0){
        fprintf(f, "\n%04hx: ", address - cnt);
        for(int i = 0; i < cnt; i++) fprintf(f, "%02hhx ", 0);
      }
    }
    else{
      //not first section
      if(last_section_address != address){
        int cnt = (8 - (last_section_address % 8)) % 8;
        for(int i = 0; i < cnt; i++) fprintf(f, "%02hhx ", 0);
        cnt = address % 8;
        if(cnt != 0){
          fprintf(f, "\n%04hx: ", address - cnt);
          for(int i = 0; i < cnt; i++) fprintf(f, "%02hhx ", 0);
        }
      }
    }
    //
    //
    for(int i = 0; i < sect->size; i++){
      if(address % 8 == 0) fprintf(f, "\n%04hx: ", address);
      fprintf(f, "%02hhx ", sect->data[i]);
      address++;
    }
    sect->outputted = 1;
    outputted++;
    //map sections with same name
    for(int j = 0; j<sec_tab_size; j++){
      Section* same_section = sec_tab[j];
      if(strcmp(sect->name, same_section->name) == 0 && same_section != sect){
        address = same_section->offs;
        for(int i = 0; i < same_section->size; i++){
          if(address % 8 == 0) fprintf(f, "\n%04hx: ", address);
          fprintf(f, "%02hhx ", same_section->data[i]);
          address++;
        }
        same_section->outputted = 1;
        outputted++;
      }
    }
  }


  while(address % 8) {
    fprintf(f, "%02hhx ", 0);
    address++;
  }
  fprintf(f, "\n#end\n");
  return 0;
}

int link(Parsed_command* pc){
  if(init_linker() != 0){
    printf("fatal error: could not init linker\n");
    return -1;
  }
  //print_sym_table();
  if(load_files(pc->file_list) != 0){
    printf("fatal error: could not process input files\n");
    destroy_linker();
    return -1;
  }
  if(check_unresolved_symbols() != 0){
    print_sym_table();
    printf("fatal error: undefined symbols\n");
    return -1;
  }
  if(map_sections(pc->place_list) != 0){
    printf("fatal error: could not map sections\n");
    destroy_linker();
    return -1;
  }
  if(patch_relocs() != 0){
    printf("fatal error: could not resolve relocs\n");
    destroy_linker();
    return -1;
  }
  //output to file
  if(generate_mem_init_file(pc->out_file_name) != 0){

  }
  //print_sym_table();
  //print_sec_table();
  destroy_linker();
}