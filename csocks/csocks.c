#include "csocks.h"

unsigned char IsInitialized = 0;
unsigned long ioctlmode = 1;
int sosz = sizeof(struct sockaddr_in);
int sos = sizeof(struct sockaddr);

/*  --- GENERIC --- */

int RunWinsock(WSADATA* wsa) {
	DEBUG_conns = 0;
	pks = 0;
	int res = WSAStartup(MAKEWORD(2,2), wsa);
	if(res == 0)
		IsInitialized = 1;
	else
		printf("WSAStartup failed with error code %i\n", WSAGetLastError());
	
	return res;
}

const char* inet_ntop(int af, const void* src, char* dst, int cnt){
//https://memset.wordpress.com/2010/10/09/inet_ntop-for-win32/
 
    struct sockaddr_in srcaddr;
 
    memset(&srcaddr, 0, sizeof(struct sockaddr_in));
    memcpy(&(srcaddr.sin_addr), src, sizeof(srcaddr.sin_addr));
 
    srcaddr.sin_family = af;
    if (WSAAddressToString((struct sockaddr*) &srcaddr, sizeof(struct sockaddr_in), 0, dst, (LPDWORD) &cnt) != 0) {
        DWORD rv = WSAGetLastError();
        printf("WSAAddressToString() : %d\n",rv);
        return NULL;
    }
    return dst;
}

void GetPeerName(struct sockaddr_in coninfo, char* str, int len)
{
	inet_ntop(coninfo.sin_family, &coninfo.sin_addr.s_addr, str, len);
	return;
}

SOCKET CreateSocket(int af, int type, int protocol) {
	if(af > 32 || af < 0)
		af = AF_INET;
	if(type > 5 || type < 1)
		type = SOCK_STREAM;
	if(protocol < 0 || protocol > 113)
		protocol = 0;
	
	return socket(af,type,protocol);
}

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

int tcpstripComm(char** buff, int* dp, int* dt, int* dor, int* dbl, SOCKET sc)
{
	MSGPACK packet;
	int rv = 0, recvt = 0, x = 0;
	while(recvt < sizeof(MSGPACK))
	{
		rv = recv(sc, (char*)&packet+recvt, sizeof(MSGPACK)-recvt,0);
		//printf("RECV: %i RECVT: %i\n", rv,recvt);
		if(rv == 0)
		{
			printf("The remote peer closed the connection.\n");
			return -3;
		}
		
		if(rv == SOCKET_ERROR)
		{
			int eoc = WSAGetLastError();
			if(eoc != WSAEWOULDBLOCK)
			{
				//printf("Socket error on tcpRecv: %i\n", eoc);
				return -4;
			}else{ rv = 0; }
		}
		recvt += rv;
	}
	
	if(packet.SOH == 0x01 &&
		strcmp(packet.HEAD,HEADSIGN) == 0 &&
		strcmp(packet.HEADFOOT,FOOTSIGN) == 0 &&
		strcmp(packet.DATAHEAD,BINHEADSIGN) == 0 &&
		strcmp(packet.DATAFOOT,BINFOOTSIGN) == 0 &&
		packet.ETB == 0x17)
	{
		//printf("Got a valid packet with ID %i\n", packet.pID);
	}else{
		//printf("Got an invalid packet.\n");
		return -5;
	}
	
	char* data = (char*)&packet.data;
	*dt = packet.pSize;
	*dor = packet.partno;
	*dbl = packet.dbLength;
	
	DWORD check = 0;
	while (x < packet.dSize)
	{
		if(x%2 == 0)
			check += data[x];
		else
			check += ~data[x];
		x++;
	}
	if(check == packet.chkSum)
		Sleep(0);
		//printf("Integrity check OK\n");
	else
		printf("Integrity check FAIL e(%i)c(%i)\n",packet.chkSum,check);
	
	*buff = (char*)calloc(packet.dSize, 1);
	//printf("Allocation done %#x\n",*buff);
	memcpy(*buff, data, packet.dSize);
	//printf("Data copied: %s\n",*buff);
	*dp = packet.dSize;
	//printf("Size set\n");
	
	return (packet.packets-1)-packet.partno;
}

long tcpRecv(void** buffer, long* bLength, SOCKET sc)
{
	//printf("Call to tcpRecv\n");
	int rect = 1, rb = 0, total = 0, dt = 0, dor = 0, dbl = 0;
	while(rect > 0)
	{
		char* tb = 0L;
		rect = tcpstripComm(&tb, &rb, &dt, &dor, &dbl, sc);
		//printf("rect = %i, tb: %#x\n", rect, tb);
		if(rb > 0)
		{
			if(total == 0)
			{
				//printf("Allocating Buffer\n");
				*buffer = (char*)calloc(1,dt);
			}
			
			//printf("bLength set\n");
			*bLength = dt;
			
			//printf("Copy to buffer (dbl: %i dor: %i tb: %#x rb: %i dt: %i)\n",dbl,dor,tb,rb, dt);
			memcpy(*buffer+(dbl*dor),tb,rb);
			//printf("Buffer: %s\n", *buffer);
			total += rb;
			
		}else{
			
		}
		//printf("All good until this point.\n");
		if(tb != 0)
			free(tb);
	}
	
	return total;
}

