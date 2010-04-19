#ifndef _H_YELLHTTP

#include <time.h>

#define YELLHTTP_SRVFLAG_RESUME 1<<0//The server supports resuming if this flag is set. libyellhttp doesn't support resuming though.
#define YELLHTTP_SRVFLAG_DISABLEREDIR 1<<1//optional flag to disable automatically sending a request for redirection.
#define YELLHTTP_SRVFLAG_NOCACHE 1<<2//optional flag to disable caching: don't send the If-Modified-Since header, always download the file regardless of modification timestamp.
#define YELLHTTP_SRVFLAG_USRFLAGS YELLHTTP_SRVFLAG_DISABLEREDIR | YELLHTTP_SRVFLAG_NOCACHE //Flags that the user can set, used to determine which flags to not clear.

#define YELLHTTP_EINVAL -1
#define YELLHTTP_ESSLDISABLED -2
#define YELLHTTP_EDNSRESOLV -3
#define YELLHTTP_ESOCK -4
#define YELLHTTP_ECONN -5
#define YELLHTTP_EFILE -6
#define YELLHTTP_LASTERROR -6

typedef struct sYellHttp_Ctx
{
	unsigned short port;
	int SSL;
	int sock_client;
	unsigned char *sendbuf;
	unsigned char *recvbuf;
	unsigned int http_status;
	unsigned int server_flags;
	unsigned int content_length;
	time_t server_date;//Current timestamp from server Date header.
	time_t lastmodified;//From Last-Modified server header.
	unsigned int authenticated;
	unsigned int range_start, range_end;//Zero based. Can only be used if server supports resuming. If this is used and the server supports resuming/partial downloads, the http status should be 216, otherwise 200. If the range_end is outside the file size, http status should be 416.

	char url[256];
	char uri[256];
	char hostname[256];
	char portstr[8];
	char filename[256];
	char redirecturl[256];
	char useragent[256];//YellHttp_InitCtx sets this to the default user agent, which is "libyellhttp v%s" where %s is the version number.
	char request_type[8];//If not set, default is GET.(This isn't modified when it is not set.)
	char headers[256];//Optional custom headers, each line must end with \r\n.
} YellHttp_Ctx;

typedef void (*YellHttp_HeaderCb)(char *hdr, char *hdrfield, char *hdrval, YellHttp_Ctx *ctx, void* usrarg);
typedef void (*YellHttp_WWWAuthenticateCb)(YellHttp_Ctx *ctx, char *realm, char *authout, void* usrarg);//authout is the user:pass string.

YellHttp_Ctx *YellHttp_InitCtx();
void YellHttp_FreeCtx(YellHttp_Ctx *ctx);
int YellHttp_ExecRequest(YellHttp_Ctx *ctx, char *url);
int YellHttp_SetHeaderCb(YellHttp_HeaderCb cb, char *header);//Sets a header handler callback. These callbacks are global, they are not specific to YellHttp_Ctx structs. If cb is NULL, the callback handler is removed. This function can override default libyellhttp hdr handlers, see yellhttp.c source for currently implementated handlers.
void YellHttp_SetAuthCb(YellHttp_WWWAuthenticateCb cb, void* usrarg);//Set an authentication callback.
void YellHttp_GetErrorStr(int error, char *errstr, int errstrlen);

#endif

