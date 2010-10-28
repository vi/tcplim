all: tcplim

tcplim: *.c
		gcc -O2 -g3 tcplim.c -o tcplim
