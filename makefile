# Target to build the 'myshell' executable
all: myshell

# Rule to link the 'myshell' executable
myshell: myshell.o LineParser.o
	gcc -m32 -g -Wall -o myshell myshell.o LineParser.o

# Rule to compile 'myshell.c' into 'myshell.o'
myshell.o: myshell.c
	gcc -m32 -g -Wall -c -o myshell.o myshell.c

# Rule to compile 'myshell.c' into 'myshell.o'
LineParser.o: LineParser.c
	gcc -m32 -g -Wall -c -o LineParser.o LineParser.c

# Phony target to clean up object files and the executable
.PHONY: clean
clean:
	rm -f *.o myshell
