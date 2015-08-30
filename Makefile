all:
	gcc -static main.c -L. -lcursor -o conway.bin
