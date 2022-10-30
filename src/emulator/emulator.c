#include "../../inc/emulator.h"


FILE* log_f;

int main(int argc, char* argv[]){
  if(argc < 2){
    printf("missing argument: you must enter memory image file\n");
    return 1;
  }
  if(argc > 3){
    printf("too many arguments: usage ./emulator mem_img.hex [logfile.txt]\n");
    return 1;
  }
  //no log
  char* log_file = "/dev/null";
  if(argc == 3){
    log_file = argv[2];
  }
  FILE* in = fopen(argv[1], "r");
  if(!in){
    printf("fatal error: file does not exist\n");
    return -1;
  }
  FILE* out = fopen("mem_state.hex", "w");
  if(!in){
    printf("fatal error: could not create memory output file\n");
    return -1;
  }
  // /dev/null -> no log
  log_f = fopen(log_file, "w");
  if(!in){
    printf("fatal error: could not create log file\n");
    return -1;
  }
  if(init_mem(in) == -1) return -1;
  if(init_cpu() == -1) return -1;
  //mem_dump();
  emulate();
  print_cpu_state();
  mem_dump(out);
  fclose(out);
  fclose(log_f);
  destroy_mem();
  destroy_cpu();
}