// RemoteCtrl.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include <conio.h>
#include"CEdoyunQueue.h"
#include <MSWSock.h>
#include "EdoyunServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "EdoyunTool.h"

#define INVOKE_PATH _T("C:\\Users\\edoyun\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")

// The one and only application object

CWinApp theApp;

bool ChooseAutoInvoke(const CString& strPath)
{
	if (PathFileExists(strPath))
	{
		return true; 
	}
	
	CString strInfo = _T("该程序只允许用于合法的用途！\n");
	strInfo += _T("继续运行该程序，将使得这台机器处于被监控的状态！\n");
	strInfo += _T("如果你不希望这样， 请按”取消“按钮，退出程序。\n");
	strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
	strInfo += _T("按下“否”按钮， 该程序只运行一次，不会在系统内留下任何东西！\n");
	int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL|MB_ICONWARNING|MB_TOPMOST);

	if (ret == IDYES)
	{
		//WriteRegisterTable(strPath);
		if (!CEdoyunTool::WriteStartupDir(strPath))
		{
			MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}

	}
	else if (ret == IDCANCEL)
	{
		return false;
	}
	return true;
}

void iocp();

int main()
{
	if (!CEdoyunTool::Init()) return 1;
	iocp();
	return 0;
}

class COverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;
	char m_buffer[4096];

	COverlapped()
	{
		m_operator = 0;
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		memset(&m_buffer, 0, sizeof(m_buffer));
	}
};

void iocp()
{
	EdoyunServer server;
	server.StartService();
	getchar();

	////SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	//SOCKET sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	//if (sock == INVALID_SOCKET)
	//{
	//	CEdoyunTool::ShowError();
	//	return;
	//}

	//HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, sock, 4);
	//SOCKET client = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	//CreateIoCompletionPort((HANDLE)sock, hIOCP, 0, 0);

	//sockaddr_in addr;
	//addr.sin_family = PF_INET;
	//addr.sin_addr.s_addr = inet_addr("0.0.0.0");
	//addr.sin_port = htons(9527);
	//bind(sock, (sockaddr*)&addr, sizeof sockaddr);
	//listen(sock, 5);

	//COverlapped overlapped;
	//memset(&overlapped, 0, sizeof overlapped);
	//overlapped.m_operator = 1;//accept


	//char buffer[4096] = "";

	//DWORD received = 0;
	//if (AcceptEx(sock, client, overlapped.m_buffer, 0, sizeof(sockaddr_in) + 16,
	//	sizeof(sockaddr_in) + 16, &received, &overlapped.m_overlapped) == FALSE)
	//{
	//	CEdoyunTool::ShowError();
	//}

	//overlapped.m_operator = 2;
	//WSASend();
	//overlapped.m_operator = 3;
	//WSARecv();

	//while (true)
	//{
	//	LPOVERLAPPED pOverlapped = NULL;
	//	DWORD transferred = 0;
	//	DWORD key = 0;
	//	if (GetQueuedCompletionStatus(hIOCP, &transferred, &key, &pOverlapped, INFINITY))
	//	{
	//		COverlapped* p0 = CONTAINING_RECORD(pOverlapped, COverlapped, m_operator);
	//		switch(p0->m_operator)
	//		{
	//		case 1://处理accept的操作
	//			break;
	//		default:
	//			break;
	//		}
	//		
	//	}
	//}
}
