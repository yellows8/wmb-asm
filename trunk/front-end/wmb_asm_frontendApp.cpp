//---------------------------------------------------------------------------
//
// Name:        wmb_asm_frontendApp.cpp
// Author:      yellowstar
// Created:     6/17/2008 8:27:27 PM
// Description: 
//
//---------------------------------------------------------------------------

#include "wmb_asm_frontendApp.h"
#include "wmb_asm_frontendFrmMain.h"

/*
Wmb Asm and all software in the Wmb Asm package are licensed under the MIT license:
Copyright (c) 2008 yellowstar

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the “Software”), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#define DLLMAIN//This define is for the wmb_asm.h header, see that header for details
#include "../dll/include/wmb_asm.h"

IMPLEMENT_APP(wmb_asm_frontendFrmMainApp)

//Global argc/argv from the application. The frames can access these via extern.
wxChar **argv=NULL;
int argc=0;
void Successcallback();//Protyped here because this is needed because of InitAsm; See the InitAsm call in OnInit. Also see *FrmMain.cpp for what this function does.

bool wmb_asm_frontendFrmMainApp::OnInit()
{
    //Get the argc/argv passed to the application.(Command-line parameters)
    argc=this->argc;
    argv=this->argv;
    
    char *error_buffer = (char*)malloc(256);//Allocate memory for errors dealing with modules
	if(!LoadAsmDLL("../dll/wmb_asm",error_buffer))
	{
        //Try to load the module in the dll directory, in the parent directory of this working directory, first.
        //If that fails, try loading the module in the working directory. If that fails, display an error message, then quit.
        if(!LoadAsmDLL("wmb_asm",error_buffer))//LoadAsmDLL takes care of the extension
        {
        wxMessageDialog msg(NULL, error_buffer, "Error", wxOK, wxDefaultPosition);
	    msg.ShowModal();
	    free(error_buffer);
	    return 0;//If this function returns zero, that terminates the application.
        }
    }
    if(InitAsm!=NULL)
    {
    InitAsm(&Successcallback);//Initialize the assembly-related stuff, if the module was loaded without any errors.
    }
    free(error_buffer);
    
    //Create and display the Main Frame.
    wmb_asm_frontendFrmMain* frame = new wmb_asm_frontendFrmMain(NULL);
    SetTopWindow(frame);
    frame->Show();
    return true;
}
 
int wmb_asm_frontendFrmMainApp::OnExit()
{
    //If the module was loading fine, call the function in the module for deinitializing.
    if(ExitAsm!=NULL)ExitAsm();

    //Close the module, if the module was already loaded fine.
    CloseAsmDLL();
    
	return 0;
}
