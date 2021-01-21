%{
/***************************
 * Ronnie Keating
 * CS445
 * Assignment 2
 * parser.y
 ***************************/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "scanType.h"
#include "treeNodes.h"
#include "semantics.h"
#include "yyerror.h"
#include "gencode.h"

extern int yylex();    
extern int yydebug;  

FILE *code;

// Used for flags from command line
bool p;
bool m;

//FUNCTIONS
void parseFlags(char * string);
void trashCollector();

#define YYERROR_VERBOSE
/*void yyerror(const char *msg) {
    printf("ERROR(PARSER): %s\n", msg);
}*/

//Declare a TreeNode pointer (root)
TreeNode *syntaxTree;

%}

// this is the type of yylval
%union {
    TokenData *tokenData;       //token data
    TreeNode *tree;             // treenode type
    ExpType type;               // exp type type
}

%type <tree> program declList decl varDecl scopeVarDecl varDeclList
%type <tree> varDeclInit varDeclId matched unmatched
%type <tree> funDecl params paramList paramTypeList paramIdList paramId
%type <tree> stmt expressionStmt compoundStmt localDecl stmtList
%type <tree> iterationStmt returnStmt breakStmt
%type <tree> exp simpleExp andExp unaryRelExp relExp
%type <tree> sumExp mulExp unaryExp factor
%type <tree> mutable immutable call args argList constant
%type <type> typeSpec

%type <tokenData> sumop mulop relop unaryop

%token <tokenData> ID NUMCONST CHARCONST STRINGCONST BOOLCONST
//KEYWORDS
%token <tokenData> TRUE FALSE IF ELSE STATIC INT BOOL CHAR WHILE FOR RETURN BREAK IN
//SINGLE OPERATORS
%token <tokenData> SUB MUL QUESTION DOESNOT ADD DIV SEMICOLON COMMA COLON
%token <tokenData> EQUAL OR AND LESS GREAT MOD CHSIGN
//CONTAINERS
%token <tokenData> LEFTPARAN RIGHTPARAN LEFTBRACK RIGHTBRACK LEFTSQBRACK RIGHTSQBRACK
//DOUBLE OPERATORS (Dont need comments anymore I think)
%token <tokenData> ADDASS SUBASS MULASS DIVASS EQ GEQ LEQ NEQ DEC INC
//Dont know what to do with yet
%token <tokenData> BLANK LCHARCONST

%%
//-----------------------------------------------------------------

program: declList                                                   { syntaxTree = $1; }
       ;

declList: declList decl                                             { $$ = addSibling($1, $2); }
        | decl                                                      { $$ = $1; }
        ;

decl: varDecl                                                       { $$ = $1; }
    | funDecl                                                       { $$ = $1; }
    | error                                                         { $$ = NULL; }
    ;

//-----------------------------------------------------------------

varDecl: typeSpec varDeclList SEMICOLON                             { $$ = $2; if($2 != NULL) { $$->expType = $1; typeSpecRoutine($$, $1, 0); yyerrok; } }
       | error varDeclList SEMICOLON                                { $$ = NULL; }
       | typeSpec error SEMICOLON                                   { $$ = NULL; yyerrok; }
       ;

scopeVarDecl: STATIC typeSpec varDeclList SEMICOLON                 { $$ = $3; if($3 != NULL) { $$->expType = $2; typeSpecRoutine($$, $2, 1); yyerrok; } }
            | typeSpec varDeclList SEMICOLON                        { $$ = $2; if($2 != NULL) { $$->expType = $1; typeSpecRoutine($$, $1, 0); yyerrok; } }
            | error varDeclList SEMICOLON                           { $$ = NULL; yyerrok; }
            ;

varDeclList: varDeclList COMMA varDeclInit                          { $$ = addSibling($1, $3); yyerrok; }
           | varDeclList COMMA error                                { $$ = NULL; }
           | varDeclInit                                            { $$ = $1; }
           | error                                                  { $$ = NULL; }
           ;

varDeclInit: varDeclId                                              { $$ = $1; }
           | varDeclId COLON simpleExp                              { $$ = $1; if($1 != NULL) { $$->child[0] = $3; $1->isInit = true; $1->isColon = true; } }
           | error COLON simpleExp                                  { $$ = NULL; yyerrok; }
           | varDeclId COLON error                                  { $$ = NULL; }
           ;

