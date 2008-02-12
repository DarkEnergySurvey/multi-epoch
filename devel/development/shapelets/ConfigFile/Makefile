# Makefile for ConfigFile class

CC = g++
CFLAGS = -Wall -ansi
#CC = icc
#CFLAGS = -ansi

all: ConfigFile.o example tester test_convstring

example: example.cpp ConfigFile.cpp
	$(CC) $(CFLAGS) -o example example.cpp ConfigFile.cpp

tester: tester.cpp ConfigFile.cpp
	$(CC) $(CFLAGS) -o tester tester.cpp ConfigFile.cpp

test_convstring: test_convstring.cpp
	$(CC) $(CFLAGS) -o test_convstring test_convstring.cpp 

ConfigFile.o: ConfigFile.cpp ConfigFile.h
	$(CC) $(CFLAGS) -c ConfigFile.cpp

run: example
	./example | tee example.out

test: tester
	./tester | tee test.out

tidy:
	@ rm -f *.o

clean: tidy
	@ rm -f example tester example.out test.out core* test_convstring
