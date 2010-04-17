#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef LINUX
#include <arpa/inet.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#else
	#ifndef HW_RVL
	#include <netinet/in.h>
	#include <netdb.h>
	#include <sys/socket.h>
	#else
	#include <ogcsys.h>
	#include <gccore.h>
	#include <network.h>
	#include <debug.h>
	#include <errno.h>
	#define socket net_socket
	#define gethostbyname net_gethostbyname
	#define connect net_connect
	#define shutdown net_shutdown
	#define send net_send
	#define recv net_recv
	#endif
#endif

#ifdef ARM9
#include <nds.h>
#include <dswifi9.h>

#ifndef IPPROTO_TCP
#define IPPROTO_TCP 0
#endif

#endif

#ifdef ENABLESSL
#include <openssl/ssl.h>
#endif

#include "yellhttp.h"
#define LIBYELLHTTPVERSIONSTR "v1.0.0"
#define SENDBUFSZ 1024
#define RECVBUFSZ 1024
#define TOTALHDRCBSTRUCTS 32

typedef struct sYellHttp_HeaderCbStruct
{
	YellHttp_HeaderCb cb;
	void* usrarg;
	char hdrfield[256];
} YellHttp_HeaderCbStruct;

void YellHttp_HdrCbAcceptRanges(char *hdr, char *hdrfield, char *hdrval, YellHttp_Ctx *ctx, void* usrarg);
void YellHttp_HdrCbLocation(char *hdr, char *hdrfield, char *hdrval, YellHttp_Ctx *ctx, void* usrarg);
void YellHttp_HdrCbDate(char *hdr, char *hdrfield, char *hdrval, YellHttp_Ctx *ctx, void* usrarg);
void YellHttp_HdrCbContentLength(char *hdr, char *hdrfield, char *hdrval, YellHttp_Ctx *ctx, void* usrarg);

void YellHttp_GenDate(char *outdate, time_t date);

YellHttp_HeaderCbStruct headercb_array[32] = {{YellHttp_HdrCbAcceptRanges, 0, "Accept-Ranges"}, {YellHttp_HdrCbLocation,  0, "Location"}, {YellHttp_HdrCbDate, 0, "Date"}, {YellHttp_HdrCbDate, 0, "Last-Modified"}, {YellHttp_HdrCbContentLength, 0, "Content-Length"}};

