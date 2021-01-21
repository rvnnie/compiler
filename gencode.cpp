/*******************
 * Ronald Keating
 * CS445
 * gencode.cpp
*******************/

#include "gencode.h"

//All definitions, saves time and easier to read
#define ST (char *)"ST"
#define JMP (char *)"JMP"
#define LD (char *)"LD"
#define LDC (char *)"LDC"
#define LDA (char *)"LDA"
#define HALT (char *)"HALT"
#define SUB (char *)"SUB"
#define ADD (char *)"ADD"
#define TLT (char *)"TLT"
#define TGT (char *)"TGT"
#define TEQ (char *)"TEQ"
#define TNE (char *)"TNE"
#define TGE (char *)"TGE"
#define TLE (char *)"TLE"
#define JZR (char *)"JZR"
#define MUL (char *)"MUL"
#define NEG (char *)"NEG"
#define JNZ (char *)"JNZ"
#define OR (char *)"OR"
#define MOD (char *)"MOD"
#define DIV (char *)"DIV"
#define XOR (char *)"XOR"

//Globals
int IOLoc[7];
int IOcount = 0, toffset = 0, temp1;
bool isParam = false;
TreeNode *temp;
int loopBegin = 0; //attempt to do break stuff
int whileLoops[10]; //more break stuff

// IO function stuff
const int numIO = 7;
char IO[numIO][numIO + 1] = {"input", "inputb", "inputc", "output", "outputb", "outputc", "outnl"};

