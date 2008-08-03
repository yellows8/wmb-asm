#ifndef BUILDING_DLL//Not all compilers/IDEs define this, so make sure it's defined, as this define is used in the code.
#define BUILDING_DLL
#endif
#include "..\include\dll.h"
#include "..\..\SDK\include\wmb_asm_sdk_plugin.h"

DllClass::DllClass()
{

}


DllClass::~DllClass ()
{

}


BOOL APIENTRY DllMain (HINSTANCE hInst     /* Library instance handle. */ ,
                       DWORD reason        /* Reason this function is being called. */ ,
                       LPVOID reserved     /* Not used. */ )
{
    switch (reason)
    {
      case DLL_PROCESS_ATTACH:
        break;

      case DLL_PROCESS_DETACH:
        break;

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }

    /* Returns TRUE on success, FALSE on failure */
    return TRUE;
}
