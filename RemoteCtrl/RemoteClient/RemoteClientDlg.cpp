
// RemoteClientDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "ClientSocket.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "CWatchDialog.h"


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
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


// CRemoteClientDlg dialog



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

void CRemoteClientDlg::threadEntryForWatchData(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadWatchData();
	_endthread();
}

void CRemoteClientDlg::threadWatchData()
{
	Sleep(50);
	CClientSocket* pClient = NULL;
	do {
		pClient = CClientSocket::getInstance();
	} while (pClient == NULL);
	ULONGLONG tick = GetTickCount64();
	for (;;)
	{
		if (m_isFull == false)
		{
			int ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 1, NULL);
			if (ret == 6)
			{
				BYTE* pData = (BYTE*)pClient->GetPacket().strData.c_str();
				//TODO:存入CImage
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
				if (hMem == NULL)
				{
					TRACE("内存不足！");
					Sleep(1);
					continue;
				}

				IStream* pStream = NULL;
				HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
				if (hRet == S_OK)
				{
					ULONG length = 0;
					pStream->Write(pData, pClient->GetPacket().strData.size(), &length);
					LARGE_INTEGER bg = { 0 };
					pStream->Seek(bg, STREAM_SEEK_SET, NULL);//流指针设置为流头
					m_image.Load(pStream);
					m_isFull = true;
				}
			}
			else
			{
				Sleep(1);
			}
		}
		else
		{
			Sleep(1);
		}
	}
}

void CRemoteClientDlg::threadEntryForDownFile(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadDownFile();
	_endthread();
}

void CRemoteClientDlg::threadDownFile()
{
	int nListSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nListSelected, 0);

	CFileDialog dlg(FALSE, "*", m_List.GetItemText(nListSelected, 0), OFN_READONLY |
		OFN_OVERWRITEPROMPT, NULL, this);

	if (dlg.DoModal() == IDOK)
	{
		HTREEITEM hSelected = m_Tree.GetSelectedItem();
		strFile = GetPath(hSelected) + strFile;

		FILE* pFile = fopen(dlg.GetPathName(), "wb+");
		if (pFile == NULL)
		{
			AfxMessageBox(_T("本地没有权限保存该文件， 或者无法创建！！！"));
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}

		TRACE("%s\r\n", (LPCSTR)strFile);
		CClientSocket* pClient = CClientSocket::getInstance();
		do {
			//int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
			int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile);
			if (ret < 0)
			{
				AfxMessageBox("执行下载命令失败！！");
				TRACE("执行下载失败：ret = %d\r\n", ret);
				break;
			}


			//第一个包返回文件长度
			long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
			if (nLength == 0)
			{
				AfxMessageBox("文件长度为零或者无法读取文件！！！");
				break;
			}

			long long nCount = 0;
			while (nCount < nLength)
			{
				ret = pClient->DealCommand();
				if (ret < 0)
				{
					AfxMessageBox("传输失败！！");
					TRACE("传输失败：ret = %d\r\n", ret);
					break;
				}
				//const char* buf = pClient->GetPacket().strData.c_str();
				fwrite(pClient->GetPacket().strData.c_str(), 1, 
					pClient->GetPacket().strData.size(), pFile);
				nCount += pClient->GetPacket().strData.size();
			}

		} while (false);

		fclose(pFile);
		pClient->CloseSocket();
	}
	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
	MessageBox(_T("下载完成"), _T("完成"));
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL) return;
	if (m_Tree.GetChildItem(hTreeSelected) == NULL) return;
	
	DeleteTreeItem(hTreeSelected);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);
	int nCmnd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	int cout = 0;

	while (pInfo->HasNext) { //文件还有下一个文件，找下一个文件
		if (pInfo->IsDirectory)
		{
			if (CString(pInfo->szFileName) == "." || (CString(pInfo->szFileName) == ".."))
			{
				int cmd = pClient->DealCommand();
				//TRACE("ack:%d\r\n", cmd);
				if (cmd < 0) break;
				pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
			m_Tree.InsertItem("", hTemp, TVI_LAST);
		}
		else
		{
			m_List.InsertItem(0, pInfo->szFileName);
		}
		cout++;
		int cmd = pClient->DealCommand();
		//TRACE("ack: %d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	TRACE("CLIENT cout = %d\r\n", cout);
	pClient->CloseSocket();
}

void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);

	m_List.DeleteAllItems();
	int nCmnd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();

	while (pInfo->HasNext) {
		if (!pInfo->IsDirectory)
		{
			m_List.InsertItem(0, pInfo->szFileName);
		}
		int cmd = pClient->DealCommand();
		TRACE("ack: %d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}

	pClient->CloseSocket();
}

int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	//UpdateData() 函数的作用是将控件的值从对话框的数据结构更新到控件关联的变量中，
	//或者将变量的值更新到对应的控件中。
	UpdateData();
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_nPort));
	if (!ret)
	{
		AfxMessageBox(_T("网络初始化失败！"));
		return -1;
	}

	CPacket pack(nCmd, pData, nLength);

	ret = pClient->Send(pack);
	//TRACE("Send ret %d\r\n", ret);
	int cmd = pClient->DealCommand();
	//TRACE("ack:%d\r\n", cmd);
	if(bAutoClose)
		pClient->CloseSocket();
	return cmd;
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	//ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	//WM_SEND_PACKET 的自定义消息被派发到 CRemoteClientDlg 类的窗口时，
	//将调用 CRemoteClientDlg::OnSendPacket 这个成员函数来处理该消息。
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)
	
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CRemoteClientDlg message handlers

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	UpdateData();
	m_server_address = 0x7f000001;
	m_nPort = _T("9527");
	UpdateData(FALSE);//当bSave为FALSE时，函数将从数据成员中读取数据，并将其显示到窗口控件上。
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
	m_isFull = false;

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRemoteClientDlg::OnPaint()
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
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CRemoteClientDlg::OnBnClickedBtnTest()
{
	SendCommandPacket(1981);
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	// TODO: 在此添加控件通知处理程序代码
	int ret = SendCommandPacket(1);
	TRACE("ret = %d\r\n", ret);
	if (ret == -1)
	{
		AfxMessageBox(_T("命令处理失败！！！"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drivers = pClient->GetPacket().strData;
	std::string dr;
	m_Tree.DeleteAllItems();
	for (size_t i = 0; i < drivers.size(); i++)
	{
		if (drivers[i] != ',')
		{
			dr = drivers[i];
			dr += ':';
			HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
			dr.clear();
		}
	}
}


 CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = m_Tree.GetParentItem(hTree);
	} while (hTree);
	return strRet;
}

 void CRemoteClientDlg::DeleteTreeItem(HTREEITEM hTree)
 {
	 HTREEITEM hSub = NULL;
	 do {
		 hSub = m_Tree.GetChildItem(hTree);
		 if(hSub!=NULL) m_Tree.DeleteItem(hSub);
	 } while (hSub!=NULL);
 }

//void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
//{
//	// TODO: 在此添加控件通知处理程序代码
//	*pResult = 0;
//	LoadFileInfo();
//}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0) return;

	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK); //从资源中加载菜单资源 IDR_MENU_RCLICK，并将其关联到 menu 对象。
	CMenu* pPupup = menu.GetSubMenu(0);//获取菜单中的第一个子菜单（弹出菜单），并将其赋值给指针 pPupup。
	if (pPupup)
	{
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);//如果弹出菜单存在（pPupup 不为空），则以指定的位置参数调用 TrackPopupMenu 函数显示菜单。

		//其中，TPM_LEFTALIGN 表示菜单左对齐，TPM_RIGHTBUTTON 表示使用右键激活菜单。ptMouse.x 和 ptMouse.y 表示菜单显示的位置，this 表示在当前窗口上显示菜单。
	}
}


