#include "../../inc/emulator.h"
#include <unistd.h>
#include <termios.h>

CPU* cpu;
Byte* mem;
Address* ivt;
enum state state;
FILE* log_f;

void load_program_start_address(){
  *cpu->pc = ivt[IVT_START];
}

Byte read_instr_byte(){
  Address pc = (Address)(*cpu->pc); //cast pc (Reg/short) to Address (unsigned short)
  Byte byte = mem[pc]; // get byte MEM[PC]
  *cpu->pc = *cpu->pc + 1; // PC++
  return byte;
}

Data buld_data(Byte hi, Byte low){
  Data data = 0x0000;
  unsigned short hi16 = (unsigned short)(hi << 8);
  unsigned short low16 = (unsigned short)(low  & 0x00ff); //delete sign
  data = hi16 | low16;
  return data;
}

Data read_word_from_memory(Address addr){
  if(addr == MAXADDR){
    printf("read from address 0xffff + 1\n");
  }
  Byte low = mem[addr];
  Byte hi = mem[addr+1];
  fprintf(log_f,"READ: addr: %04hx byte: %02hhx mem[addr]: %02hhx\n", addr, low, mem[addr]);
  fprintf(log_f,"READ: addr: %04hx byte: %02hhx mem[addr]: %02hhx\n", addr + 1, hi, mem[addr + 1]);
  return buld_data(hi, low);
}

int write_word_to_memory(Address addr, Data data){
  if(addr == MAXADDR){
    printf("write to address 0xffff + 1\n");
    return -1;
  }
  //write to terminal 
  if(addr == TERM_OUT){
    printf("Terminal: %c\n", (char)data);
  }
  Byte hi = (Byte)(data >> 8);
  Byte low = (Byte)(data);
  mem[addr] = low;
  mem[addr + 1] = hi;
  fprintf(log_f,"WRITE: addr: %04hx byte: %02hhx mem[addr]: %02hhx\n", addr, low, mem[addr]);
  fprintf(log_f,"WRITE: addr: %04hx byte: %02hhx mem[addr]: %02hhx\n", addr + 1, hi, mem[addr + 1]);
  return 0;
}

Data get_instr_payload(){
  Byte data_hi = read_instr_byte();
  Byte data_low = read_instr_byte();
  return buld_data(data_hi, data_low);
}

int push_to_stack(Data data){
  // Byte hi = (Byte)(data >> 8);
  // Byte low =  (Byte)(data);
  Address sp = (Address)(*cpu->sp - 2); //cast sp (Reg/short) to Address (unsigned short)
  fprintf(log_f,"*PUSH* -> data: %04hx -> new sp: %04hx (%d)\n", data, sp, sp);
  *cpu->sp = *cpu->sp - 2; //update sp
  // mem[sp] = low;
  // mem[sp + 1] = hi;
  write_word_to_memory(sp, data);
  //printf("*PUSH* MEM: addr: %04hx data: %02hx\n", sp, mem[sp]);
  //printf("*PUSH* MEM: addr: %04hx data: %02hx\n", sp + 1, mem[sp + 1]);
  return 0;
}

Data pop_from_stack(){
  Address sp = (Address)(*cpu->sp);
  //printf("*POP* MEM: addr: %04hx data: %02hx\n", sp, mem[sp]);
  //printf("*POP* MEM: addr: %04hx data: %02hx\n", sp + 1, mem[sp + 1]);
  // Byte hi = mem[sp+1];
  // Byte low = mem[sp];
  // Data data = buld_data(hi, low);
  Data data = read_word_from_memory(sp);
  fprintf(log_f,"*POP* -> data: %04hx -> new sp: %04hx (%d)\n", data, sp+2, sp+2);
  *cpu->sp = *cpu->sp + 2;
  return data;
}

Address get_address_from_reg_and_update_reg(Byte update, Byte Rs){
   //get address
  Address addr = 0;
  switch(update){
    case NOUPDATE:{
      addr = (Address)cpu->reg[Rs];
      break;
    }
    case PREDEC2:{
      cpu->reg[Rs] = cpu->reg[Rs] - 2;
      addr = (Address)cpu->reg[Rs];
      break;
    }
    case PREINC2:{
      cpu->reg[Rs] = cpu->reg[Rs] + 2;
      addr = (Address)cpu->reg[Rs];
      break; 
    }
    case POSTDEC2:{
      addr = (Address)cpu->reg[Rs];
      cpu->reg[Rs] = cpu->reg[Rs] - 2;
      break;
    }
    case POSTINC2:{
      //pop
      addr = (Address)cpu->reg[Rs];
      cpu->reg[Rs] = cpu->reg[Rs] + 2;
      break;
    }
    default:{
      state = BAD_UPDATE;
      printf("fatal error: unsupported update mode for JMP instruciton\n");
      return -1;
    }
  }
  return addr;
}

int jump_on_interrupt_routine(Byte entry){
  if(entry < 0 || entry >= IVT_ENTRIES_NUM){
    printf("fatal error: invalid IVT entry: %d - valid entries: 0 - %d\n", entry, IVT_ENTRIES_NUM - 1);
    return -1;
  }
  fprintf(log_f,"INTERRUPT: entry: %d ret_pc: %04hx psw: %04hx\n",entry, *cpu->pc, cpu->psw);
  //push PC
  push_to_stack(*cpu->pc);
  //push PSW
  push_to_stack(cpu->psw);
  //fetch routine address
  Address new_pc = (Address)ivt[entry]; 
  //update PC
  *cpu->pc = new_pc;
  //clear flags
  cpu->psw = 0;
  fprintf(log_f,"INTERRUPT: new_pc: %04hx new_psw: %04hx\n", *cpu->pc, cpu->psw);
  //disable interrupts - no nested interrupt routines
  set_flag(FLG_I);
  set_flag(FLG_Tr);
  set_flag(FLG_Tl);
  return 0;
}

