main: libshell.a
	gcc -L. -o exe main.c -lshell
libshell.a: shell.o
	ar -cvq libshell.a shell.o
shell.o:
	gcc -c shell.c
clean:
	rm -f exe *.a *.o