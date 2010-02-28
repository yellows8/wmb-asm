/*
Wmb Asm and all software in the Wmb Asm package are licensed under the MIT license:
Copyright (c) 2008 - 2010 yellowstar

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

#include "wmb_asm_sdk_module.h"
#include "network.h"

#ifdef NDS
int nds_init_network();
int nds_deinit_network();
int nds_network_send(unsigned char *data, int length);
unsigned char *nds_network_receive(int *length);
#endif

#ifdef HW_RVL//Wii
int rvl_init_network();
int rvl_deinit_network();
int rvl_network_send(unsigned char *data, int length);
unsigned char *rvl_network_receive(int *length);
#endif

#ifdef WIN32
int win32_init_network();
int win32_deinit_network();
int win32_network_send(unsigned char *data, int length);
unsigned char *win32_network_receive(int *length);
#endif

#ifdef unix
int unix_init_network();
int unix_deinit_network();
int unix_network_send(unsigned char *data, int length);
unsigned char *unix_network_receive(int *length);
#endif

int init_network()
{
    #ifdef NDS
    return nds_init_network();
    #endif
    
    #ifdef HW_RVL
    return rvl_init_network();
    #endif
    
    #ifdef WIN32
    return win32_init_network();
    #endif

    #ifdef unix
    return unix_init_network();
    #endif
    
    return 1;
}

int deinit_network()
{
    #ifdef NDS
    return nds_deinit_network();
    #endif

    #ifdef HW_RVL
    return rvl_deinit_network();
    #endif

    #ifdef WIN32
    return win32_deinit_network();
    #endif

    #ifdef unix
    return unix_deinit_network();
    #endif
    
    return 1;
}

int network_send(unsigned char *data, int length)
{
    #ifdef NDS
    return nds_network_send(data, length);
    #endif
    
    #ifdef HW_RVL
    return rvl_network_send(data, length);
    #endif
    
    #ifdef WIN32
    return win32_network_send(data, length);
    #endif

    #ifdef unix
    return unix_network_send(data, length);
    #endif
    
    return 1;
}

unsigned char *network_receive(int *length)
{
    #ifdef NDS
    return nds_network_receive(length);
    #endif

    #ifdef HW_RVL
    return rvl_network_receive(length);
    #endif

    #ifdef WIN32
    return win32_network_receive(length);
    #endif

    #ifdef unix
    return unix_network_receive(length);
    #endif
    
    return NULL;
}

#ifdef NDS

int nds_init_network()
{
    return 3;//No Wmb Asm network interface available for NDS.
}

int nds_deinit_network()
{
    return 1;
}

int nds_network_send(unsigned char *data, int length)
{
    
    
    return 1;
}

unsigned char *nds_network_receive(int *length)
{
    return NULL;
}

#endif

#ifdef HW_RVL//Wii

int rvl_init_network()
{
    return 3;//No Wmb Asm network interface available for Wii.
}

int rvl_deinit_network()
{
    return 1;
}

int rvl_network_send(unsigned char *data, int length)
{


    return 1;
}

unsigned char *rvl_network_receive(int *length)
{
    return NULL;
}

#endif

#ifdef WIN32

int win32_init_network()
{
    return 3;//No Wmb Asm network interface available for Windows.
}

int win32_deinit_network()
{
    return 1;
}

int win32_network_send(unsigned char *data, int length)
{


    return 1;
}

unsigned char *win32_network_receive(int *length)
{
    return NULL;
}

#endif

#ifdef unix

int unix_init_network()
{
    return 3;//No Wmb Asm network interface available for Unix.
}

int unix_deinit_network()
{
    return 1;
}

int unix_network_send(unsigned char *data, int length)
{


    return 1;
}

unsigned char *unix_network_receive(int *length)
{
    return NULL;
}

#endif
