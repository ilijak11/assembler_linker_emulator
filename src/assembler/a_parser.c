#include "../../inc/parser.h"


int parse_line(struct line** data, char* line){
  int i;
	yyscan_t scanner;
  YY_BUFFER_STATE buf;

	if ((i = yylex_init(&scanner)) != 0) exit(i);
  
  buf = yy_scan_string(line, scanner);

	int e = yyparse(data, scanner);

  yy_delete_buffer(buf, scanner);
	yylex_destroy(scanner);
	return e;
}

void print_line_data(struct line* data){
			if(!data){
			printf("neradi!\n");
			return;
		}
		switch(data->type){
			case INSTR:{
					printf("this is an instruction!\n");
					switch(data->content.instr->type){
						case PUSH: {
							printf("push r%d\n", data->content.instr->op1.reg);
							break;
						}
						case LDR: {
							struct instruction* instr = data->content.instr;
							printf("ldr load in reg r%d ", instr->op1.reg);
							if(instr->op2.data_op->type == PCREL){
								struct data_operand* data_op = instr->op2.data_op;
								printf("sym: %s using pcrel addressing\n", data_op->operand.sym);
							}
							else if(instr->op2.data_op->type == REGINDIRPOM_SYM){
								struct data_operand* data_op = instr->op2.data_op;
								printf("regindir with offs from address [r%d + %s]\n", data_op->operand.reg, data_op->offs.sym);
							}
							break;
						}
						default: printf("some other instruction!\n"); break;
					}
					break;
			} 
			case DIR: {
				printf("this is a directive!\n");
				switch(data->content.dir->type){
					case EXTERN: {
						printf("directive: .extern - extern symbols: \n");
						struct symbol_list_node* head = data->content.dir->param.sym_list;
						int num = 0;
						while(head != 0){
							printf("sym: %d - %s\n", num++, head->sym);
							head = head->next;
						}
						break;
					}
					case EQU: {
						printf("directive: .equ \n");
						printf("sym: %s exprvalue: %d\n", data->content.dir->param.sym, data->content.dir->exprval);
						break;
					}
					case ASCII: {
						printf("directive: .ascii\n");
						printf("string: %s\n", data->content.dir->param.string);
						break;
					}
					default: printf("some other directive!\n"); break;
				}
				break;
			}
			case LAB_DIR: {
				printf("lab: %s ", data->lab->sym);
				printf("this is a directive!\n");
				switch(data->content.dir->type){
					case EXTERN: {
						printf("directive: .extern - extern symbols: \n");
						struct symbol_list_node* head = data->content.dir->param.sym_list;
						int num = 0;
						while(head != 0){
							printf("sym: %d - %s\n", num++, head->sym);
							head = head->next;
						}
						break;
					}
					case WORD: {
						printf("directive: .word \n");
						struct mix_list_node* head = data->content.dir->param.mix_list;
						int num = 0;
						while(head != 0){
							switch(head->type){
								case MLN_LIT: printf("%d lit: %#x\n", num++, head->data.lit); break;
								case MLN_SYM: printf("%d sym: %s\n", num++, head->data.sym); break;
							}
							head = head->next;
						}
						break;
					}
					case EQU: {
						printf("directive: .equ \n");
						printf("sym: %s exprvalue: %d\n", data->content.dir->param.sym, data->content.dir->exprval);
						break;
					}
					case ASCII: {
						printf("directive: .ascii\n");
						printf("string: %s\n", data->content.dir->param.string);
						break;
					}
					default: printf("some other directive!\n"); break;
				}
				break;
			}
			case COMM: printf("this is a comment!\n"); break;
			case LAB: printf("this is a label\nlab: %s\n", data->lab->sym); break;
			default: printf("lol\n");
		}
		printf("---------------------\n");
		free_line(data);
}