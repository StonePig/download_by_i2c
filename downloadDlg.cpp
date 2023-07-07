
// downloadDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "MyProgressCtrl.h"
#include "download.h"
#include "downloadDlg.h"
#include "afxdialogex.h"


#include "CH341DLL.H"

#pragma comment (lib,"CH341DLLA64.lib") //添加库文件


#define PACKET_SIZE 128

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

HWND mainhwnd;
CdownloadDlg* p;
void CALLBACK CH341_NOTIFY_ROUTINE(ULONG	iEventStatus);  // 设备事件通知回调程序

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CdownloadDlg 对话框



CdownloadDlg::CdownloadDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DOWNLOAD_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CdownloadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS2, m_myprg);
}

BEGIN_MESSAGE_MAP(CdownloadDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_KEYUP()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CdownloadDlg::OnBnClickedButtonOpen)
	ON_BN_CLICKED(IDC_BUTTON_DOWNLOAD, &CdownloadDlg::OnBnClickedButtonDownload)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_CHECK, &CdownloadDlg::OnBnClickedButtonCheck)
	ON_BN_CLICKED(IDC_BUTTON_REBOOT, &CdownloadDlg::OnBnClickedButtonReboot)
END_MESSAGE_MAP()


void CALLBACK CH341_NOTIFY_ROUTINE(  // 设备事件通知回调程序
	ULONG	iEventStatus)
{	//由于中断程序中不能对系统资源操作过多,所以插拔事件处理过程通过PostMessage()放到 CCH341PARDlg::OnKeyUp()过程中
	::PostMessage(mainhwnd, WM_KEYUP, iEventStatus, 0);
}

