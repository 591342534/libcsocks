#include "headers/csocks.h"

int BindSocket(int af, unsigned long address, unsigned short port, struct sockaddr_in* s, SOCKET* sock)
{
	if(af > 32 || af < 0)
		af = AF_INET;
	if(address == 0)
	{
		address = INADDR_ANY;
	}
	
	s->sin_family = af;
	s->sin_addr.s_addr = address;
	s->sin_port = htons(port);
	
	return bind(*sock, (struct sockaddr *)s, sizeof(*s));
}

SOCKETSERVER ipv4_TCPStreamServer(char* address, unsigned short port,
int (*clstptr)(SERVERCLIENT*), int (*clfptr)(SERVERCLIENT*),
int (*clbrptr)(SERVERCLIENT*))
{
	SOCKETSERVER ss;
	ss.serrno = 0;
	ss.listen = 0;
	ss.lt = 0L;
	unsigned long ul_addr = inet_addr(address);
	
	if(!IsInitialized)
		RunWinsock(&ss.wsa);
	
	ss.initialized = IsInitialized;
	if(!IsInitialized)
		return ss;
	
	ss.ss = CreateSocket(AF_INET, SOCK_STREAM, 0);
	int sockres = BindSocket(AF_INET,ul_addr,port, &ss.server, &ss.ss);
	
	if(sockres == SOCKET_ERROR)
	{
		ss.initialized = 0;
		ss.serrno = 1;
	}
	
	ss.clst = clstptr;
	ss.clfn = clfptr;
	ss.clbr = clbrptr;
	
	return ss;
}

int ListenServer(SOCKETSERVER* ss, int backlog)
{
	if(ss == 0L)
		return SOCKET_ERROR;
	if(backlog < 0 || backlog > SOMAXCONN)
		backlog = SOMAXCONN;
	
	ss->listen = listen(ss->ss, backlog);
	return ss->listen;
}

DWORD WINAPI ServerListener(LPVOID ssg)
{
	Sleep(1);
	EnterCriticalSection(&mcs);
	SOCKETSERVER* ss = (SOCKETSERVER*)ssg;
	DWORD dWaitResult = WaitForSingleObject(ss->lmt, INFINITE);
	if(dWaitResult == WAIT_ABANDONED)
		return 0;
	
	SERVERCLIENT* temp = NULL;
	
	ss->listen = 1;
	LeaveCriticalSection(&mcs);
	
	Sleep(100);
	
	while(ss->listen > 0)
	{
		if(temp == NULL)
			temp = (SERVERCLIENT*)calloc(1,sizeof(SERVERCLIENT));
		temp->cs = accept(ss->ss, (struct sockaddr *)&temp->client_in, &sosz);
		if(temp->cs == INVALID_SOCKET)
		{
			continue;
		}else{
			GetPeerName(temp->client_in, temp->pName, 20);
			temp->owner = ss;
			printf("New connection from %s\n", temp->pName);
			
			ioctlsocket(temp->cs, FIONBIO, &ioctlmode);
			
			MakeListenClient(temp);
			temp = NULL;
		}
	}
	
	ReleaseMutex(ss->lmt);
	ServerCleanup(ss);
	return 0;
}

DWORD WINAPI ClientListener(LPVOID sclg)
{
	SERVERCLIENT* scl = (SERVERCLIENT*)sclg;
	EnterCriticalSection(&mcs);
	scl->listening = 1;
	LeaveCriticalSection(&mcs);
	DWORD dWaitResult = WaitForSingleObject(scl->lmt, INFINITE);
	if(dWaitResult == WAIT_ABANDONED)
		return 0;
	
	
	if(scl->owner->clst != 0L)
		scl->owner->clst(scl);
	
	int rc = 0;
	Sleep(100);
	while(scl->listening > 0)
	{
		Sleep(1);
		if(scl->owner->clfn != 0L)
				scl->owner->clfn(scl);
		
		if(!scl->owner->listen)
			break;
	}
	
	if(scl->owner->clbr != 0L)
			scl->owner->clbr(scl);
	
	DEBUG_conns++;
	ReleaseMutex(scl->lmt);
	CloseHandle(scl->lmt);
	CloseHandle(scl->lt);
	free(scl);
	printf("Connection over\n");
	return 0;
}

void MakeListenServer(SOCKETSERVER* ss)
{
	ss->lmt = CreateMutex(NULL,FALSE,NULL);
	ss->lt = CreateThread(NULL, 0L, ServerListener, ss, 0, NULL);
	Sleep(100);
	return;
}

void MakeListenClient(SERVERCLIENT* sc)
{
	sc->lmt = CreateMutex(NULL,FALSE,NULL);
	sc->lt = CreateThread(NULL, 0L, ClientListener, sc, 0, NULL);
	Sleep(100);
	return;
}

void ServerCleanup(SOCKETSERVER* ss)
{
	EnterCriticalSection(&mcs);
	ss->listen = 0;
	closesocket(ss->ss);
	
	int x = 0;

	CloseHandle(ss->lt);
	CloseHandle(ss->lmt);
	LeaveCriticalSection(&mcs);
	return;
}