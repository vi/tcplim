all: tcplim

tcplim: *.c VERSION
	gcc -O2 -g3 main.c -o tcplim

githead=$(wildcard .git/HEAD)

ifeq (${githead}, .git/HEAD)
VERSION: .git
	( echo -n '#define VERSION "'; git describe --dirty 2> /dev/null | tr -d '\n'; echo '"' ) > VERSION
else
VERSION:
	echo '#define VERSION "unknown"' > VERSION
endif

clean:
	rm -f VERSION tcplim *.o

#install target for checkinstall
install: tcplim
	install -o root -g staff tcplim /usr/bin/
