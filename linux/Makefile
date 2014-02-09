#/*****************************************************************************
# Assignment 1A
# FILENAME: Makefile
# AUTHOR:   Jason Feriante & Rohit Kumar Jain
# DATE:     8 Feb 2014
# 
# Usage: # shell% make
# 
# Email: feriante@cs.wisc.edu, rjain@cs.wisc.edu
# Semester: CS537 Spring 2014
# Section: LEC 001
# Professor: Daniel Scott Myers
#****************************************************************************/
CC     = gcc
CFLAGS = -Wall -Werror

all: clean mysh

mysh:
	$(CC) -o mysh mysh.c $(CFLAGS)

clean:
	rm -rf mysh