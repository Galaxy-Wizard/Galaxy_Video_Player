
// Galaxy_Video_PlayerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/avutil.h"
}

#include "Galaxy_Video_Player.h"
#include "Galaxy_Video_PlayerDlg.h"
#include "DlgProxy.h"
#include "afxdialogex.h"

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")


FLOAT color[] = { 0.25, 0.25, 0.25, 0.0 };


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


enum AVPixelFormat get_pixel_format(struct AVCodecContext* s, const enum AVPixelFormat* fmt)
{
	while (*fmt != AV_PIX_FMT_NONE)
	{
		if (*fmt == AV_PIX_FMT_D3D11)
		{
			return *fmt;
		}
		++fmt;
	}

	return AV_PIX_FMT_NONE;
}

#define PLAY_TIMER_EVENT 1981
#define DECODE_TIMER_EVENT 1987

// CGalaxyVideoPlayerDlg dialog


IMPLEMENT_DYNAMIC(CGalaxyVideoPlayerDlg, CDialogEx);

CGalaxyVideoPlayerDlg::CGalaxyVideoPlayerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_GALAXY_VIDEO_PLAYER_DIALOG, pParent)
{
	EnableActiveAccessibility();
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pAutoProxy = nullptr;

	pDevice = nullptr;
	pDeviceContext = nullptr;
	pSwapChain = nullptr;
	pRenderTargetView = nullptr;

	pDirectSound = nullptr;

	format_context = nullptr;

	decoder = nullptr;
	frames_rate = 0;

	vstrm = nullptr;
	vstrm_idx = 0;
}

CGalaxyVideoPlayerDlg::~CGalaxyVideoPlayerDlg()
{
	// If there is an automation proxy for this dialog, set
	//  its back pointer to this dialog to null, so it knows
	//  the dialog has been deleted.
	if (m_pAutoProxy != nullptr)
		m_pAutoProxy->m_pDialog = nullptr;
}

void CGalaxyVideoPlayerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, file_path);
	DDX_Control(pDX, IDC_STATIC_PICTURE, video_window);
	DDX_Control(pDX, IDC_STATIC_STATUS, application_status);
}

BEGIN_MESSAGE_MAP(CGalaxyVideoPlayerDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CGalaxyVideoPlayerDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CGalaxyVideoPlayerDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CGalaxyVideoPlayerDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CGalaxyVideoPlayerDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CGalaxyVideoPlayerDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CGalaxyVideoPlayerDlg::OnBnClickedButton6)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CGalaxyVideoPlayerDlg message handlers

BOOL CGalaxyVideoPlayerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon


	// initialize DirectX
	{
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.Windowed = TRUE;
		swapChainDesc.OutputWindow = video_window.m_hWnd;
		swapChainDesc.BufferCount = 2;
		swapChainDesc.BufferDesc.Width = 0;
		swapChainDesc.BufferDesc.Height = 0;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		D3D_FEATURE_LEVEL featureLevel;
		const D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_1, };
		if (
			SUCCEEDED(
				D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, featureLevelArray, 3, D3D11_SDK_VERSION, &swapChainDesc, &pSwapChain, &pDevice, &featureLevel, &pDeviceContext)
			)
			)
		{

			ID3D11Texture2D* pBackBuffer;
			if (pSwapChain != nullptr)
			{
				if (
					SUCCEEDED(
						pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))
					)
					)
				{
					if (pDevice != nullptr)
					{
						if (
							SUCCEEDED(
								pDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView)
							)
							)
						{
							if (pBackBuffer != nullptr)
							{
								pBackBuffer->Release();
							}
						}
					}
				}
			}


			RECT rc;
			video_window.GetWindowRect(&rc);

			D3D11_VIEWPORT viewport;
			viewport.MinDepth = 0;
			viewport.MaxDepth = 1;
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = FLOAT(rc.right - rc.left);
			viewport.Height = FLOAT(rc.bottom - rc.top);
			pDeviceContext->RSSetViewports(1u, &viewport);
		}
	}

	// draw the initial gray window
	pDeviceContext->ClearRenderTargetView(pRenderTargetView, color);
	pSwapChain->Present(1, 0);

	DirectSoundCreate8(nullptr, &pDirectSound, nullptr);
	if (pDirectSound != nullptr)
	{
		pDirectSound->SetCooperativeLevel(m_hWnd, DSSCL_NORMAL);
	}

	SetTimer(PLAY_TIMER_EVENT, 1000 / 10, nullptr);
	SetTimer(DECODE_TIMER_EVENT, 1000 / 100, nullptr);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGalaxyVideoPlayerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();

		Render();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGalaxyVideoPlayerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// Automation servers should not exit when a user closes the UI
