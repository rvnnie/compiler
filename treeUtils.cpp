/***************************
 * Ronnie Keating
 * CS445
 * Assignment 2
 * treeUtils.cpp
 ***************************/

#include "scanType.h"
#include "treeNodes.h"
#include "semantics.h"
#include <string.h>

//need this mainly for CHSIGN
#include "parser.tab.h"

bool isMain = false;
bool isWhile = false;
bool isCurr = false;
int warningsCount;
int paramCount;
std::string currFunction;
TreeNode *currFunc;
TreeNode *curr;
TreeNode *curr2;

// global and local offsets
int foffset = 0, goffset = 0;
int coffset = 0; //current scope offset
int curr_offset[10];
int curr_iter = 0;
int for_offset = 0;

//enable printing of HW6
extern bool m;

//much of the code was used from the tiny compiler util.c file
//spacing (mainly leading tabs) are off, didnt feel like fixing them quite yet

/* Function newStmtNode creates a new statement
 * node for syntax tree construction
 */
TreeNode *newStmtNode(StmtKind kind, TokenData *tokenData)
{
	TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
	int i;
	if (t == NULL)
		printf("Out of memory error at line %d\n", tokenData->linenum);
	else
	{
		for (i = 0; i < MAXCHILDREN; i++)
			t->child[i] = NULL;
		t->sibling = NULL;
		t->nodekind = StmtK;
		t->kind.stmt = kind;
		t->names = strdup(tokenData->tokenstr);
		t->lineno = tokenData->linenum;
	}
	return t;
}

/* Function newExpNode creates a new expression 
 * node for syntax tree construction
 */
TreeNode *newExpNode(ExpKind kind, TokenData *tokenData)
{
	TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
	int i;
	if (t == NULL)
		printf("Out of memory error at line %d\n", tokenData->linenum);
	else
	{
		for (i = 0; i < MAXCHILDREN; i++)
			t->child[i] = NULL;
		t->sibling = NULL;
		t->nodekind = ExpK;
		if (tokenData->tokenclass == CHSIGN)
			t->names = strdup("-");
		else
			t->names = strdup(tokenData->tokenstr);
		t->kind.exp = kind;
		t->lineno = tokenData->linenum;
		t->expType = UndefinedType;
	}
	return t;
}

//new decl node
TreeNode *newDeclNode(DeclKind kind, TokenData *tokenData)
{
	TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
	int i;
	if (t == NULL)
		printf("Out of memory error at line %d\n", tokenData->linenum);
	else
	{
		for (i = 0; i < MAXCHILDREN; i++)
			t->child[i] = NULL;
		t->sibling = NULL;
		t->names = strdup(tokenData->tokenstr);
		t->nodekind = DeclK;
		t->kind.decl = kind;
		t->lineno = tokenData->linenum;
	}
	return t;
}

/* Variable indentno is used by printTree to
 * store current number of spaces to indent
 */
static int indentno = 0;

/* macros to increase/decrease indentation */
#define INDENT indentno += 1
#define UNINDENT indentno -= 1

/* printSpaces indents by printing spaces */
static void printSpaces(void)
{
	int i;
	for (i = 0; i < indentno; i++)
		printf(".   "); //fixed print to have a single dot (.) followed by exactly two spaces
}

/* procedure printTree prints a syntax tree to the 
 * listing file using indentation to indicate subtrees
 */
void printTree(TreeNode *tree, bool isSibling, bool isChild, int childVal)
{
	int i, j;
	while (tree != NULL)
	{
		printSpaces(); //print spaces up here, indent in child scenario
		if (isChild)
			printf("Child: %d  ", childVal);
		else if (isSibling)
			printf("Sibling: ");
		if (isSibling)
			printf("%d  ", ++j);
		else
			j = 0;
		//stmt kind print routine
		if (tree->nodekind == StmtK)
		{
			switch (tree->kind.stmt)
			{
			case IfK:
				printf("If [line: %d]\n", tree->lineno);
				break;
			case NullK:
				printf("Null [line: %d]\n", tree->lineno);
				break;
			case WhileK:
				printf("While [line: %d]\n", tree->lineno);
				break;
			case ForK:
				printf("For [line: %d]\n", tree->lineno);
				break;
			case CompoundK:
				printf("Compound [line: %d]\n", tree->lineno);
				break;
			case ReturnK:
				printf("Return [line: %d]\n", tree->lineno);
				break;
			case BreakK:
				printf("Break [line: %d]\n", tree->lineno);
				break;
			default:
				printf("Unknown ExpNode kind\n");
				break;
			}
		}
		//exp kind print routine, some function calls as well
		else if (tree->nodekind == ExpK)
		{
			switch (tree->kind.exp)
			{
			case OpK:
				printf("Op %s : ", tree->names);
				printTypes(tree->expType, tree->lineno);
				break;
			case ConstantK:
				switch (tree->expType)
				{
				case Boolean:
					if (tree->attr.value == 1)
						printf("Const true : ");
					else if (tree->attr.value == 0)
						printf("Const false : ");
					printTypes(tree->expType, tree->lineno);
					break;
				case Char:
					if(!tree->isArray)
					{
						printf("Const: ");
						printCharacters(tree->names);
						printf(" : ");
						printTypes(tree->expType, tree->lineno);
					}
					else 
					{
						printf("Const ");
						printCharacters(tree->names);
						printf(" : array of ");
						printf("type %s ", returnTypes(tree->expType));
						if(m) printf("[mem: %s  size: %d  loc: %d] ", memType(tree->varKind), tree->size, tree->loc);
						printf("[line: %d]\n", tree->lineno);
					}
					break;
				default:
					printf("Const %s : type int [line: %d]\n", tree->names, tree->lineno);
					break;
				}
				break;
			case IdK:
				printf("Id %s: ", tree->names);
				if(tree->isStatic) printf("static ");
				if(tree->isArray && tree->expType != UndefinedType) printf("array of ");
				printf("type %s ", returnTypes(tree->expType));
				if(m) printf("[mem: %s  size: %d  loc: %d] ", memType(tree->varKind), tree->size, tree->loc);
				printf("[line: %d]\n", tree->lineno);
				break;
			case AssignK:
				printf("Assign %s : ", tree->names);
				if(tree->isArray) printf("array of ");
				printTypes(tree->expType, tree->lineno);
				break;
			case InitK:
				printf("Init [line: %d]\n", tree->lineno);
				break;
			case CallK:
				printf("Call %s: ", tree->names);
				printTypes(tree->expType, tree->lineno);
				break;
			case ParamIdK:
				if (tree->isArray)
				{
					printf("Param %s: array of ", tree->names);
					printf("type %s ", returnTypes(tree->expType));
					if(m) printf("[mem: %s  size: %d  loc: %d] ", memType(tree->varKind), tree->size, tree->loc);
					printf("[line: %d]\n", tree->lineno);
				}
				else
				{
					printf("Param %s: ", tree->names);
					printf("type %s ", returnTypes(tree->expType));
					if(m) printf("[mem: %s  size: %d  loc: %d] ", memType(tree->varKind), tree->size, tree->loc);
					printf("[line: %d]\n", tree->lineno);
				}
				break;
			default:
				printf("Unknown ExpNode kind\n");
				break;
			}
		}
		else if (tree->nodekind == DeclK)
		{
			switch (tree->kind.decl)
			{
			case VarK:
				if (tree->isArray)
				{
					printf("Var %s: ", tree->names);
					if(tree->isStatic) printf("static array of ");
					else printf("array of ");
					printf("type %s ", returnTypes(tree->expType));
					if(m) printf("[mem: %s  size: %d  loc: %d] ", memType(tree->varKind), tree->size, tree->loc);
					printf("[line: %d]\n", tree->lineno);
				}
				else
				{
					printf("Var %s: ", tree->names);
					if(tree->isStatic) printf("static ");
					printf("type %s ", returnTypes(tree->expType));
					if(m) printf("[mem: %s  size: %d  loc: %d] ", memType(tree->varKind), tree->size, tree->loc);
					printf("[line: %d]\n", tree->lineno);
				}
				break;
			case FuncK:
				printf("Func %s: returns ", tree->names);
				printTypes(tree->expType, tree->lineno);
				break;
			case ParamK:
				printf("ParamK %s of ", tree->names);
				printf("type %s ", returnTypes(tree->expType));
				if(m) printf("[mem: %s  size: %d  loc: %d] ", memType(tree->varKind), tree->size, tree->loc);
				printf("[line: %d]\n", tree->lineno);
				break;
			default:
				printf("Unknown DeclNode kind\n");
				break;
			}
		}
		else
			printf("Unknown node kind\n");
		for (i = 0; i < MAXCHILDREN; i++)
		{
			INDENT;
			printTree(tree->child[i], 0, 1, i);
		}
		isSibling = true;
		isChild = false;
		tree = tree->sibling;
	}
	UNINDENT;
}

