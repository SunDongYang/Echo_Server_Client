.PHONY:clean
CC=g++
CPPFLAGS=-Wall -g
BIN=echosrv_epoll echocli_epoll
all:$(BIN)
%.o:%.c
	$(CC) $(CPPFLAGS) -c $< -o $@
clean:
	rm -f *.o $(BIN)
