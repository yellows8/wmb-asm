/*
httpsforwarder is licensed under the MIT license:
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

#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>

unsigned char bufclient[256];
unsigned char bufserver[256];
int bufclient_len, bufserver_len;
int sock_client, sock_connectedclient, sock_server;

void sigterm(int sig)
{
	close(sock_connectedclient);
	close(sock_server);
	close(sock_client);
	exit(0);
}

int main(int argc, char **argv)
{
	char *hostname;
	struct hostent *host;
        struct sockaddr_in client_addr, server_addr, connectedclient_addr;
	unsigned int clientlen = sizeof(connectedclient_addr);

	if(argc<2)
	{
		printf("httpsforwarder v1.0 by yellowstar6 23/03/10\n");
		printf("HTTPS forwarder. Input is host to forward to.\nUsage:\n");
		printf("httpsforwarder <host>\n");
		return 0;
	}
	hostname = argv[1];

	if ((host = gethostbyname(hostname)) == NULL)
        {
		printf("Failed to resolve %s\n", hostname);
		return -1;
        }

	sock_server = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	//sock_connectedclient = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock_server<0)
	{
		printf("Failed to create socket.\n");
		return -2;
	}

	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(443);
	client_addr.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(443);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(sock_server, (struct sockaddr *)&server_addr, sizeof(server_addr))<0)
	{
		int error = errno;
		printf("Failed to bind. %s\n", strerror(error));
		return -3;
	}

	if(listen(sock_server, 1)<0)
	{
		printf("Failed to listen.\n");
		return -3;
	}

	signal(SIGTERM, sigterm);

	while(1)
	{
		sock_connectedclient = accept(sock_server, (struct sockaddr *)&connectedclient_addr, &clientlen);
		if(sock_connectedclient<0)
		{
			printf("Failed to accept incoming connection.\n");
			return -4;
		}
		printf("Connection accepted\n");

		sock_client = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(connect(sock_client, (struct sockaddr *)&client_addr, sizeof(client_addr))<0)
		{
			printf("Failed to connect to server.\n");
			close(sock_connectedclient);
			close(sock_server);
			close(sock_client);
			return -3;
		}
		
		int closenow = 0;
		while(1)
		{
			bufclient_len = recv(sock_connectedclient, bufclient, 256, MSG_DONTWAIT);
			if((bufclient_len<1) || bufclient_len==0)
			{
				int error = errno;
				if((error!=EAGAIN && error!=EWOULDBLOCK) || bufclient_len==0)
				{
					printf("%d %d\n", bufclient_len, bufserver_len);
					printf("client Err %s\n", strerror(error));
					printf("Connection closed\n");
					shutdown(sock_connectedclient,0);
					shutdown(sock_client,0);
					//close(sock_connectedclient);
					close(sock_client);
					closenow = 1;
					break;
				}
			}
			//if(closenow)break;

			bufserver_len = recv(sock_client, bufserver, 256, MSG_DONTWAIT);
			if((bufserver_len<1))
			{
				int error = errno;
				if((error!=EAGAIN && error!=EWOULDBLOCK) || bufserver_len==0)
				{
					printf("%d %d\n", bufclient_len, bufserver_len);
					printf("server Err %s\n", strerror(error));
					printf("Connection closed\n");
					shutdown(sock_connectedclient,0);
					shutdown(sock_client,0);
					//close(sock_connectedclient);
					close(sock_client);
					closenow = 1;
					break;
				}
			}
			//if(closenow)break;

			if(bufclient_len>0)send(sock_client, bufclient, bufclient_len, 0);
			if(bufserver_len>0)send(sock_connectedclient, bufserver, bufserver_len, 0);
			if(bufclient_len>0 || bufserver_len>0)printf("%d %d\n", bufclient_len, bufserver_len);
		}
	}
}

