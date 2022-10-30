%{
	#define YY_EXIT_FAILURE ((void)yyscanner, EXIT_FAILURE)
	#define _XOPEN_SOURCE 600
	#include <stdio.h>
	#include <stdlib.h>
	#include "as.tab.h"

	int yyerror(const char *msg);
%}

/* don't generate functions we don't need */
%option nounput noinput noyywrap


/* generate a scanner that's thread safe */
%option reentrant bison-bridge
%option header-file="./inc/as.lex.h"


sym \.?[_a-zA-Z][_a-zA-Z0-9]*
str \".*\"
hexliteral 0x[0-9a-fA-F]{1,4}
decliteral [0-9]+
comment #.*

%%
[ \t]				;
".global"   {return DIR_GLOB;}
".extern"   {return DIR_EXT;}
".section"  {return DIR_SECT;}
".word"     {return DIR_WORD;}
".skip"     {return DIR_SKIP;}
".ascii"    {return DIR_ASCII;}
".equ"      {return DIR_EQU;}
".end"      {return DIR_END;}
"halt"      {return I_HALT;}
"int"       {return I_INT;}
"iret"      {return I_IRET;}
"call"      {return I_CALL;}
"ret"       {return I_RET;}
"jmp"       {return I_JMP;}
"jeq"       {return I_JEQ;}
"jne"       {return I_JNE;}
"jgt"       {return I_JGT;}
"push"      {return I_PUSH;}
"pop"       {return I_POP;}
"xchg"      {return I_XCHG;}
"add"       {return I_ADD;}
"sub"       {return I_SUB;}
"mul"       {return I_MUL;}
"div"       {return I_DIV;}
"cmp"       {return I_CMP;}
"not"       {return I_NOT;}
"and"       {return I_AND;}
"or"        {return I_OR;}
"xor"       {return I_XOR;}
"test"      {return I_TEST;}
"shl"       {return I_SHL;}
"shr"       {return I_SHR;}
"ldr"       {return I_LDR;}
"str"       {return I_STR;}
"r0"				{yylval->regnum = 0; return R0;}
"r1"				{yylval->regnum = 1; return R1;}
"r2"				{yylval->regnum = 2; return R2;}
"r3"				{yylval->regnum = 3; return R3;}
"r4"				{yylval->regnum = 4; return R4;}
"r5"				{yylval->regnum = 5; return R5;}
"r6"				{yylval->regnum = 6; return R6;}
"r7"				{yylval->regnum = 7; return R7;}
"pc"        {yylval->regnum = 7; return R7;}
"sp"				{yylval->regnum = 6; return R6;}
"psw"       {yylval->regnum = 8; return R8;}
"$"         {return DOLL;}
"%"         {return PERC;}
"["         {return LSQRB;}
"]"         {return RSQRB;}
"+"         {return PLUS;}
"*"         {return STAR;}
","					{return COMMA;}
":"					{return COLON;}
"-"					{return MINUS;}
"/"					{return DIVIDE;}
{comment}				{return COMMENT;}
{str}       {yylval->string = strdup(yytext); return STRING;}
{sym}				{yylval->sym = strdup(yytext); return SYMBOL;}
{hexliteral}		{long lit = strtol(yytext, 0, 16); yylval->lit = (int)lit; return HEXLIT;}
{decliteral}		{long lit = strtol(yytext, 0, 10); yylval->lit = (int)lit; return DECLIT;}
%%