//  if a controller still holds on to one of its objects.  These
//  message handlers make sure that if the proxy is still in use,
//  then the UI is hidden but the dialog remains around if it
//  is dismissed.

void CGalaxyVideoPlayerDlg::OnClose()
{
	clear_direct_sound();
	clear_direct_3d();
	clear_memory();
	clear_decoding();

	if (CanExit())
		CDialogEx::OnClose();
}

void CGalaxyVideoPlayerDlg::OnOK()
{
	clear_direct_sound();
	clear_direct_3d();
	clear_memory();
	clear_decoding();

	if (CanExit())
		CDialogEx::OnOK();
}

void CGalaxyVideoPlayerDlg::OnCancel()
{
	if (CanExit())
		CDialogEx::OnCancel();
}

BOOL CGalaxyVideoPlayerDlg::CanExit()
{
	// If the proxy object is still around, then the automation
	//  controller is still holding on to this application.  Leave
	//  the dialog around, but hide its UI.
	if (m_pAutoProxy != nullptr)
	{
		ShowWindow(SW_HIDE);
		return FALSE;
	}

	return TRUE;
}



void CGalaxyVideoPlayerDlg::OnBnClickedButton1()
{
	CFileDialog file_dialog(TRUE);

	if (file_dialog.DoModal() == IDOK)
	{
		input_file = file_dialog.GetPathName();

		file_path.SetWindowTextW(input_file);
	}
}


void CGalaxyVideoPlayerDlg::OnBnClickedButton2()
{
	clear_memory();

	int response = 0;

	format_context = avformat_alloc_context();
	if (format_context == nullptr)
	{
		set_application_status(L"Could not allocate memory for the input format context.\n");

		return;
	}

	// Open video file
	CStringA input_file_a = CW2A(input_file, CP_UTF8);
	response = avformat_open_input(&format_context, input_file_a, NULL, NULL);
	if (response < 0)
	{
		set_application_status(L"Could not open the input file.\n");
		return;
	}

	// Retrieve stream information
	response = avformat_find_stream_info(format_context, NULL);
	if (response < 0) {
		set_application_status(L"Unable to find stream info\n");
		return;
	}

	// find primary video stream
	const AVCodec* vcodec = nullptr;
	response = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &vcodec, 0);
	if (response < 0) {
		set_application_status(L"Failed to find best stream.\n");
		return;
	}
	vstrm_idx = response;
	vstrm = format_context->streams[vstrm_idx];

	// open video decoder context
	decoder = avcodec_alloc_context3(vcodec);
	if (decoder != nullptr)
	{
		if (avcodec_parameters_to_context(decoder, vstrm->codecpar) < 0) {
			set_application_status(L"Can not copy input video codec parameters to video decoder context\n");
			return;
		}

		response = avcodec_open2(decoder, vcodec, nullptr);
		if (response < 0) {
			set_application_status(L"Failed to open codec.\n");
			return;
		}

		frames_rate = decoder->framerate.num / decoder->framerate.den;

		decoder->get_format = get_pixel_format;
	}

	KillTimer(PLAY_TIMER_EVENT);

	if (frames_rate == 0)
	{
		frames_rate = 10;
	}
	SetTimer(PLAY_TIMER_EVENT, 1000 / frames_rate, nullptr);
}


void CGalaxyVideoPlayerDlg::OnBnClickedButton3()
{
	// TODO: Add your control notification handler code here
}


void CGalaxyVideoPlayerDlg::OnBnClickedButton4()
{
	// TODO: Add your control notification handler code here
}


void CGalaxyVideoPlayerDlg::OnBnClickedButton5()
{
	KillTimer(PLAY_TIMER_EVENT);
	clear_memory();
}


void CGalaxyVideoPlayerDlg::OnBnClickedButton6()
{
	OnOK();
}

