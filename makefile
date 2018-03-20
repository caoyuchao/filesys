main: test

test: test.o sysfile.o
	g++ -o test test.o sysfile.o
clean:
	rm sysfile.o test.o test
