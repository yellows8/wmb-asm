#include "..\..\SDK\include\wmb_asm_sdk_module.h"
#include "..\include\network.h"

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
