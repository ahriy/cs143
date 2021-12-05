#!/bin/bash

if [ "$2" = "debug" ]; then
	sed -i 's/hang = 0/hang = 1/g' semant-phase.cc
	make semant
	./lexer $1 | ./parser $* | ./semant $* &
	pid=$(ps aux | grep './semant' | grep -v "grep" | awk -F " " '{print $2}')
	gdb attach $pid --command=cmd.gdb
else
	sed -i 's/hang = 1/hang = 0/g' semant-phase.cc
	make semant
	./lexer $1 | ./parser $* | ./semant $* > tmp
fi