varDeclId: ID                                                       { $$ = newDeclNode(VarK, $1); }
         | ID LEFTBRACK NUMCONST RIGHTBRACK                         { $$ = newDeclNode(VarK, $1); $$->isArray = true; $$->attr.value = $3->numValue; }
         | ID LEFTBRACK error                                       { $$ = NULL; }
         | error RIGHTBRACK                                         { $$ = NULL; yyerrok; }
         ;

typeSpec: INT                                                       { $$ = Integer; }
        | BOOL                                                      { $$ = Boolean; }
        | CHAR                                                      { $$ = Char; }
        ;

//-----------------------------------------------------------------

funDecl: typeSpec ID LEFTPARAN params RIGHTPARAN stmt               { $$ = newDeclNode(FuncK, $2); $$->expType = $1; $$->child[0] = $4; $$->child[1] = $6; }
       | ID LEFTPARAN params RIGHTPARAN stmt                        { $$ = newDeclNode(FuncK, $1); $$->child[0] = $3; $$->child[1] = $5; }
       | typeSpec error                                             { $$ = NULL; }
       | typeSpec ID LEFTPARAN error                                { $$ = NULL; }
       | typeSpec ID LEFTPARAN params RIGHTPARAN error              { $$ = NULL; }
       | ID LEFTPARAN error                                         { $$ = NULL; }
       | ID LEFTPARAN params RIGHTPARAN error                       { $$ = NULL; }
       ;

params: paramList                                                   { $$ = $1; }
      |                                                             { $$ = NULL; }
      ;

paramList: paramList SEMICOLON paramTypeList                        { $$ = addSibling($1, $3); }
         | paramTypeList                                            { $$ = $1; }
         | paramList SEMICOLON error                                { $$ = NULL; }
         | error                                                    { $$ = NULL; }
         ;

paramTypeList: typeSpec paramIdList                                 { $$ = $2; if($2 != NULL) { $$->expType = $1; typeSpecRoutine($$, $1, 0); } }
             | typeSpec error                                       { $$ = NULL; }
             ;

paramIdList: paramIdList COMMA paramId                              { $$ = addSibling($1, $3); yyerrok; }
           | paramId                                                { $$ = $1; }
           | paramIdList COMMA error                                { $$ = NULL; } //seg fault around here
           | error                                                  { $$ = NULL; }
           ;

paramId: ID                                                         { $$ = newExpNode(ParamIdK, $1); $$->isArray = false; }
       | ID LEFTBRACK RIGHTBRACK                                    { $$ = newExpNode(ParamIdK, $1); $$->isArray = true; }
       | error RIGHTBRACK                                           { yyerrok; $$ = NULL; }
       ;

//-----------------------------------------------------------------

stmt: matched                                                       { $$ = $1; }
    | unmatched                                                     { $$ = $1; }
    ;

matched: IF LEFTPARAN simpleExp RIGHTPARAN matched ELSE matched     { $$ = newStmtNode(IfK, $1); $$->child[0] = $3; $$->child[1] = $5; $$->child[2] = $7; }
       | WHILE LEFTPARAN simpleExp RIGHTPARAN matched               { $$ = newStmtNode(WhileK, $1); $$->child[0] = $3; $$->child[1] = $5; }
       | FOR LEFTPARAN ID IN ID RIGHTPARAN matched                  { $$ = newStmtNode(ForK, $1); $$->child[0] = newDeclNode(VarK, $3); $$->child[1] = newExpNode(IdK, $5); $$->child[2] = $7; $$->child[0]->expType = UndefinedType; }
       | expressionStmt                                             { $$ = $1; }
       | compoundStmt                                               { $$ = $1; }
       | returnStmt                                                 { $$ = $1; }
       | breakStmt                                                  { $$ = $1; }
       | IF error                                                   { $$ = NULL; }
       | IF error ELSE matched                                      { $$ = NULL; yyerrok; }
       | IF error RIGHTPARAN matched ELSE matched                   { $$ = NULL; yyerrok; }
       | WHILE error RIGHTPARAN matched                             { $$ = NULL; yyerrok; }
       | WHILE error                                                { $$ = NULL; }
       | FOR error RIGHTPARAN matched                               { $$ = NULL; yyerrok; }
       | FOR error                                                  { $$ = NULL; }
       ;

