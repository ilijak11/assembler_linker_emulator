#include <stdio.h>
#include <stdlib.h>

#include "as.tab.h"
#include "as.lex.h"


int parse_line(struct line** data, char* line);
void print_line_data(struct line* line);