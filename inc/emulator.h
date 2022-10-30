#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMCAP 0x10000
#define MAXADDR 0xffff

//flags
#define FLG_Z 0x0001
#define FLG_O 0x0002
#define FLG_C 0x0004
#define FLG_N 0x0008
#define FLG_Tr 0x2000
#define FLG_Tl 0x4000
#define FLG_I 0x8000

enum flag {Z, O, C, N, Tr, Tl, I};

//regs
#define REGNUM 8

#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7

#define PC 7
#define SP 6

//IVT entries
#define IVTP 0x0000

#define IVT_ENTRIES_NUM 8

#define IVT_START 0
#define IVT_ERROR 1
#define IVT_TIMER 2
#define IVT_TERMINAL 3
#define IVT_USER0 4
#define IVT_USER1 5
#define IVT_USER2 6
#define IVT_USER3 7

//Terminal - mapped registers
#define TERM_OUT 0xff00
#define TERM_IN 0xff02

//Instructions
#define OC_HALT 0x00 // ok
#define OC_INT 0x10 // ok
#define OC_IRET 0x20 // ok
#define OC_CALL 0x30 // ok 
#define OC_RET 0x40 // ok
#define OC_JMP 0x50 // ok 
#define OC_JEQ 0x51 // ok 
#define OC_JNE 0x52 // ok 
#define OC_JGT 0x53 // ok 
#define OC_XCHG 0x60 // ok
#define OC_ADD 0x70 // ok
#define OC_SUB 0x71 // ok
#define OC_MUL 0x72 // ok
#define OC_DIV 0x73 // ok
#define OC_CMP 0x74 // ok
#define OC_NOT 0x80 // ok
#define OC_AND 0x81 // ok
#define OC_OR 0x82 // ok
#define OC_XOR 0x83 // ok
#define OC_TEST 0x84 // ok
#define OC_SHL 0x90 // ok
#define OC_SHR 0x91 // ok
#define OC_LDR 0xa0 // ok
#define OC_STR 0xb0 // ok

enum instr {HALT = 0, INT, IRET, CALL, RET, JMP, JEQ, JNE, JGT, XCHG,
            ADD, SUB, MUL, DIV, CMP, NOT, AND, OR, XOR, TEST, SHL, SHR, LDR, STR, BAD_INSTR};

//update
#define NOUPDATE 0x00
#define PREDEC2 0x01
#define PREINC2 0x02
#define POSTDEC2 0x03
#define POSTINC2 0x04

//address mode
#define IMMED 0x00 //k k
#define REGDIR 0x01 //k k
#define REGINDIR 0x02 //k k
#define REGINDIRPOM 0x03 //k
#define MEMDIR 0x04 //k k
#define REGDIRPOM 0x05 //k k (PCREL)

//emulation state
enum state {RUNNING, STOPPED, BAD_OP_CODE, BAD_ADDR_MODE, BAD_UPDATE};
enum state state;

//data types
typedef char Byte;
typedef unsigned short Address;
typedef short Data;
typedef short Reg;
typedef short PSW;

Byte* mem;

typedef struct cpu{
  Reg reg[8];
  Reg* pc;
  Reg* sp;
  PSW psw;
} CPU;

CPU* cpu;
Address* ivt;
FILE* log_f;

int init_mem();
int init_cpu();
void destroy_cpu();
void destroy_mem();

void set_flag(short flag);
void reset_flag(short flag);
Byte get_flag(short flag);

void print_cpu_state();
void mem_dump();

int emulate();