unmatched: IF LEFTPARAN simpleExp RIGHTPARAN matched                { $$ = newStmtNode(IfK, $1); $$->child[0] = $3; $$->child[1] = $5; }
         | IF LEFTPARAN simpleExp RIGHTPARAN unmatched              { $$ = newStmtNode(IfK, $1); $$->child[0] = $3; $$->child[1] = $5; }
         | IF LEFTPARAN simpleExp RIGHTPARAN matched ELSE unmatched { $$ = newStmtNode(IfK, $1); $$->child[0] = $3; $$->child[1] = $5; $$->child[2] = $7; }
         | iterationStmt                                            { $$ = $1; }
         | IF error RIGHTPARAN stmt                                 { $$ = NULL; yyerrok; }
         | IF error ELSE unmatched                                  { $$ = NULL; yyerrok; }
         | IF error RIGHTPARAN matched ELSE unmatched               { $$ = NULL; yyerrok; }
         | WHILE error RIGHTPARAN unmatched                         { $$ = NULL; yyerrok; }
         | FOR error RIGHTPARAN unmatched                           { $$ = NULL; yyerrok; }
         ;

expressionStmt: exp SEMICOLON                                       { $$ = $1; yyerrok; }
              | error SEMICOLON                                     { $$ = NULL; yyerrok; }
              | SEMICOLON                                           { $$ = NULL; yyerrok; }
              ;

compoundStmt: LEFTSQBRACK localDecl stmtList RIGHTSQBRACK           { $$ = newStmtNode(CompoundK, $1); $$->isComp = true; $$->child[0] = $2; $$->child[1] = $3; yyerrok; }
            | LEFTSQBRACK error stmtList RIGHTSQBRACK               { $$ = NULL; yyerrok; }
            | LEFTSQBRACK localDecl error RIGHTSQBRACK              { $$ = NULL; yyerrok; }
            ;

localDecl: localDecl scopeVarDecl                                   { $$ = addSibling($1, $2); }
         |                                                          { $$ = NULL; }
         ;

stmtList: stmtList stmt                                             { $$ = addSibling($1, $2); }
        |                                                           { $$ = NULL; }
        ;

iterationStmt: WHILE LEFTPARAN simpleExp RIGHTPARAN unmatched       { $$ = newStmtNode(WhileK, $1); $$->child[0] = $3; $$->child[1] = $5; }
             | FOR LEFTPARAN ID IN ID RIGHTPARAN unmatched          { $$ = newStmtNode(ForK, $1); $$->child[0] = newDeclNode(VarK, $3); $$->child[1] = newExpNode(IdK, $5); $$->child[2] = $7; $$->child[0]->expType = UndefinedType; }
             ;

returnStmt: RETURN SEMICOLON                                        { $$ = newStmtNode(ReturnK, $1); }
          | RETURN exp SEMICOLON                                    { $$ = newStmtNode(ReturnK, $1); $$->child[0] = $2;  yyerrok; }
          ;

breakStmt: BREAK SEMICOLON                                          { $$ = newStmtNode(BreakK, $1); }
         ;

//-----------------------------------------------------------------

exp: mutable EQUAL exp                                              { $$ = newExpNode(AssignK, $2); $$->child[0] = $1; $$->child[1] = $3; }
   | mutable ADDASS exp                                             { $$ = newExpNode(AssignK, $2); $$->child[0] = $1; $$->child[1] = $3; }
   | mutable SUBASS exp                                             { $$ = newExpNode(AssignK, $2); $$->child[0] = $1; $$->child[1] = $3; }
   | mutable MULASS exp                                             { $$ = newExpNode(AssignK, $2); $$->child[0] = $1; $$->child[1] = $3; }
   | mutable DIVASS exp                                             { $$ = newExpNode(AssignK, $2); $$->child[0] = $1; $$->child[1] = $3; }
   | mutable INC                                                    { $$ = newExpNode(AssignK, $2); $$->child[0] = $1; yyerrok; }
   | mutable DEC                                                    { $$ = newExpNode(AssignK, $2); $$->child[0] = $1; yyerrok; }
   | simpleExp                                                      { $$ = $1; }
   | error EQUAL error                                              { $$ = NULL; }
   | error ADDASS error                                             { $$ = NULL; }
   | error SUBASS error                                             { $$ = NULL; }
   | error MULASS error                                             { $$ = NULL; }
   | error DIVASS error                                             { $$ = NULL; }
   | error INC                                                      { $$=NULL; yyerrok; }
   | error DEC                                                      { $$=NULL; yyerrok; }
   ;

