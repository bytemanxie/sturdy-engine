﻿
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
#include "ClientController.h"


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
public:
	
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

//void CRemoteClientDlg::threadEntryForWatchData(void* arg)
//{
//	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
//	thiz->threadWatchData();
//	_endthread();
//}
//
//void CRemoteClientDlg::threadWatchData()
//{
//	Sleep(50);
//	CClientController* pCtrl = CClientController::getInstance();
//	while(!m_isClosed)
//	{
//		if (m_isFull == false)
//		{
//			int ret = pCtrl->SendCommandPacket(6);
//			
//			if (ret == 6)
//			{
//				if(pCtrl->GetImage(m_image) == 0){
//					m_isFull = true;
//				}
//				else
//				{
//					TRACE("获取图片失败！\r\n");
//				}
//			}
//			else
//			{
//				Sleep(1);
//			}
//		}
//		else
//		{
//			Sleep(1);
//		}
//	}
//}

//void CRemoteClientDlg::threadEntryForDownFile(void* arg)
//{
//	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
//	thiz->threadDownFile();
//	_endthread();
//}

//void CRemoteClientDlg::threadDownFile()
//{
//	int nListSelected = m_List.GetSelectionMark();
//	CString strFile = m_List.GetItemText(nListSelected, 0);
//
//	CFileDialog dlg(FALSE, "*", m_List.GetItemText(nListSelected, 0), OFN_READONLY |
//		OFN_OVERWRITEPROMPT, NULL, this);
//
//	if (dlg.DoModal() == IDOK)
//	{
//		HTREEITEM hSelected = m_Tree.GetSelectedItem();
//		strFile = GetPath(hSelected) + strFile;
//
//		FILE* pFile = fopen(dlg.GetPathName(), "wb+");
//		if (pFile == NULL)
//		{
//			AfxMessageBox(_T("本地没有权限保存该文件， 或者无法创建！！！"));
//			m_dlgStatus.ShowWindow(SW_HIDE);
//			EndWaitCursor();
//			return;
//		}
//
//		TRACE("%s\r\n", (LPCSTR)strFile);
//		CClientSocket* pClient = CClientSocket::getInstance();
//		do {
//			//int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
//			int ret = CClientController::getInstance()->SendCommandPacket(4, false,
//				(BYTE*)(LPCSTR)strFile, strFile.GetLength());
//
//			if (ret < 0)
//			{
//				AfxMessageBox("执行下载命令失败！！");
//				TRACE("执行下载失败：ret = %d\r\n", ret);
//				break;
//			}
//
//
//			//第一个包返回文件长度
//			long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
//			if (nLength == 0)
//			{
//				AfxMessageBox("文件长度为零或者无法读取文件！！！");
//				break;
//			}
//
//			long long nCount = 0;
//			while (nCount < nLength)
//			{
//				ret = pClient->DealCommand();
//				if (ret < 0)
//				{
//					AfxMessageBox("传输失败！！");
//					TRACE("传输失败：ret = %d\r\n", ret);
//					break;
//				}
//				//const char* buf = pClient->GetPacket().strData.c_str();
//				fwrite(pClient->GetPacket().strData.c_str(), 1, 
//					pClient->GetPacket().strData.size(), pFile);
//				nCount += pClient->GetPacket().strData.size();
//			}
//
//		} while (false);
//
//		fclose(pFile);
//		pClient->CloseSocket();
//	}
//	m_dlgStatus.ShowWindow(SW_HIDE);
//	EndWaitCursor();
//	MessageBox(_T("下载完成"), _T("完成"));
//}

void CRemoteClientDlg::LoadFileInfo()
{
	
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL) return;
	
	DeleteTreeItem(hTreeSelected);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);
	TRACE("hTreeSelected %08X\r\n", hTreeSelected);
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false,
		(BYTE*)(LPCTSTR)strPath, strPath.GetLength(), (WPARAM)hTreeSelected);

}

void CRemoteClientDlg::DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam)
{
	switch (nCmd)
	{
	case 1://获取驱动信息
		Str2Tree(strData, m_Tree);
		break;
	case 2://获取文件信息
	{
		UpdataFileInfo(*(PFILEINFO)strData.c_str(), (HTREEITEM)lParam);
	}
	break;
	case 3:
		MessageBox(_T("打开文件完成"), _T("操作完成"), MB_ICONINFORMATION);
		break;
	case 4:
		UpdataDownloadFile(strData, (FILE*)lParam);
		break;
	case 9:
		MessageBox(_T("删除文件完成"), _T("操作完成"), MB_ICONINFORMATION);
		break;
	case 1981:
		MessageBox(_T("连接测试成功！"), _T("连接成功"), MB_ICONINFORMATION);
		break;
	default:
		TRACE("unknow data recevied! %d\r\n",nCmd);
		break;
	}
}

void CRemoteClientDlg::InitUIData()
{
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	UpdateData();
	//m_server_address = 0x7F000001;
	m_server_address = 0xC0A80A10;
	m_nPort = _T("9527");
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
	UpdateData(FALSE);//当bSave为FALSE时，函数将从数据成员中读取数据，并将其显示到窗口控件上。
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
}

