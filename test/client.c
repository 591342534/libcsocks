#include <stdio.h>
#include "../csocks/csocks.h"
char* msg = "I agree.\n";
int ListenerAction(CLIENTCONN* sc)
{
	char* bIn = 0L;
	long dLen = 0;
	long rb = tcpRecv((void**)&bIn, &dLen, sc->cs);
	if(rb > 0)
	{
		printf("Server: %s\n", bIn);
		long sb = tcpSend(msg, strlen(msg)+1, sc->cs);
		if(sb > 0)
			printf("Response sent\n");
	}
	if(bIn != 0L)
		free(bIn);
	return 0;
}

int main() {
	//printf("TX Header is %i bytes long.\nPKG is %i bytes long.\n",sizeof(APPACKHEAD),sizeof(APPACKCONT));
	CLIENTCONN* myClient;
	myClient = ipv4_StartTCPClient("127.0.0.1", 8888, 0L, ListenerAction, 0L);
	printf("Client created. LastError: %i\n", WSAGetLastError());
	if(myClient == 0L)
	{
		printf("Failed to connect. Abort.\n");
		return 1;
	}
	MakeClientListen(myClient);
	printf("Listener created. LastError: %i\n", WSAGetLastError());
	while(myClient->listening)
		Sleep(3);
	EndSockets();
	return 0;
}