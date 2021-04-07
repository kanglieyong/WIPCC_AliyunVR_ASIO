CC = g++

# Uncomment one of the following to switch between debug and opt mode
#OPT = -O2 -DNDEBUG
OPT = -g2

LDFLAGS= -pthread -lboost_system

PROGRAMS = WIPCC_AliyunVR_ASIO
SRC = Main.c

all: Main.o
	$(CC) Main.o $(LDFLAGS) -o $(PROGRAMS) 

Main.o: Main.cpp
	$(CC) -c Main.cpp

clean:
	rm -f $(PROGRAMS) Main.o

run:
	./$(PROGRAMS)