//The add sibling function (linked list)
TreeNode *addSibling(TreeNode *t, TreeNode *s)
{
	TreeNode *tmp;

	if (t != NULL)
	{
		tmp = t;
		while (tmp->sibling != NULL)
			tmp = tmp->sibling;
		tmp->sibling = s;
		return t;
	}
	return s;
}

//Print type based on value of type from enum
void printTypes(int type, int line)
{
	switch (type)
	{
	case 0:
		printf("type void [line: %d]\n", line);
		break;
	case 1:
		printf("type int [line: %d]\n", line);
		break;
	case 2:
		printf("type bool [line: %d]\n", line);
		break;
	case 3:
		printf("type char [line: %d]\n", line);
		break;
	case 4:
		printf("type charint [line: %d]\n", line);
		break;
	case 5:
		printf("type equal [line: %d]\n", line);
		break;
	default:
		printf("undefined type [line: %d]\n", line);
		break;
	}
}

char *memType(int type)
{
	char *returnType;
	switch (type)
	{
	case 0:
		returnType = strdup("None");
		break;
	case 1:
		returnType = strdup("Local");
		break;
	case 2:
		returnType = strdup("Global");
		break;
	case 3:
		returnType = strdup("Param");
		break;
	case 4:
		returnType = strdup("Static");
		break;
	}
	return returnType;
}

char *returnTypes(int type)
{
	char *returnType;
	switch (type)
	{
	case 0:
		returnType = strdup("void");
		break;
	case 1:
		returnType = strdup("int");
		break;
	case 2:
		returnType = strdup("bool");
		break;
	case 3:
		returnType = strdup("char");
		break;
	case 4:
		returnType = strdup("charint");
		break;
	case 5:
		returnType =strdup("equal");
		break;
	default:
		returnType = strdup("undefined");
		break;
	}
	return returnType;
}

// Print characters out correctly from string and char consts
void printCharacters(char *string)
{
	int i, length = strlen(string);
	char nullChar = '\0';
	char endLine = '\n';
	for (i = 0; i < length; i++)
	{
		if (string[i] != '\\')
			printf("%c", string[i]);
		else
		{
			if (string[i + 1] == 'n')
			{
				printf("%c", endLine);
			}
			else if (string[i + 1] == '0')
			{
				printf("%c", nullChar);
			}
			else
				printf("%c", string[i + 1]);
			i++;
		}
	}
}

void typeSpecRoutine(TreeNode *curr, ExpType expType, bool isStatic)
{
	while (curr != NULL)
	{
		curr->expType = expType;
		if (isStatic)
			curr->isStatic = true;
		else
			curr->isStatic = false;
		curr = curr->sibling;
	}
}

