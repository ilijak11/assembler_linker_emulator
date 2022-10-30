#include "../../inc/emulator.h"

Byte* mem;
Address* ivt;

int load_data(FILE* in){
  if(!in){
    printf("fatal error: no input file\n");
    return -1;
  }
  Address address;
  Byte data;
  char file_name[20];
  if(fscanf(in, "%*s %s\n", file_name) == 0) return -1;
  fscanf(in, "%*s\n");
  //printf("file_name: %s\n", file_name);
  //for(int k = 0; k<10; k++){
  while(1){
    if(fscanf(in, "%4hx: ", &address) == 0) return 0;
    //printf("address: 0x%04x\t", address);
    for(int i = 0; i<8; i++){
      if(fscanf(in, "%2hhx ", &data) == 0) return -1;
      //printf("%02x ", data);
      mem[address+i] = data;
    }
    //putchar('\n');
  }
}

int init_mem(FILE* in){
  mem = (Byte*)calloc(MEMCAP, sizeof(Byte));
  if(!mem){
    printf("fatal error: could not initialize memory\n");
    return -1;
  }
  if(load_data(in) == -1){
    printf("fatal error: could not load data from file\n");
    return -1;
  }
  //ivtp 0x0000 in emulated space + &mem - memory start address in program address space 
  ivt = IVTP + (Address*)mem;
}

void mem_dump(FILE* out){
  Address addres = 0;
  for(int i = 0; i<MEMCAP/8; i++){
    fprintf(out, "0x%04hx: ", addres);
    for(int j = 0; j<8; j++){
      fprintf(out, "%02hhx ", mem[addres++]);
    }
    fprintf(out, "\n");
  }
}

void destroy_mem(){
  free(mem);
}