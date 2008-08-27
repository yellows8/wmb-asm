#include "..\..\SDK\include\wmb_asm_sdk_module.h"
#include "..\include\network.h"

#ifdef NDS
int nds_init_network();
int nds_deinit_network();
#endif

#ifdef HW_RVL//Wii
int rvl_init_network();
int rvl_deinit_network();
#endif

#ifdef WIN32
int win32_init_network();
int win32_deinit_network();
#endif

#ifdef unix
int unix_init_network();
int unix_deinit_network();
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

#ifdef NDS

int nds_init_network()
{
    return 3;//No Wmb Asm network interface available for NDS.
}

int nds_deinit_network()
{
    return 1;
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

#endif