void CGalaxyVideoPlayerDlg::clear_direct_sound()
{
	if (pDirectSound != nullptr)
	{
		pDirectSound->Release();
		pDirectSound = nullptr;
	}
}

void CGalaxyVideoPlayerDlg::clear_direct_3d()
{
	if (pRenderTargetView != nullptr)
	{
		pRenderTargetView->Release();
		pRenderTargetView = nullptr;
	}

	if (pSwapChain != nullptr)
	{
		pSwapChain->Release();
		pSwapChain = nullptr;
	}

	if (pDeviceContext != nullptr)
	{
		pDeviceContext->Release();
		pDeviceContext = nullptr;
	}

	if (pDevice != nullptr)
	{
		pDevice->Release();
		pDevice = nullptr;
	}
}

void CGalaxyVideoPlayerDlg::clear_memory()
{
	if (format_context != nullptr)
	{
		avformat_close_input(&format_context);
		avformat_free_context(format_context);

		format_context = nullptr;
	}
}

void CGalaxyVideoPlayerDlg::clear_decoding()
{
	avcodec_close(decoder);
	avcodec_free_context(&decoder);
	avformat_close_input(&format_context);
}

void CGalaxyVideoPlayerDlg::Render()
{
	CSingleLock lock(&render_critical_section);

	lock.Lock();

	if (pDevice)
	{
		{
			if (pDeviceContext != nullptr && pSwapChain != nullptr)
			{
				// fill with random color every time the timer fires
				//color[0] = FLOAT((rand() % 100) / 100.0);
				//color[1] = FLOAT((rand() % 100) / 100.0);
				//color[2] = FLOAT((rand() % 100) / 100.0);
				//color[3] = 0.0;
				pDeviceContext->ClearRenderTargetView(pRenderTargetView, color);

				if (frames.size() != 0)
				{
					auto iterator_1 = frames.begin();

					auto current_frame = *iterator_1;
					if (current_frame != nullptr)
					{
/**
* Hardware surfaces for Direct3D11.
*
* This is preferred over the legacy AV_PIX_FMT_D3D11VA_VLD. The new D3D11
* hwaccel API and filtering support AV_PIX_FMT_D3D11 only.
*
* data[0] contains a ID3D11Texture2D pointer, and data[1] contains the
* texture array index of the frame as intptr_t if the ID3D11Texture2D is
* an array texture (or always 0 if it's a normal texture).
*/
//											AV_PIX_FMT_D3D11,

						ID3D11Texture2D* pBackBuffer = (ID3D11Texture2D*)current_frame->buf[0];
						if (pBackBuffer != nullptr)
						{
							if (pSwapChain != nullptr)
							{
								if (
									SUCCEEDED(
										pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))
									)
									)
								{
									if (pDevice != nullptr)
									{
										if (
											SUCCEEDED(
												pDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView)
											)
											)
										{
											pBackBuffer->Release();
										}
									}
								}
							}
						}
					}

					frames.pop_front();
				}

				pSwapChain->Present(1, 0);
			}
		}
	}
}

void CGalaxyVideoPlayerDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default

	if (nIDEvent == PLAY_TIMER_EVENT)
	{
		Render();
	}

	if (nIDEvent == DECODE_TIMER_EVENT)
	{
		if (format_context != nullptr)
		{
			// decoding loop		
			int response = 0;
			AVPacket* packet = av_packet_alloc();

			if (packet != nullptr)
			{
				// read packet from input file
				response = av_read_frame(format_context, packet);

				if (response < 0 && response != AVERROR_EOF)
				{
					set_application_status(L"Failed to read frame.\n");
				}
				else
				{
					if (response != AVERROR_EOF)
					{
						if (packet->stream_index == vstrm_idx)
						{
							response = avcodec_send_packet(decoder, packet);

							if (response < 0)
							{
								set_application_status(L"Failed to read frame.\n");
							}
							else
							{
								AVFrame* frame = av_frame_alloc();

								if (frame != nullptr)
								{
									avcodec_receive_frame(decoder, frame);
									frames.push_back(frame);
								}
							}
						}
					}
				}
				av_packet_unref(packet);

				av_packet_free(&packet);
				packet = nullptr;
			}
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CGalaxyVideoPlayerDlg::set_application_status(CString message)
{
	application_status.SetWindowTextW(message.GetBuffer());
}