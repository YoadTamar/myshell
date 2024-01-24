CC = gcc
FLAGS = -Wall -g
TARGETS = myshell

.PHONY: all clean

all: $(TARGETS)

myshell: myshell.c myshell.h
	$(CC) $(FLAGS) -c $^
	$(CC) $(FLAGS) -o $@ $@.o


clean:
	rm -f *.o *.h.gch $(TARGETS)