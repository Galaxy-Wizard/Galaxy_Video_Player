
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

static inline int
clamp(int x)
{
	if (x > 255) return 255;
	if (x < 0)   return 0;
	return x;
}

#define YUV_TO_R(C, D, E) clamp((298 * (C) + 409 * (E) + 128) >> 8)
#define YUV_TO_G(C, D, E) clamp((298 * (C) - 100 * (D) - 208 * (E) + 128) >> 8)
#define YUV_TO_B(C, D, E) clamp((298 * (C) + 516 * (D) + 128) >> 8)

#define RGB32(r, g, b) static_cast<uint32_t>((((static_cast<uint32_t>(r) << 8) | g) << 8) | b)


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


enum AVPixelFormat get_format_argb(struct AVCodecContext* s, const enum AVPixelFormat* fmt)
{
	while (*fmt != AV_PIX_FMT_NONE)
	{
		if (*fmt == AV_PIX_FMT_YUVJ420P)
		{			
			return *fmt;
		}
		++fmt;
	}

	return AV_PIX_FMT_NONE;
}

#define PLAY_TIMER_EVENT 2004
#define DECODE_TIMER_EVENT 2005

// CGalaxyVideoPlayerDlg dialog


IMPLEMENT_DYNAMIC(CGalaxyVideoPlayerDlg, CDialogEx);

