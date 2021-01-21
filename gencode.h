#ifndef GENCODE_H
#define GENCODE_H

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "scanType.h"
#include "treeNodes.h"
#include "semantics.h"
#include "yyerror.h"
#include "gencode.h"
#include "emitcode.h"

void genCode(TreeNode *tree, SymbolTable *table, char *fileComp);
int isGlobal(TreeNode *tree);
void midInput();
void IOFunctions(char *function, int loc);
void treeGenCode(TreeNode *tree, SymbolTable *table);

#endif