/***************************
 * Ronnie Keating
 * CS445
 * Assignment 2
 * treeNodes.h
 ***************************/

#ifndef _TREENODES_H_
#define _TREENODES_H_
#include <stdio.h>
#include <stdlib.h>
#include "semantics.h"


typedef int OpKind;
typedef enum {DeclK, StmtK, ExpK} NodeKind;
typedef enum {VarK, FuncK, ParamK} DeclKind;
typedef enum {NullK, IfK, WhileK, ForK, CompoundK, ReturnK, BreakK} StmtKind;
typedef enum {OpK, ConstantK, IdK, AssignK, InitK, CallK, ParamIdK} ExpKind;
typedef enum {Void, Integer, Boolean, Char, CharInt, Equal, UndefinedType} ExpType;
typedef enum {None, Local, Global, Parameter, LocalStatic} VarKind;

#define MAXCHILDREN 3

typedef struct treeNode
{
    // connectivity in the tree
    struct treeNode *child[MAXCHILDREN];   // children of the node
    struct treeNode *sibling;              // siblings for the node

    // what kind of node
    int lineno;                            // linenum relevant to this node
    NodeKind nodekind;                     // type of node
    union                                  // subtype of type
    {
	    DeclKind decl;                     // used when DeclK
	    StmtKind stmt;                     // used when StmtK
	    ExpKind exp;                       // used when ExpK
    } kind;
    
    // extra properties about the node depending on type of the node
    union                                   // relevant data to type -> attr
    {
        OpKind op;                          // type of token (same as in bison)
	    int value;                          // used when an integer constant or boolean
        unsigned char cvalue;               // used when a character
	    char *string;                       // used when a string constant
	    char *name;                         // used when IdK
    } attr;                                 
    ExpType expType;		                // used when ExpK for type checking
    bool isArray;                           // is this an array
    bool isStatic;                          // is staticly allocated?
    bool isConst;

    //values to store, like child num and array num
    int childVal;
    int arrayVal;
    char *names;
    bool isUsed;
    bool isComp;
    bool isInit;
    bool isPassed;
    bool isPassed2;
    bool isPassed3;
    bool isReturn;
    bool returned;
    int paramTotal;
    bool isError;
    bool isColon;
    int temp1, temp2;

    //stuff for mem, location, and size
    VarKind varKind;
    int loc;
    int size;

    //code gen stuff
    int offset;
    bool elseStmt;
    bool used;
    bool isRegFunc;
    int breakStuff;

    // even more semantic stuff will go here in later assignments.
} TreeNode;

//FUNCTIONS
void printTree(TreeNode * tree, bool isSibling, bool isChild, int childVal);
void printTypes(int type, int line);
char *returnTypes(int type);
char *memType(int type);
void printCharacters(char *string);
void typeSpecRoutine(TreeNode *curr, ExpType expType, bool isStatic);
void semanticAnalysis(TreeNode *tree, SymbolTable *table, bool isFunction);
void newSemanticAnalysis(TreeNode *tree, SymbolTable *table, int &warnings, int &errors, bool setInit);
//void printWarnings(TreeNode *tree, SymbolTable *table, int &warnings);
void printWarnings(std::string sym, void *ptr);
void printIsMain(int &errors);
TreeNode *addSibling(TreeNode *t, TreeNode *s);
TreeNode * newDeclNode(DeclKind kind, TokenData *tokenData);
TreeNode * newExpNode(ExpKind kind, TokenData *tokenData);
TreeNode * newStmtNode(StmtKind kind, TokenData *tokenData);

#endif