void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);

	m_List.DeleteAllItems();
	int nCmnd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false,
		(BYTE*)(LPCTSTR)strPath);
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	

	while (pInfo->HasNext) {
		if (!pInfo->IsDirectory)
		{
			m_List.InsertItem(0, pInfo->szFileName);
		}
		int cmd = CClientController::getInstance()->DealCommand();
		TRACE("ack: %d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
}

void CRemoteClientDlg::Str2Tree(const std::string& drivers, CTreeCtrl& tree)
{
	std::string dr;
	tree.DeleteAllItems();
	for (size_t i = 0; i < drivers.size(); i++)
	{
		if (drivers[i] != ',')
		{
			dr = drivers[i];
			dr += ':';
			HTREEITEM hTemp = tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			tree.InsertItem(NULL, hTemp, TVI_LAST);
			dr.clear();
		}
	}
}

void CRemoteClientDlg::UpdataFileInfo(FILEINFO& pInfo, HTREEITEM hParent)
{
	TRACE("hasnext %d isdirectory %d %s\r\n",
		pInfo.HasNext, pInfo.IsDirectory, pInfo.szFileName);

	if (pInfo.HasNext == FALSE) return;
	if (pInfo.IsDirectory)
	{
		if (CString(pInfo.szFileName) == "." || (CString(pInfo.szFileName) == ".."))
		{
			return;
		}
		TRACE("hselected %08X %08X\r\n", hParent, m_Tree.GetSelectedItem());
		HTREEITEM hTemp = m_Tree.InsertItem(pInfo.szFileName, (HTREEITEM)hParent);
		m_Tree.InsertItem("", hTemp, TVI_LAST);
		m_Tree.Expand((HTREEITEM)hParent, TVE_EXPAND);
	}
	else
	{
		m_List.InsertItem(0, pInfo.szFileName);
	}
}

void CRemoteClientDlg::UpdataDownloadFile(const std::string& strData, FILE* pFile)
{
	static LONGLONG length = 0, index = 0;
	TRACE("length %d index %d\r\n", length, index);
	if (length == 0)
	{
		length = *(long long*)strData.c_str();
		if (length == 0)
		{
			AfxMessageBox(_T("本地没有权限保存该文件， 或者无法创建！！！"));
			CClientController::getInstance()->DownloadEnd();

			return;
		}
	}
	else if (length > 0 && (index >= length))
	{
		fclose(pFile);
		length = 0;
		index = 0;
		CClientController::getInstance()->DownloadEnd();
	}
	else
	{
		fwrite(strData.c_str(), 1, strData.size(), pFile);
		index += strData.size();
		TRACE("index = %d\r\n", index);
		if (index >= length)
		{
			fclose(pFile);
			length = 0;
			index = 0;
			CClientController::getInstance()->DownloadEnd();
		}
	}
}


BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)//WM_COMMAND
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)//WM_COMMAND
	//ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)//WM_NOTIFY
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)//WM_NOTIFY
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)//WM_NOTIFY
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)//WM_COMMAND
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)//WM_COMMAND
	//WM_SEND_PACKET 的自定义消息被派发到 CRemoteClientDlg 类的窗口时，
	//将调用 CRemoteClientDlg::OnSendPacket 这个成员函数来处理该消息。
	//ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)
	
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)//WM_COMMAND
	ON_WM_TIMER()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPackAck)
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

	

	// TODO: Add extra initialization here
	InitUIData();
	

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
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1981);
	TRACE("ret = %d\r\n", ret);
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	// TODO: 在此添加控件通知处理程序代码
	//std::list<CPacket> lstPackets;
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1,
		true, NULL, 0, 0);
	TRACE("ret = %d\r\n", ret);
	if (ret == 0)
	{
		AfxMessageBox(_T("命令处理失败！！！"));
		return;
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
	int nListSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nListSelected, 0);

	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	strFile = GetPath(hSelected) + strFile;
	int ret = CClientController::getInstance()->DownFile(strFile);
	if (ret)
	{
		MessageBox(_T("下载失败！"));
		TRACE("下载失败 ret = %d\r\n", ret);
	}
	
	m_dlgStatus.m_info.SetWindowText(_T("命令正在执行中！"));
	
	//Sleep(50);
}


void CRemoteClientDlg::OnDeleteFile()
{
	// TODO: 在此添加命令处理程序代码
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);

	strFile = strPath + strFile;
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9, true, 
		(BYTE*)(LPCSTR)strFile, strFile.GetLength());
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
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 3, true,
		(BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0)
	{
		AfxMessageBox("打开文件命令执行失败 ！！！");
	}

}

//LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
//{
//	int cmd = wParam >> 1;
//	int ret = 0;
//	switch (cmd)
//	{
//	case 4:
//		{
//			CString strFile = (LPCSTR)lParam;
//			ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
//		}
//		break;
//	case 5:
//	{
//		ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1, (BYTE*)lParam, sizeof MOUSEEV);
//	}
//		break;
//	case 6:
//		{
//			ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1, NULL, 0);
//		}
//		break;
//	case 7:
//	case 8:
//	{
//		ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1);
//	}
//	break;
//	default:
//		ret = -1;
//	}
//	
//	return ret;
//}

void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	// TODO: 在此添加控件通知处理程序代码
	CClientController::getInstance()->StartWatchScreen();
	
}




void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
}


void CRemoteClientDlg::OnEnChangeEditPort()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
}

LRESULT CRemoteClientDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || (lParam == -2))
	{
		TRACE("socket is error %d \r\n", lParam);
	}
	else if (lParam == 1)
	{
		TRACE("socket is closed %d \r\n", lParam);
	}
	else
	{
		if (wParam != NULL)
		{
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			DealCommand(head.sCmd, head.strData, lParam);
			
		}
	}
	
	return 0;
}