void CRemoteClientDlg::OnDownloadFile()
{
	_beginthread(CRemoteClientDlg::threadEntryForDownFile, 0, this); 
	BeginWaitCursor();
	m_dlgStatus.m_info.SetWindowText(_T("命令正在执行中！"));
	m_dlgStatus.ShowWindow(SW_SHOW);
	m_dlgStatus.CenterWindow(this);
	m_dlgStatus.SetActiveWindow();
	Sleep(50);
}


void CRemoteClientDlg::OnDeleteFile()
{
	// TODO: 在此添加命令处理程序代码
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);

	strFile = strPath + strFile;
	int ret = SendCommandPacket(9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0)
	{
		AfxMessageBox("删除文件命令执行失败 ！！！");
	}
	LoadFileCurrent();
}


void CRemoteClientDlg::OnRunFile()
{
	// TODO: 在此添加命令处理程序代码
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);

	strFile = strPath + strFile;
	int ret = SendCommandPacket(3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0)
	{
		AfxMessageBox("打开文件命令执行失败 ！！！");
	}

}

LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
{
	int cmd = wParam >> 1;
	int ret = 0;
	switch (cmd)
	{
	case 4:
		{
			CString strFile = (LPCSTR)lParam;
			ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
		}
		break;
	case 5:
	{
		ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)lParam, sizeof MOUSEEV);
	}
		break;
	case 6:
		{
			ret = SendCommandPacket(cmd, wParam & 1, NULL, 0);
		}
		break;
	default:
		ret = -1;
	}
	
	return ret;
}

void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	// TODO: 在此添加控件通知处理程序代码
	CWatchDialog dlg(this);
	_beginthread(CRemoteClientDlg::threadEntryForWatchData, 0, this);
	dlg.DoModal();
}


