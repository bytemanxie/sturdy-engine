﻿// RemoteCtrl.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <atlimage.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

void Dump(BYTE* pData, size_t nSize)
{
	std::string strOut;
	for (size_t i = 0; i < nSize; i++)
	{
		char buf[8] = "";
		if (i > 0 && (i % 16 == 0))strOut += "\n";
		snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
		strOut += buf;
	}
	strOut += "\n";
	OutputDebugStringA(strOut.c_str());
}

int MakeDriverInfo() {//1==>A 2==>B 3==>C ... 26==>Z
	std::string result;
	for (int i = 1; i <= 26; i++) {
		if (_chdrive(i) == 0) {
			if (result.size() > 0)
				result += ',';
			result += 'A' + i - 1;
		}
	}
	CPacket pack(1, (BYTE*)result.c_str(), result.size());//将result打包
	Dump((BYTE*)pack.Data(), pack.Size());
	CServerSocket::getInstance()->Send(pack);
	return 0;
}

#include <stdio.h>
#include <io.h>
#include <list>



int MakeDirectoryInfo() {
	std::string strPath;
	//std::list<FILEINFO> lstFileInfos;
	if (CServerSocket::getInstance()->GetFilePath(strPath) == false)
	{
		OutputDebugString(_T("当前的命令，不是获取文件列表，命令解析错误！！\n"));
		return -1;
	}
	if (_chdir(strPath.c_str()) != 0)
	{
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		CPacket pack(2, (BYTE*)&finfo, sizeof finfo);
		CServerSocket::getInstance()->Send(pack);
		OutputDebugString(_T("没有权限，访问目录！！\n"));

		return -2;
	}
	_finddata_t fdata;
	int hfind = 0;
	if ((hfind = _findfirst("*", &fdata)) == -1)
	{
		OutputDebugString(_T("没有找到任何文件！！\n"));
		return -3;
	}
	int count = 0;
	do {
		FILEINFO finfo;
		finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
		memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
		//lstFileInfos.push_back(finfo);
		count++;
		CPacket pack(2, (BYTE*)&finfo, sizeof finfo);
		CServerSocket::getInstance()->Send(pack);
		//Sleep(1);
	} while (!_findnext(hfind, &fdata));
	TRACE("server count = %d\r\n", count);
	FILEINFO finfo;
	finfo.HasNext = FALSE;
	CPacket pack(2, (BYTE*)&finfo, sizeof finfo);
	CServerSocket::getInstance()->Send(pack);
	return 0;
}

int RunFile()
{
	std::string strPath;
	CServerSocket::getInstance()->GetFilePath(strPath);
	ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);//打开文件
	CPacket pack(3, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
	return 0;
}

//第一个包发送文件的长度
int DownloadFile() {
	std::string strPath;
	CServerSocket::getInstance()->GetFilePath(strPath);
	long long data = 0;
	FILE* pFile = NULL;
	errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
	if (err != 0)
	{
		CPacket pack(4, (BYTE*)&data, 8);
		CServerSocket::getInstance()->Send(pack);
		return -1;
	}
	if (pFile != NULL)
	{
		fseek(pFile, 0, SEEK_END);
		data = _ftelli64(pFile);
		CPacket head(4, (BYTE*)&data, 8);
		CServerSocket::getInstance()->Send(head);//发送一个文件长度
		fseek(pFile, 0, SEEK_SET);

		char buffer[1024] = "";
		size_t rlen = 0;
		do {
			rlen = fread(buffer, 1, 1024, pFile);
			CPacket pack(4, (BYTE*)buffer, rlen);
			//Sleep(1);
			CServerSocket::getInstance()->Send(pack);//读1K发1K
		} while (rlen >= 1024);//不足1024说明读到文件尾
		fclose(pFile);
	}

	CPacket pack(4, NULL, 0);
	CServerSocket::getInstance()->Send(pack);//结束再发一个包

	return 0;
}

