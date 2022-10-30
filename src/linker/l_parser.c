
#include "../../inc/linker.h"

char* dupstr(char* str){
  int len = sizeof(str) + 1;
  char* dupstr = (char*)malloc(len*sizeof(char));
  if(!dupstr) return 0;
  strcpy(dupstr, str);
  return dupstr;
}

Parsed_command* init_object(){
  Parsed_command* parsed_command = (Parsed_command*)malloc(sizeof(Parsed_command));
  if(!parsed_command){
    printf("fatal error: no memory\n");
    exit(1);
  }
  parsed_command->type = NOTYPE;
  parsed_command->out_file_name = 0;
  parsed_command->file_list = 0;
  parsed_command->place_list = 0;
  return parsed_command;
}

void delete_object(Parsed_command* pc){
  if(pc->out_file_name) free(pc->out_file_name);
  if(pc->file_list){
    File_node* head = pc->file_list;
    File_node* old = 0;
    while(head){
      old = head;
      head = head->next;
      if(old->file_name) free(old->file_name);
      if(old->file) fclose(old->file);
      free(old);
    }
  }
  if(pc->place_list){
    Place_node* head = pc->place_list;
    Place_node* old = 0;
    while(head){
      old = head;
      head = head->next;
      if(old->sect) free(old->sect);
      free(old);
    }
  }
}

void print_parsed_command(Parsed_command* pc){
  switch (pc->type){
    case HEX:{
      printf("type: hex\n");
      break;
    }
    case RELOCATABLE:{
      printf("type: relocatable\n");
      break;    
    }
    default:{
      printf("type: notype\n");
      break; 
    }
  }
  if(pc->out_file_name) printf("name: %s\n", pc->out_file_name);
  else printf("name: null\n");
  File_node* file_head = pc->file_list;
  printf("files: ");
  while(file_head){
    printf("%s ", file_head->file_name);
    file_head = file_head->next;
  }
  printf("\nplaces: ");
  Place_node* place_head = pc->place_list;
  while(place_head){
    printf("%s-%x ", place_head->sect, place_head->offset);
    place_head = place_head->next;
  }
  printf("\n");
}

Parsed_command* parse_comand_line_arguments(int argc, char* argv[]){
  Parsed_command* parsed_command = init_object();

  for(int i = 1; i<argc; i++){
    if(strcmp(argv[i], CHEX) == 0){
      if(parsed_command->type != NOTYPE){
        printf("fatal error: bad argument - either -hex or -relocatable\n");
        delete_object(parsed_command);
        return 0;
      }
      else{
        parsed_command->type = HEX;
      }
    }
    else if(strcmp(argv[i], CRELOCATABLE) == 0){
      if(parsed_command->type != NOTYPE){
        printf("fatal error: bad argument - either -hex or -relocatable\n");
        delete_object(parsed_command);
        return 0;
      }
      else{
        parsed_command->type = RELOCATABLE;
      }
    }
    else if(strcmp(argv[i], CNAME) == 0){
      if(parsed_command->out_file_name){
        printf("fatal error: bad argument - output file name already set\n");
        delete_object(parsed_command);
        return 0;
      }
      else{
        parsed_command->out_file_name = dupstr(argv[++i]);
      }
    }
    else if(strncmp(argv[i], CPLACE, 7) == 0){
      char* sec = &argv[i][7];
      int len = 0;
      while(*(sec+len) != '@') len++;
      Place_node* node = (Place_node*)malloc(sizeof(Place_node));
      node->sect = (char*)malloc((len+1)*sizeof(char));
      strncpy(node->sect, sec, len);
      node->sect[len] = '\0';
      node->next = 0;
      node->offset = (int)strtol(sec+len+1, 0, 16);

      if(!parsed_command->place_list){
        parsed_command->place_list = node;
      }
      else{
        Place_node* head, *curr, *prev;
        head = curr = parsed_command->place_list;
        prev = 0;
        while(curr && node->offset > curr->offset) {
          prev = curr;
          curr = curr->next;
        }
        if(!prev){
          if(node->offset > curr->offset){
            node->next = head->next;
            head->next = node;
          }
          else{
            node->next = head;
            parsed_command->place_list = node;
          }
        }
        else{
          node->next = prev->next;
          prev->next = node;
        }
      }
    }
    else{
      char* file = argv[i];
      int len = strlen(file);

      File_node* node = (File_node*)malloc(sizeof(File_node));
      node->file_name = dupstr(file);
      node->file = fopen(file, "r");
      if(!node->file){
        printf("fatal error: bad argument - file does not exist\n");
        delete_object(parsed_command);
        return 0;
      }

      if(!parsed_command->file_list){
        parsed_command->file_list = node;
      }
      else{
        File_node* head = parsed_command->file_list;
        while(head->next) head = head->next;
        head->next = node;
      }
    }
  }
  return parsed_command;
}
