all:myElf 

myElf:myElf.o
	gcc -m32 -g -Wall -o myElf myElf.o 

myElf.o:myElf.c
	gcc -c -m32 -g -Wall myElf.c -o myElf.o


clean:
	rm -f *.o myElf