#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef LINUX
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#endif

#ifdef NDS
#include <dswifi9.h>
#endif

#ifdef ENABLESSL
#include <openssl/ssl.h>
#endif

#include "yellhttp.h"
#define SENDBUFSZ 1024
#define RECVBUFSZ 1024

typedef struct sYellHttp_HeaderCbStruct
{
	YellHttp_HeaderCb cb;
	char hdrfield[256];
} YellHttp_HeaderCbStruct;

void YellHttp_HdrCbAcceptRanges(char *hdr, char *hdrfield, char *hdrval, YellHttp_Ctx *ctx);
void YellHttp_HdrCbLocation(char *hdr, char *hdrfield, char *hdrval, YellHttp_Ctx *ctx);

YellHttp_HeaderCbStruct headercb_array[32] = {{YellHttp_HdrCbAcceptRanges, "Accept-Ranges"}, {YellHttp_HdrCbLocation, "Location"}};

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

int YellHttp_DoRequest(YellHttp_Ctx *ctx, char *url)
{
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
	

	if(url==NULL)return -1;
	memset(ctx->url, 0, 256);
	strncpy(ctx->url, url, 255);	
	ctx->server_flags &= YELLHTTP_SRVFLAG_USRFLAGS;

	if(strncmp(ctx->url, "https", 5)==0)
	{
		#ifdef ENABLESSL
		ctx->SSL = 1;
		#else
		return -2;
		#endif
		i+= 8;
	}
	else if(strncmp(ctx->url, "https", 4)==0)
	{
		ctx->SSL = 0;
		i+= 7;
	}
	else
	{
		return -2;
	}
	hostnamei = i;

	memset(ctx->hostname, 0, 256);
	while(ctx->url[i]!='/' && ctx->url[i]!=':')i++;
	if(i>255)return -2;
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

	if ((host = gethostbyname(ctx->hostname)) == NULL)
        {
		return -3;
        }

	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(ctx->port);
	client_addr.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);

	ctx->sock_client = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(connect(ctx->sock_client, (struct sockaddr *)&client_addr, sizeof(client_addr))<0)
	{
		return -4;
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
        	ssl = SSL_new(sslctx);
		SSL_set_fd(ssl, ctx->sock_client);
		SSL_CTX_set_verify(sslctx, SSL_VERIFY_NONE, 0);
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

	snprintf((char*)ctx->sendbuf, SENDBUFSZ, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", ctx->uri, ctx->hostname);
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

	fhttpdump = fopen("httpheaders", "wb");
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
							headercb_array[i].cb((char*)ctx->recvbuf, hdrfield, hdrval, ctx);
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
		fhttpdump = fopen(ctx->filename, "wb");
		if(!ctx->SSL)
		{
			while((recvlen = YellHttp_RecvData(ctx->sock_client, ctx->recvbuf, RECVBUFSZ))!=0)
			{
				fwrite(ctx->recvbuf, 1, recvlen, fhttpdump);
			}
		}
		else
		{
			#ifdef ENABLESSL
			while((recvlen = SSL_read(ssl, ctx->recvbuf, RECVBUFSZ))!=0)
			{
				fwrite(ctx->recvbuf, 1, recvlen, fhttpdump);
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
		YellHttp_DoRequest(ctx, ctx->redirecturl);
	}
	if(ctx->http_status>=400)return -ctx->http_status;
	return 0;
}

void YellHttp_HdrCbAcceptRanges(char *hdr, char *hdrfield, char *hdrval, YellHttp_Ctx *ctx)
{
	ctx->server_flags |= YELLHTTP_SRVFLAG_RESUME;
}

void YellHttp_HdrCbLocation(char *hdr, char *hdrfield, char *hdrval, YellHttp_Ctx *ctx)
{
	strncpy(ctx->redirecturl, hdrval, 255);
}

