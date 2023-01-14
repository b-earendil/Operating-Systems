# Project

- A Least Recently Used Paging Algorithm
- An Incomplete Predictive Paging Algorithm

# Files

- Makefile - GNU makefile to build all relevant code
- pager-basic.c - Basic paging strategy implementation that runs one process at a time.
- pager-lru.c - LRU paging strategy implementation (you code this).
- pager-predict.c - Predictive paging strategy implementation (you code this).
- api-test.c - A pageit() implmentation that tests that simulator state changes
- simulator.c - Core simualtor code (look but don't touch)
- simulator.h - Exported functions and structs for use with simulator
- pager-predict.h - Markov page-transition probability matrices gathered from observed data
- programs.c - Defines test "programs" for simulator to run
- pgm*.pseudo - Pseudo code of test programs from which programs.c was generated.

# Executables

- test-* 
    - Runs simulator using "programs" defined in programs.c and paging strategy defined in pager-*.c.
    - Includes various run-time options. Run with '-help' for details.
- test-api 
    - Runs a test of the simulator state changes

# Usage

Build:
 make

Clean:
 make clean

View test options:
 ./test-* -help

Run pager-basic test:
 ./test-basic

Run API test:
 ./test-api