char weekdaystr[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
char month_strs[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

YellHttp_Ctx *YellHttp_InitCtx()
{
	YellHttp_Ctx *ctx = (YellHttp_Ctx*)malloc(sizeof(YellHttp_Ctx));
	if(ctx==NULL)return ctx;
	memset(ctx, 0, sizeof(YellHttp_Ctx));
	ctx->sendbuf = (unsigned char*)malloc(SENDBUFSZ);
	ctx->recvbuf = (unsigned char*)malloc(RECVBUFSZ);
	if(ctx->sendbuf==NULL || ctx->recvbuf==NULL)
	{
		free(ctx);
		return NULL;
	}
	memset(ctx->sendbuf, 0, SENDBUFSZ);
	memset(ctx->recvbuf, 0, RECVBUFSZ);
	memset(ctx->useragent, 0, 256);
	memset(ctx->request_type, 0, 8);
	sprintf(ctx->useragent, "libyellhttp %s", LIBYELLHTTPVERSIONSTR);
	memset(ctx->headers, 0, 256);

	return ctx;
}

void YellHttp_FreeCtx(YellHttp_Ctx *ctx)
{
	if(ctx)
	{
		if(ctx->sendbuf)free(ctx->sendbuf);
		if(ctx->recvbuf)free(ctx->recvbuf);
		free(ctx);
	}
}

int YellHttp_SendData(int sock, unsigned char *data, int len)
{
	int retval;
	while(len>0)
	{
		retval = send(sock, data, len, 0);
		if(retval<=0)return retval;
		len-= retval;
		data+= retval;
	}
	return len;
}

int YellHttp_RecvData(int sock, unsigned char *data, int len)
{
	int retval;
	int firstrun = 1;
	int recievedlen = 0;
	while(len>0)
	{
		retval = recv(sock, data, len, 0);
		if(retval<=0 && firstrun)return retval;
		if(retval<=0 && !firstrun)return recievedlen;
		len-= retval;
		data+= retval;
		recievedlen+= retval;
		firstrun = 0;
	}
	return recievedlen;
}

int YellHttp_ExecRequest(YellHttp_Ctx *ctx, char *url)
{
	int retval;
	int i=0;
	int hostnamei;
	int porti = 0;
	int recvlen;
	int stop = 0;
	int headerspos = 0;
	int linenum = 0;
	int pos=0, pos2 = 0;
	char *hdrfield, *hdrval;
	struct hostent *host;
	struct sockaddr_in client_addr;
	FILE *fhttpdump;
	struct stat filestatus;
	char modifiedsincedate[256];
	char hdrstr[256];
	int send_modifiedsince_hdr = 0;
	int valid_domain_name;
	unsigned long serverip;
	char request_type[8];
	unsigned int content_len;

	if(url==NULL)return YELLHTTP_EINVAL;
	memset(ctx->url, 0, 256);
	strncpy(ctx->url, url, 255);	
	ctx->server_flags &= YELLHTTP_SRVFLAG_USRFLAGS;

	if(strncmp(ctx->url, "https", 5)==0)
	{
		#ifdef ENABLESSL
		ctx->SSL = 1;
		#else
		return YELLHTTP_ESSLDISABLED;
		#endif
		i+= 8;
	}
	else if(strncmp(ctx->url, "http", 4)==0)
	{
		ctx->SSL = 0;
		i+= 7;
	}
	else
	{
		return YELLHTTP_EINVAL;
	}
	hostnamei = i;

	memset(ctx->hostname, 0, 256);
	while(ctx->url[i]!='/' && ctx->url[i]!=':')i++;
	if(i>255)return YELLHTTP_EINVAL;
	strncpy(ctx->hostname, &ctx->url[hostnamei], i - hostnamei);

	memset(ctx->uri, 0, 256);
	memset(ctx->portstr, 0, 8);
	ctx->port = 80;
	if(ctx->SSL)ctx->port = 443;
	if(ctx->url[i]==':')
	{
		i++;
		memset(ctx->portstr, 0, 8);
		while((ctx->url[i]>('0'-1) && ctx->url[i]<('9'+1)) && porti<8)
		{
			ctx->portstr[porti] = ctx->url[i];
			i++;
			porti++;
		}
		sscanf(ctx->portstr, "%hd", &ctx->port);
	}
	strcpy(ctx->uri, &ctx->url[i]);
	i = strlen(ctx->url) - 1;

	memset(ctx->filename, 0, 256);
	while(ctx->url[i]!='/' && i>0)i--;
	if(strcmp(&ctx->url[i], "/")==0)
	{
		strncpy(ctx->filename, "index.html", 256);
	}
	else
	{
		i++;
		strncpy(ctx->filename, &ctx->url[i], 256);
	}

	printf("Looking up %s...\n", ctx->hostname);
	valid_domain_name = 0;
	for(hostnamei=0; hostnamei<strlen(ctx->hostname); hostnamei++)
	{
		if(ctx->hostname[hostnamei]>='A' && ctx->hostname[hostnamei]<='z')
		{
			valid_domain_name = 1;
			break;
		}
	}

	if(valid_domain_name)
	{
		if((host = gethostbyname(ctx->hostname)) == NULL)
        	{
			return YELLHTTP_EDNSRESOLV;
        	}
		serverip = *((unsigned long *) host->h_addr_list[0]);
	}
	else
	{
		serverip = inet_addr(ctx->hostname);
	}

	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(ctx->port);
	client_addr.sin_addr.s_addr = serverip;

	ctx->sock_client = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(ctx->sock_client<0)
	{
		if(ctx->sock_client == -1)
		{
			printf("Failed to create socket: %d\n", ctx->sock_client);
			return YELLHTTP_ESOCK;
		}
		else
		{
			printf("socket() returned %d, ignoring...\n", ctx->sock_client);
		}
	}
	printf("Connecting to %s...\n", ctx->hostname);
	if((retval = connect(ctx->sock_client, (struct sockaddr *)&client_addr, sizeof(client_addr)))<0)
	{
		printf("Failed: %d\n", retval);
		return YELLHTTP_ECONN;
	}

	#ifdef ENABLESSL
	SSL_METHOD* method;
	SSL_CTX* sslctx;
	SSL* ssl;
	if(ctx->SSL)
	{
		InitCyaSSL();
		CyaSSL_Debugging_ON();
		method = SSLv3_client_method();
    		sslctx = SSL_CTX_new(method);
		SSL_CTX_set_verify(sslctx, SSL_VERIFY_NONE, 0);
        	ssl = SSL_new(sslctx);
		SSL_set_fd(ssl, ctx->sock_client);
		printf("Connecting to %s with SSL...\n", ctx->hostname);
     		if(SSL_connect(ssl)!=SSL_SUCCESS)
		{
			int  err = SSL_get_error(ssl, 0);
			char errbuffer[80];
			printf("sslerr = %d, %s\n", err, ERR_error_string(err, errbuffer));
			shutdown(ctx->sock_client,0);
			close(ctx->sock_client);
			SSL_CTX_free(sslctx);
     			SSL_free(ssl);
			FreeCyaSSL();
			return err;
		}
	}	
	#endif

	if(stat(ctx->filename, &filestatus)==0 && !(ctx->server_flags & YELLHTTP_SRVFLAG_NOCACHE))
	{
		memset(modifiedsincedate, 0, 256);
		memset(hdrstr, 0, 256);
		YellHttp_GenDate(modifiedsincedate, filestatus.st_mtime);
		sprintf(hdrstr, "If-Modified-Since: %s\r\n", modifiedsincedate);
		send_modifiedsince_hdr = 1;
	}

	memset(request_type, 0, 8);
	if(strlen(ctx->request_type)>0)
	{
		strncpy(request_type, ctx->request_type, 8);
		if(strncmp(request_type, "HEAD", 4)==0)stop = 1;
	}
	else
	{
		strncpy(request_type, "GET", 8);
	}
	snprintf((char*)ctx->sendbuf, SENDBUFSZ, "%s %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\nConnection: close\r\n", request_type, ctx->uri, ctx->hostname, ctx->useragent);
	if(strlen(ctx->headers)>0)strncat((char*)ctx->sendbuf, ctx->headers, SENDBUFSZ);

	if(send_modifiedsince_hdr)strncat((char*)ctx->sendbuf, hdrstr, SENDBUFSZ);

	strncat((char*)ctx->sendbuf, "\r\n", SENDBUFSZ);
	printf("Sending request...\n");
	if(!ctx->SSL)
	{
		YellHttp_SendData(ctx->sock_client, ctx->sendbuf, strlen((char*)ctx->sendbuf));
	}
	else
	{
		#ifdef ENABLESSL
		SSL_write(ssl, ctx->sendbuf, strlen((char*)ctx->sendbuf));
		#endif
	}

	hdrfield = (char*)malloc(256);
	hdrval = (char*)malloc(256);

	printf("Waiting for response...(Headers)\n");
	fhttpdump = fopen("httpheaders", "wb");
	if(fhttpdump==NULL)
	{
		#ifdef ENABLESSL
		if(ctx->SSL)
		{
			SSL_CTX_free(sslctx);
     			SSL_free(ssl);
			FreeCyaSSL();
		}	
		#endif
		shutdown(ctx->sock_client,0);
		close(ctx->sock_client);
		free(hdrfield);
		free(hdrval);
		return YELLHTTP_EFILE;
	}
	while(1)
	{
		if(!ctx->SSL)
		{
			recvlen = YellHttp_RecvData(ctx->sock_client, &ctx->recvbuf[headerspos], 1);
		}
		else
		{
			#ifdef ENABLESSL
			recvlen = SSL_read(ssl, &ctx->recvbuf[headerspos], 1);
			#endif
		}
		if(recvlen<=0)
		{
			stop = 1;
			break;
		}
		fwrite(&ctx->recvbuf[headerspos], 1, 1, fhttpdump);
		if(ctx->recvbuf[headerspos]==0x0a)
		{
			ctx->recvbuf[headerspos+1]=0;
			if(strlen((char*)ctx->recvbuf)<3)
			{
				headerspos = 0;
				break;
			}

			if(headerspos>4)
			{
				if(linenum==0)
				{
					sscanf((char*)&ctx->recvbuf[9], "%d", &ctx->http_status);
					printf("HTTP status: %d\n", ctx->http_status);
					if(ctx->http_status==304)stop = 1;
				}
				else
				{
					memset(hdrfield, 0, 256);
					memset(hdrval, 0, 256);
					pos = 0;
					pos2 = 0;
					while(ctx->recvbuf[pos]!=':' && ctx->recvbuf[pos]!=0x0d && pos2<255)
					{
						hdrfield[pos2] = ctx->recvbuf[pos];
						pos++;
						pos2++;
					}

					if(ctx->recvbuf[pos]==':')
					{
						pos+=2;
						pos2 = 0;
						while(ctx->recvbuf[pos]!=0x0d && pos2<255)
						{
							hdrval[pos2] = ctx->recvbuf[pos];
							pos++;
							pos2++;
						}
					}

					for(i=0; i<32; i++)
					{
						if(headercb_array[i].cb==NULL)continue;
						if(strcmp(headercb_array[i].hdrfield, hdrfield)==0)
						{
							headercb_array[i].cb((char*)ctx->recvbuf, hdrfield, hdrval, ctx, headercb_array[i].usrarg);
						}
					}
				}
				linenum++;
				headerspos = 0;
				continue;
			}
		}
		headerspos++;
	}
	fclose(fhttpdump);
	free(hdrfield);
	free(hdrval);

	if(!stop)
	{
		printf("Receiving content, filename %s ...\n", ctx->filename);
		fhttpdump = fopen(ctx->filename, "wb");
		content_len = ctx->content_length;
		if(fhttpdump==NULL)
		{
			#ifdef ENABLESSL
			if(ctx->SSL)
			{
				SSL_CTX_free(sslctx);
     				SSL_free(ssl);
			FreeCyaSSL();
			}	
			#endif
			shutdown(ctx->sock_client,0);
			close(ctx->sock_client);
			return YELLHTTP_EFILE;
		}

		if(!ctx->SSL)
		{
			if(content_len)
			{
				while(content_len)
				{
					recvlen = YellHttp_RecvData(ctx->sock_client, ctx->recvbuf, RECVBUFSZ);
					fwrite(ctx->recvbuf, 1, recvlen, fhttpdump);
					content_len-= recvlen;
				}
			}
			else
			{
				while((recvlen = YellHttp_RecvData(ctx->sock_client, ctx->recvbuf, RECVBUFSZ))!=0)
				{
					fwrite(ctx->recvbuf, 1, recvlen, fhttpdump);
				}
			}
		}
		else
		{
			#ifdef ENABLESSL
			if(content_len)
			{
				while(content_len)
				{
					recvlen = SSL_read(ssl, ctx->recvbuf, RECVBUFSZ);
					fwrite(ctx->recvbuf, 1, recvlen, fhttpdump);
					content_len-= recvlen;
				}
			}
			else
			{
				while((recvlen = SSL_read(ssl, ctx->recvbuf, RECVBUFSZ))!=0)
				{
					fwrite(ctx->recvbuf, 1, recvlen, fhttpdump);
				}
			}
			#endif
		}
		fclose(fhttpdump);
	}

	#ifdef ENABLESSL
	if(ctx->SSL)
	{
		SSL_CTX_free(sslctx);
     		SSL_free(ssl);
		FreeCyaSSL();
	}	
	#endif
	shutdown(ctx->sock_client,0);
	close(ctx->sock_client);

	if((ctx->http_status>=301 && ctx->http_status<=303) && !(ctx->server_flags & YELLHTTP_SRVFLAG_DISABLEREDIR))
	{
		printf("Redirected: %s\n", ctx->redirecturl);
		YellHttp_ExecRequest(ctx, ctx->redirecturl);
	}
	if(ctx->http_status>=400)return -ctx->http_status;
	return 0;
}

void YellHttp_GetErrorStr(int error, char *errstr, int errstrlen)
{
	if(errstr==NULL)return;
	memset(errstr, 0, errstrlen);
	if(error>=YELLHTTP_LASTERROR)
	{
		switch(error)
		{
			case 0:
				snprintf(errstr, errstrlen, "Success.\n");
			break;

			case YELLHTTP_EINVAL:
				snprintf(errstr, errstrlen, "Invalid input.\n");
			break;

			case YELLHTTP_ESSLDISABLED:
				snprintf(errstr, errstrlen, "User attempted to use SSL when SSL is disabled.\n");
			break;

			case YELLHTTP_EDNSRESOLV:
				snprintf(errstr, errstrlen, "Failed to resolve DNS domain name.\n");
			break;

			case YELLHTTP_ESOCK:
				snprintf(errstr, errstrlen, "Failed to create socket.\n");
			break;

			case YELLHTTP_ECONN:
				snprintf(errstr, errstrlen, "Failed to connect to server.\n");
			break;

			case YELLHTTP_EFILE:
				snprintf(errstr, errstrlen, "Failed to open file.\n");
			break;
		}
		return;
	}
	
	if(error>-400)
	{
		snprintf(errstr, errstrlen, "SSL error: %d\n", error);
		return;
	}
	else
	{
		snprintf(errstr, errstrlen, "HTTP error: %d\n", -error);
		return;
	}
}

int YellHttp_SetHeaderCb(YellHttp_HeaderCb cb, char *header)
{
	int i, found = -1;
	char hdr[256];
	if(header==NULL)return -2;
	strncpy(hdr, header, 256);
	for(i=0; i<TOTALHDRCBSTRUCTS; i++)
	{
		if(strncmp(headercb_array[i].hdrfield, hdr, 256)==0)
		{
			found = i;
			break;
		}
	}
	
	if(found==-1)
	{
		for(i=0; i<TOTALHDRCBSTRUCTS; i++)
		{
			if(headercb_array[i].cb==NULL)
			{
				found = i;
				break;
			}
		}
	}

	if(found==-1)return -1;
	headercb_array[found].cb = cb;
	if(strlen(hdr)>0)
	{
		strncpy(headercb_array[found].hdrfield, hdr, 256);
	}
	else
	{
		memset(headercb_array[found].hdrfield, 0, 256);
	}
	return 0;
}

int YellHttp_GetTimezoneoffset()
{
	int timezoneoffset;
	int localhour;	
	time_t curtime = time(NULL);
	time_t utctime;
	struct tm *Time = gmtime(&curtime);
	struct tm timetm;
	memcpy(&timetm, Time, sizeof(struct tm));	
	timetm.tm_hour = 12;
	localhour = timetm.tm_hour;
	utctime = mktime(&timetm);
	Time = gmtime(&utctime);
	if(localhour < Time->tm_hour)//UTC- timezones
	{
		timezoneoffset = Time->tm_hour - localhour;
		timezoneoffset = -timezoneoffset;
	}
	else//UTC+ timezones
	{
		timezoneoffset = localhour - Time->tm_hour;
	}

	return timezoneoffset;
}

time_t YellHttp_ParseDate(char *date)
{
	struct tm time;
	int i, found = -1;
	char weekday[4];
	char month[4];
	int monthday, year, hour, minute, second;
	time_t timestamp;
	time.tm_isdst = 0;
	sscanf(date, "%s %02d %s %04d %02d:%02d:%02d", weekday, &monthday, month, &year, &hour, &minute, &second);
	for(i=0; i<7; i++)
	{
		if(strncmp(weekday, weekdaystr[i], 3)==0)
		{
			found = i;
			break;
		}
	}

	if(found==-1)
	{
		printf("Invalid weekday.\n");
		return 0;
	}
	time.tm_wday = found;
	time.tm_mday = monthday;

	found = -1;
	for(i=0; i<12; i++)
	{
		if(strncmp(month, month_strs[i], 3)==0)
		{
			found = i;
			break;
		}
	}

	if(found==-1)
	{
		printf("Invalid month.\n");
		return 0;
	}
	time.tm_mon = found;
	time.tm_year = year - 1900;
	time.tm_hour = hour;
	time.tm_min = minute;
	time.tm_sec = second;

	time.tm_hour+= YellHttp_GetTimezoneoffset();
	timestamp = mktime(&time);

	return timestamp;
}

void YellHttp_GenDate(char *outdate, time_t date)
{
	struct tm *time = gmtime(&date);
	char weekday[4];
	char month[4];
	strncpy(weekday, weekdaystr[time->tm_wday], 4);
	strncpy(month, month_strs[time->tm_mon], 4);
	sprintf(outdate, "%s, %02d %s %04d %02d:%02d:%02d GMT", weekday, time->tm_mday, month, time->tm_year + 1900, time->tm_hour, time->tm_min, time->tm_sec);
}

void YellHttp_HdrCbAcceptRanges(char *hdr, char *hdrfield, char *hdrval, YellHttp_Ctx *ctx, void* usrarg)
{
	ctx->server_flags |= YELLHTTP_SRVFLAG_RESUME;
}

void YellHttp_HdrCbLocation(char *hdr, char *hdrfield, char *hdrval, YellHttp_Ctx *ctx, void* usrarg)
{
	strncpy(ctx->redirecturl, hdrval, 255);
}

void YellHttp_HdrCbDate(char *hdr, char *hdrfield, char *hdrval, YellHttp_Ctx *ctx, void* usrarg)
{
	time_t timestamp = YellHttp_ParseDate(hdrval);
	if(strcmp(hdrfield, "Date")==0)
	{
		ctx->server_date = timestamp;
		printf("Date: %s", ctime(&timestamp));
	}
	if(strcmp(hdrfield, "Last-Modified")==0)
	{
		ctx->lastmodified = timestamp;
		printf("Last-Modified: %s", ctime(&timestamp));
	}
}

void YellHttp_HdrCbContentLength(char *hdr, char *hdrfield, char *hdrval, YellHttp_Ctx *ctx, void* usrarg)
{
	sscanf(hdrval, "%d", &ctx->content_length);
}

