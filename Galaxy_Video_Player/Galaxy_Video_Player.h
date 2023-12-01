
// Galaxy_Video_Player.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CGalaxyVideoPlayerApp:
// See Galaxy_Video_Player.cpp for the implementation of this class
//

class CGalaxyVideoPlayerApp : public CWinApp
{
public:
	CGalaxyVideoPlayerApp();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CGalaxyVideoPlayerApp theApp;