long tcpSend(void* buffer, long bLength, SOCKET sc)
{
	if(bLength < 0)
		return -1;
	if(buffer == 0L)
		return -2;
	
	//printf("Call to tcpSEND\n");
	
	MSGPACK packet;
	
	int tbs = 1, x = 0, i = 0, sent = 0, res = 0;
	DWORD chk = 0;
	if(bLength > PBLEN)
		tbs = (int)ceil(bLength / PBLEN);
	
	//printf("tbs: %i\n", tbs);
	
	while(x < tbs)
	{
		//printf("Packet %i\n", x);
		chk = 0;
		memset(&packet, 0, sizeof(packet));
		packet.SOH = 0x01;
		packet.ETB = 0x17;
		packet.dbLength = PBLEN;
		memcpy(&packet.HEAD, &HEADSIGN, SIGNLENGTH);
		memcpy(&packet.HEADFOOT, &FOOTSIGN, SIGNLENGTH);
		memcpy(&packet.DATAHEAD, &BINHEADSIGN, SIGNLENGTH);
		memcpy(&packet.DATAFOOT, &BINFOOTSIGN, SIGNLENGTH);
		packet.pID = pks;
		
		if(bLength > PBLEN && x == tbs - 1)
			packet.dSize = bLength % PBLEN;
		else if (bLength <= PBLEN)
			packet.dSize = bLength;
		else
			packet.dSize = PBLEN;
		
		//printf("Packet dsize: %i\n", packet.dSize);
		
		packet.pSize = bLength;
		//printf("Packet psize: %i\n", packet.pSize);
		packet.partno = x;
		//printf("Packet partno: %i\n", packet.partno);
		packet.packets = tbs;
		//printf("Packet total: %i\n", packet.packets);
		packet.sSize = sizeof(MSGPACK);
		
		char* bp = buffer + (x*PBLEN);
		i = 0;
		while(i < packet.dSize)
		{
			if(i%2 == 0)
				packet.chkSum += bp[i];
			else
				packet.chkSum += ~bp[i];
			i++;
		}
		
		memcpy(&packet.data, bp, packet.dSize);
		
		sent = 0;
		while(sent < sizeof(MSGPACK))
		{
			res = 0;
			res = send(sc, (char*)&packet+sent, sizeof(MSGPACK)-sent, 0);
			//printf("res: %i sent: %i\n", res, sent);
			if(res == SOCKET_ERROR)
				break;
			sent += res;
		}
		
		if(res == SOCKET_ERROR)
		{
			//printf("Socket error %i on packet %i\n", WSAGetLastError(), x);
			return res;
		}else{
			pks++; x++;
		}
	}
	
	return sent;
}

/*  --- SERVER CODE --- */

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
	SOCKETSERVER* ss = (SOCKETSERVER*)ssg;
	DWORD dWaitResult = WaitForSingleObject(ss->lmt, INFINITE);
	if(dWaitResult == WAIT_ABANDONED)
		return 0;
	
	SERVERCLIENT* temp = NULL;
	
	ss->listen = 1;
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
	scl->listening = 1;
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
		/*memset(scl->buffer, 0, PBLEN);
		rc = recv(scl->cs, scl->buffer, PBLEN,0);
		if(rc == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
				continue;
			if(err == WSAECONNRESET)
			{
				printf("Connection reset by peer.\n");
				break;
			}
			printf("WSAGetLastError() %i\n",err);
		}
		if(rc > 0)
		{
			if(scl->owner->clfn != 0L)
				scl->owner->clfn(scl->buffer, scl);
		}else{
			break;
		}
		
		if(scl->owner->clbr != 0L)
			scl->owner->clbr(scl);*/
		
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
	ss->listen = 0;
	closesocket(ss->ss);
	
	int x = 0;

	CloseHandle(ss->lt);
	CloseHandle(ss->lmt);
	Sleep(1000);
	return;
}

/*  --- CLIENT CODE --- */

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
	CLIENTCONN* ss = (CLIENTCONN*)ssg;
	DWORD dWaitResult = WaitForSingleObject(ss->lmt, INFINITE);
	if(dWaitResult == WAIT_ABANDONED)
		return 0;
	ss->listening = 1;
	Sleep(5);
	//printf("Listening...");
	Sleep(1);
	if(ss->clst != 0L)
		ss->clst(ss);
		
	while(ss->listening)
	{
		Sleep(1);
		if(ss->clfn != 0L)
			ss->clfn(ss);
		//printf("Is listening!\n");
		/*long bs = 0;
		char* buf_in;
		int rc = tcpRecv((void**)&buf_in, &bs, ss->cs);
		if(rc != 0)
			printf("Rv: %s\n", buf_in);
		free(buf_in);*/
		//ss->listening = 0;
	}
	
	if(ss->clbr != 0L)
		ss->clbr(ss);
	
	closesocket(ss->cs);
	CloseHandle(ss->lmt);
	CloseHandle(ss->lt);
	free(ss);
	//printf("Connection over\n");
	return 0;
}

void MakeClientListen(CLIENTCONN* ss)
{
	ss->lmt = CreateMutex(NULL,FALSE,NULL);
	ss->lt = CreateThread(NULL, 0L, ClientIO, ss, 0, NULL);
	return;
}

/*  --- CLEANUP --- */

void EndSockets()
{
	WSACleanup();
	return;
}