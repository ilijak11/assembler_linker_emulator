#include "../../inc/libs.h"

int open_files(FILE** in, FILE** out, char** in_file_name, char** out_file_name, int argc, char** argv){

  char* outfile = 0;
  char* infile = 0;

  if(argc < 2){
    printf("No input file!\n");
    return -1;
  }
  else if(argc > 4){
    printf("to many arguments!\n");
    return -1;
  }
  else if(argc == 3){
    printf("to few argumenst!\n");
    return -1;
  }
  else if(argc == 2){
    int len = strlen(argv[1])+1;
    printf("%d\n", len);
    infile = (char*)malloc(len*sizeof(char));
    outfile = (char*)malloc(len*sizeof(char));
    strcpy(infile, argv[1]);
    strcpy(outfile, argv[1]);
    printf("%s\n", infile);
    outfile[len-2] = 'o';
    printf("%s\n", outfile);
  }
  else if(argc == 4){
    if(strncmp("-o", argv[1], 2) != 0){
      printf("no option: %s\n option -o\n", argv[1]);
      return -1;
    }
    else{
      int lenout = strlen(argv[2]) + 1;
      int lenin = strlen(argv[3]) + 1;
      infile = (char*)malloc(lenin*sizeof(char));
      outfile = (char*)malloc(lenout*sizeof(char));
      strcpy(infile, argv[3]);
      strcpy(outfile, argv[2]);
    }
  }
  *in = fopen(infile, "r");
  if(!*in){
    printf("input file doesnt exist!\n");
    free(infile);
    free(outfile);
    return -1;
  }
  *out = fopen(outfile, "w");
  if(!*out){
    printf("output file creation failed!\n");
    free(infile);
    free(outfile);
    free(in);
    return -1;
  }

  *in_file_name = infile;
  *out_file_name = outfile;

  return 0;
}

int get_next_line(FILE* in, char* line){
    if(!line) return -2;
    if(feof(in)) return -1;
    char c = getc(in);
    int curs = 0;
    int not_empty = 0;
    while(c != '\n' && c != EOF){
      if(c != ' ' && c != '\t') not_empty = 1; //liniju sa space ili tab ne parsira
      line[curs++] = c;
      c = getc(in);
    }
    line[curs]='\0';
    return not_empty;
}