DWORD  WINAPI  DownloadThread(LPVOID  lparam)
{
	UCHAR wbuffer[mMAX_BUFFER_LENGTH] = "";
	UCHAR rbuffer[mMAX_BUFFER_LENGTH] = "";
	ULONG mwlen = 0, mrlen = 0;
	ULONG i;

	while (1)
	{
		if (p->is_downloading == FALSE)
		{
			p->m_myprg.SetPos(0);
			Sleep(1000);
			p->download_time = 0;
			continue;
		}

		// 打开文件
		CFile file;
		if (file.Open(p->filePath, CFile::modeRead))
		{
			UINT16 total_packet;
			UINT16 cur_packet = 0;
			UINT16 fail_counter = 0;
			// 获取文件大小
			UINT fileSize = (UINT)file.GetLength();
			total_packet = fileSize / PACKET_SIZE;

			// 创建缓冲区
			char* buffer = new char[fileSize + 1];

			// 读取文件内容到缓冲区
			file.Read(buffer, fileSize);

			//1.	从0xF8寄存器读取一个字节，如果最高位为0，表示不忙，执行步骤2
			//2.	0xD9寄存器写入00
			//3.	0xFB寄存器中写入表6-1中的数据
			//4.	0xFA寄存器中写入数据0x26
			//5.	从0xD9寄存器读取一个字节，如值为0，则继续读取，读到1，表示MCU接收成功，执行步骤1，开始发送下一个拆分包；超时或读到值为0xFF，表示MCU接收失败，执行步骤1，重发数据包。

			while (p->is_downloading)
			{
				p->m_myprg.SetPos(((cur_packet + 1) * 100) / total_packet);
				
				char temp[10];
				CString str_temp;
				sprintf_s(temp, "%02d:%02d", p->download_time / 60, p->download_time % 60);
				str_temp = temp;
				p->GetDlgItem(IDC_STATIC_TIMER)->SetWindowTextW(str_temp);

				wbuffer[0] = 0xB8;
				wbuffer[1] = 0xF8;

				for (i = 0; i < 100; i++)
				{
					if (CH341StreamI2C(p->mIndex, 2, &wbuffer[0], 1, &rbuffer[0]))
					{
						break;
					}
				}
				//超时
				if (i == 100)
				{
					break;
				}

				//2.	0xD9寄存器写入00
				wbuffer[0] = 0xB8;
				wbuffer[1] = 0xD9;
				wbuffer[2] = 0x00;
				CH341StreamI2C(p->mIndex, 3, &wbuffer[0], 0, &rbuffer[0]);

				//3.	0xFB寄存器中写入表6-1中的数据
				wbuffer[0] = 0xB8;
				wbuffer[1] = 0xFB;
				wbuffer[2] = 0xA5;
				wbuffer[3] = total_packet / 256;
				wbuffer[4] = total_packet % 256;
				wbuffer[5] = cur_packet / 256;
				wbuffer[6] = cur_packet % 256;
				file.Seek(cur_packet * PACKET_SIZE, CFile::begin);
				file.Read(&wbuffer[7], PACKET_SIZE);
				//for (i = 0; i < PACKET_SIZE; i++)
				//	wbuffer[7 + i] = 0;

				UINT8 check_sum = 0;
				for (i = 0; i < PACKET_SIZE + 5; i++)
					check_sum ^= wbuffer[i + 2];
				wbuffer[PACKET_SIZE + 7] = check_sum;
				wbuffer[PACKET_SIZE + 8] = 0x5A;

				CH341StreamI2C(p->mIndex, PACKET_SIZE + 9, &wbuffer[0], 0, &rbuffer[0]);

				//4.	0xFA寄存器中写入数据0x26
				wbuffer[0] = 0xB8;
				wbuffer[1] = 0xFA;
				wbuffer[2] = 0x26;
				CH341StreamI2C(p->mIndex, 3, &wbuffer[0], 0, &rbuffer[0]);

				//5.	从0xD9寄存器读取一个字节，如值为0，则继续读取，读到1，表示MCU接收成功，执行步骤1
				for (i = 0; i < 1000; i++)
				{
					/*if (cur_packet % 32 == 00)
						Sleep(10);
					else
						Sleep(10);
						*/
					rbuffer[0] = 0xDD;
					wbuffer[0] = 0xB8;
					wbuffer[1] = 0xD9;
					CH341StreamI2C(p->mIndex, 2, &wbuffer[0], 1, &rbuffer[0]);
					if (rbuffer[0] == 1)
						break;
					if (rbuffer[0] == 0xFF)
						break;
				}

				if (rbuffer[0] == 1 && i != 1000)
				{
					// fail_counter = 0;
					//下一包数据
					cur_packet++;
					if (cur_packet == total_packet)
					{
						p->MessageBox(L"下载完成！", NULL, MB_OK | MB_ICONINFORMATION);
						break;
					}
				}
				else if (rbuffer[0] == 0xFF && i != 1000)
				{
					cur_packet = cur_packet / 32 * 32;
					fail_counter++;
					if (fail_counter > 10)
					{
						p->MessageBox(L"下载失败！", NULL, MB_OK | MB_ICONSTOP);
						break;
					}
				}
				else
				{
					p->MessageBox(L"下载失败！", NULL, MB_OK | MB_ICONSTOP);
					break;
				}
			}
			// 关闭文件
			file.Close();
			// 释放缓冲区内存
			delete[] buffer;
			p->is_downloading = FALSE;
			p->GetDlgItem(IDC_BUTTON_DOWNLOAD)->SetWindowTextW(L"开始下载");
		}
	}
	return 0;
}


// CdownloadDlg 消息处理程序

BOOL CdownloadDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	SetTimer(1, 1000, NULL);//每隔0.5秒触发ontimer事件使其前进
	m_myprg.SetRange(0, 100);//设置进度条数值变化范围
	m_myprg.SetPos(0);      //设置进度条默认初始进度

	mIndex = 0;  //打开的设备序号
	m_open = FALSE;  //设备打开标志

	if (CH341OpenDevice(mIndex) == INVALID_HANDLE_VALUE)
	{
		MessageBox(L"请插上I2C下载器!",NULL,MB_OK|MB_ICONSTOP);
		m_open = FALSE;
	}
	else
	{
		m_open = true;
		CH341SetStream(mIndex, 3);
	}

	if (m_open)  //窗体标题显示
		SetWindowText(L"I2C下载器已插上");
	else
		SetWindowText(L"I2C下载器已拔出");

	mainhwnd = GetSafeHwnd();
	CH341SetDeviceNotify(mIndex, NULL, CH341_NOTIFY_ROUTINE);   //注册监视通知

	GetDlgItem(IDC_BUTTON_DOWNLOAD)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_REBOOT)->ShowWindow(FALSE);

	p = this;

	DWORD  dwThreadId;
	CreateThread(NULL, NULL, DownloadThread,(LPVOID)NULL, 0, &dwThreadId);

	
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CdownloadDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CdownloadDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CdownloadDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