CGalaxyVideoPlayerDlg::CGalaxyVideoPlayerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_GALAXY_VIDEO_PLAYER_DIALOG, pParent)
{
	EnableActiveAccessibility();
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pAutoProxy = nullptr;

	pD3D = nullptr;
	pDevice = nullptr;

	texture = nullptr;

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

	pD3D = Direct3DCreate9(D3D_SDK_VERSION);

	if (pD3D == nullptr)
	{
		return FALSE;
	}

	D3DPRESENT_PARAMETERS params;
	ZeroMemory(&params, sizeof(params));
	params.Windowed = TRUE;
	params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	params.BackBufferFormat = D3DFMT_A8R8G8B8;
	params.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	params.BackBufferCount = 0;

	HRESULT hr = pD3D->CreateDevice(
		D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, video_window.m_hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&params, &pDevice);
	if (FAILED(hr))
	{
		pD3D->Release();
		pD3D = nullptr;
		return FALSE;
	}

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
		av_log(NULL, AV_LOG_ERROR, "Could not allocate memory for the input format context.\n");

		return;
	}

	// Open video file
	CStringA input_file_a = CW2A(input_file, CP_UTF8);
	response = avformat_open_input(&format_context, input_file_a, NULL, NULL);
	if (response < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not open the input file: %s.\n", input_file_a);
		return;
	}

	// Retrieve stream information
	response = avformat_find_stream_info(format_context, NULL);
	if (response < 0) {
		av_log(NULL, AV_LOG_ERROR, "Unable to find stream info\n");
		return;
	}

	// find primary video stream
	const AVCodec* vcodec = nullptr;
	response = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &vcodec, 0);
	if (response < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to find best stream.\n");
		return;
	}
	vstrm_idx = response;
	vstrm = format_context->streams[vstrm_idx];

	// open video decoder context
	decoder = avcodec_alloc_context3(vcodec);
	if (decoder != nullptr)
	{
		if (avcodec_parameters_to_context(decoder, vstrm->codecpar) < 0) {
			av_log(NULL, AV_LOG_ERROR, "Can not copy input video codec parameters to video decoder context\n");
			return;
		}

		response = avcodec_open2(decoder, vcodec, nullptr);
		if (response < 0) {
			av_log(NULL, AV_LOG_ERROR, "Failed to open codec.\n");
			return;
		}

		frames_rate = decoder->framerate.num / decoder->framerate.den;

		decoder->get_format = get_format_argb;
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

void CGalaxyVideoPlayerDlg:: clear_direct_3d()
{
	if (pDevice != nullptr)
	{
		pDevice->Release();
		pDevice = nullptr;
	}

	if (pD3D != nullptr)
	{
		pD3D->Release();
		pD3D = nullptr;
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
		// Очистим область вывода темно-синим цветом
		pDevice->Clear(0, NULL,
			D3DCLEAR_TARGET,
			D3DCOLOR_XRGB(127, 127, 127), 0.0f, 0);

		// Рисуем сцену
		if (SUCCEEDED(pDevice->BeginScene()))
		{
			RECT rectangle;

			video_window.GetClientRect(&rectangle);

			if (texture != nullptr)
			{
				texture->Release();
				texture = nullptr;
			}

			texture = LoadTexture();

			if (texture != nullptr)
			{
				BlitD3D(texture, &rectangle, D3DCOLOR_ARGB(1, 1, 1, 1), 0);
			}

			pDevice->EndScene();
		}

		// Выводим содержимое вторичного буфера
		pDevice->Present(NULL, NULL, NULL, NULL);
	}
}

//Draw a textured quad on the back-buffer
void CGalaxyVideoPlayerDlg::BlitD3D(IDirect3DTexture9* texture, RECT* rDest, D3DCOLOR vertexColour, float rotate)
{
	const DWORD D3DFVF_TLVERTEX = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;

	struct TLVERTEX
	{
		float x, y, z, rhw;
		D3DCOLOR color;
		float u;
		float v;
	};

	IDirect3DVertexBuffer9* vertexBuffer;

	if (pDevice != nullptr)
	{
		// Set vertex shader.
		pDevice->SetVertexShader(NULL);
		pDevice->SetFVF(D3DFVF_TLVERTEX);

		// Create vertex buffer.
		pDevice->CreateVertexBuffer(sizeof(TLVERTEX) * 4, NULL,
			D3DFVF_TLVERTEX, D3DPOOL_MANAGED, &vertexBuffer, NULL);
		pDevice->SetStreamSource(0, vertexBuffer, 0, sizeof(TLVERTEX));

		if (vertexBuffer != nullptr)
		{
			TLVERTEX* vertices;

			//Lock the vertex buffer
			vertexBuffer->Lock(0, 0, (void**)&vertices, NULL);

			if (vertices != nullptr)
			{
				//Setup vertices
				//A -0.5f modifier is applied to vertex coordinates to match texture
				//and screen coords. Some drivers may compensate for this
				//automatically, but on others texture alignment errors are introduced
				//More information on this can be found in the Direct3D 9 documentation
				vertices[0].color = vertexColour;
				vertices[0].x = (float)rDest->left - 0.5f;
				vertices[0].y = (float)rDest->top - 0.5f;
				vertices[0].z = 0.0f;
				vertices[0].rhw = 1.0f;
				vertices[0].u = 0.0f;
				vertices[0].v = 0.0f;

				vertices[1].color = vertexColour;
				vertices[1].x = (float)rDest->right - 0.5f;
				vertices[1].y = (float)rDest->top - 0.5f;
				vertices[1].z = 0.0f;
				vertices[1].rhw = 1.0f;
				vertices[1].u = 1.0f;
				vertices[1].v = 0.0f;

				vertices[2].color = vertexColour;
				vertices[2].x = (float)rDest->right - 0.5f;
				vertices[2].y = (float)rDest->bottom - 0.5f;
				vertices[2].z = 0.0f;
				vertices[2].rhw = 1.0f;
				vertices[2].u = 1.0f;
				vertices[2].v = 1.0f;

				vertices[3].color = vertexColour;
				vertices[3].x = (float)rDest->left - 0.5f;
				vertices[3].y = (float)rDest->bottom - 0.5f;
				vertices[3].z = 0.0f;
				vertices[3].rhw = 1.0f;
				vertices[3].u = 0.0f;
				vertices[3].v = 1.0f;
			}
			//Unlock the vertex buffer
			vertexBuffer->Unlock();
		}
		
		//Set texture
		if (texture != nullptr)
		{
			pDevice->SetTexture(0, texture);
			pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		}

		//Draw image
		pDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);
	}
}