int MouseEvent()
{
	MOUSEEV mouse;
	if (CServerSocket::getInstance()->GetMouseEvent(mouse))
	{

		DWORD nFlags = 0;

		switch (mouse.nButton)
		{
		case 0://左键
			nFlags = 1;
			break;
		case 1://右键
			nFlags = 2;
			break;
		case 2://中键
			nFlags = 4;
			break;
		case 4://没有按键
			nFlags = 8;
			break;
		}
		if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
		switch (mouse.nAction)
		{
		case 0://单击
			nFlags |= 0x10;
			break;
		case 1://双击
			nFlags |= 0x20;
			break;
		case 2://按下
			nFlags |= 0x40;
			break;
		case 3://放开
			nFlags |= 0x80;
			break;
		default:
			break;
		}
		/*if (mouse.nButton == 8 && mouse.nAction == 0) {
			printf("nButton %d nAction %d\r\n", mouse.nButton, mouse.nAction);
			printf("mouse event : %08X x %d y %d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
		}*/
		
		switch (nFlags)
		{
		case 0x21://触发左键双击效果
			//printf("0x21\r\n");

			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x11:
			//printf("0x11\r\n");

			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x41:
			//printf("0x41\r\n");

			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81:
			//printf("0x81\r\n");

			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x22:
			//printf("22\r\n");

			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x12:
			//printf("0x12\r\n");

			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;

		case 0x42://右键按下
			//printf("右键按下\r\n");

			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82://右键放开
			//printf("右键放开\r\n");

			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;

		case 0x24://中键双击
			//printf("中键双击\r\n");
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x14:
			//printf("0x14\r\n");
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;

		case 0x44:
			//printf("44\r\n");

			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84:
			//printf("0x84\r\n");
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x08://单纯鼠标移动
			//printf("Move:%d \r\n", nFlags);
			mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
			break;
		}
		CPacket pack(5, NULL, 0);
		CServerSocket::getInstance()->Send(pack);
	}
	else
	{
		OutputDebugString(_T("获取鼠标参数失败！！！"));
		return -1;
	}
	return 0;
}

int SendScreen()
{
	CImage screen;//GDI
	HDC hScreen = ::GetDC(NULL);
	int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
	int nwidth = GetDeviceCaps(hScreen, HORZRES);
	int nHeight = GetDeviceCaps(hScreen, VERTRES);
	screen.Create(nwidth, nHeight, nBitPerPixel);

	BitBlt(screen.GetDC(), 0, 0, nwidth, nHeight, hScreen, 0, 0, SRCCOPY);
	ReleaseDC(NULL, hScreen);

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
	if (hMem == NULL) return -1;

	IStream* pStream = NULL;
	HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
	if (ret == S_OK)
	{
		screen.Save(pStream, Gdiplus::ImageFormatPNG);
		LARGE_INTEGER bg = { 0 };
		pStream->Seek(bg, STREAM_SEEK_SET, NULL);//流指针设置为流头
		PBYTE pData = (PBYTE)GlobalLock(hMem);
		SIZE_T nSize = GlobalSize(hMem);

		CPacket pack(6, pData, nSize);
		CServerSocket::getInstance()->Send(pack);
		GlobalUnlock(hMem);
	}

	//DWORD tick = GetTickCount();

	//screen.Save(_T("test2020.png"), Gdiplus::ImageFormatPNG);
	/*TRACE("png %d\r\n", GetTickCount() - tick);

	tick = GetTickCount();
	screen.Save(_T("test2020.jpg"), Gdiplus::ImageFormatJPEG);
	TRACE("jpg %d\r\n", GetTickCount() - tick);*/
	pStream->Release();
	GlobalFree(hMem);
	screen.ReleaseDC();
	return 0;
}

#include "LockInfoDialog.h"
CLockInfoDialog dlg;
unsigned int threadid;

