#ifndef _H_YELLHTTP

#include <time.h>

#define YELLHTTP_SRVFLAG_RESUME 1<<0
#define YELLHTTP_SRVFLAG_DISABLEREDIR 1<<1//optional flag to disable automatically sending a request for redirection.
#define YELLHTTP_SRVFLAG_NOCACHE 1<<2//optional flag to disable caching: don't send the If-Modified-Since header, always download the file regardless of modification timestamp.
#define YELLHTTP_SRVFLAG_USRFLAGS YELLHTTP_SRVFLAG_DISABLEREDIR | YELLHTTP_SRVFLAG_NOCACHE //Flags that the user can set, used to determine which flags to not clear.

typedef struct sYellHttp_Ctx
{
	char url[256];
	char uri[256];
	char hostname[256];
	char portstr[8];
	char filename[256];
	char redirecturl[256];
	unsigned short port;
	int SSL;
	int sock_client;
	unsigned char *sendbuf;
	unsigned char *recvbuf;
	unsigned int http_status;
	unsigned int server_flags;
	time_t server_date;//Current timestamp from server Date header.
	time_t lastmodified;//From Last-Modified server header.
} YellHttp_Ctx;

typedef void (*YellHttp_HeaderCb)(char *hdr, char *hdrfield, char *hdrval, YellHttp_Ctx *ctx);

YellHttp_Ctx *YellHttp_InitCtx();
void YellHttp_FreeCtx(YellHttp_Ctx *ctx);
int YellHttp_DoRequest(YellHttp_Ctx *ctx, char *url);

#endif

