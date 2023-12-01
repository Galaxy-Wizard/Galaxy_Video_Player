
// DlgProxy.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Galaxy_Video_Player.h"
#include "DlgProxy.h"
#include "Galaxy_Video_PlayerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CGalaxyVideoPlayerDlgAutoProxy

IMPLEMENT_DYNCREATE(CGalaxyVideoPlayerDlgAutoProxy, CCmdTarget)

CGalaxyVideoPlayerDlgAutoProxy::CGalaxyVideoPlayerDlgAutoProxy()
{
	EnableAutomation();

	// To keep the application running as long as an automation
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();

	// Get access to the dialog through the application's
	//  main window pointer.  Set the proxy's internal pointer
	//  to point to the dialog, and set the dialog's back pointer to
	//  this proxy.
	ASSERT_VALID(AfxGetApp()->m_pMainWnd);
	if (AfxGetApp()->m_pMainWnd)
	{
		ASSERT_KINDOF(CGalaxyVideoPlayerDlg, AfxGetApp()->m_pMainWnd);
		if (AfxGetApp()->m_pMainWnd->IsKindOf(RUNTIME_CLASS(CGalaxyVideoPlayerDlg)))
		{
			m_pDialog = reinterpret_cast<CGalaxyVideoPlayerDlg*>(AfxGetApp()->m_pMainWnd);
			m_pDialog->m_pAutoProxy = this;
		}
	}
}

CGalaxyVideoPlayerDlgAutoProxy::~CGalaxyVideoPlayerDlgAutoProxy()
{
	// To terminate the application when all objects created with
	// 	with automation, the destructor calls AfxOleUnlockApp.
	//  Among other things, this will destroy the main dialog
	if (m_pDialog != nullptr)
		m_pDialog->m_pAutoProxy = nullptr;
	AfxOleUnlockApp();
}

void CGalaxyVideoPlayerDlgAutoProxy::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(CGalaxyVideoPlayerDlgAutoProxy, CCmdTarget)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CGalaxyVideoPlayerDlgAutoProxy, CCmdTarget)
END_DISPATCH_MAP()

// Note: we add support for IID_IGalaxy_Video_Player to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .IDL file.

// {63c0bff3-15ae-4865-b2ed-2f2f295d0aeb}
static const IID IID_IGalaxy_Video_Player =
{0x63c0bff3,0x15ae,0x4865,{0xb2,0xed,0x2f,0x2f,0x29,0x5d,0x0a,0xeb}};

BEGIN_INTERFACE_MAP(CGalaxyVideoPlayerDlgAutoProxy, CCmdTarget)
	INTERFACE_PART(CGalaxyVideoPlayerDlgAutoProxy, IID_IGalaxy_Video_Player, Dispatch)
END_INTERFACE_MAP()

// The IMPLEMENT_OLECREATE2 macro is defined in pch.h of this project
// {4da58d18-ace5-4c36-9c89-62745167f641}
IMPLEMENT_OLECREATE2(CGalaxyVideoPlayerDlgAutoProxy, "Galaxy_Video_Player.Application", 0x4da58d18,0xace5,0x4c36,0x9c,0x89,0x62,0x74,0x51,0x67,0xf6,0x41)


// CGalaxyVideoPlayerDlgAutoProxy message handlers
