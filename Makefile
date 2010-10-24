all: tcplim

tcplim: *.c
		gcc -O2 tcplim.c -o tcplim