void newSemanticAnalysis(TreeNode *tree, SymbolTable *table, int &warnings, int &errors, bool setInit)
{
	int i, j, setWhile = 0;
	bool scopedUsed = false;
	std::string currScope;
	TreeNode *temp, *temp2;
	while (tree != NULL)
	{
		//stmt kind semantic analysis
		if (tree->nodekind == StmtK)
		{
			switch (tree->kind.stmt)
			{
			case IfK:
				//for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
				newSemanticAnalysis(tree->child[0], table, warnings, errors, 0);
				if(tree->child[0] != NULL && tree->child[0]->expType != Boolean && !tree->isPassed && tree->child[0]->expType != UndefinedType)
				{
					printf("ERROR(%d): Expecting Boolean test condition in if statement but got type %s.\n", tree->lineno, returnTypes(tree->child[0]->expType));
					errors++;
					tree->isPassed = true;
				}
				if(tree->child[0] != NULL && tree->child[0]->isArray && !tree->isPassed2)
				{
					printf("ERROR(%d): Cannot use array as test condition in if statement.\n", tree->lineno);
					errors++;
					tree->isPassed2 = true;
				}
				break;
			case NullK:
				break;
			case WhileK:
				//for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
				newSemanticAnalysis(tree->child[0], table, warnings, errors, 0);
				if(!isWhile) { isWhile = true; setWhile = 1; }
				if(tree->child[0] != NULL && tree->child[0]->expType != Boolean && !tree->isPassed && tree->child[0]->expType != UndefinedType)
				{
					printf("ERROR(%d): Expecting Boolean test condition in while statement but got type %s.\n", tree->lineno, returnTypes(tree->child[0]->expType));
					errors++;
					tree->isPassed = true;
				}
				if(tree->child[0] != NULL && tree->child[0]->isArray && !tree->isPassed2)
				{
					printf("ERROR(%d): Cannot use array as test condition in while statement.\n", tree->lineno);
					errors++;
					tree->isPassed2 = true;
				}
				break;
			case ForK:
				table->enter("For");
				for_offset = foffset;
				if(tree->child[2] != NULL && tree->child[2]->kind.stmt == CompoundK) tree->child[2]->isComp = false;
				if(tree->child[1] != NULL) temp = (TreeNode *)table->lookup(tree->child[1]->names);
				if(temp != NULL) 
				{
					tree->child[0]->expType = temp->expType;
					if(!temp->isArray)
					{
						printf("ERROR(%d): For statement requires that symbol '%s' be an array to loop through.\n", tree->lineno, temp->names);
						errors++;
					}
				}
				else if(tree->child[1] != NULL) {
					printf("ERROR(%d): For statement requires that symbol '%s' be an array to loop through.\n", tree->lineno, tree->child[1]->names);
					errors++;
				}
				tree->child[0]->isInit = true;
				break;
			case CompoundK:
				if(tree->isComp)
				{
					table->enter("Compound");
					//coffset = foffset;
					curr_iter++;
					curr_offset[curr_iter] = foffset;
					//for(i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table);
				}
				break;
			case ReturnK:
				for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
				temp = (TreeNode *)table->lookupGlobal(currFunction);
				if(tree->child[0] != NULL)
				{
					if(tree->child[0]->isArray)
					{
						printf("ERROR(%d): Cannot return an array.\n", tree->lineno);
						errors++;
					}
					//-------
					if(currFunc != NULL)
					{
						if(tree->child[0] != NULL && currFunc->expType != tree->child[0]->expType && currFunc->expType != Void && tree->child[0]->expType != UndefinedType)
						{
							printf("ERROR(%d): Function '%s' at line %d is expecting to return type %s but got type %s.\n", tree->lineno, temp->names, temp->lineno, returnTypes(temp->expType), returnTypes(tree->child[0]->expType));
							errors++;
						}
						else if(currFunc->expType == Void)
						{
							printf("ERROR(%d): Function '%s' at line %d is expecting no return value, but return has return value.\n", tree->lineno, temp->names, temp->lineno);
							errors++;
						}
						currFunc->returned = true;
					}
				}
				else {
					if(temp != NULL && temp->isReturn)
					{
						printf("ERROR(%d): Function '%s' at line %d is expecting to return type %s but return has no return value.\n", tree->lineno, temp->names, temp->lineno, returnTypes(temp->expType));
						errors++;
						temp->returned = true;
					}
				}
				break;
			case BreakK:
				if(!isWhile && !tree->isPassed)
				{
					printf("ERROR(%d): Cannot have a break statement outside of loop.\n", tree->lineno);
					errors++;
					tree->isPassed = true;
				}
				else tree->isPassed = true;
				break;
			default:
				break;
			}
		}
		//exp kind semantic analysis
		else if (tree->nodekind == ExpK)
		{
			switch (tree->kind.exp)
			{
			case OpK:
				if(tree->names[0] == '&' || tree->names[0] == '|')
				{
					for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
					tree->expType = Boolean;
					if(tree->child[0] != NULL)
					{
						temp = (TreeNode *)table->lookup(tree->child[0]->names);
						if(tree->child[1] != NULL) temp2 = (TreeNode *)table->lookup(tree->child[1]->names);
						else temp2 = NULL;
						if(temp == NULL && tree->child[0]->expType != Boolean && tree->child[0]->expType != UndefinedType && tree->child[0]->expType != Void && !tree->isPassed)
						{
							printf("ERROR(%d): '%s' requires operands of type bool but lhs is of type %s.\n", tree->lineno, tree->names, returnTypes(tree->child[0]->expType));
							errors++;
						}
						if(temp2 == NULL && tree->child[1] != NULL && tree->child[1]->expType != Boolean && tree->child[1]->expType != UndefinedType && tree->child[1]->expType != Void && !tree->isPassed)
						{
							printf("ERROR(%d): '%s' requires operands of type bool but rhs is of type %s.\n", tree->lineno, tree->names, returnTypes(tree->child[1]->expType));
							errors++;
						}
						if(temp != NULL && temp->expType != Boolean && temp->expType != UndefinedType && !tree->isPassed)
						{
								printf("ERROR(%d): '%s' requires operands of type bool but lhs is of type %s.\n", tree->lineno, tree->names, returnTypes(temp->expType));
								errors++;
						}
						if(temp2 != NULL && temp2->expType != Boolean && temp2->expType != UndefinedType && !tree->isPassed)
						{
							printf("ERROR(%d): '%s' requires operands of type bool but rhs is of type %s.\n", tree->lineno, tree->names, returnTypes(temp2->expType));
							errors++;
						}

						if(temp != NULL && temp->isArray)
						{
							printf("ERROR(%d): The operation '%s' does not work with arrays.\n", tree->lineno, tree->names);
							errors++;
						}
						else if(tree->child[1] != NULL && !tree->isPassed)
						{
							temp2 = (TreeNode *)table->lookup(tree->child[1]->names);
							if(temp2 != NULL && temp2->isArray)
							{
								printf("ERROR(%d): The operation '%s' does not work with arrays.\n", tree->lineno, tree->names);
								errors++;
							}
						}
						tree->isPassed = true;
					}
				}
				else if((tree->names[1] == '=') && (tree->names[0] == '<' || tree->names[0] == '>' || tree->names[0] == '='))
				{
					tree->expType = Boolean;
					for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
					if(tree->child[0] != NULL && !tree->isPassed)
					{
						temp = (TreeNode *)table->lookup(tree->child[0]->names);
						if(tree->child[1] != NULL)
						{
							temp2 = (TreeNode *)table->lookup(tree->child[1]->names);
							if(temp2 != NULL)
							{
								if(temp != NULL && temp2->expType != temp->expType)
								{
									printf("ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is type %s.\n", tree->lineno, tree->names, returnTypes(temp->expType), returnTypes(temp2->expType));
									errors++;
								}
							}
							else if(temp == NULL && temp2 != NULL)
							{
								if(tree->child[0]->expType != temp2->expType)
								{
									printf("ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is type %s.\n", tree->lineno, tree->names, returnTypes(tree->child[0]->expType), returnTypes(temp2->expType));
									errors++;
								}
							}
							else if(temp2 == NULL && temp != NULL)
							{
								if(temp->expType != tree->child[1]->expType)
								{
									printf("ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is type %s.\n", tree->lineno, tree->names, returnTypes(temp->expType), returnTypes(tree->child[1]->expType));
									errors++;
								}
							}
							else if(temp == NULL && temp2 == NULL)
							{
								if(tree->child[0]->expType != tree->child[1]->expType)
								{
									printf("ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is type %s.\n", tree->lineno, tree->names, returnTypes(temp->expType), returnTypes(tree->child[1]->expType));
									errors++;
								}
							}
						}
					}
					tree->isPassed = true;
				}
				else if(tree->names[0] == '>' || tree->names[0] == '<')
				{
					for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
					if(!tree->isPassed)
					{
						tree->expType = Boolean;
						if(tree->child[0] != NULL) temp = (TreeNode *)table->lookup(tree->child[0]->names);
						if(tree->child[1] != NULL) temp2 = (TreeNode *)table->lookup(tree->child[1]->names);
						if(temp2 != NULL && temp != NULL && temp2->expType != temp->expType && !tree->isPassed && temp->expType != UndefinedType && temp2->expType != UndefinedType)
						{
							printf("ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is type %s.\n", tree->lineno, tree->names, returnTypes(temp->expType), returnTypes(temp2->expType));
							errors++;
						}
						else if(temp == NULL && tree->child[0] != NULL && temp2 != NULL && temp2->expType != tree->child[0]->expType && !tree->isPassed && temp2->expType != UndefinedType && tree->child[0]->expType != UndefinedType)
						{
							printf("ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is type %s.\n", tree->lineno, tree->names, returnTypes(tree->child[0]->expType), returnTypes(temp2->expType));
							errors++;
						}
						else if(temp != NULL && tree->child[1] != NULL && temp2 == NULL && temp->expType != tree->child[1]->expType && !tree->isPassed && temp->expType != UndefinedType && tree->child[1]->expType != UndefinedType)
						{
							printf("ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is type %s.\n", tree->lineno, tree->names, returnTypes(temp->expType), returnTypes(tree->child[1]->expType));
							errors++;
						}
						else if(temp == NULL && temp2 == NULL && tree->child[0] != NULL && tree->child[1] != NULL && tree->child[0]->expType != tree->child[1]->expType && !tree->isPassed && tree->child[0]->expType != UndefinedType && tree->child[1]->expType != UndefinedType)
						{
							printf("ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is type %s.\n", tree->lineno, tree->names, returnTypes(tree->child[0]->expType), returnTypes(tree->child[1]->expType));
							errors++;
						}
						if(temp2 != NULL && temp != NULL && temp->isArray && !temp2->isArray)
						{	
							printf("ERROR(%d): '%s' requires both operands be arrays or not but lhs is an array and rhs is not an array.\n", tree->lineno, tree->names);
							errors++;
						}
						else if(temp2 != NULL && temp != NULL && !temp->isArray && temp2->isArray)
						{
							printf("ERROR(%d): '%s' requires both operands be arrays or not but lhs is not an array and rhs is an array.\n", tree->lineno, tree->names);
							errors++;
						}
						tree->isPassed = true;
					}
				}
				else if(strcmp(tree->names, "=") == 0)
				{
					if(tree->child[0] != NULL) tree->expType = tree->child[0]->expType;
				}
				else if((strcmp(tree->names, "*=") == 0) || (strcmp(tree->names, "-=") == 0) || (strcmp(tree->names, "+=") == 0))
				{
					tree->expType = Integer;
					if(tree->child[0] != NULL)
					{
						temp = (TreeNode *)table->lookup(tree->child[0]->names);
						if(temp != NULL && temp->isArray)
						{
							printf("ERROR(%d): The operation '%s' does not work with arrays.\n", tree->lineno, tree->names);
							errors++;
						}
					}
				}
				else if(strcmp(tree->names,"+") == 0 || strcmp(tree->names,"/") == 0 || strcmp(tree->names,"%") == 0)
				{
					tree->expType = Integer;
					/*if(tree->child[0] != NULL && tree->child[1] != NULL && !tree->isPassed)
					{
						temp = (TreeNode *)table->lookup(tree->child[0]->names);
						temp2 = (TreeNode *)table->lookup(tree->child[1]->names);
						if(temp != NULL && temp2 != NULL && temp == temp2) { printf("WARNING(%d): Variable %s may be uninitialized when used here.\n", tree->lineno, temp->names); temp->isInit = true; warnings++; }
					}*/
					for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
					if(tree->child[0] != NULL)
					{ 
						temp = (TreeNode *)table->lookup(tree->child[0]->names);
						/*if(temp != NULL && !temp->isInit)
						{
							printf("WARNING(%d): Variable %s may be uninitialized when used here.\n", tree->lineno, tree->names); temp->isInit = true; warnings++;
						}*/
						if(tree->child[1] != NULL) temp2 = (TreeNode *)table->lookup(tree->child[1]->names);
						if(temp == NULL && tree->child[0]->expType != Integer && tree->child[0]->expType != UndefinedType && !tree->isPassed)
						{
							printf("ERROR(%d): '%s' requires operands of type int but lhs is of type %s.\n", tree->lineno, tree->names, returnTypes(tree->child[0]->expType));
							errors++;
						}
						else if(temp != NULL && temp->expType != Integer && temp->expType != UndefinedType && /*temp->expType != Void &&*/ !tree->isPassed && temp->kind.decl != FuncK)
						{
							printf("ERROR(%d): '%s' requires operands of type int but lhs is of type %s.\n", tree->lineno, tree->names, returnTypes(temp->expType));
							errors++;
						}
						if(temp2 == NULL && tree->child[1]->expType != Integer && tree->child[1]->expType != UndefinedType && !tree->isPassed)
						{
							printf("ERROR(%d): '%s' requires operands of type int but rhs is of type %s.\n", tree->lineno, tree->names, returnTypes(tree->child[1]->expType));
							errors++;
						}
						/*if(temp != NULL && temp->expType != Integer && temp->expType != UndefinedType && /*temp->expType != Void && !tree->isPassed)
						{
							printf("ERROR(%d): '%s' requires operands of type int but lhs is of type %s.\n", tree->lineno, tree->names, returnTypes(temp->expType));
							errors++;
						}*/
						else if(temp2 != NULL && temp2->expType != Integer && temp2->expType != UndefinedType && /*temp2->expType != Void &&*/ !tree->isPassed /*&& /*temp2->kind.decl != FuncK*/) 
						{
							printf("ERROR(%d): '%s' requires operands of type int but rhs is of type %s.\n", tree->lineno, tree->names, returnTypes(temp2->expType));
							errors++;
						}
						if(temp != NULL && temp->isArray && !tree->isPassed)
						{
							printf("ERROR(%d): The operation '%s' does not work with arrays.\n", tree->lineno, tree->names);
							errors++;
						}
						else if(tree->child[1] != NULL && !tree->isPassed)
						{
							temp2 = (TreeNode *)table->lookup(tree->child[1]->names);
							if(temp2 != NULL && temp2->isArray)
							{
								printf("ERROR(%d): The operation '%s' does not work with arrays.\n", tree->lineno, tree->names);
								errors++;
							}
						}
						tree->isPassed = true;
					}
				}
				else if((tree->names[0] == '*' || tree->names[0] == '-') && tree->child[1] != NULL)
				{
					//if(setInit) newSemanticAnalysis(tree->child[0], table, warnings, errors, 1);
					tree->expType = Integer;
					for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
					if(tree->child[0] != NULL)
					{ 
						temp = (TreeNode *)table->lookup(tree->child[0]->names);
						if(tree->child[1] != NULL) temp2 = (TreeNode *)table->lookup(tree->child[1]->names);
						if(temp != NULL && temp->expType != Integer && temp->expType != UndefinedType /*&& temp->expType != Void*/ && !tree->isPassed)
						{
							printf("ERROR(%d): '%s' requires operands of type int but lhs is of type %s.\n", tree->lineno, tree->names, returnTypes(temp->expType));
							errors++;
						}
						if(temp2 != NULL && temp2->expType != Integer && temp2->expType != UndefinedType /*&& temp2->expType != Void*/ && !tree->isPassed) 
						{
							printf("ERROR(%d): '%s' requires operands of type int but rhs is of type %s.\n", tree->lineno, tree->names, returnTypes(temp2->expType));
							errors++;
						}
						if(temp == NULL && tree->child[0]->expType != Integer && tree->child[0]->expType != UndefinedType /*&& tree->child[0]->expType != Void*/ && !tree->isPassed)
						{
							printf("ERROR(%d): '%s' requires operands of type int but lhs is of type %s.\n", tree->lineno, tree->names, returnTypes(tree->child[0]->expType));
							errors++;
						}
						if(temp2 == NULL && tree->child[1]->expType != Integer && tree->child[1]->expType != UndefinedType /*&& tree->child[1]->expType != Void*/ && !tree->isPassed)
						{
							printf("ERROR(%d): '%s' requires operands of type int but rhs is of type %s.\n", tree->lineno, tree->names, returnTypes(tree->child[1]->expType));
							errors++;
						}
						if(temp != NULL && temp->isArray && !tree->isPassed)
						{
							printf("ERROR(%d): The operation '%s' does not work with arrays.\n", tree->lineno, tree->names);
							errors++;
						}
						else if(tree->child[1] != NULL && !tree->isPassed)
						{
							temp2 = (TreeNode *)table->lookup(tree->child[1]->names);
							if(temp2 != NULL && temp2->isArray)
							{
								printf("ERROR(%d): The operation '%s' does not work with arrays.\n", tree->lineno, tree->names);
								errors++;
							}
						}
						tree->isPassed = true;
					}
				}
				else if(tree->names[0] == '[')
				{ 
					if(!tree->isPassed)
					{
						if(setInit && tree->child[0] != NULL) tree->child[0]->isInit = true; 
						if(setInit && tree->child[1] != NULL) tree->child[1]->isInit = true;
						for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
						if(tree->child[0] != NULL) tree->expType = tree->child[0]->expType;
						if(tree->child[0] != NULL)
						{
							temp = (TreeNode *)table->lookup(tree->child[0]->names);
							if(tree->child[1] != NULL) temp2 = (TreeNode *)table->lookup(tree->child[1]->names);
							if(temp2 != NULL)
							{
								if(temp2->expType != Integer /*&& temp2->expType != Void*/ && temp2->expType != UndefinedType && temp2->kind.decl != FuncK)
								{
									printf("ERROR(%d): Array '%s' should be indexed by type int but got type %s.\n", tree->lineno, temp->names, returnTypes(temp2->expType));
									errors++;
								}
								else if(tree->child[1] != NULL && tree->child[1]->kind.exp == CallK && tree->child[1]->expType != Integer && tree->child[1]->expType != UndefinedType)
								{
									printf("ERROR(%d): Array '%s' should be indexed by type int but got type %s.\n", tree->lineno, temp->names, returnTypes(temp2->expType));
									errors++;
								}
							}
							else 
							{
								if(tree->child[1] != NULL && tree->child[1]->expType != Integer /*&& tree->child[1]->expType != Void*/ && tree->child[1]->expType != UndefinedType)
								{
									printf("ERROR(%d): Array '%s' should be indexed by type int but got type %s.\n", tree->lineno, temp->names, returnTypes(tree->child[1]->expType));
									errors++;
								}
							}
							if(temp != NULL)
							{
								if(!temp->isArray && !tree->child[0]->isPassed2)
								{
									printf("ERROR(%d): Cannot index nonarray '%s'.\n", tree->lineno, temp->names);
									tree->child[0]->isPassed2 = true;
									errors++;
								}
							}
							if(temp == NULL && !tree->child[0]->isPassed2)
							{
								printf("ERROR(%d): Cannot index nonarray '%s'.\n", tree->lineno, tree->child[0]->names);
								tree->child[0]->isPassed2 = true;
								errors++;
							}
							if(tree->child[1] != NULL)
							{
								if(temp2 != NULL && temp2->isArray)
								{
									if(tree->child[1]->child[0] != NULL && tree->child[1]->child[0]->names[0] != '[')
									{
										printf("ERROR(%d): Array index is the unindexed array '%s'.\n", tree->lineno, temp2->names);
										errors++;
									}
									else if(tree->child[1]->child[0] == NULL)
									{
										printf("ERROR(%d): Array index is the unindexed array '%s'.\n", tree->lineno, temp2->names);
										errors++;
									}
								}
							}
						}
						tree->isPassed = true;
					}
				}
				else if(strcmp(tree->names, "++") == 0 || strcmp(tree->names, "--") == 0)
				{
					tree->expType = Integer;
					if(tree->child[0] != NULL)
					{
						temp = (TreeNode *)table->lookup(tree->child[0]->names);
						if(temp != NULL && temp->isArray)
						{
							printf("ERROR(%d): The operation '%s' does not work with arrays.\n", tree->lineno, tree->names);
							errors++;
						}
					}
				}
				else if(strcmp(tree->names, "!=") == 0)
				{
					tree->expType = Boolean;
					if(tree->child[0] != NULL)
					{
						temp = (TreeNode *)table->lookup(tree->child[0]->names);
						if(tree->child[1] != NULL)
						{
							temp2 = (TreeNode *)table->lookup(tree->child[1]->names);
							if(temp2 != NULL)
							{
								if(temp != NULL && temp2->expType != temp->expType)
								{
									printf("ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is type %s.\n", tree->lineno, tree->names, returnTypes(temp->expType), returnTypes(temp2->expType));
									errors++;
								}
							}
						}
					}
				}
				else if(tree->names[0] == '!')
				{
					if(!tree->isPassed)
					{
						tree->expType = Boolean;
						if(tree->child[0] != NULL)
						{
							temp = (TreeNode *)table->lookup(tree->child[0]->names);
							if(temp != NULL && temp->expType != Boolean)
							{
								printf("ERROR(%d): Unary '%s' requires an operand of type bool but was given type %s.\n", tree->lineno, tree->names, returnTypes(temp->expType));
								errors++;
							}
							if(temp == NULL && tree->child[0]->expType != Boolean)
							{
								printf("ERROR(%d): Unary '%s' requires an operand of type bool but was given type %s.\n", tree->lineno, tree->names, returnTypes(tree->child[0]->expType));
								errors++;
							}
							if(temp != NULL && temp->isArray)
							{
								printf("ERROR(%d): The operation '%s' does not work with arrays.\n", tree->lineno, tree->names);
								errors++;
							}
						}
						tree->isPassed = true;
					}
				}
				else if(tree->names[0] == '*' && tree->child[1] == NULL)
				{
					if(!tree->isPassed)
					{
						tree->expType = Integer;
						for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
						if(tree->child[0] != NULL)
						{
							temp = (TreeNode *)table->lookup(tree->child[0]->names);
							if(temp != NULL && !temp->isArray)
							{
								printf("ERROR(%d): The operation '%s' only works with arrays.\n", tree->lineno, tree->names);
								errors++;
							}
							else {
								if(tree->child[0] != NULL and !tree->child[0]->isArray && tree->child[0]->expType != UndefinedType)
								{
									printf("ERROR(%d): The operation '%s' only works with arrays.\n", tree->lineno, tree->names);
									errors++;
								}
							}
						}
						tree->isPassed = true;
					}
				}
				else if(tree->names[0] == '-' && tree->child[1] == NULL)
				{
					for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
					if(tree->child[0] != NULL)
					{
						temp = (TreeNode *)table->lookup(tree->child[0]->names);
						if(temp != NULL && temp->expType != Integer && !tree->child[0]->isPassed)
						{
							printf("ERROR(%d): Unary '%s' requires an operand of type int but was given type %s.\n", tree->lineno, tree->names, returnTypes(temp->expType));
							tree->child[0]->isPassed = true;
							errors++;
						}
						else 
						{
							if(tree->child[0]->expType != Integer && !tree->child[0]->isPassed)
							{
								printf("ERROR(%d): Unary '%s' requires an operand of type int but was given type %s.\n", tree->lineno, tree->names, returnTypes(tree->child[0]->expType));
								tree->child[0]->isPassed = true;
								errors++;
							}
						}
						if(temp != NULL && temp->isArray)
						{
							printf("ERROR(%d): The operation '%s' does not work with arrays.\n", tree->lineno, tree->names);
							errors++;
						}
					}
					tree->expType = Integer;
				}
				else if(tree->names[0] == '?')
				{
					if(!tree->isPassed)
					{
						for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
						if(tree->child[0]->kind.exp == IdK || tree->child[0]->kind.exp == ConstantK) tree->expType = tree->child[0]->expType;
						else tree->expType = Integer;
						if(tree->child[0] != NULL)
						{
							temp = (TreeNode *)table->lookup(tree->child[0]->names);
							if(temp != NULL && temp->expType != Integer)
							{
								printf("ERROR(%d): Unary '?' requires an operand of type int but was given type %s.\n", tree->lineno, returnTypes(temp->expType));
								errors++;
							}
							else {
								if(tree->child[0]->expType != Integer)
								{
									printf("ERROR(%d): Unary '?' requires an operand of type int but was given type %s.\n", tree->lineno, returnTypes(tree->child[0]->expType));
									errors++;
								}
							}
							if(temp != NULL && temp->isArray)
							{
								printf("ERROR(%d): The operation '%s' does not work with arrays.\n", tree->lineno, tree->names);
								errors++;
							}
						}
						tree->isPassed = true;
					}
				}
				break;
			case ConstantK:
				if(tree->isArray)
				{
					if(tree->attr.string != NULL && !tree->isPassed)
					{
						int strLength = strlen(tree->names), i = 0;
						int stringSize = 0;
						for(i = 0; i < strLength; i++)
						{
							if (tree->names[i] != '\\') stringSize++;
							else { stringSize++; i++; }
						}
						tree->size = stringSize - 1;
						if(table->scopeName() == "Global")
						{
							tree->varKind = Global;
							tree->loc = goffset - 1;
							goffset = goffset - tree->size;
						}
						else if(tree->isStatic)
						{
							tree->varKind = Global;
							tree->loc = goffset - 1;
							goffset = goffset - tree->size;
						}
						//local case ?
						else 
						{
							tree->varKind = Global;
							tree->loc = goffset - 1;
							goffset = goffset - tree->size;
						}
						tree->isPassed = true;
					}
				}
				break;
			case IdK:
				temp2 = (TreeNode *)table->lookupGlobal(tree->names);
				if(temp2 != NULL) temp2->isInit = true;
				temp = (TreeNode *)table->lookup(tree->names);
				if(temp != NULL) 
				{
					if(setInit || tree->isInit || temp->isStatic) temp->isInit = true;
					if(temp->kind.exp != ParamIdK && temp->kind.decl != VarK && temp->kind.decl != ParamK) 
					{
						tree->expType = UndefinedType;
						temp->isUsed = true;
					}
					else
					{
						tree->expType = temp->expType;
						tree->isStatic = temp->isStatic;
						tree->isArray = temp->isArray;
						tree->varKind = temp->varKind;
						tree->size = temp->size;
						tree->loc = temp->loc;
						if(!temp->isUsed) temp->isUsed = true;
						if(temp->isInit != true && temp->expType != UndefinedType && temp->kind.decl != ParamK && temp->kind.exp != ParamIdK) { printf("WARNING(%d): Variable %s may be uninitialized when used here.\n", tree->lineno, tree->names); temp->isInit = true; warnings++; }
					}
					if(temp->kind.decl == FuncK && !tree->isPassed) { printf("ERROR(%d): Cannot use function '%s' as a variable.\n", tree->lineno, temp->names); errors++; tree->isPassed = true; }
				}  
				else 
				{
					temp = (TreeNode *)table->lookupGlobal(tree->names);
					if(temp != NULL)
					{
						if(temp->kind.exp != ParamIdK && temp->kind.decl != VarK && temp->kind.decl != ParamK) { tree->expType = UndefinedType; temp->isUsed = true; }
						else
						{
							tree->expType = temp->expType;
							tree->isStatic = temp->isStatic;
							tree->isArray = temp->isArray;
							tree->varKind = temp->varKind;
							tree->size = temp->size;
							tree->loc = temp->loc;
							temp->isUsed = true;
						}
						//if(temp->kind.decl == FuncK) { printf("ERROR(%d): Cannot use function '%s' as a variable.\n", tree->lineno, temp->names); errors++; }
					}
					else if(!tree->isPassed) { printf("ERROR(%d): Variable '%s' is not declared.\n", tree->lineno, tree->names); errors++; tree->isPassed = true; }
				}
				break;
			case AssignK:
				if(!tree->isPassed) {
				//for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
				if(tree->names[0] == '=') 
				{
					if(tree->child[1] != NULL && (strcmp(tree->child[1]->names, "+=") == 0 || strcmp(tree->child[1]->names, "-=") == 0))
						newSemanticAnalysis(tree->child[1], table, warnings, errors, 1);
					if(tree->child[1] != NULL && tree->child[0] != NULL && strcmp(tree->child[0]->names, tree->child[1]->names) == 0 && tree->child[0]->names[0] != '[')
					{
						if(tree->child[1] != NULL)
							newSemanticAnalysis(tree->child[1], table, warnings, errors, 0);
						if(tree->child[0] != NULL)
							newSemanticAnalysis(tree->child[0], table, warnings, errors, 1);
					}
					else if(tree->child[1] != NULL && tree->child[0] != NULL)
					{
						if(tree->child[1]->names[0] == '[' && tree->child[1]->child[0] != NULL && strcmp(tree->child[0]->names, tree->child[1]->child[0]->names) == 0)
							newSemanticAnalysis(tree->child[1], table, warnings, errors, 0);
						if((tree->child[1]->names[0] == '/' || tree->child[1]->names[0] == '|') && tree->child[1]->child[0] != NULL && strcmp(tree->child[0]->names, tree->child[1]->child[0]->names) == 0)
						{
							if(tree->child[0]->kind.exp == IdK)
							{
								printf("WARNING(%d): Variable %s may be uninitialized when used here.\n", tree->lineno, tree->child[0]->names); tree->child[0]->isInit = true; warnings++;
							}
						}
						newSemanticAnalysis(tree->child[0], table, warnings, errors, 1);
					}
					else {
						if(tree->child[0] != NULL)
							newSemanticAnalysis(tree->child[0], table, warnings, errors, 1);
						if(tree->child[1] != NULL)
							newSemanticAnalysis(tree->child[1], table, warnings, errors, 0);
					}

				}
				if(tree->child[0] != NULL && setInit)
				{
					temp = (TreeNode *)table->lookup(tree->child[0]->names);
					if(temp != NULL) temp->isInit = true;
				}
				for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
				if(tree->child[0] != NULL)
				{
					if(strcmp(tree->names,"+=") == 0 || strcmp(tree->names,"-=") == 0)
					{
						tree->expType = Integer;
						if(tree->child[1] != NULL)
						{
							if(tree->child[0]->expType != Integer && tree->child[0]->expType != UndefinedType)
							{
								printf("ERROR(%d): '%s' requires operands of type int but lhs is of type %s.\n", tree->lineno, tree->names, returnTypes(tree->child[0]->expType));
								errors++;
							}
							if(tree->child[1]->expType != Integer && tree->child[0]->expType != UndefinedType)
							{
								printf("ERROR(%d): '%s' requires operands of type int but rhs is of type %s.\n", tree->lineno, tree->names, returnTypes(tree->child[1]->expType));
								errors++;
							}
						}
					}
					else if(strcmp(tree->names, "++") != 0 && strcmp(tree->names, "--") != 0) 
					{
						tree->expType = tree->child[0]->expType; 
						tree->isArray = tree->child[0]->isArray;
						if(tree->child[1] != NULL)
						{
							if(tree->child[0]->isArray && !tree->child[1]->isArray)
							{	
								printf("ERROR(%d): '%s' requires both operands be arrays or not but lhs is an array and rhs is not an array.\n", tree->lineno, tree->names);
								errors++;
							}
							else if(!tree->child[0]->isArray && tree->child[1]->isArray)
							{
								printf("ERROR(%d): '%s' requires both operands be arrays or not but lhs is not an array and rhs is an array.\n", tree->lineno, tree->names);
								errors++;
							}

							if(tree->child[0]->expType != tree->child[1]->expType && tree->child[0]->expType != UndefinedType && tree->child[1]->expType != UndefinedType)
							{
								printf("ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is type %s.\n", tree->lineno, tree->names, returnTypes(tree->child[0]->expType), returnTypes(tree->child[1]->expType));
								errors++;
							}
						} 
					}
					else 
					{ 
						tree->expType = Integer; 
						tree->isArray = false;
						temp = (TreeNode *)table->lookup(tree->child[0]->names);
						if(temp != NULL && temp->isArray)
						{
							printf("ERROR(%d): The operation '%s' does not work with arrays.\n", tree->lineno, tree->names);
							errors++;
						}
					}
					temp = (TreeNode *)table->lookup(tree->child[0]->names);
					if(temp != NULL) temp->isInit = true;
				}
				}
				tree->isPassed = true;
				break;
			case InitK:
				break;
			case CallK:
				paramCount = 0;
				if(tree->names != NULL)
				{
					temp = (TreeNode *)table->lookup(tree->names);
					if(temp != NULL && temp->kind.decl == FuncK)
					{
						tree->expType = temp->expType;
					}
					else if(temp != NULL && temp->kind.decl != FuncK)
					{
						printf("ERROR(%d): '%s' is a simple variable and cannot be called.\n", tree->lineno, tree->names);
						//temp->isUsed = true;
						errors++;
					}
					else
					{
						temp = (TreeNode *)table->lookupGlobal(tree->names);
						if(temp != NULL && temp->kind.decl == FuncK) tree->expType = temp->expType;
						else if(strcmp(tree->names, "output") == 0)
						{
							tree->expType = Void;
							/*if(tree->child[0] != NULL)
							{
								if(tree->child[0]->expType != (Boolean | Char | Integer)) temp = (TreeNode *)table->lookup(tree->child[0]->names);
								else temp = NULL;
								if(temp != NULL)
								{
									if(temp->expType != Integer)
									{
										printf("ERROR(%d): Expecting type int in parameter 1 of call to 'output' declared on line -1 but got type %s.\n", tree->lineno, returnTypes(tree->child[0]->expType));
										errors++;
									}
								}
								else 
								{
									if(tree->child[0]->expType != Integer)
									{
										printf("ERROR(%d): Expecting type int in parameter 1 of call to 'output' declared on line -1 but got type %s.\n", tree->lineno, returnTypes(tree->child[0]->expType));
										errors++;
									}
								}
							}*/
						}
						else if(strcmp(tree->names, "outputb") == 0)
						{
							tree->expType = Void;
							/*if(tree->child[0] != NULL)
							{
								if(tree->child[0]->expType != (Boolean | Char | Integer)) temp = (TreeNode *)table->lookup(tree->child[0]->names);
								else temp = NULL;
								if(temp != NULL)
								{
									if(temp->expType != Boolean)
									{
										printf("ERROR(%d): Expecting type bool in parameter 1 of call to 'outputb' declared on line -1 but got type %s.\n", tree->lineno, returnTypes(tree->child[0]->expType));
										errors++;
									}
								}
								else 
								{
									if(tree->child[0]->expType != Boolean)
									{
										printf("ERROR(%d): Expecting type bool in parameter 1 of call to 'outputb' declared on line -1 but got type %s.\n", tree->lineno, returnTypes(tree->child[0]->expType));
										errors++;
									}
								}
							}*/
						}
						else if(strcmp(tree->names, "outputc") == 0)
						{
							tree->expType = Void;
						}
						else if(strcmp(tree->names, "input") == 0)
						{
							tree->expType = Integer;
						}
						else if(strcmp(tree->names, "inputb") == 0)
						{
							tree->expType = Boolean;
						}
						else if(strcmp(tree->names, "inputc") == 0)
						{
							tree->expType = Char;
						}
						else if(strcmp(tree->names, "outnl") == 0)
						{
							tree->expType = Void;
						}
						else if (!tree->isPassed && strcmp(tree->names, "output") != 0 && strcmp(tree->names, "outputb") != 0 && strcmp(tree->names, "outputc") != 0 && strcmp(tree->names, "input") != 0 && strcmp(tree->names, "inputb") != 0 && strcmp(tree->names, "inputc") != 0 && strcmp(tree->names, "outnl") != 0) { printf("ERROR(%d): Function '%s' is not declared.\n", tree->lineno, tree->names); errors++; tree->isPassed = true; tree->expType = UndefinedType; tree->isError = true;}
					}

					//param stuff
					for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
					if(!tree->isPassed2)
					{
						int k = 1;
						if(tree->child[0] != NULL) curr = tree->child[0];
						else curr = NULL;
						if(temp != NULL && temp->child[0] != NULL) curr2 = temp->child[0];
						else curr2 = NULL;
						while(curr != NULL && curr2 != NULL)
						{
							if(curr->expType != curr2->expType && curr->expType != UndefinedType)
							{
								printf("ERROR(%d): Expecting type %s in parameter %d of call to '%s' declared on line %d but got type %s.\n", curr->lineno, returnTypes(curr2->expType), k, temp->names, temp->lineno, returnTypes(curr->expType));
								errors++;
							}
							if(curr->isArray && !curr2->isArray)
							{
								printf("ERROR(%d): Not expecting array in parameter %d of call to '%s' declared on line %d.\n", tree->lineno, k, temp->names, temp->lineno);
								errors++;
							}
							if(!curr->isArray && curr2->isArray)
							{
								printf("ERROR(%d): Expecting array in parameter %d of call to '%s' declared on line %d.\n", tree->lineno, k, temp->names, temp->lineno);
								errors++;
							}
							curr = curr->sibling;
							curr2 = curr2->sibling;
							k++;
						}
						if(curr == NULL && curr2 != NULL && !tree->isError && temp != NULL)
						{
							printf("ERROR(%d): Too few parameters passed for function '%s' declared on line %d.\n", tree->lineno, temp->names, temp->lineno);
							errors++;
						}
						else if(curr != NULL && curr2 == NULL && !tree->isError && temp != NULL)
						{
							printf("ERROR(%d): Too many parameters passed for function '%s' declared on line %d.\n", tree->lineno, temp->names, temp->lineno);
							errors++;
						}
						tree->isPassed2 = true;
					}
					
				}
				break;
			case ParamIdK:
				temp = (TreeNode *)table->lookupRecent(tree->names);
				if(temp != NULL) { printf("ERROR(%d): Symbol '%s' is already declared at line %d.\n", tree->lineno, tree->names, temp->lineno); temp->isInit = true; errors++; }
				else { table->insert(tree->names, (TreeNode *)tree); tree->isUsed = false; tree->varKind = Parameter; tree->size = 1; tree->loc = foffset; foffset--; }
				break;
			default:
				break;
			}
		}
		// decl kind semantic analysis
		else if (tree->nodekind == DeclK)
		{
			switch (tree->kind.decl)
			{
			case VarK:
				if(tree->child[0] != NULL && strcmp(tree->child[0]->names, tree->names) == 0)
				{
					temp = (TreeNode *)table->lookup(tree->child[0]->names);
					if(temp == NULL)
					{
						printf("ERROR(%d): Variable '%s' is not declared.\n", tree->lineno, tree->names);
						errors++;
					}
				}
				temp = (TreeNode *)table->lookupRecent(tree->names);
				if(temp != NULL) { printf("ERROR(%d): Symbol '%s' is already declared at line %d.\n", tree->lineno, tree->names, temp->lineno); errors++; }
				else if(!tree->isPassed) { 
					table->insert(tree->names, (TreeNode *)tree); 
					tree->isUsed = false; if(setInit) tree->isInit = true; 
					if(tree->child[0] != NULL && !tree->child[0]->isPassed) { newSemanticAnalysis(tree->child[0], table, warnings, errors, 0); tree->child[0]->isPassed = true; }
					if(table->scopeName() == "Global") 
					{ 
						if(tree->isArray) goffset--; 
						tree->varKind = Global; 
						tree->loc = goffset; 
						if(tree->isArray && tree->child[0] != NULL) goffset++;
						if(!tree->isArray && tree->child[0] != NULL) ;
						if(tree->isArray && tree->child[0] == NULL) goffset++;
					} 
					else if(tree->isStatic) 
					{
						if(tree->isArray) goffset--;
						tree->varKind = LocalStatic; 
						tree->loc = goffset; 
						if(tree->isArray) goffset++;
					} 
					else 
					{ 
						if(tree->isArray) { foffset--; }
						tree->varKind = Local; 
						tree->loc = foffset; 
					} 
					if(!tree->isArray) 
					{
						tree->size = 1; 
						if(tree->varKind == Global || tree->varKind == LocalStatic) goffset = goffset - tree->size; 
						else { foffset = foffset - tree->size; }
					}
					else
					{ 
						tree->size = tree->attr.value + 1; 
						tree->loc = tree->loc;
						if(tree->varKind == Global || tree->varKind == LocalStatic) goffset = goffset - tree->size; 
						else { foffset = foffset - tree->size + 1; }
					}
					tree->isPassed = true;
				} //put array size here and change foffset / goffset here
				if(tree->child[0] != NULL && tree->child[0]->names[0] == '*')
				{
					if(tree->child[0]->child[0] != NULL)
					{
						temp = (TreeNode *)table->lookup(tree->child[0]->child[0]->names);
						if(temp != NULL)
						{
							if(!temp->isConst)
							{
								printf("ERROR(%d): Initializer for variable '%s' is not a constant expression.\n", tree->lineno, tree->names);
								errors++;
							}
						}
					}
				}
				if(tree->child[0] != NULL)
				{
					temp = (TreeNode *)table->lookup(tree->child[0]->names);
					if(temp == NULL)
					{
						/*if(!tree->child[0]->isConst)
						{
							printf("ERROR(%d): Initializer for variable '%s' is not a constant expressions.\n", tree->lineno, tree->names);
							errors++;
						}*/
						if(tree->child[0]->expType != tree->expType)
						{
							printf("ERROR(%d): Variable '%s' is of type %s but is being initialized with an expression of type %s.\n", tree->lineno, tree->names, returnTypes(tree->expType), returnTypes(tree->child[0]->expType));
							tree->isConst = false;
							errors++;
						}
						else tree->isConst = true;
					}
					else 
					{
						if(!temp->isConst)
						{
							printf("ERROR(%d): Initializer for variable '%s' is not a constant expression.\n", tree->lineno, tree->names);
							tree->isConst = false;
							errors++;
						}
						else tree->isConst = true;
					}
				}
				break;
			case FuncK:
				//--------
				paramCount = 0;
				foffset = -2;
				temp = (TreeNode *)table->lookupGlobal(tree->names);
				if(temp != NULL) { printf("ERROR(%d): Symbol '%s' is already declared at line %d.\n", tree->lineno, tree->names, temp->lineno); errors++; }
				table->insertGlobal(tree->names, (TreeNode *)tree);
				currFunc = tree;
				currFunction = tree->names;
				// Param stuff
				if(tree->child[0] != NULL) curr = tree->child[0];
				while(curr != NULL) { paramCount++; curr = curr->sibling; }
				tree->paramTotal = paramCount;
				//-----^
				if(strcmp(tree->names, "main") == 0) isMain = true;
				table->enter(tree->names);
				if(tree->child[1] != NULL && tree->child[1]->kind.stmt == CompoundK) tree->child[1]->isComp = false;
				//----------
				if(tree->expType != Void && tree->expType != UndefinedType) tree->isReturn = true;
				else tree->isReturn = false;
				//for(i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table);
				break;
			case ParamK:
				temp = (TreeNode *)table->lookupRecent(tree->names);
				if(temp != NULL) { printf("ERROR(%d): Symbol '%s' is already declared at line %d.\n", tree->lineno, tree->names, temp->lineno); temp->isInit = true; errors++; }
				else { table->insert(tree->names, (TreeNode *)tree); tree->isUsed = false; tree->loc = foffset; foffset--; tree->varKind = Parameter; tree->size = 1; }
				break;
			default:
				break;
			}
		}
		for (i = 0; i < MAXCHILDREN; i++) newSemanticAnalysis(tree->child[i], table, warnings, errors, 0);
		if((tree->nodekind == DeclK && tree->kind.decl == FuncK) || (tree->nodekind == StmtK && tree->kind.stmt == CompoundK && tree->isComp == true) || (tree->nodekind == StmtK && tree->kind.stmt == ForK))
		{
			//-------
			if(tree->kind.decl == FuncK)
			{
				tree = currFunc;
				if(tree->expType != UndefinedType && tree->expType != Void && tree->isReturn && !tree->returned)
				{
					printf("WARNING(%d): Expecting to return type %s but function '%s' has no return statement.\n", tree->lineno, returnTypes(tree->expType), tree->names);
					warnings++;
				}
			}
			if(tree->kind.stmt == CompoundK && tree->isComp) { /*foffset = coffset;*/ foffset = curr_offset[curr_iter]; curr_iter--; }
			if(tree->kind.stmt == ForK) foffset = for_offset;
			//for(i = 0; i < MAXCHILDREN; i++) printWarnings(tree->child[i], table, warnings);
			warningsCount = 0;
			table->applyToAll(printWarnings);
			warnings += warningsCount;
			table->leave();
		}
		if(tree->kind.stmt == WhileK && setWhile == 1) isWhile = false;
		tree = tree->sibling;
	}
}

void printWarnings(std::string sym, void *ptr)
{
	TreeNode *temp = (TreeNode *)ptr;
	if(!temp->isUsed)
	{
		printf("WARNING(%d): The variable %s seems not to be used.\n", temp->lineno, temp->names);
		warningsCount++;
	}
}

/*void printWarnings(TreeNode *tree, SymbolTable *table, int &warnings)
{
	int i;
	while(tree != NULL)
	{
		for(i = 0; i < MAXCHILDREN; i++) printWarnings(tree->child[i], table, warnings);
		TreeNode *temp = (TreeNode *)table->lookup(tree->names);
		if(temp != NULL && (temp->kind.decl == VarK || temp->kind.decl == ParamK || temp->kind.exp == ParamIdK))
		{
			if(!temp->isUsed && !temp->isPassed3) { printf("WARNING(%d): The variable %s seems not to be used.\n", tree->lineno, tree->names); temp->isPassed3 = true; warnings++; }
		}
		tree = tree->sibling;
	}
}*/

void printIsMain(int &errors)
{
	if(!isMain)	{ printf("ERROR(LINKER): Procedure main is not declared.\n"); errors++; }
}

