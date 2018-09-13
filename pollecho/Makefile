.PHONY:clean
CC=g++
CPPFLAGS=-Wall -g
BIN=echosrv_poll echocli_poll
all:$(BIN)
%.o:%.c
	$(CC) $(CPPFLAGS) - c $< -o $@
clean :
	rm - f *.o $(BIN)
