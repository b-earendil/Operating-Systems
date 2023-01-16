# Makefile for the CS:APP Shell Lab

TEAM = NOBODY
TSH = ./tsh
TSHARGS = "-p"
CC = gcc
CXX = g++

##
## This assignment does not depend on ia32 vs. ia64,
## so we don't include the -m32 option
##
CFLAGS = -Wall -O -g
CXXFLAGS=$(CFLAGS)
FILES = $(TSH) ./myspin

all: $(FILES)

tsh: tsh.o jobs.o helper-routines.o
	$(CXX) -o tsh tsh.o jobs.o helper-routines.o

# clean up
clean:
	rm -f $(FILES) *.o *~