#define TP68_FW_MAGIC_START 0x572F4668L
#define TP68_FW_MAGIC_END 0x8664F275L

void CdownloadDlg::OnBnClickedButtonOpen()
{
	// TODO: 在此添加控件通知处理程序代码
		// 创建文件选择对话框
	CFileDialog dlg(TRUE);

	// 显示文件选择对话框
	if (dlg.DoModal() == IDOK)
	{
		// 获取选中的文件名
		filePath = dlg.GetPathName();

		// 打开文件
		CFile file;
		if (file.Open(filePath, CFile::modeRead))
		{
			char read_buf[PACKET_SIZE];
			UINT fileSize = (UINT)file.GetLength();
			file.Seek(0xF4, CFile::begin);
			file.Read(read_buf, 16);
			if (*((unsigned long*)(read_buf)) != TP68_FW_MAGIC_START || *((unsigned long*)(read_buf + 8)) != TP68_FW_MAGIC_END || fileSize % PACKET_SIZE != 0)
			{
				MessageBox(L"文件格式错误！", NULL, MB_OK | MB_ICONSTOP);
				filePath = "";
				GetDlgItem(IDC_STATIC_FILE_FW_VERSION)->SetWindowTextW(L"");
				GetDlgItem(IDC_STATIC_FILE_RESOURCE_VERSION)->SetWindowTextW(L"");
				GetDlgItem(IDC_STATIC_FILE_CHECKSUM)->SetWindowTextW(L"");
				GetDlgItem(IDC_BUTTON_DOWNLOAD)->EnableWindow(FALSE);
			}
			else
			{

				UINT32 checksum = 0;
				UINT32 i;
				unsigned char* buffer = new unsigned char[fileSize + 1];

				file.Seek(0, CFile::begin);
				file.Read(buffer, fileSize);
				for (i = 0; i < fileSize - 4; i++)
				{
					checksum += buffer[i];
				}
				if (checksum != (buffer[i + 0] << 24) + (buffer[i + 1] << 16) + (buffer[i + 2] << 8) + buffer[i + 3] && filePath)
				{
					MessageBox(L"文件格式错误！", NULL, MB_OK | MB_ICONSTOP);
					GetDlgItem(IDC_STATIC_FILE_FW_VERSION)->SetWindowTextW(L"");
					GetDlgItem(IDC_STATIC_FILE_RESOURCE_VERSION)->SetWindowTextW(L"");
					GetDlgItem(IDC_STATIC_FILE_CHECKSUM)->SetWindowTextW(L"");
					GetDlgItem(IDC_BUTTON_DOWNLOAD)->EnableWindow(FALSE);
					filePath = "";
				}

				char temp[20];
				CString str_temp;

				sprintf_s(temp, "v%d.%02d", buffer[0xFA], buffer[0xFB]);
				str_temp = temp;
				GetDlgItem(IDC_STATIC_FILE_FW_VERSION)->SetWindowTextW(str_temp);

				if (fileSize > 0xE000)
				{
					sprintf_s(temp, "v%d.%02d", buffer[0xE000 + 6], buffer[0xE000 + 7]);
					str_temp = temp;
					GetDlgItem(IDC_STATIC_FILE_RESOURCE_VERSION)->SetWindowTextW(str_temp);
				}
				else
				{
					GetDlgItem(IDC_STATIC_FILE_RESOURCE_VERSION)->SetWindowTextW(L"当前固件不包含Resource");
				}

				sprintf_s(temp, "%02X%02X%02X%02X", buffer[0xE000 - 4], buffer[0xE000 - 3], buffer[0xE000 - 2], buffer[0xE000 - 1]);
				str_temp = temp;
				GetDlgItem(IDC_STATIC_FILE_CHECKSUM)->SetWindowTextW(str_temp);

				GetDlgItem(IDC_BUTTON_DOWNLOAD)->EnableWindow(TRUE);

				delete[] buffer;
			}

			file.Close();
		}

		GetDlgItem(IDC_STATIC_PATH)->SetWindowTextW(filePath);
		//GetDlgItem(IDC_EDIT_PATH)->SetWindowTextW(filePath);
	}
}


