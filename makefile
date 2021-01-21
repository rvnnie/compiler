BIN = parser
CC = g++
EXE = c-

CPP = treeUtils.cpp semantics.cpp yyerror.cpp gencode.cpp emitcode.cpp
SRCS = $(BIN).y $(BIN).l
HDRS = scanType.h treeNodes.h semantics.h yyerror.h gencode.h emitcode.h
OBJS = lex.yy.o $(BIN).tab.o

$(BIN) : $(OBJS)
	$(CC) $(OBJS) $(CPP) -o $(EXE)

lex.yy.c : $(BIN).l $(BIN).tab.h $(HDRS)
	flex $(BIN).l

$(BIN).tab.h $(BIN).tab.c : $(BIN).y
	bison -v -t -d $(BIN).y

clean :
	rm -f *~ $(OBJS) $(EXE) lex.yy.c $(BIN).tab.h $(BIN).tab.c $(BIN).output

tar:
	tar -cvf $(BIN).tar $(SRCS) $(HDRS) $(CPP) makefile tm.c
	ls -l $(BIN).tar
