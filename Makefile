all: doit proj

doit: doit.c
	gcc -o doit doit.c -Wall
proj: projx.c
	gcc -o proj projx.c -lpthread -lrt
clean:
	/bin/rm -f *.o core doit proj
