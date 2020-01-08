all : webcserver util.o CGI

webcserver : util.o webcserver.c
	gcc -o webcserver util.o webcserver.c

util.o : util.c util.h
	gcc -c -fPIC util.c -o util.o

CGI :
	(cd cgi-bin;make)


