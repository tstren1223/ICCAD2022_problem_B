all: main

CXX = g++

LINKFLAGS = -pedantic -Wall -fomit-frame-pointer -funroll-all-loops -O3 -std=c++11 -pthread

main: main.o Fm.o data.o ripple.o testing.o terminal.o
	$(CXX) $(LINKFLAGS) main.o Fm.o data.o ripple.o testing.o terminal.o -o main

main.o: main.cpp
	$(CXX) $(LINKFLAGS) -DCOMPILETIME="\"`date`\"" main.cpp -c
Fm.o: Fm.cpp Fm.h
	$(CXX) $(LINKFLAGS) Fm.cpp -c
data.o: data.cpp data.h
	$(CXX) $(LINKFLAGS) data.cpp -c
ripple.o: ripple.cpp ripple.h
	$(CXX) $(LINKFLAGS) ripple.cpp -c
terminal.o: terminal.cpp terminal.h
	$(CXX) $(LINKFLAGS) terminal.cpp -c
testing.o: testing.cpp testing.h
	$(CXX) $(LINKFLAGS) testing.cpp -c

clean:
	rm -rf *.o *.gch main
