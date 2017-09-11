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
pflags = -O2 -DBUILDLIB
dflags = -g -DBUILDLIB
sflags = -shared -DBUILDING_DYNAMIC_LIB
wsflags = -Wl,--out-implib $(x86out)/libcsocks.a \
	-Wl,--export-all-symbols \
	-Wl,--enable-auto-import
dwsflags = -Wl,--out-implib $(dx86out)/libcsocks.a \
	-Wl,--export-all-symbols \
	-Wl,--enable-auto-import
dbgpref = dbg

all:
	$(make) tcpclient
	$(make) tcpserver
	$(make) core
	$(make) dbgtcpclient
	$(make) dbgtcpserver
	$(make) dbgcore
	$(make) testserver
	$(make) testclient
	$(make) mttcpserver
	$(make) mttcpclient
	$(make) slib
	$(make) dbgslib
	$(make) shlib
	$(make) dbgshlib
	
testing:
	$(make) dbgcore
	$(make) dbgtcpclient
	$(make) dbgtcpserver
	$(make) testserver
	$(make) testclient
	$(make) mttcpserver
	$(make) mttcpclient
	
staticlib:
	$(make) core
	$(make) tcpclient
	$(make) tcpserver
	$(make) slib
	
dynamiclib:
	$(make) staticlib
	$(make) shlib
	
dbgstaticlib:
	$(make) dbgcore
	$(make) dbgtcpclient
	$(make) dbgtcpserver
	$(make) dbgslib
	
dbgdynamiclib:
	$(make) dbgstaticlib
	$(make) dbgshlib

tcpclient:
	$(cc) -c $(pflags) $(srcdir)/tcpclient.c -o $(objdir)/tcpclient.o
	
tcpserver:
	$(cc) -c $(pflags) $(srcdir)/tcpserver.c -o $(objdir)/tcpserver.o

core:
	$(cc) -c $(pflags) $(srcdir)/csocks.c -o $(objdir)/csocks.o
	
dbgtcpclient:
	$(cc) -c $(dflags) $(srcdir)/tcpclient.c -o $(objdir)/$(dbgpref)tcpclient.o
	
dbgtcpserver:
	$(cc) -c $(dflags) $(srcdir)/tcpserver.c -o $(objdir)/$(dbgpref)tcpserver.o

dbgcore:
	$(cc) -c $(dflags) $(srcdir)/csocks.c -o $(objdir)/$(dbgpref)csocks.o
	
testserver:
	$(cc) -c $(dflags) $(testdir)/server.c -o $(objdir)/SockServdbg.o
	
testclient:
	$(cc) -c $(dflags) $(testdir)/client.c -o $(objdir)/clientdbg.o
	
mttcpserver:
	$(cc) $(dflags) $(objdir)/$(dbgpref)csocks.o $(objdir)/$(dbgpref)tcpserver.o $(objdir)/$(dbgpref)tcpclient.o $(objdir)/SockServdbg.o -o $(debtest)/SockServ $(libs)

mttcpclient:
	$(cc) $(dflags) $(objdir)/$(dbgpref)csocks.o $(objdir)/$(dbgpref)tcpserver.o $(objdir)/$(dbgpref)tcpclient.o $(objdir)/clientdbg.o -o $(debtest)/SockClient $(libs)
	
slib:
	$(ar) rcs $(x86out)/libcsocks.a $(objdir)/csocks.o $(objdir)/tcpserver.o $(objdir)/tcpclient.o
	
dbgslib:
	$(ar) rcs $(dx86out)/libcsocks.a $(objdir)/$(dbgpref)csocks.o $(objdir)/$(dbgpref)tcpserver.o $(objdir)/$(dbgpref)tcpclient.o

shlib:
	$(cc) $(pflags) $(sflags) -o $(x86out)/libcsocks.dll $(objdir)/csocks.o $(wsflags) $(libs)
	
dbgshlib:
	$(cc) $(dflags) $(sflags) -o $(dx86out)/libcsocks.dll $(objdir)/$(dbgpref)csocks.o $(dwsflags) $(libs)
	
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