#include "headers/csocks.h"

HOSTENT* ResolveAddress(char* addr)
{
	HOSTENT* rname = 0L;
	
	if(inet_addr(addr) == INADDR_NONE)
		rname = gethostbyname(addr);
	else{
		unsigned long uladdr = inet_addr(addr);
		rname = gethostbyaddr((char*)&uladdr,sizeof(uladdr),AF_INET);
	}
	return rname;
}

CLIENTCONN* ipv4_StartTCPClient(char* addr, unsigned short port,
	int (*clstptr)(CLIENTCONN*), int (*clfptr)(CLIENTCONN*),
	int (*clbrptr)(CLIENTCONN*))
{
	CLIENTCONN* cc = (CLIENTCONN*)calloc(1,sizeof(CLIENTCONN));
	
	if(!IsInitialized)
		while(!IsInitialized)
			RunWinsock(&cc->wsa);
	
	HOSTENT* hinf = ResolveAddress(addr);
	
	cc->clst = clstptr;
	cc->clfn = clfptr;
	cc->clbr = clbrptr;
	cc->cs = CreateSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	cc->cname.sin_addr.s_addr=*((unsigned long*)hinf->h_addr);
	cc->cname.sin_family = AF_INET;
	cc->cname.sin_port = htons(port);
	connect(cc->cs,(struct sockaddr*)&cc->cname,sizeof(cc->cname));
	int lerr = WSAGetLastError();
	switch(lerr)
	{
		case WSAECONNREFUSED:
			printf("Connection refused by the destination host.\n");
			return 0L;
		case WSAEHOSTDOWN:
			printf("The destination host is down.\n");
			return 0L;
		case WSAEHOSTUNREACH:
			printf("Destination unreachable. Check your network connection (if the interface is up, if you have WiFi, etc).\n");
			return 0L;
		case WSAEDISCON:
			printf("Server is shutting down. Try again later.\n");
			return 0L;
		case WSAEADDRINUSE:
			printf("A connection is already estabilished on this address/port.\n");
			return 0L;
		case WSAEADDRNOTAVAIL:
			printf("Invalid address.\n");
			return 0L;
		case WSAENETDOWN:
			printf("Your network seems down, or there is a problem with it. Check if you have connectivity (if the interface is up, if you have WiFi, etc). Also check for viruses.\n");
			return 0L;
		case WSAENETRESET:
			printf("Dropped connection on reset.\n");
			return 0L;
		case WSAECONNABORTED:
			printf("Connection aborted.\n");
			return 0L;
		case WSANOTINITIALISED:
			printf("Winsock failed to initialize.\n");
			return 0L;
	}
	ioctlsocket(cc->cs, FIONBIO, &ioctlmode);
	return cc;
}

DWORD WINAPI ClientIO(LPVOID ssg)
{
	EnterCriticalSection(&mcs);
	CLIENTCONN* ss = (CLIENTCONN*)ssg;
	DWORD dWaitResult = WaitForSingleObject(ss->lmt, INFINITE);
	if(dWaitResult == WAIT_ABANDONED)
		return 0;
	ss->listening = 1;

	if(ss->clst != 0L)
		ss->clst(ss);
	LeaveCriticalSection(&mcs);
	while(ss->listening)
	{
		Sleep(1);
		if(ss->clfn != 0L)
			ss->clfn(ss);
	}
	
	if(ss->clbr != 0L)
		ss->clbr(ss);
	
	closesocket(ss->cs);
	CloseHandle(ss->lmt);
	CloseHandle(ss->lt);
	free(ss);
	return 0;
}

void MakeClientListen(CLIENTCONN* ss)
{
	ss->lmt = CreateMutex(NULL,FALSE,NULL);
	ss->lt = CreateThread(NULL, 0L, ClientIO, ss, 0, NULL);
	return;
}