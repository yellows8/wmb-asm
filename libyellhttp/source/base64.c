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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char Base64_EncodeChar(unsigned char input)
{
	if(input==0x3e)return 0x2b;
    	if(input==0x3f)return 0x2f;
	if(input>=0x34 && input<=0x3d)return (input - 0x34) + 0x30;
	if(input>=0x1a && input<=0x33)return (input - 0x1a) + 0x61;
	if(input>=0 && input<=0x19)return input + 0x41;
	return 0;
}

void Base64_EncodeChars(unsigned char *input, char *output, int inlen, int outmaxlen)
{
	unsigned int pos, i, temp, len;
	if(input==NULL || output==NULL || inlen==0 || outmaxlen==0)return;
	len = inlen;
	while(len>0)
	{
		pos = 0;
		temp = 0;
		for(i=0; i<3; i++)
		{
			temp |= *input << ((2-i)*8);
			input++;
		}
		if(len>=3)
		{
			len-= 3;
		}
		else
		{
			len = 0;
		}

		for(i=0; i<4; i++)
		{
			*output = Base64_EncodeChar((temp >> (3-i)*6) & 0x3f);
			output++;
			outmaxlen--;
			if(outmaxlen<=0)return;
		}
	}
	len = inlen % 3;
	while(len>0)
	{
		*output = '=';
		output++;
		outmaxlen--;
		len--;
		if(outmaxlen<=0)return;
	}
}