simpleExp: simpleExp OR andExp                                      { $$ = newExpNode(OpK, $2); $$->child[0] = $1; $$->child[1] = $3; $$->expType = Boolean; }
         | andExp                                                   { $$ = $1; }
         | simpleExp OR error                                       { $$=NULL; };
         ;

andExp: andExp AND unaryRelExp                                      { $$ = newExpNode(OpK, $2); $$->child[0] = $1; $$->child[1] = $3; $$->expType = Boolean;}
      | unaryRelExp                                                 { $$ = $1; }
      | andExp AND error                                            { $$ = NULL; }
      ;

unaryRelExp: DOESNOT unaryRelExp                                    { $$ = newExpNode(OpK, $1); $$->child[0] = $2; $$->expType = Boolean; }
           | relExp                                                 { $$ = $1; }
           | DOESNOT error                                          { $$ = NULL; }
           ;

relExp: sumExp relop sumExp                                         { $$ = newExpNode(OpK, $2); $$->child[0] = $1; $$->child[1] = $3; $$->expType = Boolean; }
      | sumExp                                                      { $$ = $1; }
      | sumExp relop error                                          { $$ = NULL; yyerrok; }
      ;

relop: LEQ                                                          { $$ = $1; }
     | LESS                                                         { $$ = $1; }
     | GREAT                                                        { $$ = $1; }
     | GEQ                                                          { $$ = $1; }
     | EQ                                                           { $$ = $1; }
     | NEQ                                                          { $$ = $1; }
     ;

sumExp: sumExp sumop mulExp                                         { $$ = newExpNode(OpK, $2); $$->child[0] = $1; $$->child[1] = $3; $$->expType = Integer; }
      | mulExp                                                      { $$ = $1; }
      | sumExp sumop error                                          { $$ = NULL; yyerrok; }
      ;

sumop: ADD                                                          { $$ = $1; }
     | SUB                                                          { $$ = $1; }
     ;

mulExp: mulExp mulop unaryExp                                       { $$ = newExpNode(OpK, $2); $$->child[0] = $1; $$->child[1] = $3; $$->expType = Integer; }
      | unaryExp                                                    { $$ = $1; }
      | mulExp mulop error                                          { $$ = NULL; }
      ;

mulop: MUL                                                          { $$ = $1; }
     | DIV                                                          { $$ = $1; }
     | MOD                                                          { $$ = $1; }
     ;

unaryExp: unaryop unaryExp                                          { $$ = newExpNode(OpK, $1); $$->child[0] = $2; $$->expType = Integer; }
        | factor                                                    { $$ = $1; }
        | unaryop error                                             { $$ = NULL; }
        ;

unaryop: SUB                                                        { $$ = $1; $1->tokenclass = CHSIGN; }
       | MUL                                                        { $$ = $1; }
       | QUESTION                                                   { $$ = $1; }
       ;

factor: immutable                                                   { $$ = $1; }
      | mutable                                                     { $$ = $1; }
      ;

mutable: ID                                                         { $$ = newExpNode(IdK, $1); }
       | mutable LEFTBRACK exp RIGHTBRACK                           { $$ = newExpNode(OpK, $2); $$->child[0] = $1; $$->child[1] = $3; $1->isArray = true; }
       ;

immutable: LEFTPARAN exp RIGHTPARAN                                 { $$ = $2; yyerrok; }
         | LEFTPARAN error                                          { $$ = NULL; }
         | error RIGHTPARAN                                         { $$ = NULL; yyerrok; }
         | call                                                     { $$ = $1; }
         | constant                                                 { $$ = $1; }
         ;

call: ID LEFTPARAN args RIGHTPARAN                                  { $$ = newExpNode(CallK, $1); $$->child[0] = $3; $$->expType = UndefinedType; } //$$->attr.name = $1->tokenstr;
    | error LEFTPARAN                                               { $$ = NULL; yyerrok; }
    ;

args: argList                                                       { $$ = $1; }
    |                                                               { $$ = NULL; }
    ;

