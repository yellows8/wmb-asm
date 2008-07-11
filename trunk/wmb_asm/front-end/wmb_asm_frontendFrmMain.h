//---------------------------------------------------------------------------
//
// Name:        wmb_asm_frontendFrmMain.h
// Author:      yellowstar
// Created:     6/17/2008 8:27:27 PM
// Description: wmb_asm_frontendFrmMain class declaration
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

#ifndef __WMB_ASM_FRONTENDFRMMAIN_h__
#define __WMB_ASM_FRONTENDFRMMAIN_h__

#ifdef __BORLANDC__
	#pragma hdrstop
#endif

#ifndef WX_PRECOMP
	#include <wx/wx.h>
	#include <wx/frame.h>
#else
	#include <wx/wxprec.h>
#endif

//Do not add custom headers between 
//Header Include Start and Header Include End.
//wxDev-C++ designer will remove them. Add custom headers after the block.
////Header Include Start
#include <wx/menu.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include <wx/statusbr.h>
////Header Include End

////Dialog Style Start
#undef wmb_asm_frontendFrmMain_STYLE
#define wmb_asm_frontendFrmMain_STYLE wxCAPTION | wxSYSTEM_MENU | wxMINIMIZE_BOX | wxCLOSE_BOX
////Dialog Style End

class wmb_asm_frontendFrmMain : public wxFrame
{
	private:
		DECLARE_EVENT_TABLE();
		
	public:
		wmb_asm_frontendFrmMain(wxWindow *parent, wxWindowID id = 1, const wxString &title = wxT("Wmb Asm Front End"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wmb_asm_frontendFrmMain_STYLE);
		virtual ~wmb_asm_frontendFrmMain();
	void Mnuopen1004Click(wxCommandEvent& event);
	void Mnuopen1004UpdateUI(wxUpdateUIEvent& event);
	void Mnuexit1005Click(wxCommandEvent& event);
	void Mnuexit1005UpdateUI(wxUpdateUIEvent& event);
		void FileNameBoxUpdateUI(wxUpdateUIEvent& event);
		void FileNameBoxSelected(wxCommandEvent& event);
		void FileNameBoxDoubleClicked(wxCommandEvent& event);
	void Mnufile1003Click(wxCommandEvent& event);
	void Mnufile1003UpdateUI(wxUpdateUIEvent& event);
		
	private:
		//Do not add custom control declarations between
		//GUI Control Declaration Start and GUI Control Declaration End.
		//wxDev-C++ will remove them. Add custom code after the block.
		////GUI Control Declaration Start
		wxMenuBar *WxMenuBar1;
		wxListBox *FileNameBox;
		wxPanel *WxPanel1;
		wxStatusBar *WxStatusBar1;
		////GUI Control Declaration End
		
	private:
		//Note: if you receive any error with these enum IDs, then you need to
		//change your old form code that are based on the #define control IDs.
		//#defines may replace a numeric value for the enum names.
		//Try copy and pasting the below block in your old form header files.
		enum
		{
			////GUI Enum Control ID Start
			ID_MNU_FILE_1003 = 1003,
			ID_MNU_OPEN_1004 = 1004,
			ID_MNU_EXIT_1005 = 1005,
			
			ID_FILENAMEBOX = 1007,
			ID_WXPANEL1 = 1003,
			ID_WXSTATUSBAR1 = 1002,
			////GUI Enum Control ID End
			ID_DUMMY_VALUE_ //don't remove this value unless you have other enum values
		};
		
	private:
		void OnClose(wxCloseEvent& event);
		void CreateGUIControls();
};

#endif
