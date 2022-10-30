
ASS_SRC_DIR = ./src/assembler
LINK_SRC_DIR = ./src/linker
EMULATOR_SRC_DIR = ./src/emulator

all: assembler linker emulator

$(ASS_SRC_DIR)/as.tab.c ./inc/as.tab.h: ./misc/as.y
	bison -o $(ASS_SRC_DIR)/as.tab.c --defines=./inc/as.tab.h  $^

$(ASS_SRC_DIR)/lex.yy.c: ./misc/as.l ./inc/as.tab.h
	flex -o $(ASS_SRC_DIR)/lex.yy.c --header-file=./inc/as.lex.h ./misc/as.l 

assembler: $(ASS_SRC_DIR)/assembler.c $(ASS_SRC_DIR)/a_parser.c $(ASS_SRC_DIR)/lex.yy.c $(ASS_SRC_DIR)/as.tab.c $(ASS_SRC_DIR)/filehandler.c $(ASS_SRC_DIR)/assemble.c   
	gcc -o $@ $^ -I./inc

linker: $(LINK_SRC_DIR)/linker.c $(LINK_SRC_DIR)/l_parser.c $(LINK_SRC_DIR)//link.c
	gcc -o $@ $^ -I./inc

emulator: $(EMULATOR_SRC_DIR)/emulator.c $(EMULATOR_SRC_DIR)/mem.c $(EMULATOR_SRC_DIR)/cpu.c $(EMULATOR_SRC_DIR)/emulate.c
	gcc -o $@ $^ -I./inc

clear: 
	rm ./assembler
	rm ./linker
	rm ./emulator
	rm $(ASS_SRC_DIR)/as.tab.c
	rm $(ASS_SRC_DIR)/lex.yy.c
	rm ./inc/as.lex.h
	rm ./inc/as.tab.h	