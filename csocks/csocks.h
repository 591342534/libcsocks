#ifndef __CSOCKS_ENIGMA_H__
#define __CSOCKS_ENIGMA_H__

#define _WIN32_WINNT _WIN32_WINNT_WIN8
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>

#define BUFLEN 8192
#define PBLEN 4096
#define SIGNLENGTH 6
#define HEADSIGN "<PDH>"
#define FOOTSIGN "<PDF>"
#define BINHEADSIGN "<BDH>"
#define BINFOOTSIGN "<BDF>"

//Compatibility for non-windows environments (Linux support to be implemented)
#ifndef _WINDEF_H
	typedef uint64_t QWORD;
	typedef uint32_t DWORD;
	typedef uint16_t WORD;
	typedef uint8_t BYTE;
#endif


int DEBUG_conns;
DWORD pks;

struct sockserver;
struct serverclient;
typedef struct clientconn CLIENTCONN;
typedef struct sockserver SOCKETSERVER;
typedef struct serverclient SERVERCLIENT;

#pragma pack(push, 1)
typedef struct sockserver {
	WSADATA wsa;
	SOCKET ss;
	struct sockaddr_in server;
	DWORD socksiz;
	unsigned char initialized;
	int serrno;
	int listen;
	SERVERCLIENT* temp;
	HANDLE lmt;
	HANDLE lt;
	int (*clst)(SERVERCLIENT*);
	int (*clfn)(SERVERCLIENT*);
	int (*clbr)(SERVERCLIENT*);
}SOCKETSERVER;

typedef struct serverclient {
	SOCKET cs;
	struct sockaddr_in client_in;
	SOCKETSERVER* owner;
	HANDLE lmt;
	HANDLE lt;
	int listening;
	char pName[20];
}SERVERCLIENT;

typedef struct msgpack {
	BYTE SOH; //This bite should always be 0x01
	BYTE HEAD[SIGNLENGTH]; //Fill with HEADSIGN
	DWORD pID; //Package ID
	DWORD dSize; //Total size of the data
	DWORD pSize; //Total size of the whole data sent
	DWORD partno; //Packet order
	DWORD packets; //Total packets sent for pID.
	DWORD chkSum; //Data checksum.
	DWORD dbLength; //Maximum length of the buffer.
	DWORD sSize; //Size of this structure.
	DWORD dType; //Data type (reserved)
	BYTE reserved[80]; //Reserved space(20 DWORDs)
	BYTE HEADFOOT[SIGNLENGTH]; //Fill with FOOTSIGN
	BYTE DATAHEAD[SIGNLENGTH]; //Fill with BINHEADSIGN
	BYTE data[PBLEN]; //Data being sent
	BYTE DATAFOOT[SIGNLENGTH]; //Fill with FOOTHEADSIGN
	BYTE ETB; //This byte should always be 0x17
}MSGPACK;

typedef struct clientconn {
	WSADATA wsa;
	SOCKET cs;
	struct sockaddr_in cname;
	unsigned char listening;
	HANDLE lmt;
	HANDLE lt;
	//Add function pointers here
	char buffer_in[PBLEN];
	char buffer_out[PBLEN];
	int biData;
	int boData;
	int (*clst)(CLIENTCONN*);
	int (*clfn)(CLIENTCONN*);
	int (*clbr)(CLIENTCONN*);
}CLIENTCONN;
#pragma pack(pop)

/*  --- GENERIC --- */
int RunWinsock(WSADATA* wsa);
const char* inet_ntop(int af, const void* src, char* dst, int cnt);
void GetPeerName(struct sockaddr_in coninfo, char* str, int len);
SOCKET CreateSocket(int af, int type, int protocol);
int BindSocket(int af, unsigned long address, unsigned short port, struct sockaddr_in* s, SOCKET* sock);
long tcpRecv(void** buffer, long* bLength, SOCKET sc);
long tcpSend(void* buffer, long bLength, SOCKET sc);

/*  --- SERVER CODE --- */
SOCKETSERVER ipv4_TCPStreamServer(char* address, unsigned short port,
	int (*clstptr)(SERVERCLIENT*), int (*clfptr)(SERVERCLIENT*),
	int (*clbrptr)(SERVERCLIENT*));
	
int ListenServer(SOCKETSERVER* ss, int backlog);
DWORD WINAPI ServerListener(LPVOID ssg);
DWORD WINAPI ClientListener(LPVOID sclg);
void MakeListenServer(SOCKETSERVER* ss);
void MakeListenClient(SERVERCLIENT* sc);
void ServerCleanup(SOCKETSERVER* ss);

/*  --- CLIENT CODE --- */
HOSTENT* ResolveAddress(char* addr);
CLIENTCONN* ipv4_StartTCPClient(char* addr, unsigned short port,
	int (*clstptr)(CLIENTCONN*), int (*clfptr)(CLIENTCONN*),
	int (*clbrptr)(CLIENTCONN*));
	
DWORD WINAPI ClientIO(LPVOID ssg);
void MakeClientListen(CLIENTCONN* ss);

/*  --- CLEANUP --- */
void EndSockets();

#endif