void CdownloadDlg::OnBnClickedButtonDownload()
{
	// TODO: 在此添加控件通知处理程序代码
#if 1
	if (!m_open)
	{
		MessageBox(L"设备未打开！", NULL, MB_OK | MB_ICONSTOP);
		return;
	}

	if (is_downloading == FALSE)
	{
		CFile file;
		if (!file.Open(filePath, CFile::modeRead))
		{
			MessageBox(L"未选择下载文件！", NULL, MB_OK | MB_ICONSTOP);
			return;
		}
		file.Close();
	}

	if (is_downloading)
	{
		is_downloading = FALSE;
		GetDlgItem(IDC_BUTTON_DOWNLOAD)->SetWindowTextW(L"开始下载");
		return;
	}
	else
	{
		is_downloading = TRUE;
		GetDlgItem(IDC_BUTTON_DOWNLOAD)->SetWindowTextW(L"停止下载");
		return;
	}

#endif
}


//设置按钮是否可用
void CdownloadDlg::enablebtn(BOOL bEnable)  //bEnable=true :able ;=false:enable
{
	//GetDlgItem(IDC_BUTTON_DOWNLOAD)->EnableWindow(bEnable);
	GetDlgItem(IDC_BUTTON_CHECK)->EnableWindow(bEnable);
	GetDlgItem(IDC_BUTTON_REBOOT)->EnableWindow(bEnable);

	if (bEnable)  //窗体标题显示
		SetWindowText(L"I2C下载器已插上");
	else
		SetWindowText(L"I2C下载器已拔出");
}
BOOL CdownloadDlg::mClose()  //关闭CH341设备
{
	CH341CloseDevice(mIndex);
	m_open = FALSE;
	return TRUE;
}

void CdownloadDlg::OnDestroy()
{
	mClose();
	CH341SetDeviceNotify(mIndex, NULL, NULL);  //注销监视通知
	KillTimer(1);
	CDialog::OnDestroy();
}

void CdownloadDlg::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) //插拔事件过程
{
	ULONG iEventStatus;
	iEventStatus = nChar;  //插拔事件

	if (iEventStatus == CH341_DEVICE_ARRIVAL) { // 设备插入事件,已经插入
		if (CH341OpenDevice(mIndex) == INVALID_HANDLE_VALUE) {
			MessageBox(L"打开设备失败!", L"CH341PAR", MB_OK | MB_ICONSTOP);
			m_open = FALSE;
		}
		else
			m_open = TRUE; //打开成功	
	}
	else if (iEventStatus == CH341_DEVICE_REMOVE) { // 设备拔出事件,已经拔出
		CH341CloseDevice(mIndex);
		m_open = FALSE;
	}
	enablebtn(m_open); //设备打开,按钮可用,设备没打开,按钮禁用
}

void CdownloadDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	download_time++;
	CDialogEx::OnTimer(nIDEvent);
}


void CdownloadDlg::OnBnClickedButtonCheck()
{
	UINT16 cmd;
	UCHAR wbuffer[mMAX_BUFFER_LENGTH] = "";
	UCHAR rbuffer[mMAX_BUFFER_LENGTH] = "";
	ULONG mwlen = 0, mrlen = 0;
	ULONG i;

	if (is_downloading)
		return;

	// TODO: 在此添加控件通知处理程序代码
	wbuffer[0] = 0xB8;
	wbuffer[1] = 0xF8;

	for (i = 0; i < 100; i++)
	{
		if (CH341StreamI2C(p->mIndex, 2, &wbuffer[0], 1, &rbuffer[0]))
		{
			break;
		}
	}
	//超时
	if (i == 100)
	{
		MessageBox(L"固件信息获取失败！", NULL, MB_OK | MB_ICONSTOP);
		return;
	}

	for (cmd = 1000; cmd < 1010; cmd++)
	{
		//1.	0xFB寄存器中写入指令
		wbuffer[0] = 0xB8;
		wbuffer[1] = 0xFB;
		wbuffer[2] = 0xA5;
		wbuffer[3] = cmd / 256;
		wbuffer[4] = cmd % 256;
		wbuffer[5] = cmd / 256;
		wbuffer[6] = cmd % 256;

		for (i = 0; i < PACKET_SIZE; i++)
			wbuffer[7 + i] = 0xFF;

		UINT8 check_sum = 0;
		for (i = 0; i < PACKET_SIZE + 5; i++)
			check_sum ^= wbuffer[i + 2];
		wbuffer[PACKET_SIZE + 7] = check_sum;
		wbuffer[PACKET_SIZE + 8] = 0x5A;

		CH341StreamI2C(p->mIndex, PACKET_SIZE + 9, &wbuffer[0], 0, &rbuffer[0]);

		//2.	0xFA寄存器中写入数据0x26
		wbuffer[0] = 0xB8;
		wbuffer[1] = 0xFA;
		wbuffer[2] = 0x26;
		CH341StreamI2C(p->mIndex, 3, &wbuffer[0], 0, &rbuffer[0]);

		//3.	从0xD9寄存器读取一个字节
		Sleep(50);


		wbuffer[0] = 0xB8;
		wbuffer[1] = 0xD9;
		CH341StreamI2C(p->mIndex, 2, &wbuffer[0], 1, &rbuffer[cmd-1000]);	
	}

	char temp[20];
	CString str_temp;
	
	sprintf_s(temp, "v%d.%02d", rbuffer[0], rbuffer[1] );
	str_temp = temp;
	GetDlgItem(IDC_STATIC_FW_VERSION)->SetWindowTextW(str_temp);

	sprintf_s(temp, "v%d.%02d", rbuffer[2], rbuffer[3]);
	str_temp = temp;
	GetDlgItem(IDC_STATIC_BOOTLOADER_VERSION)->SetWindowTextW(str_temp);

	sprintf_s(temp, "v%d.%02d", rbuffer[4], rbuffer[5]);
	str_temp = temp;
	GetDlgItem(IDC_STATIC_RESOURCE_VERSION)->SetWindowTextW(str_temp);

	sprintf_s(temp, "%02X%02X%02X%02X", rbuffer[6], rbuffer[7], rbuffer[8], rbuffer[9]);
	str_temp = temp;
	GetDlgItem(IDC_STATIC_CHECKSUM)->SetWindowTextW(str_temp);

}


