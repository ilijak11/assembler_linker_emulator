#include <stdio.h>
#include "../../inc/linker.h"



int main(int argc, char* argv[]){
  //printf("Hello linker\n");
  Parsed_command* pc = parse_comand_line_arguments(argc, argv);
  if(!pc) return -1;
  //print_parsed_command(pc);

  if(link(pc) != 0){
    return -1;
  }

  delete_object(pc);
  
  return 0;
}