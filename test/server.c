#include <stdio.h>
#include "../csocks/headers/csocks.h"

char* msg = "We need more squirrel girls.\n";

int InitAction(SERVERCLIENT* sc)
{
	char* sendval = "Greetings, earthling!\n";
	
	tcpSend(sendval, strlen(sendval)+1,sc->cs);
	return 0;
}

int ListenerAction(SERVERCLIENT* sc)
{
	char* bIn = 0L;
	long dLen = 0;
	long rb = tcpRecv((void**)&bIn, &dLen, sc->cs);
	if(rb > 0)
	{
		printf("%s: %s\n", sc->pName, bIn);
	}
	long sb = tcpSend(msg, strlen(msg)+1, sc->cs);
	
	if(sb > 0)
		printf("Response sent\n");
	
	if(bIn != 0L)
		free(bIn);
	return 0;
}

int main() {
	//printf("TX Header is %i bytes long.\nPKG is %i bytes long.\n",sizeof(APPACKHEAD),sizeof(APPACKCONT));
	SOCKETSERVER myServer = ipv4_TCPStreamServer("0", 8888, InitAction, ListenerAction, 0L);
	printf("Server errno: %i initialized %i socket %#x02 (WSAGetLastError returns %i)\n", myServer.serrno, myServer.initialized, myServer.ss, WSAGetLastError());
	int lr = ListenServer(&myServer, SOMAXCONN);
	printf("ListenServer returns %i socket %#x02 (WSAGetLastError returns %i)\n", lr, myServer.ss, WSAGetLastError());
	MakeListenServer(&myServer);
	Sleep(3);
	printf("Awaiting connections...\n");
	
	while(myServer.listen)
	{
		Sleep(1);
	}
	
	Sleep(1000);
	EndSockets();
	printf("Done");
	return 0;
}