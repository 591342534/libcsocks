#include "headers/csocks.h"

int DEBUG_conns = 0;
DWORD pks = 0;
unsigned char IsInitialized = 0;
unsigned long ioctlmode = 1;
int sosz = sizeof(struct sockaddr_in);
int sos = sizeof(struct sockaddr);
CRITICAL_SECTION mcs;

int RunWinsock(WSADATA* wsa) {
	DEBUG_conns = 0;
	pks = 0;
	InitializeCriticalSection(&mcs);
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

int tcpstripComm(char** buff, int* dp, int* dt, int* dor, int* dbl, SOCKET sc)
{
	MSGPACK packet;
	int rv = 0, recvt = 0, x = 0;
	while(recvt < sizeof(MSGPACK))
	{
		rv = recv(sc, (char*)&packet+recvt, sizeof(MSGPACK)-recvt,0);
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
	}else{
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
	else
		printf("Integrity check FAIL e(%i)c(%i)\n",packet.chkSum,check);
	
	*buff = (char*)calloc(packet.dSize, 1);
	memcpy(*buff, data, packet.dSize);
	*dp = packet.dSize;
	
	return (packet.packets-1)-packet.partno;
}

long tcpRecv(void** buffer, long* bLength, SOCKET sc)
{
	int rect = 1, rb = 0, total = 0, dt = 0, dor = 0, dbl = 0;
	while(rect > 0)
	{
		char* tb = 0L;
		rect = tcpstripComm(&tb, &rb, &dt, &dor, &dbl, sc);
		if(rb > 0)
		{
			if(total == 0)
			{
				*buffer = (char*)calloc(1,dt);
			}
			
			*bLength = dt;
			memcpy(*buffer+(dbl*dor),tb,rb);
			total += rb;
			
		}else{
			
		}
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
	
	
	MSGPACK packet;
	
	int tbs = 1, x = 0, i = 0, sent = 0, res = 0;
	DWORD chk = 0;
	if(bLength > PBLEN)
		tbs = (int)ceil(bLength / PBLEN);
	
	while(x < tbs)
	{
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
		
		packet.pSize = bLength;
		packet.partno = x;
		packet.packets = tbs;
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
			if(res == SOCKET_ERROR)
				break;
			sent += res;
		}
		
		if(res == SOCKET_ERROR)
		{
			return res;
		}else{
			pks++; x++;
		}
	}
	
	return sent;
}

void EndSockets()
{
	WSACleanup();
	return;
}