argList: argList COMMA exp                                          { $$ = addSibling($1, $3); yyerrok; }
       | argList COMMA error                                        { $$ = NULL; }
       | exp                                                        { $$ = $1; }
       ;

constant: NUMCONST                                                  { $$ = newExpNode(ConstantK, $1); $$->attr.value = $1->numValue; $$->expType = Integer; $$->isConst = true; }
        | CHARCONST                                                 { $$ = newExpNode(ConstantK, $1); $$->attr.cvalue = $1->tokenstr[1]; $$->expType = Char; $$->isConst = true; }
        | STRINGCONST                                               { $$ = newExpNode(ConstantK, $1); $$->attr.string = $1->tokenstr; $$->expType = Char; $$->isArray = true; $$->isConst = true; }
        | TRUE                                                      { $$ = newExpNode(ConstantK, $1); $$->attr.value = 1; $$->expType = Boolean; $$->isConst = true; }
        | FALSE                                                     { $$ = newExpNode(ConstantK, $1); $$->attr.value = 0; $$->expType = Boolean; $$->isConst = true; }
        ;

//-----------------------------------------------------------------
%%
int main(int argc, char *argv[])
{
    // yydebug = 1;
    // I skipped using get op and made my own version that i think works fine
    extern FILE *yyin;
    initErrorProcessing();
    char *tmFile;
    SymbolTable *symtab = new SymbolTable();
    int i = 1, fileLoc = 0;
    p = false;
    m = false;
    if(argc == 1) yyparse;
    else {
        while(i != argc)
        {
            if(argv[i][0] == '-')
                parseFlags(argv[i]);
            else if(argv[i][0] == '<')
            {
                break;
            }
            else fileLoc = i;
            i++;
        }
    }
    if(fileLoc != 0)
    {
        if(yyin = fopen(argv[fileLoc], "r")) {
            yyparse();
            fclose(yyin);
        }
        int fileStart = 0;
        for(i = 0; i < strlen(argv[fileLoc]); i++)
        {
            if(argv[fileLoc][i] == '/')
            {
                fileStart = i + 1;
            }
        }
        int tmLen = strlen(argv[fileLoc]) + 1;
        tmFile = (char *)malloc((tmLen)*sizeof(char));
        int tmCount = 0;
        for(i = fileStart; i < tmLen - 1; i++)
        {
            if(argv[fileLoc][i] == '.')
            {
                tmFile[tmCount] = argv[fileLoc][i];
                tmCount++;
                i++;
                tmFile[tmCount] = 't';
                tmCount++;
                i++;
                tmFile[tmCount] = 'm';
            }
            else { tmFile[tmCount] = argv[fileLoc][i]; tmCount++; }
            
        }
        tmCount++;
        tmFile[tmCount] = '\0';
    }
    else yyparse();
    if(numErrors == 0) 
    { 
        newSemanticAnalysis(syntaxTree, symtab, numWarnings, numErrors, 0);
        printIsMain(numErrors);
    }
    if(p == true) printTree(syntaxTree, 0, 0, 0);
    printf("Number of warnings: %d\nNumber of errors: %d\n", numWarnings, numErrors);
    if(numErrors == 0)
    {
        //printf("------------------------------------\n");
        //printf("Loading file: %s\n", tmFile);
        code = fopen(tmFile, "w");
        genCode(syntaxTree, symtab, argv[fileLoc]);
        //printf("Bye.\n");
    }
    if(syntaxTree != NULL) trashCollector();
    return 0;
}

void parseFlags(char *string)
{
    int i = 1;
    while(i != strlen(string))
    {
        if(string[i] == 'P')
            p = true;
        else if(string[i] == 'd')
            printf("Debug option\n"); //yydebug = 1;
        else if(string[i] == 'h')
            printf("This is the help message. I cannot help you.\n");
        else if(string[i] == 's')
        {
            ;
        }
        else if(string[i] == 'M')
        {
            m = true;
            p = true;
        }
        else printf("No flag option. Throw away.\n");
        i++;
    }
}

void trashCollector()
{
    if(syntaxTree->sibling == NULL) ; //do nothing
    else {
        while(syntaxTree->sibling != NULL)
        {
            free(syntaxTree->sibling);
            syntaxTree = syntaxTree->sibling;
        }
    }
}