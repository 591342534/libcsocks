cc = gcc
ar = ar
make = make
testdir = test
srcdir = csocks
bindir = bin
x86out = bin/x86
x64out = bin/x64
dx86out = bin/debug/x86
dx64out = bin/debug/x64
debtest = bin/test
objdir = obj
libs = -lws2_32
pflags = -O2
dflags = -g
libflags = 

testing:
	$(cc) -c $(dflags) $(srcdir)/csocks.c -o $(objdir)/csocksdbg.o
	$(cc) -c $(dflags) $(testdir)/server.c -o $(objdir)/SockServdbg.o
	$(cc) -c $(dflags) $(testdir)/client.c -o $(objdir)/clientdbg.o
	$(cc) $(dflags) $(objdir)/csocksdbg.o $(objdir)/SockServdbg.o -o $(debtest)/SockServ $(libs)
	$(cc) $(dflags) $(objdir)/csocksdbg.o $(objdir)/clientdbg.o -o $(debtest)/SockClient $(libs)

all:
	$(cc) -c $(pflags) $(srcdir)/csocks.c -o $(objdir)/csocks.o
	$(cc) -c $(pflags) $(testdir)/server.c -o $(objdir)/SockServ.o
	$(cc) -c $(pflags) $(testdir)/client.c -o $(objdir)/client.o
	$(cc) $(objdir)/csocks.o $(objdir)/SockServ.o -o $(x86out)/SockServ $(libs)
	$(cc) $(objdir)/csocks.o $(objdir)/client.o -o $(x86out)/SockClient $(libs)
	
lib:
	$(cc) -c $(libflags) $(srcdir)/csocks.c -o $(objdir)/libcsocks.o
	$(ar) rcs $(x86out)/libcsocks.a $(objdir)/libcsocks.o
	
libdbg:
	$(cc) -c $(libflags) $(srcdir)/csocks.c -o $(objdir)/libcsocks.o
	$(ar) rcs $(x86out)/libcsocks.a $(objdir)/libcsocks.o
	
.PHONY: init
init:
	mkdir bin
	mkdir bin/x86
	mkdir bin/x64
	mkdir bin/debug
	mkdir bin/debug/x86
	mkdir bin/debug/x64
	mkdir bin/test
	mkdir obj
	mkdir docs
	
.PHONY: clean
clean:
	rm -r bin
	rm -r obj
	rm -r docs