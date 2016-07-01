# chdep.exe is needed to generate dependencies

.PHONY: clean

chdep.exe: chdep.c
	gcc -O3 -o chdep.exe chdep.c; strip --strip-all chdep.exe; \
	strip --strip-all chdep.exe
clean:
	del chdep.o; del chdep.exe
