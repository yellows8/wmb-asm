//---------------------------------------------------------------------------
//
// Name:        wmb_asm_frontendFrmMain.cpp
// Author:      yellowstar
// Created:     6/17/2008 8:27:27 PM
// Description: wmb_asm_frontendFrmMain class implementation
//
//---------------------------------------------------------------------------

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

#include "wmb_asm_frontendFrmMain.h"
#include "../dll/include/wmb_asm.h"//So we can use the assembler module
#include <stdio.h>

//Do not add custom headers between
//Header Include Start and Header Include End
//wxDev-C++ designer will remove them
////Header Include Start
#include "Images/Self_wmb_asm_frontendFrmMain_XPM.xpm"
////Header Include End

//----------------------------------------------------------------------------
// wmb_asm_frontendFrmMain
//----------------------------------------------------------------------------
//Add Custom Events only in the appropriate block.
//Code added in other places will be removed by wxDev-C++
////Event Table Start
BEGIN_EVENT_TABLE(wmb_asm_frontendFrmMain,wxFrame)
	////Manual Code Start
	////Manual Code End
	
	EVT_CLOSE(wmb_asm_frontendFrmMain::OnClose)
	EVT_MENU(ID_MNU_OPEN_1004, wmb_asm_frontendFrmMain::Mnuopen1004Click)
	EVT_UPDATE_UI(ID_MNU_OPEN_1004, wmb_asm_frontendFrmMain::Mnuopen1004UpdateUI)
	EVT_MENU(ID_MNU_EXIT_1005, wmb_asm_frontendFrmMain::Mnuexit1005Click)
	EVT_UPDATE_UI(ID_MNU_EXIT_1005, wmb_asm_frontendFrmMain::Mnuexit1005UpdateUI)
	EVT_LISTBOX(ID_FILENAMEBOX,wmb_asm_frontendFrmMain::FileNameBoxSelected)
	EVT_LISTBOX_DCLICK(ID_FILENAMEBOX,wmb_asm_frontendFrmMain::FileNameBoxDoubleClicked)
	EVT_UPDATE_UI(ID_FILENAMEBOX,wmb_asm_frontendFrmMain::FileNameBoxUpdateUI)
END_EVENT_TABLE()
////Event Table End

//Called when assembly of a demo was successful
void Successcallback()
{
    
}

//So we can access argc/argv from *App.cpp
extern wxChar **argv;
extern int argc;

//Contains the Status Bar's text
char StatusText[256];
//Contains the list of files displayed
char **FileNameBoxText=NULL;
//Total files contained in the FileNameBoxText buffer
int TotalFiles = 0;
wmb_asm_frontendFrmMain::wmb_asm_frontendFrmMain(wxWindow *parent, wxWindowID id, const wxString &title, const wxPoint &position, const wxSize& size, long style)
: wxFrame(parent, id, title, position, size, style)
{
	CreateGUIControls();
	
	//Clear the StatusText buffer, then copy in "Ready".
	memset(StatusText,0,256);
	strcpy(StatusText,"Ready");
	//Set the Status Bar text to the text contained in the StatusText buffer.
	WxStatusBar1->SetStatusText(_(StatusText),0);
    
    //Create a matrix/2D array for the FileNameBoxText buffer. First index will be the 0-based fileindex, the second would the character index.
    FileNameBoxText = new char*[256];
    for(int i=0; i<256; i++)
    {
        FileNameBoxText[i] = new char[256];
            if(i<3)
            {
                sprintf(&FileNameBoxText[i][0],"capture%d",i);
                FileNameBox->Append(FileNameBoxText[i]);
            }
    }
}

wmb_asm_frontendFrmMain::~wmb_asm_frontendFrmMain()
{
    //Clean up when exiting.
    //Delete/free the list of characters/strings for each file in the ListBoxText buffer
     for(int i=0; i<256; i++)
    {
        delete[] FileNameBoxText[i];
    }
    delete[] FileNameBoxText;//Delete/free the list that contains pointers to the list of filenames
}

