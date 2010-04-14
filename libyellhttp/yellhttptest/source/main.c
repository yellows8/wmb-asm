#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yellhttp.h>

int main(int argc, char **argv)
{
	int retval;	
	printf("yellhttptest\n");
	if(argc!=2)return -2;
	YellHttp_Ctx *ctx = YellHttp_InitCtx();
	if(ctx==NULL)
	{
		printf("initctx alloc fail\n");
		return -1;
	}
	printf("Executing DoRequest...\n");
	retval = YellHttp_DoRequest(ctx, argv[1]);
	printf("retval = %d\n", retval);	
	YellHttp_FreeCtx(ctx);
	return 0;
}