enum instr decode_instr(unsigned char op_code){
  if(op_code == OC_HALT) return HALT;
  if(op_code == OC_INT) return INT;
  if(op_code == OC_IRET) return IRET;
  if(op_code == OC_CALL) return CALL;
  if(op_code == OC_RET) return RET;
  if(op_code == OC_JMP) return JMP;
  if(op_code == OC_JEQ) return JEQ;
  if(op_code == OC_JNE) return JNE;
  if(op_code == OC_JGT) return JGT;
  if(op_code == OC_XCHG) return XCHG;
  if(op_code == OC_ADD) return ADD;
  if(op_code == OC_SUB) return SUB;
  if(op_code == OC_MUL) return MUL;
  if(op_code == OC_DIV) return DIV;
  if(op_code == OC_CMP) return CMP;
  if(op_code == OC_NOT) return NOT;
  if(op_code == OC_AND) return AND;
  if(op_code == OC_OR) return OR;
  if(op_code == OC_XOR) return XOR;
  if(op_code == OC_TEST) return TEST;
  if(op_code == OC_SHL) return SHL;
  if(op_code == OC_SHR) return SHR;
  if(op_code == OC_LDR) return LDR;
  if(op_code == OC_STR) return STR;
  return BAD_INSTR;
}

int read_and_execute_instruction(){
  //get 1. byte - Op Code
  fprintf(log_f, "--------------------------\nPC: %04hx\n", *cpu->pc);
  Byte op_code = read_instr_byte();
  enum instr instr = decode_instr((unsigned char)op_code);
  if(instr == BAD_INSTR){
    state = BAD_OP_CODE;
    //jump to err handler
    printf("Emulated processor tried to execute invalid instruction\n");
    return -1;
  }
  // decode and execute
  switch(instr){
    case HALT:{
      fprintf(log_f, "HALT\n");
      state = STOPPED;
      printf("--------------------------------------------\n");
      printf("Emulated processor executed HALT instruction\n");
      return 0;
    }
    case INT:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4);
      fprintf(log_f, "INT - entry: %d\n", cpu->reg[Rd]);
      jump_on_interrupt_routine((Byte)cpu->reg[Rd]);
      return 0;
    }
    case IRET:{
      //pop PSW
      cpu->psw = pop_from_stack();
      //pop PC
      *cpu->pc = (Address)pop_from_stack();
      fprintf(log_f, "IRET\n");
      return 0;
    }
    case CALL:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4); 
      //get address mode and update
      Byte update_addr_mode = read_instr_byte(); //3rd byte
      Byte address_mode = (Byte)(update_addr_mode & 0x0f);
      Byte update = (Byte)(update_addr_mode >> 4);


      //save ret pc
      Address ret_pc = 0;
      //get operand
      switch (address_mode){
        case IMMED:{
          //read immed data
          Address new_pc = (Address)get_instr_payload();
          //store data in dest reg
          //save ret pc
          ret_pc = *cpu->pc;
          *cpu->pc = new_pc;
          fprintf(log_f, "CALL - immed new_pc: %04hx\n", *cpu->pc);
          break;
        }
        case MEMDIR:{
          //read immed data
          Address addr = (Address)get_instr_payload();
          //fetch operand mem[address]
          Address new_pc = (Address)read_word_from_memory(addr);
          // if(address == MAXADDR){
          //   printf("fatal error: instr LDR tried to load data from address 0xffff\n");
          //   return -1;
          // }
          // Byte data_low = mem[address];
          // Byte data_hi = mem[address + 1];

          // Data data = buld_data(data_hi, data_low);
          // //store data in dest reg
          //save ret pc
          ret_pc = *cpu->pc;
          *cpu->pc = new_pc;
          fprintf(log_f, "CALL - memdir new_pc = mem[%04hx]: %04hx\n",addr, *cpu->pc);
          break;
        }
        case REGDIR:{
          Address new_pc = (Address)cpu->reg[Rs];
          //save ret pc
          ret_pc = *cpu->pc;
          *cpu->pc = new_pc;
          fprintf(log_f, "CALL - regdir new_pc = r%d: %04hx\n",Rs, *cpu->pc);
          break;
        }
        case REGDIRPOM:{
          //read immed data
          Data offs = get_instr_payload();
          Address data = (Address)cpu->reg[Rs];
          //save ret pc
          ret_pc = *cpu->pc;
          *cpu->pc = data + offs;
          fprintf(log_f, "CALL - regdir + offs: [r%d]: %04hx + offs: %04x (%d) = new pc: %04hx\n",Rs, data, offs, offs, data+offs);
          break;
        }
        case REGINDIR:{
          Address addr = get_address_from_reg_and_update_reg(update, Rs);
          Address new_pc = (Address)read_word_from_memory(addr);
          //update pc
          //save ret pc
          ret_pc = *cpu->pc;
          *cpu->pc = new_pc;
          fprintf(log_f, "CALL - regindir: [r%d]: [%04hx]: %04hx = new pc: %04hx\n",Rs, addr, new_pc, *cpu->pc);
          break;
        }
        case REGINDIRPOM:{
          //read instr data
          Data offs = get_instr_payload();
          Address addr = get_address_from_reg_and_update_reg(update, Rs);
          fprintf(log_f, "CALL - regindir: [r%d]: %04hx + offs: %04hx (%d) -> addr: %02x\n",Rs ,addr, offs, offs, addr+offs);

          addr = (Address)(addr + offs);

          Address new_pc = (Address)read_word_from_memory(addr);
          //update pc
          //save ret pc
          ret_pc = *cpu->pc;
          *cpu->pc = new_pc;
          fprintf(log_f, "CALL - regindir:  new pc: [%04hx] = %04hx\n", addr, *cpu->pc);
          break;
        }
        default:{
          state = BAD_ADDR_MODE;
          printf("fatal error: unsupported address mode for JMP instruciton\n");
          return -1;
        }
      }

      //pc updated
      //push ret_pc on stack
      push_to_stack(ret_pc);
      return 0;
    }
    case RET:{
      Address ret_pc = (Address)pop_from_stack();
      fprintf(log_f, "RET: new_pc: %04xh\n", ret_pc);
      *cpu->pc = ret_pc;
      return 0;
    }
    case JMP:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4); 
      //get address mode and update
      Byte update_addr_mode = read_instr_byte(); //3rd byte
      Byte address_mode = (Byte)(update_addr_mode & 0x0f);
      Byte update = (Byte)(update_addr_mode >> 4);

      //get operand
      switch (address_mode){
        case IMMED:{
          //read immed data hi
          Address new_pc = (Address)get_instr_payload();
          //store data in dest reg
          *cpu->pc = new_pc;
          fprintf(log_f, "JMP - immed new_pc: %04hx\n", *cpu->pc);
          return 0;
        }
        case MEMDIR:{
          //read immed data hi
          Address address = (Address)get_instr_payload();
          //fetch operand mem[address]
          Address new_pc = (Address)read_word_from_memory(address);
          //store data in dest reg
          *cpu->pc = new_pc;
          fprintf(log_f, "JMP - memdir new_pc = mem[%04hx]: %04hx\n",address, *cpu->pc);
          return 0;
        }
        case REGDIR:{
          Address new_pc = (Address)cpu->reg[Rs];
          *cpu->pc = new_pc;
          fprintf(log_f, "JMP - regdir new_pc = r%d: %04hx\n",Rs, *cpu->pc);
          return 0;
        }
        case REGDIRPOM:{
          //read immed data
          Data offs = get_instr_payload();
          Address data = (Address)cpu->reg[Rs];
          *cpu->pc = (Address)(data + offs);
          fprintf(log_f, "JMP - regdir + offs: [r%d]: %04hx + offs: %04x (%d) = new pc: %04hx\n",Rs, data, offs, offs, data+offs);
          return 0;
        }
        case REGINDIR:{
          Address addr = get_address_from_reg_and_update_reg(update, Rs);
          Address new_pc = (Address)read_word_from_memory(addr);
          //update pc
          *cpu->pc = new_pc;
          fprintf(log_f, "JMP - regindir: [r%d]: [%04hx]: %04hx = new pc: %04hx\n",Rs, addr, new_pc, *cpu->pc);
          break;
        }
        case REGINDIRPOM:{
          Data offs = get_instr_payload();
          Address addr = get_address_from_reg_and_update_reg(update, Rs);
          fprintf(log_f, "JMP - regindir: [r%d]: %04hx + offs: %04hx (%d) -> addr: %02x\n",Rs ,addr, offs, offs, addr+offs);

          addr = (Address)(addr + offs);

          Address new_pc = (Address)read_word_from_memory(addr);
          //update pc

          *cpu->pc = new_pc;
          fprintf(log_f, "JMP - regindir:  new pc: [%04hx] = %04hx\n", addr, *cpu->pc);
          break;
        }
        default:{
          state = BAD_ADDR_MODE;
          printf("fatal error: unsupported address mode for JMP instruciton\n");
          return -1;
        }
      }

      return 0;
    }
    case JEQ:{
      //jeq condition Z = 1
      Byte jmp = get_flag(FLG_Z)?1:0;
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4); 
      //get address mode and update
      Byte update_addr_mode = read_instr_byte(); //3rd byte
      Byte address_mode = (Byte)(update_addr_mode & 0x0f);
      Byte update = (Byte)(update_addr_mode >> 4);

      //get operand
      switch (address_mode){
        case IMMED:{
          if(!jmp){
            *cpu->pc = *cpu->pc + 2;
            fprintf(log_f, "JEQ - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          //read immed data hi
          Address new_pc = (Address)get_instr_payload();
          //store data in dest reg
          *cpu->pc = new_pc;
          fprintf(log_f, "JEQ - immed new_pc: %04hx\n", *cpu->pc);
          return 0;
        }
        case MEMDIR:{
          //read immed data hi
          if(!jmp){
            *cpu->pc = *cpu->pc + 2;
            fprintf(log_f, "JEQ - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          Address address = (Address)get_instr_payload();
          //fetch operand mem[address]
          Address new_pc = (Address)read_word_from_memory(address);
          //store data in dest reg
          *cpu->pc = new_pc;
          fprintf(log_f, "JEQ - memdir new_pc = mem[%04hx]: %04hx\n",address, *cpu->pc);
          return 0;
        }
        case REGDIR:{
          if(!jmp){
            fprintf(log_f, "JEQ - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          Address new_pc = (Address)cpu->reg[Rs];
          *cpu->pc = new_pc;
          fprintf(log_f, "JEQ - regdir new_pc = r%d: %04hx\n",Rs, *cpu->pc);
          return 0;
        }
        case REGDIRPOM:{
          if(!jmp){
            *cpu->pc = *cpu->pc + 2;
            fprintf(log_f, "JEQ - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          //read immed data hi
          Data offs = get_instr_payload();
          Address data = (Address)cpu->reg[Rs];
          *cpu->pc = (Address)(data + offs);
          fprintf(log_f, "JEQ - regdir + offs: [r%d]: %04hx + offs: %04x (%d) = new pc: %04hx\n",Rs, data, offs, offs, data+offs);
          return 0;
        }
        case REGINDIR:{
          if(!jmp){
            fprintf(log_f, "JEQ - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          Address addr = get_address_from_reg_and_update_reg(update, Rs);
          Address new_pc = (Address)read_word_from_memory(addr);
          //update pc
          *cpu->pc = new_pc;
          fprintf(log_f, "JEQ - regindir: [r%d]: [%04hx]: %04hx = new pc: %04hx\n",Rs, addr, new_pc, *cpu->pc);
          break;
        }
        case REGINDIRPOM:{
          if(!jmp){
            *cpu->pc = *cpu->pc + 2;
            fprintf(log_f, "JEQ - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          Data offs = get_instr_payload();
          Address addr = get_address_from_reg_and_update_reg(update, Rs);
          fprintf(log_f, "JEQ - regindir: [r%d]: %04hx + offs: %04hx (%d) -> addr: %02x\n",Rs ,addr, offs, offs, addr+offs);

          addr = (Address)(addr + offs);

          Address new_pc = (Address)read_word_from_memory(addr);
          //update pc

          *cpu->pc = new_pc;
          fprintf(log_f, "JEQ - regindir:  new pc: [%04hx] = %04hx\n", addr, *cpu->pc);
          break;
        }
        default:{
          state = BAD_ADDR_MODE;
          printf("fatal error: unsupported address mode for JMP instruciton\n");
          return -1;
        }
      }

      return 0;
    }
    case JNE:{
      //jeq condition Z = 1
      Byte jmp = !get_flag(FLG_Z)?1:0;
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4); 
      //get address mode and update
      Byte update_addr_mode = read_instr_byte(); //3rd byte
      Byte address_mode = (Byte)(update_addr_mode & 0x0f);
      Byte update = (Byte)(update_addr_mode >> 4);

      //get operand
      switch (address_mode){
        case IMMED:{
          if(!jmp){
            *cpu->pc = *cpu->pc + 2;
            fprintf(log_f, "JNE - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          //read immed data hi
          Address new_pc = (Address)get_instr_payload();
          //store data in dest reg
          *cpu->pc = new_pc;
          fprintf(log_f, "JNE - immed new_pc: %04hx\n", *cpu->pc);
          return 0;
        }
        case MEMDIR:{
          //read immed data hi
          if(!jmp){
            *cpu->pc = *cpu->pc + 2;
            fprintf(log_f, "JNE - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          Address address = (Address)get_instr_payload();
          //fetch operand mem[address]
          Address new_pc = (Address)read_word_from_memory(address);
          //store data in dest reg
          *cpu->pc = new_pc;
          fprintf(log_f, "JNE - memdir new_pc = mem[%04hx]: %04hx\n",address, *cpu->pc);
          return 0;
        }
        case REGDIR:{
          if(!jmp){
            fprintf(log_f, "JNE - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          Address new_pc = (Address)cpu->reg[Rs];
          *cpu->pc = new_pc;
          fprintf(log_f, "JNE - regdir new_pc = r%d: %04hx\n",Rs, *cpu->pc);
          return 0;
        }
        case REGDIRPOM:{
          if(!jmp){
            *cpu->pc = *cpu->pc + 2;
            fprintf(log_f, "JNE - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          //read immed data hi
          Data offs = get_instr_payload();
          Address data = (Address)cpu->reg[Rs];
          *cpu->pc = (Address)(data + offs);
          fprintf(log_f, "JNE - regdir + offs: [r%d]: %04hx + offs: %04x (%d) = new pc: %04hx\n",Rs, data, offs, offs, data+offs);
          return 0;
        }
        case REGINDIR:{
          if(!jmp){
            fprintf(log_f, "JNE - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          Address addr = get_address_from_reg_and_update_reg(update, Rs);
          Address new_pc = (Address)read_word_from_memory(addr);
          //update pc
          *cpu->pc = new_pc;
          fprintf(log_f, "JNE - regindir: [r%d]: [%04hx]: %04hx = new pc: %04hx\n",Rs, addr, new_pc, *cpu->pc);
          break;
        }
        case REGINDIRPOM:{
          if(!jmp){
            *cpu->pc = *cpu->pc + 2;
            fprintf(log_f, "JNE - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          Data offs = get_instr_payload();
          Address addr = get_address_from_reg_and_update_reg(update, Rs);
          fprintf(log_f, "JNE - regindir: [r%d]: %04hx + offs: %04hx (%d) -> addr: %02x\n",Rs ,addr, offs, offs, addr+offs);

          addr = (Address)(addr + offs);

          Address new_pc = (Address)read_word_from_memory(addr);
          //update pc

          *cpu->pc = new_pc;
          fprintf(log_f, "JNE - regindir:  new pc: [%04hx] = %04hx\n", addr, *cpu->pc);
          break;
        }
        default:{
          state = BAD_ADDR_MODE;
          printf("fatal error: unsupported address mode for JMP instruciton\n");
          return -1;
        }
      }

      return 0;
    }
    case JGT:{
      //jeq condition Z = 1
      Byte jmp = (!(get_flag(FLG_N) ^ get_flag(FLG_O)) & !get_flag(FLG_Z))?1:0;
      fprintf(log_f, "JGT: !(N: %d ^ O: %d) & !(Z: %d) -> jmp: %d\n",get_flag(FLG_N), get_flag(FLG_O), get_flag(FLG_Z), jmp);
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4); 
      //get address mode and update
      Byte update_addr_mode = read_instr_byte(); //3rd byte
      Byte address_mode = (Byte)(update_addr_mode & 0x0f);
      Byte update = (Byte)(update_addr_mode >> 4);

      //get operand
      switch (address_mode){
        case IMMED:{
          if(!jmp){
            *cpu->pc = *cpu->pc + 2;
            fprintf(log_f, "JGT - immed - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          //read immed data hi
          Address new_pc = (Address)get_instr_payload();
          //store data in dest reg
          *cpu->pc = new_pc;
          fprintf(log_f, "JGT - immed new_pc: %04hx\n", *cpu->pc);
          return 0;
        }
        case MEMDIR:{
          //read immed data hi
          if(!jmp){
            *cpu->pc = *cpu->pc + 2;
            fprintf(log_f, "JGT - memdir -  no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          Address address = (Address)get_instr_payload();
          //fetch operand mem[address]
          Address new_pc = (Address)read_word_from_memory(address);
          //store data in dest reg
          *cpu->pc = new_pc;
          fprintf(log_f, "JGT - memdir new_pc = mem[%04hx]: %04hx\n",address, *cpu->pc);
          return 0;
        }
        case REGDIR:{
          if(!jmp){
            fprintf(log_f, "JGT - regdir - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          Address new_pc = (Address)cpu->reg[Rs];
          *cpu->pc = new_pc;
          fprintf(log_f, "JGT - regdir new_pc = r%d: %04hx\n",Rs, *cpu->pc);
          return 0;
        }
        case REGDIRPOM:{
          if(!jmp){
            *cpu->pc = *cpu->pc + 2;
            fprintf(log_f, "JGT - regdirpom - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          //read immed data hi
          Data offs = get_instr_payload();
          Address data = (Address)cpu->reg[Rs];
          *cpu->pc = (Address)(data + offs);
          fprintf(log_f, "JGT - regdir + offs: [r%d]: %04hx + offs: %04x (%d) = new pc: %04hx\n",Rs, data, offs, offs, data+offs);
          return 0;
        }
        case REGINDIR:{
          if(!jmp){
            fprintf(log_f, "JGT - regindir - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          Address addr = get_address_from_reg_and_update_reg(update, Rs);
          Address new_pc = (Address)read_word_from_memory(addr);
          //update pc
          *cpu->pc = new_pc;
          fprintf(log_f, "JGT - regindir: [r%d]: [%04hx]: %04hx = new pc: %04hx\n",Rs, addr, new_pc, *cpu->pc);
          break;
        }
        case REGINDIRPOM:{
          if(!jmp){
            *cpu->pc = *cpu->pc + 2;
            fprintf(log_f, "JGT - regdirpom - no jump - new_pc: %04hx\n", *cpu->pc);
            return 0;
          }
          Data offs = get_instr_payload();
          Address addr = get_address_from_reg_and_update_reg(update, Rs);
          fprintf(log_f, "JGT - regindir: [r%d]: %04hx + offs: %04hx (%d) -> addr: %02x\n",Rs ,addr, offs, offs, addr+offs);

          addr = (Address)(addr + offs);

          Address new_pc = (Address)read_word_from_memory(addr);
          //update pc

          *cpu->pc = new_pc;
          fprintf(log_f, "JGT - regindir:  new pc: [%04hx] = %04hx\n", addr, *cpu->pc);
          break;
        }
        default:{
          state = BAD_ADDR_MODE;
          printf("fatal error: unsupported address mode for JMP instruciton\n");
          return -1;
        }
      }

      return 0;
    }
    case XCHG:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4);
      Reg tmp = cpu->reg[Rd];
      cpu->reg[Rd] = cpu->reg[Rs];
      cpu->reg[Rs] = tmp;
      fprintf(log_f, "XCHG: [Rd]: %04hx [Rs]: %04hx - r%d <=> r%d - [Rd]: %04hx [Rs]: %04hx\n", cpu->reg[Rs], cpu->reg[Rd], Rd, Rs, cpu->reg[Rd], cpu->reg[Rs]);
      return 0;
    }
    case ADD:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4);
      fprintf(log_f, "ADD - [r%d]: %04hx (%d) + [r%d]: %04hx (%d) - [r%d]: %04hx (%d)\n", Rd, cpu->reg[Rd], cpu->reg[Rd], Rs, cpu->reg[Rs], cpu->reg[Rs], Rd, cpu->reg[Rd] + cpu->reg[Rs], cpu->reg[Rd] + cpu->reg[Rs]);
      cpu->reg[Rd] = cpu->reg[Rd] + cpu->reg[Rs];
      return 0;
    }
    case SUB:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4);
      fprintf(log_f, "SUB - [r%d]: %04hx (%d) - [r%d]: %04hx (%d) - [r%d]: %04hx (%d)\n", Rd, cpu->reg[Rd], cpu->reg[Rd], Rs, cpu->reg[Rs], cpu->reg[Rs], Rd, cpu->reg[Rd] - cpu->reg[Rs], cpu->reg[Rd] - cpu->reg[Rs]);
      cpu->reg[Rd] = cpu->reg[Rd] - cpu->reg[Rs];
      return 0;
    }
    case MUL:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4);
      fprintf(log_f, "MUL - [r%d]: %04hx (%d) * [r%d]: %04hx (%d) - [r%d]: %04hx (%d)\n", Rd, cpu->reg[Rd], cpu->reg[Rd], Rs, cpu->reg[Rs], cpu->reg[Rs], Rd, cpu->reg[Rd] * cpu->reg[Rs], cpu->reg[Rd] * cpu->reg[Rs]);
      cpu->reg[Rd] = cpu->reg[Rd] * cpu->reg[Rs];
      return 0;
    }
    case DIV:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4);
      if(cpu->reg[Rs] == 0){
        //error handler
        printf("fatal error: tried division with 0\n");
        return -1;
      }
      fprintf(log_f, "DIV - [r%d]: %04hx (%d) / [r%d]: %04hx (%d) - [r%d]: %04hx (%d)\n", Rd, cpu->reg[Rd], cpu->reg[Rd], Rs, cpu->reg[Rs], cpu->reg[Rs], Rd, cpu->reg[Rd] / cpu->reg[Rs], cpu->reg[Rd] / cpu->reg[Rs]);
      cpu->reg[Rd] = cpu->reg[Rd] / cpu->reg[Rs];
      return 0;
    }
    case CMP:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4);
      Reg dst = cpu->reg[Rd];
      Reg src = cpu->reg[Rs];
      Reg tmp = dst - src;
      fprintf(log_f, "CMP - [r%d]: %04hx (%d) <> [r%d]: %04hx (%d) - tmp: %04hx (%d)\nCMP - old PSW: %04hx\n", Rd, cpu->reg[Rd], cpu->reg[Rd], Rs, cpu->reg[Rs], cpu->reg[Rs], tmp, tmp, cpu->psw);
      //update N,Z,C,O flgs
      if(tmp == 0){
        set_flag(FLG_Z);
      }
      else{
        reset_flag(FLG_Z);
      }
      if(tmp < 0){
        set_flag(FLG_N);
      }
      else{
        reset_flag(FLG_N);
      }
      if((unsigned short)dst < (unsigned short)src){
        set_flag(FLG_C);
      }
      else{
        reset_flag(FLG_C);
      }
      /*
      short a = registers[destination_register_number];
        short b = registers[source_register_number];
      ((a > 0 && b < 0 && (a - b) < 0) || (a < 0 && b > 0 && (a - b) > 0))
      */
      if((dst > 0 && src < 0 && (dst - src) < 0) || (dst < 0 && src > 0 && (dst - src) > 0)){
        set_flag(FLG_O);
      }
      else{
        reset_flag(FLG_O);
      }
      fprintf(log_f, "CMP - new PSW: %04hx\n", cpu->psw);
      return 0;
    }
    case NOT:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rd = (Byte)(regs >> 4);
      fprintf(log_f, "NOT - [r%d]: %04hx ~ [r%d]: %04hx\n", Rd, cpu->reg[Rd], Rd, ~cpu->reg[Rd]);
      cpu->reg[Rd] = ~cpu->reg[Rd];
      return 0;
    }
    case AND:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4);
      fprintf(log_f, "AND - [r%d]: %04hx (%d) & [r%d]: %04hx (%d) - [r%d]: %04hx (%d)\n", Rd, cpu->reg[Rd], cpu->reg[Rd], Rs, cpu->reg[Rs], cpu->reg[Rs], Rd, cpu->reg[Rd] & cpu->reg[Rs], cpu->reg[Rd] & cpu->reg[Rs]);
      cpu->reg[Rd] = cpu->reg[Rd] & cpu->reg[Rs];
      return 0;
    }
    case OR:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4);
      fprintf(log_f, "OR - [r%d]: %04hx (%d) | [r%d]: %04hx (%d) - [r%d]: %04hx (%d)\n", Rd, cpu->reg[Rd], cpu->reg[Rd], Rs, cpu->reg[Rs], cpu->reg[Rs], Rd, cpu->reg[Rd] | cpu->reg[Rs], cpu->reg[Rd] | cpu->reg[Rs]);
      cpu->reg[Rd] = cpu->reg[Rd] | cpu->reg[Rs];
      return 0;
    }
    case XOR:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4);
      fprintf(log_f, "XOR - [r%d]: %04hx (%d) ^ [r%d]: %04hx (%d) - [r%d]: %04hx (%d)\n", Rd, cpu->reg[Rd], cpu->reg[Rd], Rs, cpu->reg[Rs], cpu->reg[Rs], Rd, cpu->reg[Rd] ^ cpu->reg[Rs], cpu->reg[Rd] ^ cpu->reg[Rs]);
      cpu->reg[Rd] = cpu->reg[Rd] ^ cpu->reg[Rs];
      return 0;
    }
    case TEST:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4);
      Reg tmp = cpu->reg[Rd] & cpu->reg[Rs];
      fprintf(log_f, "TEST - [r%d]: %04hx (%d) & [r%d]: %04hx (%d) - tmp: %04hx (%d)\nTEST - old PSW: %04hx", Rd, cpu->reg[Rd], cpu->reg[Rd], Rs, cpu->reg[Rs], cpu->reg[Rs], tmp, tmp, cpu->psw);
      // update Z, N flgs
      if(tmp == 0){//set Z flg
        set_flag(FLG_Z);
      }
      else{
        reset_flag(FLG_Z);
      }
      if(tmp < 0){ //set N flg
        set_flag(FLG_N);
      }
      else{
        reset_flag(FLG_N);
      }
      fprintf(log_f, "TEST - new PSW: %04hx\n", cpu->psw);
      return 0;
    }
    case SHL:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4);
      unsigned int tmp = (unsigned int)((unsigned int)cpu->reg[Rd] << (unsigned int)cpu->reg[Rs]);
      cpu->reg[Rd] = (Reg)tmp;
      unsigned int mask = (unsigned int)((unsigned int)(~ 0x0) << sizeof(Reg)*8);
      fprintf(log_f, "SHL: [R%d]: %0hhx\n[R%d]: %0hhx\nmask: %08x\ntmp: %08x\n",Rd, Rs, cpu->reg[Rd], cpu->reg[Rs], mask, tmp);
      fprintf(log_f, "SHL: old PSW: %04hx\n", cpu->psw);
      // update Z, C, N flgs
      if(mask & tmp){ // set C flg !! rs >= 16
        set_flag(FLG_C);
      }
      else{
        reset_flag(FLG_C);
      }
      if(cpu->reg[Rd] == 0){ // set Z flg
        set_flag(FLG_Z);
      }
      else{
        reset_flag(FLG_Z);
      }
      if(cpu->reg[Rd] < 0){ // set N flg
        set_flag(FLG_N);
      }
      else{
        reset_flag(FLG_N);
      }
      fprintf(log_f, "SHL: old PSW: %04hx\n", cpu->psw);
      return 0;
    }
    case SHR:{
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4);
      unsigned int extRd = (unsigned int)((unsigned int)cpu->reg[Rd] << sizeof(Reg)*8);
      unsigned int extRs = (unsigned int)cpu->reg[Rs];
      unsigned int tmp = (unsigned int)(extRd >> extRs);
      cpu->reg[Rd] = cpu->reg[Rd] >> cpu->reg[Rs];
      unsigned int mask = (unsigned int)((unsigned int)(~ 0x0) >> sizeof(Reg)*8);
      fprintf(log_f, "SHR: [R%d]: %0hhx extRd: %08x\n[R%d]: %0hhx extRs: %08x\nmask: %08x\ntmp: %08x\n",Rd, Rs, cpu->reg[Rd], extRd, cpu->reg[Rs], extRs, mask, tmp);
      fprintf(log_f, "SHR: old PSW: %04hx\n", cpu->psw);
      // update Z, C, N flgs
      if(mask & tmp){ // set C flg !! [rs] >= 16
        set_flag(FLG_C);
      }
      else{
        reset_flag(FLG_C);
      }
      if(cpu->reg[Rd] == 0){ // set Z flg
        set_flag(FLG_Z);
      }
      else{
        reset_flag(FLG_Z);
      }
      if(cpu->reg[Rd] < 0){ // set N flg
        set_flag(FLG_N);
      }
      else{
        reset_flag(FLG_N);
      }
      fprintf(log_f, "SHR: new PSW: %04hx\n", cpu->psw);
      return 0;
    }
    case LDR:{
      //get registers
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4); 
      //get address mode and update
      Byte update_addr_mode = read_instr_byte(); //3rd byte
      Byte address_mode = (Byte)(update_addr_mode & 0x0f);
      Byte update = (Byte)(update_addr_mode >> 4);

      //get operand
      switch (address_mode){
        case IMMED:{
          //get instruction payload
          Data data = get_instr_payload();
          //store data in dest reg
          cpu->reg[Rd] = data;
          fprintf(log_f, "LDR: - immed - [R%d] = %04hx\n", Rd, cpu->reg[Rd]);
          return 0;
        }
        case MEMDIR:{
          //get address
          Address addr = (Address)get_instr_payload();
          //fetch operand mem[address]
          Data data = read_word_from_memory(addr);
          //store data in dest reg
          cpu->reg[Rd] = data;
          fprintf(log_f, "LDR: - memdir - [R%d] = mem[%04hx] = %04hx\n", Rd, addr, cpu->reg[Rd]);
          return 0;
        }
        case REGDIR:{
          Reg data = cpu->reg[Rs];
          cpu->reg[Rd] = data;
          fprintf(log_f, "LDR: - regdir - [R%d] = %04hx\n", Rd, cpu->reg[Rd]);
          return 0;
        }
        case REGDIRPOM:{
          //get offset
          Data offset = get_instr_payload();
          //get data
          Address addr = (Address)cpu->reg[Rs];
          addr = (Address)(addr + offset);

          Data data = read_word_from_memory(addr);
          //store to reg
          cpu->reg[Rd] = data;
          fprintf(log_f, "LDR: - regdirpom(PCREL) - [R%d] = mem[addr: %04hx + offs: %04hx (%d)] = %04hx\n", Rd, addr - offset, offset, offset, cpu->reg[Rd]);
          return 0;
        }
        case REGINDIR:{
          //get address
          Address addr = get_address_from_reg_and_update_reg(update, Rs);
          Data data = read_word_from_memory(addr);
          cpu->reg[Rd] = data;
          fprintf(log_f, "LDR: - regindir - [R%d] = mem[addr: %04hx] = %04hx\n", Rd, addr, cpu->reg[Rd]);
          return 0;
        }
        case REGINDIRPOM:{
          //get offset
          Data offset = get_instr_payload();
          //get address
          Address addr = get_address_from_reg_and_update_reg(update, Rs);
          //update address
          addr = (Address)(addr + offset);
          Data data = read_word_from_memory(addr);
          cpu->reg[Rd] = data;
          fprintf(log_f, "LDR: - regindirpom - [R%d] = mem[addr: %04hx + offs: %04hx (%d)] = %04hx\n", Rd, addr - offset, offset, offset, cpu->reg[Rd]);
          return 0;
        }
        default:{
          state = BAD_ADDR_MODE;
          printf("fatal error: unsupported address mode for LDR instruciton\n");
          return -1;
        }
      }
    }
    case STR:{
      //operand << regD
      Byte regs = read_instr_byte(); //2nd byte
      Byte Rs = (Byte)(regs & 0x0f); //odabrani reg
      Byte Rd = (Byte)(regs >> 4); 
      //get address mode and update
      Byte update_addr_mode = read_instr_byte(); //3rd byte
      Byte address_mode = (Byte)(update_addr_mode & 0x0f);
      Byte update = (Byte)(update_addr_mode >> 4);

      //get operand
      switch (address_mode){
        case IMMED:{
          printf("fatal error: invalid addressing mode for STR instruction\n");
          return -1;
        }
        case MEMDIR:{
          //get instr payload
          Address addr = (Address)get_instr_payload();
          //get data to store
          Data data = cpu->reg[Rd];
          if(write_word_to_memory(addr, data) == -1) return -1;
          fprintf(log_f, "STR: - memdir - mem[%04hx] = R%d: %04hx\n", addr, Rd, data);
          return 0;
        }
        case REGDIR:{
          //???
          Reg data = cpu->reg[Rd];
          cpu->reg[Rs] = data;
          fprintf(log_f, "STR: - regdir - [R%d] = R%d: %04hx\n", Rs, Rd, data);
          return 0;
        }
        case REGDIRPOM:{
          printf("fatal error: invalid addressing mode for STR instruction\n");
          return -1;
        }
        case REGINDIR:{
          //get address
          Address addr = get_address_from_reg_and_update_reg(update, Rs);
          //get data
          Data data = cpu->reg[Rd];
          //write data to memory
          if(write_word_to_memory(addr, data) == -1) return -1;
          fprintf(log_f, "STR: - regindir - mem[R%d: %04hx] = R%d: %04hx\n",Rs, addr, Rd,  data);
          return 0;
        }
        case REGINDIRPOM:{
          //get instr payload (offset)
          Data offset = get_instr_payload();
          //get address
          Address addr = get_address_from_reg_and_update_reg(update, Rs);
          //update address
          addr = (Address)(addr + offset);
          //get data
          Data data = cpu->reg[Rd];
          if(write_word_to_memory(addr, data) == -1) return -1;
          fprintf(log_f, "STR: - regindirpom - mem[R%d: %04hx + %04hx (%d) = %04hx] = R%d: %04hx\n", Rs, addr - offset, offset, offset, addr, Rd, data);
          return 0;
        }
        default:{
          state = BAD_ADDR_MODE;
          printf("fatal error: unsupported address mode for LDR instruciton\n");
          return -1;
        }
      }
    }
    default:{
      state = BAD_OP_CODE;
      printf("Emulated processor tried to execute unsuported instruction\n");
      return -1;
    } 
  }
}

struct termios terminal_old_settings;

int save_original_terminal_settings(){
  return tcgetattr(STDIN_FILENO, &terminal_old_settings);
}

void reset_original_terminal_settings(){
  int status =  tcsetattr(STDERR_FILENO, TCSAFLUSH, &terminal_old_settings);
  if(status != 0){
    printf("fatal error: could not restore original terminal settings\n");
  }
  else{
    //printf("Original terminal settings restored!\n");
  }
}

int set_custom_terminal_settings(struct termios *ct){
  //setup control mode
  ct->c_cflag &= ~CSIZE; //custom caracter size
  ct->c_cflag |= CS8; //set caracter size to 8 bits
  ct->c_cflag &= ~PARENB;

  //setup local mode
  //no echo -> ~ECHO
  ct->c_lflag &= ~ECHO;
  //no echo new line ~ECHONL
  ct->c_lflag &= ~ECHONL;
  //custom input ~ICANON
  ct->c_lflag &= ~ICANON;
  //no extended processing
  ct->c_lflag &= ~IEXTEN;

  //setup special caracters
  ct->c_cc[VMIN] = 0; //wait for 0 caracters before read enabled
  ct->c_cc[VTIME] = 0; //no timeout


  //reset original term settings at exit
  if(atexit(reset_original_terminal_settings) != 0){
    printf("fatal error: could not initialize terminal - could not set reset function at exit\n");
    return -1;
  }
  //set changed settings
  return tcsetattr(STDIN_FILENO, TCSAFLUSH, ct);
}

int init_terminal(){
  if(save_original_terminal_settings() != 0){
    printf("fatal error: could not initialize terminal - could not save original settings\n");
    return -1;
  }
  //printf("Original terminal settings saved!\n");

  struct termios terminal_custom_settings = terminal_old_settings;
  
  if(set_custom_terminal_settings(&terminal_custom_settings) != 0){
    printf("fatal error: could not initialize terminal - could not set custom settings\n");
    return -1;
  }
  //printf("Custom terminal settings set!\n");
  return 0;
}

char term_in_data;

void handle_interrupt_requests(){
  //check if interrupts allowed
  if(get_flag(FLG_I)) return;

  //check terminal IRQ
  if(read(STDIN_FILENO, &term_in_data, 1) == 1){
    //terminal IRQ == 1
    
    //write to term_in to be processed by interrupt
    write_word_to_memory(TERM_IN, (unsigned short)term_in_data);
    //jump to interrupt if flags are clear
    if(!get_flag(FLG_Tl)){
      jump_on_interrupt_routine(IVT_TERMINAL);
    }
  }
}

int emulate(){
  if(!cpu){
    printf("fatal error: CPU not initialized\n");
    return -1;
  }
  if(!mem){
    printf("fatal error: Memory not initialized\n");
    return -1;
  }
  if(init_terminal() != 0){
    printf("fatal error: could not initialize terminal\n");
    return -1;
  }
  load_program_start_address();
  state = RUNNING;

  while(state == RUNNING){ // execution loop - while running
    if(read_and_execute_instruction() == -1){
      jump_on_interrupt_routine(IVT_ERROR);
    }
    //read input to check if data was written to terminal
    handle_interrupt_requests();
  }

  fprintf(log_f, "END\n");

  return 0;
}