%{
/***************************
 * Ronnie Keating
 * CS445
 * Assignment 2
 * parser.l
 ***************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// put this before token.tab.h!!!
#include "scanType.h"
#include "treeNodes.h"

// get definitions from bison
#include "parser.tab.h"

int line=1;
int numErrors = 0;
int numWarnings = 0;

//I use this to parse through a number in the case of leading zeros
int numFunction(char *string)
{
    int i = 0, j = 0, length = strlen(string);
    char temp[length];
    while(string[i] == '0') i++;
    while(i <= length) {
        temp[j] = string[i];
        j++;
        i++;
    }
    temp[j] = '\0';
    int num = atoi(temp);
    return num;
}

// return tokenclass and fill in yylval
int setValue(int linenum, int tokenClass, char *svalue)
{
    yylval.tokenData = new TokenData;
    // yylval.tokenData->tokenclass = tokenClass;
    
    // fill it up
    yylval.tokenData->linenum = linenum;
    yylval.tokenData->tokenstr = strdup(svalue);
    if(tokenClass == BOOLCONST) {
        if(svalue[0] == 't') yylval.tokenData->numValue = 1;
        else yylval.tokenData->numValue = 0;
    }
    else if(tokenClass == NUMCONST) {
        yylval.tokenData->numValue = numFunction(svalue);
    }
    else yylval.tokenData->numValue = atoi(svalue);

    if (tokenClass == ID) {
        yylval.tokenData->idIndex = svalue[0]-'a';
    }

    return tokenClass;
}

//make keywords all upper
char *toUpper(char *string)
{
    int i, length= strlen(string);
    for(i = 0; i < length; i++) string[i] -= 32;
    return string;
}

//  ORIGINAL CHAR (\\.|[^'\\\n\0])


%}

%option noyywrap

COMMENTS        "//".*
LETTER          [a-zA-Z_]
DIGIT           [0-9]
CHARACTER       (\\.|[^'\\\n\0])
CHARACTERS      (\\.|[^"\\\n\0])
LETDIG          {DIGIT}|{LETTER}
ID              {LETTER}{LETDIG}*
ZEROHANDLER     0+{DIGIT}+
NUMCONST        ([1-9]{DIGIT}*)|0
CHARCONST       \'{CHARACTER}\'
STRINGCONST     \"{CHARACTERS}*\"
LONGCHARCONST	\'{CHARACTER}*\'
WHITESPACE      [ \t]
STATIC          static
INT             int
BOOL            bool
CHAR            char
IF              if
ELSE            else
WHILE           while
FOR             for
RETURN          return
BREAK           break
IN              in
TRUE            true
FALSE           false
ADDASS          \+\=
SUBASS          \-\=
DIVASS          \/\=
MULASS          \*\=
EQ              \=\=
GEQ             \>\=
LEQ             \<\=
NEQ             \!\=
DEC             \-\-
INC             \+\+
COMMA           \,
SEMICOLON       \;
COLON           \:
ADD             \+
SUB             \-
MUL             \*
DIV             \/
EQUAL           \=
MOD             \%
AND             \&
QUESTION        \?
DOESNOT         \!
LESS            \<
GREAT           \>
OR              \|
RIGHTPARAN      \)
LEFTPARAN       \(
RIGHTBRACK      \]
LEFTBRACK       \[
RIGHTSQBRACK    \}
LEFTSQBRACK     \{

%%
{COMMENTS}      ;
{TRUE}          { return setValue(line, TRUE, yytext); }
{FALSE}         { return setValue(line, FALSE, yytext); }
{STATIC}        { return setValue(line, STATIC, toUpper(yytext)); }
{INT}           { return setValue(line, INT, toUpper(yytext)); }
{BOOL}          { return setValue(line, BOOL, toUpper(yytext)); }
{CHAR}          { return setValue(line, CHAR, toUpper(yytext)); }
{IF}            { return setValue(line, IF, toUpper(yytext)); }
{ELSE}          { return setValue(line, ELSE, toUpper(yytext)); }
{WHILE}         { return setValue(line, WHILE, toUpper(yytext)); }
{FOR}           { return setValue(line, FOR, toUpper(yytext)); }
{RETURN}        { return setValue(line, RETURN, toUpper(yytext)); }
{BREAK}         { return setValue(line, BREAK, toUpper(yytext)); }
{IN}            { return setValue(line, IN, toUpper(yytext)); }
{CHARCONST}     { return setValue(line, CHARCONST, yytext); }
{LONGCHARCONST}	{ if(strlen(yytext) > 2) { printf("WARNING(%d): character is %ld characters long and not a single character: '%s'.  The first char will be used.\n", line, strlen(yytext) - 2, yytext); char temp[3]; temp[0] = '\''; temp[2] = '\''; temp[1] = yytext[1]; numWarnings++; return setValue(line, CHAR, temp); } else { printf("ERROR(%d): Empty character ''.  Characters ignored.\n", line); numErrors++; } }
{ID}            { return setValue(line, ID, yytext); }
{NUMCONST}      { return setValue(line, NUMCONST, yytext); }
{STRINGCONST}   { return setValue(line, STRINGCONST, yytext); }
{ADDASS}        { return setValue(line, ADDASS, yytext); }
{SUBASS}        { return setValue(line, SUBASS, yytext); }
{DIVASS}        { return setValue(line, DIVASS, yytext); }
{MULASS}        { return setValue(line, MULASS, yytext); }
{EQ}            { return setValue(line, EQ, yytext); }
{GEQ}           { return setValue(line, GEQ, yytext); }
{LEQ}           { return setValue(line, LEQ, yytext); }
{NEQ}           { return setValue(line, NEQ, yytext); }
{DEC}           { return setValue(line, DEC, yytext); }
{INC}           { return setValue(line, INC, yytext); }
{COMMA}         { return setValue(line, COMMA, yytext); }
{SEMICOLON}     { return setValue(line, SEMICOLON, yytext); }
{COLON}         { return setValue(line, COLON, yytext); }
{ADD}           { return setValue(line, ADD, yytext); }
{SUB}           { return setValue(line, SUB, yytext); }
{MUL}           { return setValue(line, MUL, yytext); }
{DIV}           { return setValue(line, DIV, yytext); }
{EQUAL}         { return setValue(line, EQUAL, yytext); }
{MOD}           { return setValue(line, MOD, yytext); }
{AND}           { return setValue(line, AND, yytext); }
{QUESTION}      { return setValue(line, QUESTION, yytext); }
{DOESNOT}       { return setValue(line, DOESNOT, yytext); }
{LESS}          { return setValue(line, LESS, yytext); }
{GREAT}         { return setValue(line, GREAT, yytext); }
{OR}            { return setValue(line, OR, yytext); }
{RIGHTPARAN}    { return setValue(line, RIGHTPARAN, yytext); }
{LEFTPARAN}     { return setValue(line, LEFTPARAN, yytext); }
{RIGHTBRACK}    { return setValue(line, RIGHTBRACK, yytext); }
{LEFTBRACK}     { return setValue(line, LEFTBRACK, yytext); }
{RIGHTSQBRACK}  { return setValue(line, RIGHTSQBRACK, yytext); }
{LEFTSQBRACK}   { return setValue(line, LEFTSQBRACK, yytext); }
{WHITESPACE}    ;
\n              { line++; }

.               { printf("ERROR(%d): Invalid or misplaced input character: '%s'. Character Ignored.\n", line, yytext); numErrors++; }
%%
// no main