void pushLeft(TreeNode *tree, SymbolTable *table)
{
    emitRM(LDA, 3, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Load address of base of array", tree->child[0]->names);
    emitRM(ST, 3, toffset, 1, (char *)"Push left side");
    if(tree->child[1]->kind.exp == ConstantK) treeGenCode(tree->child[1], table);
    else ;
    emitRM(LD, 4, toffset, 1, (char *)"Pop left into ac1");
    emitRO(SUB, 3, 4, 3, (char *)"compute location from index");
    emitRM(LD, 3, 0, 3, (char *)"Load array element");
}

void initGlobals(TreeNode *tree, SymbolTable *table)
{
    while(tree != NULL)
    {
        if(tree->isColon) 
        {
            if(tree->child[0] != NULL)
            {
                if(tree->expType == Integer)
                    emitRM(LDC, 3, atoi(tree->child[0]->names), 6, (char *)"Load integer constant");
                else if(tree->expType == Boolean)
                {
                    if(tree->child[0]->names[0] == 't')
                        emitRM(LDC, 3, 1, 6, (char *)"Load integer constant");
                    else if(tree->child[0]->names[0] == 'f')
                        emitRM(LDC, 3, 0, 6, (char *)"Load integer constant");
                }
            }
            emitRM(ST, 3, temp->loc, 0, (char *)"Store variable", temp->names);
        }
        else if(tree->isArray && tree->kind.decl == VarK)
        {
            emitRO(LDC, 3, tree->size - 1, 6, (char *)"load size of array", tree->names);
            emitRO(ST, 3, toffset, 0, (char *)"save size of array", tree->names);
        }
        tree = tree->sibling;
    }
}

void genCode(TreeNode *tree, SymbolTable *table, char *fileComp)
{
    int i, endOfGlobal = 0;
    // start header comment
    emitComment((char *)"C- compiler version C-F20");
    emitComment((char *)"Built: Dec 11, 2020");
    emitComment((char *)"Author: Ronald Keating");
    emitComment((char *)"File compiled: ", fileComp);
    emitComment((char *)"");
    //end of header comment

    // find end of globals
    temp = tree;
    while(temp != NULL)
    {
        if(temp->kind.decl == VarK)
            endOfGlobal -= temp->size;
        temp = temp->sibling;
    }
    emitSkip(1); //start at one
    //IO functions
    for(i = 0; i < numIO; i++)
    {
        IOFunctions(IO[i], i);
        emitSkip(0);
    }
    emitComment((char *)"** ** ** ** ** ** ** ** ** ** ** **");
    treeGenCode(tree, table);
    backPatchAJumpToHere(0, (char *)"Jump to init [backpatch]");
    emitComment((char *)"INIT");
    emitRM(LD, 0, 0, 0, (char *)"Set the global pointer");
    emitRM(LDA, 1, endOfGlobal, 0, (char *)"Set first frame at end of globals");
    emitRM(ST, 1, 0, 1, (char *)"Store old fp (point to self)");
    emitComment((char *)"INIT GLOBALS AND STATICS");
    //do something with table
    temp = tree;
    initGlobals(temp, table);
    emitComment((char *)"END INIT GLOBALS AND STATICS");
    emitRM(LDA, 3, 1, 7, (char *)"Return address in ac");
    //find main offset
    temp = (TreeNode *)table->lookupGlobal("main");
    emitGotoAbs(temp->offset, (char *)"Jump to main");
    emitRM(HALT, 0 , 0, 0, (char *)"DONE!");
    emitComment((char *)"END INIT");
}

//FUNCTIONS
void midInput()
{
    emitRM(LD, 3, -1, 1, (char *)"Load return address");
    emitRM(LD, 1, 0, 1, (char *)"Adjust fp");
}

// creates IO functions and locations
void IOFunctions(char *function, int loc)
{
    emitComment((char *)"** ** ** ** ** ** ** ** ** ** ** **");
    emitComment((char *)"FUNCTION", function);
    IOLoc[loc] = emitSkip(0);
    emitRM(ST, 3, -1, 1, (char *)"Store return address");
    if(strcmp(function, "input") == 0)
    {
        emitRO((char *)"IN", 2, 2, 2, (char *)"Grab int input");
        midInput();
    }
    else if(strcmp(function, "inputb") == 0)
    {
        emitRO((char *)"INB", 2, 2, 2, (char *)"Grab bool input");
        midInput();
    }
    else if(strcmp(function, "inputc") == 0)
    {
        emitRO((char *)"INC", 2, 2, 2, (char *)"Grab char input");
        midInput();
    }
    else if(strcmp(function, "output") == 0)
    {
        emitRM(LD, 3, -2, 1, (char *)" Load parameter");
        emitRO((char *)"OUT", 3, 3, 3, (char *)"Output integer");
        midInput();
    }
    else if(strcmp(function, "outputb") == 0)
    {
        emitRM(LD, 3, -2, 1, (char *)" Load parameter");
        emitRO((char *)"OUTB", 3, 3, 3, (char *)"Output bool");
        midInput();
    }
    else if(strcmp(function, "outputc") == 0)
    {
        emitRM(LD, 3, -2, 1, (char *)"Load parameter");
        emitRO((char *)"OUTC", 3, 3, 3, (char *)"Output char");
        midInput();
    }
    else if(strcmp(function, "outnl") == 0)
    {
        emitRO((char *)"OUTNL", 3, 3, 3, (char *)"Output newline");
        midInput();
    }
    emitRM(JMP, 7, 0, 3, (char *)" Return");
    emitComment((char *)"END FUNCTION", function);
    emitComment((char *)"");
}

void treeGenCode(TreeNode *tree, SymbolTable *table)
{
    int i;
    while (tree != NULL)
	{
        // Statement kind
		if (tree->nodekind == StmtK)
		{
			switch (tree->kind.stmt)
			{
			case IfK:
                //check for else statement
                if(tree->child[2] != NULL/* && tree->child[2]->kind.stmt == IfK*/) tree->elseStmt = true;
                else tree->elseStmt = false;
                //first child
                treeGenCode(tree->child[0], table);
                //emit skip here, will backup later
                tree->temp1 = emitSkip(1);
                //second child
                treeGenCode(tree->child[1], table);
                //determine offset of distance between if and else or else if
                tree->temp2 = emitSkip(0) - tree->temp1;
                //return
                emitBackup(tree->temp1);
                //emitRO(JZR, 3, 8, 7, (char *)"Jump around the THEN if false [backpatch]");
                //if no else, then go back one because it is not there
                if(!tree->elseStmt) 
                {
                    emitRO(JZR, 3, tree->temp2 - 1, 7, (char *)"Jump around the THEN if false [backpatch]");
                    //skip back to previous location
                    emitSkip(tree->temp2 - 1);
                }
                // if there is an else, perform action
                else
                {
                    emitRO(JZR, 3, tree->temp2, 7, (char *)"Jump around the THEN if false [backpatch]");
                    //skip back to previous location
                    emitSkip(tree->temp2);
                    //save state
                    tree->temp1 = emitSkip(0);
                    //cycle to else stmt
                    treeGenCode(tree->child[2], table);
                    //save state for backup
                    tree->temp2 = emitSkip(0);
                    //backup
                    emitBackup(tree->temp1 - 1);
                    //tree->temp2 - tree->temp1 = jump location of else
                    emitRO(JZR, 3, tree->temp2 - tree->temp1, 7, (char *)"Jump around the THEN if false [backpatch]");
                    //return (end of if else)
                    emitBackup(tree->temp2);
                }
                break;
			case NullK:
				break;
			case WhileK:
                emitComment((char *)"WHILE");
                // save state 
                //loopBegin++;
                // didnt end up using
                whileLoops[loopBegin] = emitSkip(0);
                // didnt use
                loopBegin++;
                tree->offset = emitSkip(0);
                // check to make sure that there is a statement
                if(tree->child[1] != NULL) treeGenCode(tree->child[0], table);
                emitRM(JNZ, 3, 1, 7, (char *)"Jump to while part");
                // skip for backup
                tree->temp1 = emitSkip(1);
                //check if there is statement again
                if(tree->child[1] != NULL) treeGenCode(tree->child[1], table);
                else treeGenCode(tree->child[0], table);
                emitRM(JMP, 7, tree->offset - emitSkip(0) - 1, 7, (char *)"go to beginning of loop");
                //curr state
                tree->temp2 = emitSkip(0);
                //backup to previous state
                emitBackup(tree->temp1);
                //emit
                emitRM(JMP, 7, tree->temp2 - tree->temp1 - 1, 7, (char *)"Jump past loop [backpatch]");
                //return
                emitSkip(tree->temp2 - emitSkip(0));
                // no use
                loopBegin--;
				break;
			case ForK:
                // no for loops
				break;
			case CompoundK:
                //save offset state for after compound is done
                temp1 = toffset;
                emitComment((char *)"COMPOUND");
                temp = tree->child[0];
                //if is vark with initializer, decrement toffset
                while(temp != NULL)
                {
                    if(temp->isColon) toffset--;
                    temp = temp->sibling;
                }
                // go to first child of comp
                temp = tree->child[0];
                //cycle through everything
                while(temp != NULL)
                {
                    if(!temp->isArray)
                    {
                        if(temp->child[0] != NULL) treeGenCode(temp->child[0], table);
                        if(temp->isColon) emitRM(ST, 3, temp->loc, isGlobal(temp), (char *)"Store variable", temp->names);
                        if(!temp->isColon) toffset--;
                    }
                    else if(temp->isArray)
                    {
                        emitRM(LDC, 3, temp->size - 1, 6, (char *)"load size of array", temp->names);
                        emitRM(ST, 3, temp->loc + 1, isGlobal(tree), (char *)"save size of array", temp->names);
                        toffset -= temp->size;
                    }
                    temp = temp->sibling;
                }
                emitComment((char *)"Compound Body");
                //treeGenCode(tree->child[0], table);
                //continue cycle
                treeGenCode(tree->child[1], table);
                //reset toffset back to state prior to compound (function, while loop, etc)
                toffset = temp1;
                emitComment((char *)"END COMPOUND");
				break;
			case ReturnK:
                // just trying to make it work here, not effective or efficient
                emitComment((char *)"RETURN");
                if(tree->child[0] != NULL && tree->child[0]->kind.exp == IdK)
                {
                    emitRM(LD, 3, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Load variable", tree->child[0]->names);
                    emitRM(LDA, 2, 0, 3, (char *)"Copy result to return register");
                }
                else treeGenCode(tree->child[0], table);
                emitRM(LD, 3, -1, 1, (char *)"Load return address");
                emitRM(LD, 1, 0, 1, (char *)"Adjust fp");
                emitRM(JMP, 7, 0, 3, (char *)"Return");
				break;
			case BreakK:
                // 
				break;
			default:
				break;
			}
		}

        // Exp kind
		else if (tree->nodekind == ExpK)
		{
            emitComment((char *)"EXPRESSION");
            // horribly done, based on specific instances, tedious
            // Which exp kind
			switch (tree->kind.exp)
			{
			case OpK:
                if(tree->names[0] == '[')
                {
                    if(tree->child[0]->kind.decl == VarK && isGlobal(tree->child[0]) == 1 && tree->child[1]->kind.exp == ConstantK)
                    {
                        emitRO(LDC, 3, tree->child[1]->attr.value, 6, (char *)"load size of array", tree->child[0]->names);
                        emitRO(ST, 3, toffset, 1, (char *)"save size of array", tree->child[0]->names);
                        toffset -= tree->child[1]->attr.value;
                    }
                }
                else if(strcmp(tree->names, "*") == 0 && tree->child[1] != NULL)
                {
                    if(tree->child[0]->names[0] == '[')
                    {
                        pushLeft(tree->child[0], table);
                    }
                    else treeGenCode(tree->child[0], table);
                    emitRM(ST, 3, toffset, 1, (char *)"Push left side");
                    toffset--;
                    if(tree->child[1]->names[0] == '[')
                    {
                        pushLeft(tree->child[1], table);
                    }
                    else treeGenCode(tree->child[1], table);
                    toffset++;
                    emitRM(LD, 4, toffset, 1, (char *)"Pop left into ac1");
                    emitRO(MUL, 3, 4, 3, (char *)"Op *");
                }
                else if(strcmp(tree->names, "%") == 0 && tree->child[1] != NULL)
                {
                    if(tree->child[0]->names[0] == '[')
                    {
                        pushLeft(tree->child[0], table);
                    }
                    else treeGenCode(tree->child[0], table);
                    emitRM(ST, 3, toffset, 1, (char *)"Push left side");
                    toffset--;
                    if(tree->child[1]->names[0] == '[')
                    {
                        pushLeft(tree->child[1], table);
                    }
                    else treeGenCode(tree->child[1], table);
                    toffset++;
                    emitRM(LD, 4, toffset, 1, (char *)"Pop left into ac1");
                    emitRO(MOD, 3, 4, 3, (char *)"Op %");
                }
                else if(strcmp(tree->names, "/") == 0 && tree->child[1] != NULL)
                {
                    if(tree->child[0]->names[0] == '[')
                    {
                        pushLeft(tree->child[0], table);
                    }
                    else treeGenCode(tree->child[0], table);
                    emitRM(ST, 3, toffset, 1, (char *)"Push left side");
                    toffset--;
                    if(tree->child[1]->names[0] == '[')
                    {
                        pushLeft(tree->child[1], table);
                    }
                    else treeGenCode(tree->child[1], table);
                    toffset++;
                    emitRM(LD, 4, toffset, 1, (char *)"Pop left into ac1");
                    emitRO(DIV, 3, 4, 3, (char *)"Op /");
                }
                else if(strcmp(tree->names, "!") == 0 && tree->child[1] == NULL)
                {
                    if(tree->child[0]->names[0] == '[')
                    {
                        pushLeft(tree->child[0], table);
                    }
                    else treeGenCode(tree->child[0], table);
                    emitRM(LDC, 4, 1, 6, (char *)"Load 1");
                    emitRO(XOR, 3, 3, 4, (char *)"Op XOR to get logical not");
                }
                else if(strcmp(tree->names, "*") == 0 && tree->child[1] == NULL)
                {
                    emitRM(LDA, 3, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Load address of base of array", tree->child[0]->names);
                    emitRM(LD, 3, 1, 3, (char *)"Load array size");
                }
                else if(tree->names[0] == '+')
                {
                    if(tree->child[0]->names[0] == '[')
                    {
                        pushLeft(tree->child[0], table);
                    }
                    else treeGenCode(tree->child[0], table);
                    emitRM(ST, 3, toffset, 1, (char *)"Push left side");
                    toffset--;
                    if(tree->child[1]->names[0] == '[')
                    {
                        pushLeft(tree->child[1], table);
                    }
                    else treeGenCode(tree->child[1], table);
                    toffset++;
                    emitRM(LD, 4, toffset, 1, (char *)"Pop left into ac1");
                    emitRO(ADD, 3, 4, 3, (char *)"Op +");
                }
                else if(strcmp(tree->names, "-") == 0 && tree->child[1] == NULL)
                {
                    treeGenCode(tree->child[0], table);
                    emitRO(NEG, 3, 3, 3, (char *)"Op unary -");
                }
                else if(strcmp(tree->names, "-") == 0 && tree->child[1] != NULL)
                {
                    if(tree->child[0]->names[0] == '[')
                    {
                        pushLeft(tree->child[0], table);
                    }
                    else treeGenCode(tree->child[0], table);
                    emitRM(ST, 3, toffset, 1, (char *)"Push left side");
                    toffset--;
                    if(tree->child[1]->names[0] == '[')
                    {
                        pushLeft(tree->child[1], table);
                    }
                    else treeGenCode(tree->child[1], table);
                    toffset++;
                    emitRM(LD, 4, toffset, 1, (char *)"Pop left into ac1");
                    emitRO(SUB, 3, 4, 3, (char *)"Op -");
                }
                else if(strcmp(tree->names, "|") == 0 && tree->child[1] != NULL)
                {
                    if(tree->child[0]->names[0] == '[')
                    {
                        pushLeft(tree->child[0], table);
                    }
                    else treeGenCode(tree->child[0], table);
                    emitRM(ST, 3, toffset, 1, (char *)"Push left side");
                    toffset--;
                    if(tree->child[1]->names[0] == '[')
                    {
                        pushLeft(tree->child[1], table);
                    }
                    else treeGenCode(tree->child[1], table);
                    toffset++;
                    emitRM(LD, 4, toffset, 1, (char *)"Pop left into ac1");
                    emitRO(OR, 3, 4, 3, (char *)"Op OR");
                }
                else if(tree->names[0] == '[')
                {
                    treeGenCode(tree->child[1], table);
                    emitRM(ST, 3, -2, 1, (char *)"Push Index");
                
                }
                else if(strcmp(tree->names, "<") == 0)
                {
                    treeGenCode(tree->child[0], table);
                    emitRM(ST, 3, toffset, 1, (char *)"Push left side");
                    treeGenCode(tree->child[1], table);
                    emitRM(LD, 4, toffset, 1, (char *)"Pop left into ac1");
                    emitRO(TLT, 3, 4, 3, (char *)"Op <");
                }
                else if(strcmp(tree->names, ">") == 0)
                {
                    treeGenCode(tree->child[0], table);
                    emitRM(ST, 3, toffset, 1, (char *)"Push left side");
                    treeGenCode(tree->child[1], table);
                    emitRM(LD, 4, toffset, 1, (char *)"Pop left into ac1");
                    emitRO(TGT, 3, 4, 3, (char *)"Op >");
                }
                else if(strcmp(tree->names, "==") == 0)
                {
                    treeGenCode(tree->child[0], table);
                    emitRM(ST, 3, toffset, 1, (char *)"Push left side");
                    treeGenCode(tree->child[1], table);
                    emitRM(LD, 4, toffset, 1, (char *)"Pop left into ac1");
                    emitRO(TEQ, 3, 4, 3, (char *)"Op ==");
                }
                else if(strcmp(tree->names, "!=") == 0)
                {
                    treeGenCode(tree->child[0], table);
                    emitRM(ST, 3, toffset, 1, (char *)"Push left side");
                    treeGenCode(tree->child[1], table);
                    emitRM(LD, 4, toffset, 1, (char *)"Pop left into ac1");
                    emitRO(TNE, 3, 4, 3, (char *)"Op !=");
                }
                else if(strcmp(tree->names, ">=") == 0)
                {
                    treeGenCode(tree->child[0], table);
                    emitRM(ST, 3, toffset, 1, (char *)"Push left side");
                    treeGenCode(tree->child[1], table);
                    emitRM(LD, 4, toffset, 1, (char *)"Pop left into ac1");
                    emitRO(TGE, 3, 4, 3, (char *)"Op >=");
                }
                else if(strcmp(tree->names, "<=") == 0)
                {
                    treeGenCode(tree->child[0], table);
                    emitRM(ST, 3, toffset, 1, (char *)"Push left side");
                    treeGenCode(tree->child[1], table);
                    emitRM(LD, 4, toffset, 1, (char *)"Pop left into ac1");
                    emitRO(TLE, 3, 4, 3, (char *)"Op <=");
                }
				break;
			case ConstantK:
                //had to check if constants were used already, started printing out twice in some circumstances
                // quick fix
				switch (tree->expType)
				{
				case Boolean:
                    if(!tree->used) emitRM(LDC, 3, tree->attr.value, 6, (char *)"Load bool constant");
                    tree->used = true;
					break;
				case Char:
                    if(!tree->used) emitRM(LDC, 3, tree->attr.cvalue, 6, (char *)"Load char constant");
                    tree->used = true;
					break;
                case Integer:
                    if(!tree->used) emitRM(LDC, 3, tree->attr.value, 6, (char *)"Load integer constant");
                    tree->used = true;
                    break;
				default:
					break;
				}
				break;
			case IdK:
                if(tree->isArray)
                {
                    if(tree->varKind == Parameter && isGlobal(tree) == 1) emitRM(LDA, 3, tree->loc, isGlobal(tree), (char *)"Load address of base of array", tree->names);
                    else if(isGlobal(tree) == 1) emitRM(LDA, 3, tree->loc, isGlobal(tree), (char *)"Load address of base of array", tree->names);
                }
                else 
                {
                    if(isGlobal(tree) == 1 || tree->kind.exp == IdK) emitRM(LD, 3, tree->loc, isGlobal(tree), (char *)"Load variable", tree->names);
                }
                break;
			case AssignK:
                // even worse than OpK, I cant figure out arrays fully
                if(strcmp(tree->names, "=") == 0)
                {
                    if(tree->child[0]->kind.exp == IdK && tree->child[1]->kind.exp == IdK)
                    {
                        emitRM(LD, 3, tree->child[1]->loc, isGlobal(tree->child[1]), (char *)"Load variable", tree->child[1]->names);
                        emitRM(ST, 3, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Store variable", tree->child[0]->names);
                    }
                    else if(tree->child[0]->kind.exp == IdK && tree->child[1]->kind.exp == ConstantK)
                    {
                        //emitRM(LDC, 3, tree->child[1]->attr.value, 6, (char *)"Load integer constant");
                        treeGenCode(tree->child[1], table);
                        emitRM(ST, 3, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Store variable", tree->child[0]->names);
                    }
                    else if(tree->child[0]->names[0] == '[' && tree->child[1]->kind.exp == ConstantK)
                    {
                        treeGenCode(tree->child[0]->child[1], table);
                        emitRM(ST, 3, toffset, 1, (char *)"Push index");
                        treeGenCode(tree->child[1], table);
                        emitRM(LD, 4, toffset, 1, (char *)"Pop index");
                        emitRM(LDA, 5, tree->child[0]->child[0]->loc, isGlobal(tree->child[0]->child[0]), (char *)"Load address of base of array", tree->child[0]->child[0]->names);
                        emitRO(SUB, 5, 5, 4, (char *)"Compute offset of value");
                        emitRM(ST, 3, 0, 5, (char *)"Store variable", tree->child[0]->child[0]->names);
                    }
                    else if(tree->child[0]->names[0] == '[')
                    {
                        treeGenCode(tree->child[0], table);
                        treeGenCode(tree->child[1], table);
                        emitRM(LD, 4, -2, 1, (char *)"Pop index");
                        treeGenCode(tree->child[0]->child[0], table);
                        if(tree->child[0]->child[0]->varKind == Global) emitRM(LDA, 5, tree->child[0]->child[0]->loc, 0, (char *)"Load address of base of array", tree->child[0]->child[0]->names);
                        emitRO(SUB, 5, 5, 4, (char *)"Compute offset of value");
                        emitRM(ST, 3, 0, 5, (char *)"Store variable", tree->child[0]->child[0]->names);
                    }
                    else if(tree->child[0]->names[0] == '[' && tree->child[1]->names[1] == '[')
                    {
                        treeGenCode(tree->child[1], table);
                        treeGenCode(tree->child[0], table);
                        emitRM(ST, 3, tree->child[0]->child[0]->loc, isGlobal(tree->child[0]->child[0]), (char *)"Store variable", tree->child[0]->child[0]->names);
                    }
                    else 
                    {
                        treeGenCode(tree->child[0], table);
                        treeGenCode(tree->child[1], table);
                        if(tree->child[0]->kind.exp == IdK)
                            emitRM(ST, 3, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Store variable", tree->child[0]->names);
                    }
                }
                // decent here, although repetitive, need to clean up
                else if(strcmp(tree->names, "--") == 0)
                {
                    emitRM(LD, 3, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"load lhs variable", tree->child[0]->names);
                    emitRM(LDA, 3, -1, 3, (char *)"decrement value of", tree->child[0]->names);
                    emitRM(ST, 3, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Store variable", tree->child[0]->names);
                }
                else if(strcmp(tree->names, "++") == 0)
                {
                    emitRM(LD, 3, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"load lhs variable", tree->child[0]->names);
                    emitRM(LDA, 3, 1, 3, (char *)"increment value of", tree->child[0]->names);
                    emitRM(ST, 3, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Store variable", tree->child[0]->names);
                }
                else if(strcmp(tree->names, "-=") == 0)
                {
                    if(tree->child[0]->kind.exp == IdK && tree->child[1]->kind.exp == IdK)
                    {
                        emitRM(LD, 3, tree->child[1]->loc, isGlobal(tree->child[1]), (char *)"Load variable", tree->child[1]->names);
                        emitRM(LD, 4, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Load lhs variable", tree->child[0]->names);
                        emitRO(SUB, 3, 4, 3, (char *)"op -=");
                        emitRM(ST, 3, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Store variable", tree->child[0]->names);
                    }
                    else if(tree->child[0]->kind.exp == IdK && tree->child[1]->kind.exp == AssignK)
                    {
                        treeGenCode(tree->child[1], table);
                        emitRM(LD, 4, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Load lhs variable", tree->child[0]->names);
                        emitRO(SUB, 3, 4, 3, (char *)"op -=");
                        emitRM(ST, 3, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Store variable", tree->child[0]->names);
                    }
                }
                else if(strcmp(tree->names, "+=") == 0)
                {
                    if(tree->child[0]->kind.exp == IdK && tree->child[1]->kind.exp == IdK)
                    {
                        emitRM(LD, 3, tree->child[1]->loc, isGlobal(tree->child[1]), (char *)"Load variable", tree->child[1]->names);
                        emitRM(LD, 4, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Load lhs variable", tree->child[0]->names);
                        emitRO(ADD, 3, 4, 3, (char *)"op +=");
                        emitRM(ST, 3, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Store variable", tree->child[0]->names);
                    }
                    else if(tree->child[0]->kind.exp == IdK && tree->child[1]->kind.exp == AssignK)
                    {
                        treeGenCode(tree->child[1], table);
                        emitRM(LD, 4, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Load lhs variable", tree->child[0]->names);
                        emitRO(ADD, 3, 4, 3, (char *)"op +=");
                        emitRM(ST, 3, tree->child[0]->loc, isGlobal(tree->child[0]), (char *)"Store variable", tree->child[0]->names);
                    }
                }
                break;
			case CallK:
                emitComment((char *)"CALL", tree->names);
                emitRM(ST, 1, toffset, 1, (char *)"Store fp in ghost frame for output");
                toffset -= 2;
                tree->isRegFunc = true;
                // check to see if IO function, and if so NOT regular function
                for(i = 0; i < numIO; i++)
                {
                    if(strcmp(IO[i], tree->names) == 0)
                    {
                        tree->isRegFunc = false; // see, not regular function
                        isParam = true;
                        treeGenCode(tree->child[0], table);
                        isParam = false;
                        // this should always be not NULL unless outnl
                        if(tree->child[0] != NULL)
                        {
                            if(tree->child[0]->isArray) ;
                            emitRM(ST, 3, toffset, 1, (char *)"Push parameter");
                            emitComment((char *)"Param end", tree->names);
                        }
                        toffset += 2;
                        emitRM(LDA, 1, toffset, 1, (char *)"Load address of new frame");
                        emitRM(LDA, 3, 1, 7, (char *)"Return address in ac");
                        emitGotoAbs(IOLoc[i], (char *)"CALL", tree->names);
                        emitRM(LDA, 3, 0, 2, (char *)"Save the result in ac");
                        emitComment((char *)"Call end", tree->names);
                    }
                }
                //do same thing for regular function, had it cycling through all children (for whatever reason i was thinking)
                // so lots of issues until I fixed it
                if(tree->isRegFunc)
                {
                    if(tree->child[0] != NULL)
                    {
                        treeGenCode(tree->child[0], table);
                        emitRM(ST, 3, toffset, 1, (char *)"Push parameter");
                        emitComment((char *)"Param end", tree->names);
                    }
                    toffset += 2;
                    emitRM(LDA, 1, toffset, 1, (char *)"Load address of new frame");
                    emitRM(LDA, 3, 1, 7, (char *)"Return address in ac");
                    temp = (TreeNode *)table->lookupGlobal(tree->names);
                    if(temp != NULL) emitGotoAbs(temp->offset, (char *)"CALL", tree->names);
                    emitRM(LDA, 3, 0, 2, (char *)"Save the result in ac");
                    emitComment((char *)"Call end", tree->names);
                }
                break;
			case ParamIdK:
                // hmm, never seems to be used
				break;
			default:
				break;
			}
        // Declaration kind
		}
		else if (tree->nodekind == DeclK)
		{
			switch (tree->kind.decl)
			{
            // same thing as IdK
			case VarK:
                if(tree->isArray)
                {
                    if(tree->varKind == Parameter && isGlobal(tree) == 1) emitRM(LDA, 3, tree->loc, isGlobal(tree), (char *)"Load address of base of array", tree->names);
                    else if(isGlobal(tree) == 1) emitRM(LDA, 3, tree->loc, isGlobal(tree), (char *)"Load address of base of array", tree->names);
                }
                else 
                {
                    if(isGlobal(tree) == 1 || tree->kind.exp == IdK) emitRM(LD, 3, tree->loc, isGlobal(tree), (char *)"Load variable", tree->names);
                }
				break;
			case FuncK:
                //tree->offset = emitSkip(0);
                toffset = -2;
                tree->offset = emitSkip(0);
                emitComment((char *)"FUNCTION", tree->names);
                emitRM(ST, 3, -1, 1, (char *)"Store return address");
                temp = tree->child[0];
                // all siblings of compound, decrement toffset
                while(temp != NULL)
                {
                    toffset--;
                    temp = temp->sibling;
                }
                treeGenCode(tree->child[1], table);
                emitComment((char *)"Add standard closing in case there is no return statement");
                emitRM(LDC, 2, 0, 6, (char *)"Set return value to 0");
                midInput();
                emitRM(JMP, 7, 0, 3, (char *)"Return");
                emitComment((char *)"END FUNCTION", tree->names);
				break;
			case ParamK:
				break;
			default:
				break;
			}
		}
		else ;
        tree = tree->sibling;
    }
}

//var kind of treenode, used in emits
int isGlobal(TreeNode *tree)
{
    if(tree->varKind == Global)
        return 0;
    else return 1;
}