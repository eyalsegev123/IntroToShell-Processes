# Target to build both 'myshell' and 'mypipeline' executables
all: myshell mypipeline

# Rule to link the 'myshell' executable
myshell: myshell.o LineParser.o
	gcc -m32 -g -Wall -o myshell myshell.o LineParser.o

# Rule to link the 'mypipeline' executable
mypipeline: mypipeline.o
	gcc -m32 -g -Wall -o mypipeline mypipeline.o

# Rule to compile 'myshell.c' into 'myshell.o'
myshell.o: myshell.c
	gcc -m32 -g -Wall -c -o myshell.o myshell.c

# Rule to compile 'LineParser.c' into 'LineParser.o'
LineParser.o: LineParser.c
	gcc -m32 -g -Wall -c -o LineParser.o LineParser.c

# Rule to compile 'mypipeline.c' into 'mypipeline.o'
mypipeline.o: mypipeline.c
	gcc -m32 -g -Wall -c -o mypipeline.o mypipeline.c

# Phony target to clean up object files and the executables
.PHONY: clean
clean:
	rm -f *.o myshell mypipeline