void CdownloadDlg::OnBnClickedButtonReboot()
{
	// TODO: 在此添加控件通知处理程序代码
	UINT16 cmd;
	UCHAR wbuffer[mMAX_BUFFER_LENGTH] = "";
	UCHAR rbuffer[mMAX_BUFFER_LENGTH] = "";
	ULONG mwlen = 0, mrlen = 0;
	ULONG i;

	if (is_downloading)
		return;

	// TODO: 在此添加控件通知处理程序代码
	wbuffer[0] = 0xB8;
	wbuffer[1] = 0xF8;

	for (i = 0; i < 100; i++)
	{
		if (CH341StreamI2C(p->mIndex, 2, &wbuffer[0], 1, &rbuffer[0]))
		{
			break;
		}
	}
	//超时
	if (i == 100)
	{
		MessageBox(L"固件信息获取失败！", NULL, MB_OK | MB_ICONSTOP);
		return;
	}
	cmd = 1010;
	//1.	0xFB寄存器中写入指令
	wbuffer[0] = 0xB8;
	wbuffer[1] = 0xFB;
	wbuffer[2] = 0xA5;
	wbuffer[3] = cmd / 256;
	wbuffer[4] = cmd % 256;
	wbuffer[5] = cmd / 256;
	wbuffer[6] = cmd % 256;

	for (i = 0; i < PACKET_SIZE; i++)
		wbuffer[7 + i] = 0xFF;

	UINT8 check_sum = 0;
	for (i = 0; i < PACKET_SIZE + 5; i++)
		check_sum ^= wbuffer[i + 2];
	wbuffer[PACKET_SIZE + 7] = check_sum;
	wbuffer[PACKET_SIZE + 8] = 0x5A;

	CH341StreamI2C(p->mIndex, PACKET_SIZE + 9, &wbuffer[0], 0, &rbuffer[0]);

	//2.	0xFA寄存器中写入数据0x26
	wbuffer[0] = 0xB8;
	wbuffer[1] = 0xFA;
	wbuffer[2] = 0x26;
	CH341StreamI2C(p->mIndex, 3, &wbuffer[0], 0, &rbuffer[0]);
}


BOOL CdownloadDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类
	static bool m_ctrl_down = false;//此函数第一次调用的时候初始化

	if (pMsg->message == WM_KEYDOWN)
	{
		if ((GetKeyState(VK_CONTROL) & 0x80) == 0x80 && (GetKeyState(VK_SHIFT) & 0x80) == 0x80 && (GetKeyState('E') & 0x80) == 0x80)
		{
			GetDlgItem(IDC_BUTTON_REBOOT)->ShowWindow(TRUE);
			return TRUE;
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}
