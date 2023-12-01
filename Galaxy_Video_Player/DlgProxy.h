
// DlgProxy.h: header file
//

#pragma once

class CGalaxyVideoPlayerDlg;


// CGalaxyVideoPlayerDlgAutoProxy command target

class CGalaxyVideoPlayerDlgAutoProxy : public CCmdTarget
{
	DECLARE_DYNCREATE(CGalaxyVideoPlayerDlgAutoProxy)

	CGalaxyVideoPlayerDlgAutoProxy();           // protected constructor used by dynamic creation

// Attributes
public:
	CGalaxyVideoPlayerDlg* m_pDialog;

// Operations
public:

// Overrides
	public:
	virtual void OnFinalRelease();

// Implementation
protected:
	virtual ~CGalaxyVideoPlayerDlgAutoProxy();

	// Generated message map functions

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(CGalaxyVideoPlayerDlgAutoProxy)

	// Generated OLE dispatch map functions

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