IDirect3DTexture9* CGalaxyVideoPlayerDlg::LoadTexture()
{
	if (pDevice != nullptr)
	{
		auto frames_iterator = frames.begin();

		if (frames_iterator != frames.end())
		{
			auto current_frame = *frames_iterator;

			frames.pop_front();

			if (current_frame != nullptr)
			{
				if (current_frame->buf[0] != nullptr && current_frame->buf[1] != nullptr && current_frame->buf[2] != nullptr)
				{
					IDirect3DTexture9* texture;

					auto result = pDevice->CreateTexture(current_frame->width, current_frame->height, 0, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, nullptr);

					if (texture != nullptr)
					{
						D3DLOCKED_RECT locked_rectangle;
						CRect rectangle(0, 0, current_frame->width, current_frame->height);

						IDirect3DSurface9* surface = nullptr;

						result = texture->GetSurfaceLevel(0, &surface);

						if (surface != nullptr)
						{
							result = surface->LockRect(&locked_rectangle, nullptr, D3DLOCK_DISCARD);

							if (result == D3D_OK)
							{
								// Stride depends on your texture format - this is the number of bytes per texel.
								// Note that this may be less than 1 for DXT1 textures in which case you'll need 
								// some bit swizzling logic. Can be inferred from Pitch and width.
								int stride = 1;

								int rowPitch = locked_rectangle.Pitch;
								// Choose a pointer type that suits your stride.

								DWORD* pixels = (DWORD*)locked_rectangle.pBits;
								// Clear to black.

								const uint8_t* U_data = &current_frame->buf[1]->data[0];
								const uint8_t* V_data = &current_frame->buf[2]->data[0];

								const uint8_t* U_pos = U_data;
								const uint8_t* V_pos = V_data;

								for (int y = 0; y < rectangle.Height(); y++)
								{
									for (int x = 0; x < rectangle.Width(); x += 2)
									{										
										{
											int Y = current_frame->buf[0]->data[x + current_frame->linesize[0] * y];
											int U = *U_pos;
											int V = *V_pos;

											Y -= 16;
											U -= 128;
											V -= 128;

											BYTE R = YUV_TO_R(Y, U, V) & 0xff;
											BYTE G = YUV_TO_G(Y, U, V) & 0xff;
											BYTE B = YUV_TO_B(Y, U, V) & 0xff;

											DWORD pixel = RGB32(
												R,
												G,
												B
											);

											pixels[x + rowPitch / sizeof(DWORD) * y] = pixel;
										}

										{
											int Y = current_frame->buf[0]->data[x + 1 + current_frame->linesize[0] * y];
											int U = *U_pos;
											int V = *V_pos;

											Y -= 16;
											U -= 128;
											V -= 128;

											BYTE R = YUV_TO_R(Y, U, V) & 0xff;
											BYTE G = YUV_TO_G(Y, U, V) & 0xff;
											BYTE B = YUV_TO_B(Y, U, V) & 0xff;

											DWORD pixel = RGB32(
												R,
												G,
												B
											);

											pixels[x + 1 + rowPitch / sizeof(DWORD) * y] = pixel;
										}

										U_data += 1;
										V_data += 1;
									}

									if (y & 0x1)
									{
										U_pos = &current_frame->buf[1]->data[0 + current_frame->linesize[1] * y / 2];
										V_pos = &current_frame->buf[2]->data[0 + current_frame->linesize[2] * y / 2];
									}
									else
									{
										U_data = U_pos;
										V_data = V_pos;
									}

								}

								surface->UnlockRect();
							}
						}
					}

					av_frame_free(&current_frame);
					current_frame = nullptr;

					return texture;
				}
			}			
		}
	}

	return nullptr;
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
					av_log(NULL, AV_LOG_ERROR, "Failed to read frame.\n");
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
								av_log(NULL, AV_LOG_ERROR, "Failed to read frame.\n");
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