unsigned _stdcall threadLockDlg(void* arg)
{
	TRACE("%s %d %d\r\n", __FILE__, __LINE__, GetCurrentThreadId());
	dlg.Create(IDD_DIALOG_INFO, NULL);
	dlg.ShowWindow(SW_SHOW);
	//遮蔽后台窗口
	CRect rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
	rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
	rect.bottom = LONG(rect.bottom * 1.10);
	dlg.MoveWindow(rect);

	CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
	if (pText)
	{
		CRect rtText;
		pText->GetWindowRect(rtText);
		int nWidth = rtText.Width() / 2;
		int x = (rect.right - nWidth) / 2;
		int nHeight = rtText.Height();
		int y = (rect.bottom - nHeight) / 2;
		pText->MoveWindow(x, y, rtText.Width(), rtText.Height());

	}


	//窗口置顶
	dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	//限制鼠标功能
	ShowCursor(false);
	//隐藏任务栏
	::ShowWindow(::FindWindow(_T("Shell_TrayWind"), NULL), SW_HIDE);
	//CRect rect;
	dlg.GetWindowRect(rect);
	//限制鼠标范围
	rect.right = rect.left + 1;
	rect.bottom = rect.top + 1;
	ClipCursor(rect);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_KEYDOWN)
		{
			TRACE("msg:%08x wparam:%08x lparam:%08x\r\n", msg.message, msg.wParam, msg.lParam);
			if (msg.wParam == 0x41 && msg.lParam == 0x1e0001)//按ESC退出
			{
				break;
			}
		}
	}
	ClipCursor(NULL);
	//恢复鼠标
	ShowCursor(false);
	//恢复任务栏
	::ShowWindow(::FindWindow(_T("Shell_TrayWind"), NULL), SW_SHOW);
	dlg.DestroyWindow();
	_endthreadex(0);
	return 0;
}

int LockMachine()
{
	if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE))
	{
		//_beginthread(threadLockDlg, 0, NULL);
		_beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid);
	}
	CPacket pack(7, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
	return 0;
}

int UnlockMachine()
{
	//dlg.SendMessage(WM_KEYDOWN, 0x41, 0x1e0001);
	//::SendMessage(dlg, WM_KEYDOWN, 0x41, 0x1e0001);
	PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0x1e0001);
	CPacket pack(8, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
	return 0;
}

int TestConnect()
{
	CPacket pack(1981, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
	return 0;
}

int DeleteLocalFile()
{
	std::string strPath;
	CServerSocket::getInstance()->GetFilePath(strPath);
	TCHAR sPath[MAX_PATH] = _T("");
	mbstowcs(sPath, strPath.c_str(), strPath.size());
	MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath, sizeof sPath /
		sizeof TCHAR);
	DeleteFileA(strPath.c_str());


	CPacket pack(9, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
	return 0;
}

int ExcuteCommand(int nCmd)
{
	int ret = 0;
	switch (nCmd)
	{
	case 1:
		ret = MakeDriverInfo();
		break;
	case 2:
		ret = MakeDirectoryInfo();
		break;
	case 3:
		ret = RunFile();
		break;
	case 4:
		ret = DownloadFile();
		break;
	case 5:
		ret = MouseEvent();
		break;
	case 6:
		ret = SendScreen();
		break;
	case 7:
		ret = LockMachine();
		/*Sleep(50);
		LockMachine();*/
		break;
	case 8:
		ret = UnlockMachine();
		break;
	case 9:
		ret = DeleteLocalFile();
		break;
	case 1981:
		ret = TestConnect();
		break;

	}
	return ret;
}

int main()
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(nullptr);

	if (hModule != nullptr)
	{
		// initialize MFC and print and error on failure
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: code your application's behavior here.
			wprintf(L"Fatal Error: MFC initialization failed\n");
			nRetCode = 1;
		}
		else
		{
			//TODO: code your application's behavior here.
			CServerSocket* pserver = CServerSocket::getInstance();
			int count = 0;
			if (pserver->InitSocket() == false)
			{
				MessageBox(NULL, _T("网络初始化异常！"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
				exit(0);
			}
			while (CServerSocket::getInstance())
			{
				if (pserver->AcceptClient() == false)
				{
					if (count >= 3)
					{
						MessageBox(NULL, _T("多次无法接入用户， 结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
						exit(0);
					}
					MessageBox(NULL, _T("无法接入用户， 自动重试！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
					count++;
				}
				int ret = pserver->DealCommand();
				if (ret > 0)
				{
					ret = ExcuteCommand(ret);
					if (ret)
					{
						TRACE("执行命令失败：%d ret = %d\r\n", pserver->GetPacket().sCmd, ret);
					}
					pserver->CloseClient();
				}

				//TODO
			}
		}
	}
	else
	{
		// TODO: change error code to suit your needs
		wprintf(L"Fatal Error: GetModuleHandle failed\n");
		nRetCode = 1;
	}

	return nRetCode;
}
