#include "../../inc/emulator.h"

CPU* cpu;

int init_cpu(){
  cpu = (CPU*)malloc(sizeof(CPU));
  if(!cpu){
    printf("fatal error: Could not initialize CPU\n");
    return -1;
  }
  for(int i = 0; i<REGNUM; i++){
    cpu->reg[i] = 0;
  }
  cpu->pc = &cpu->reg[PC];
  cpu->sp = &cpu->reg[SP];
  cpu->psw = 0;
  return 0;
}

void destroy_cpu(){
  free(cpu);
}

void set_flag(short flg){
  cpu->psw |= flg;
}

int set_flag1(enum flag flag){
  if(!cpu){
    printf("fatal error: CPU not initialized\n");
    return -1;
  }
  switch(flag){
    case Z:{
      cpu->psw |= FLG_Z;
      return 0;
    }
    case O:{
      cpu->psw |= FLG_O;
      return 0;
    }
    case C:{
      cpu->psw |= FLG_C;
      return 0;
    }
    case N:{
      cpu->psw |= FLG_N;
      return 0;
    }
    case Tr:{
      cpu->psw |= FLG_Tr;
      return 0;
    }
    case Tl:{
      cpu->psw |= FLG_Tl;
      return 0;
    }
    case I:{
      cpu->psw |= FLG_I;
      return 0;
    }
    default:{
      printf("fatal error: illegal flag\n");
      return -1;
    }
  }
}

void reset_flag(short flg){
  cpu->psw &= ~flg;
}

int reset_flag1(enum flag flag){
  if(!cpu){
    printf("fatal error: CPU not initialized\n");
    return -1;
  }
  switch(flag){
    case Z:{
      cpu->psw &= ~FLG_Z;
      return 0;
    }
    case O:{
      cpu->psw &= ~FLG_O;
      return 0;
    }
    case C:{
      cpu->psw &= ~FLG_C;
      return 0;
    }
    case N:{
      cpu->psw &= ~FLG_N;
      return 0;
    }
    case Tr:{
      cpu->psw &= ~FLG_Tr;
      return 0;
    }
    case Tl:{
      cpu->psw &= ~FLG_Tl;
      return 0;
    }
    case I:{
      cpu->psw &= ~FLG_I;
      return 0;
    }
    default:{
      printf("fatal error: illegal flag\n");
      return -1;
    }
  }
}

Byte get_flag(short flg){
  return (cpu->psw & flg) != 0;
}

Byte get_flag1(enum flag flag){
  if(!cpu){
    printf("fatal error: CPU not initialized\n");
    return -1;
  }
  switch(flag){
    case Z:{
      return cpu->psw &= FLG_Z;
    }
    case O:{
      return cpu->psw &= FLG_O;
    }
    case C:{
      return cpu->psw &= FLG_C;
    }
    case N:{
      return cpu->psw &= FLG_N;
    }
    case Tr:{
      return cpu->psw &= FLG_Tr;
    }
    case Tl:{
      return cpu->psw &= FLG_Tr;
    }
    case I:{
      return cpu->psw &= FLG_I;
    }
    default:{
      printf("fatal error: illegal flag\n");
      return -1;
    }
  }
}


void test_flags(){
  set_flag(Z);
  set_flag(O);
  set_flag(C);
  set_flag(N);
  set_flag(Tr);
  set_flag(Tl);
  set_flag(I);
}

void print_cpu_state(){
  //test_flags();
  char psw_bin[17];
  for(int i = 0; i<16; i++){
    if((cpu->psw >> i) & 1) psw_bin[15 - i] = '1';
    else psw_bin[15 - i] = '0';
  }
  psw_bin[16] = '\0';

  printf("Emulated  processor state: psw=0b%s\n", psw_bin);
  for(int i = 0; i<8; i++){
    if(i == 4) putchar('\n');
    //if(i == 6) printf("(sp)");
    //if(i == 7) printf("(pc)");
    printf("r%d=0x%04hx\t", i, cpu->reg[i]);
  }
  putchar('\n');
  //printf("pc: 0x%04hx\n", *cpu->pc);
  //printf("sp: 0x%04hx\n", *cpu->sp);
}
