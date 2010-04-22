/*
libyellhttp is licensed under the MIT license:
Copyright (c) 2010 yellowstar6

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the Software), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

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
#define YELLHTTP_ENOCREDS -7//If a authentication callback returns this, that means it doesn't have any more users/passwords to try using when the server returns 401 again. 

typedef struct sYellHttp_Ctx
{
	unsigned short port;
	int SSL;
	int sock_client;
	unsigned char *sendbuf;
	unsigned char *recvbuf;
	unsigned char *postdata;//Raw POST data.
	unsigned int postdata_length;
	unsigned int http_status;
	unsigned int server_flags;
	unsigned int content_length;
	time_t server_date;//Current timestamp from server Date header.
	time_t lastmodified;//From Last-Modified server header.
	unsigned int authenticated;
	unsigned int auth_type;
	unsigned auth_requirederrors;//Number of times we got the 401 error. Reset to zero when we get an status code other than 401.
	unsigned int range_start, range_end;//Zero based. Can only be used if server supports resuming. If this is used and the server supports resuming/partial downloads, the http status should be 206, otherwise 200. If the range_end is outside the file size, http status should be 416.
	unsigned long long auth_nc;

	char url[512];
	char uri[512];
	char hostname[512];
	char portstr[8];
	char filename[512];
	char redirecturl[512];
	char useragent[512];//YellHttp_InitCtx sets this to the default user agent, which is "libyellhttp v%s" where %s is the version number.
	char request_type[8];//If not set, default is GET.(This isn't modified when it is not set.)
	char headers[512];//Optional custom headers, each line must end with \r\n.
	char auth_nonce[512];
	char auth_cnonce[18];
	char realm[512];
	char authorization_header[512];
} YellHttp_Ctx;

typedef void (*YellHttp_HeaderCb)(char *hdr, char *hdrfield, char *hdrval, YellHttp_Ctx *ctx, void* usrarg);
typedef int (*YellHttp_WWWAuthenticateCb)(YellHttp_Ctx *ctx, char *realm, char *authout, void* usrarg, int digest, int invalidcreds);//authout is the user:pass string. If digest is one, authout should be the user:realm:pass string for digest authentication.

YellHttp_Ctx *YellHttp_InitCtx();
void YellHttp_FreeCtx(YellHttp_Ctx *ctx);
int YellHttp_ExecRequest(YellHttp_Ctx *ctx, char *url);
int YellHttp_SetHeaderCb(YellHttp_HeaderCb cb, char *header);//Sets a header handler callback. These callbacks are global, they are not specific to YellHttp_Ctx structs. If cb is NULL, the callback handler is removed. This function can override default libyellhttp hdr handlers, see yellhttp.c source for currently implementated handlers.
void YellHttp_SetAuthCb(YellHttp_WWWAuthenticateCb cb, void* usrarg);//Set an authentication callback.
void YellHttp_GetErrorStr(int error, char *errstr, int errstrlen);

#endif

