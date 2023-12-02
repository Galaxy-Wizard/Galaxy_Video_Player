
// Galaxy_Video_PlayerDlg.h : header file
//

#pragma once

class CGalaxyVideoPlayerDlgAutoProxy;
struct AVFormatContext;
struct AVFrame;
struct AVStream;
struct AVCodecContext;

// CGalaxyVideoPlayerDlg dialog
class CGalaxyVideoPlayerDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CGalaxyVideoPlayerDlg);
	friend class CGalaxyVideoPlayerDlgAutoProxy;

// Construction
public:
	CGalaxyVideoPlayerDlg(CWnd* pParent = nullptr);	// standard constructor
	virtual ~CGalaxyVideoPlayerDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GALAXY_VIDEO_PLAYER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	CGalaxyVideoPlayerDlgAutoProxy* m_pAutoProxy;
	HICON m_hIcon;

	ID3D11Device* pDevice;
	ID3D11DeviceContext* pDeviceContext;
	IDXGISwapChain* pSwapChain;
	ID3D11RenderTargetView* pRenderTargetView;

	IDirectSound8* pDirectSound;

	CStringW input_file;
	AVFormatContext* format_context;
	
	void clear_direct_sound();
	void clear_direct_3d();
	void clear_memory();
	void clear_decoding();

	BOOL CanExit();

	void Render();


	int vstrm_idx;
	AVStream* vstrm;
	AVCodecContext* decoder;

	std::list<AVFrame*> frames;
	int frames_rate;

	CCriticalSection render_critical_section;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	virtual void OnOK();
	virtual void OnCancel();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton6();
	CEdit file_path;
	CStatic video_window;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CStatic application_status;

	void set_application_status(CString message);
};