void wmb_asm_frontendFrmMain::CreateGUIControls()
{
	//Do not add custom code between
	//GUI Items Creation Start and GUI Items Creation End
	//wxDev-C++ designer will remove them.
	//Add the custom code before or after the blocks
	////GUI Items Creation Start

	WxStatusBar1 = new wxStatusBar(this, ID_WXSTATUSBAR1, wxWANTS_CHARS | wxVSCROLL | wxHSCROLL);
	WxStatusBar1->SetFieldsCount(1);
	WxStatusBar1->SetStatusText(_("Ready"),0);
	int WxStatusBar1_Widths[1];
	WxStatusBar1_Widths[0] = -1;
	WxStatusBar1->SetStatusWidths(1,WxStatusBar1_Widths);

	WxPanel1 = new wxPanel(this, ID_WXPANEL1, wxPoint(0,0), wxSize(592,452), wxSUNKEN_BORDER);
	WxPanel1->SetFont(wxFont(11, wxSWISS, wxNORMAL,wxNORMAL, false, _("Abadi MT Condensed Light")));

	wxArrayString arrayStringFor_FileNameBox;
	FileNameBox = new wxListBox(WxPanel1, ID_FILENAMEBOX, wxPoint(-2,32), wxSize(593,188), arrayStringFor_FileNameBox, wxLB_SINGLE | wxVSCROLL | wxHSCROLL);
	FileNameBox->SetFont(wxFont(11, wxSWISS, wxNORMAL,wxNORMAL, false, _("Abadi MT Condensed Light")));

	WxMenuBar1 = new wxMenuBar();
	wxMenu *ID_MNU_FILE_1003_Mnu_Obj = new wxMenu(0);
	ID_MNU_FILE_1003_Mnu_Obj->Append(ID_MNU_OPEN_1004, _("Open"), _(""), wxITEM_NORMAL);
	ID_MNU_FILE_1003_Mnu_Obj->Append(ID_MNU_EXIT_1005, _("Exit"), _(""), wxITEM_NORMAL);
	WxMenuBar1->Append(ID_MNU_FILE_1003_Mnu_Obj, _("File"));
	SetMenuBar(WxMenuBar1);

	SetStatusBar(WxStatusBar1);
	SetTitle(_("Wmb Asm Front End"));
	SetIcon(Self_wmb_asm_frontendFrmMain_XPM);
	SetSize(8,8,600,500);
	Center();
	
	////GUI Items Creation End
}

void wmb_asm_frontendFrmMain::OnClose(wxCloseEvent& event)
{
	Destroy();//Destory the frame when exiting
}

/*
 * Mnuopen1004Click
 */
 //MenuBar->File->Open Click/OnMenu event
void wmb_asm_frontendFrmMain::Mnuopen1004Click(wxCommandEvent& event)
{
	// insert your code here
	WxStatusBar1->SetStatusText(wxT(StatusText),0);//Attempt to keep the Status Bar text the same, without becoming blank.
}

/*
 * Mnuopen1004UpdateUI
 */
 //MenuBar->File->Open UpdateUI event
void wmb_asm_frontendFrmMain::Mnuopen1004UpdateUI(wxUpdateUIEvent& event)
{
	// insert your code here
	WxStatusBar1->SetStatusText(wxT(StatusText),0);//Attempt to keep the Status Bar text the same, without becoming blank.
}

/*
 * Mnuexit1005Click
 */
 //MenuBar->File->Exit Click/OnMenu event
void wmb_asm_frontendFrmMain::Mnuexit1005Click(wxCommandEvent& event)
{
	// insert your code here
	Destroy();//Exit the program if File->Exit is pressed on the Menu.
}

/*
 * Mnuexit1005UpdateUI
 */
 //MenuBar->File->Open UpdateUI event
void wmb_asm_frontendFrmMain::Mnuexit1005UpdateUI(wxUpdateUIEvent& event)
{
	// insert your code here
	WxStatusBar1->SetStatusText(wxT(StatusText),0);//Attempt to keep the Status Bar text the same, without becoming blank.
}

/*
 * Mnufile1003Click
 */
 //MenuBar->File OnMenu/Click
void wmb_asm_frontendFrmMain::Mnufile1003Click(wxCommandEvent& event)
{
	// insert your code here
	WxStatusBar1->SetStatusText(wxT(StatusText),0);//Attempt to keep the Status Bar text the same, without becoming blank.
}

/*
 * Mnufile1003UpdateUI
 */
//MenuBar->File UpdateUI
void wmb_asm_frontendFrmMain::Mnufile1003UpdateUI(wxUpdateUIEvent& event)
{
	// insert your code here
	WxStatusBar1->SetStatusText(wxT(StatusText),0);//Attempt to keep the Status Bar text the same, without becoming blank.
}

/*
 * FileNameBoxUpdateUI
 */
void wmb_asm_frontendFrmMain::FileNameBoxUpdateUI(wxUpdateUIEvent& event)
{
	// insert your code here
	WxStatusBar1->SetStatusText(wxT(StatusText),0);//Attempt to keep the Status Bar text the same, without becoming blank.
}

/*
 * FileNameBoxSelected
 */
void wmb_asm_frontendFrmMain::FileNameBoxSelected(wxCommandEvent& event)
{
	//wxString str = event.GetString();
	char str[256];
	sprintf(str,"%d",event.GetInt());

    //wxMessageDialog msg(this, str, "Test", wxOK, wxDefaultPosition);
	//msg.ShowModal();
}

/*
 * FileNameBoxDoubleClicked
 */
void wmb_asm_frontendFrmMain::FileNameBoxDoubleClicked(wxCommandEvent& event)
{
	// insert your code here
}
