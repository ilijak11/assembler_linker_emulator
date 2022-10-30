
#include "../../inc/libs.h"



int main(int argc, char* argv[]){

  FILE* in;
  FILE* out;
  char* infile;
  char* outfile;

  if(open_files(&in, &out, &infile, &outfile, argc, argv) != 0) exit(1);

  /*
  if(parser_test(in) != 0){
    printf("assembling stopped!\n");
  }
  else{
    printf("assembler finished successfully!\n");
  }
  */
  int e = assemble(in, out, outfile);
  if(e == -1){
    printf("assembling stopped!\n");
  }
  else if(e == -2){
    printf("assembling finished unsuccessfully\n");
  }
  else{
    //printf("assembling finished successfully!\n");
  }
  
  fclose(in);
  fclose(out);
  free(infile);
  free(outfile);

  return 0;
}