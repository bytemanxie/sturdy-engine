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
#include <string>

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

void udp_server();

void udp_client(bool ishost = true);

void initsock()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
}

void clearsock()
{
	WSACleanup();
}

int main(int argc, char* argv[])
{
	if (!CEdoyunTool::Init()) return 1;
	initsock();
	//iocp();
	if (argc == 1)
	{
		char wstDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, wstDir);
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;

		memset(&si, 0, sizeof si);
		memset(&pi, 0, sizeof pi);

		std::string strCmd = argv[0];

		strCmd += " 1";
		BOOL bRet = CreateProcessA(
			NULL,
			(LPSTR)strCmd.c_str(),
			NULL,
			NULL,
			FALSE,
			/*CREATE_NEW_CONSOLE*/0,
			NULL,
			wstDir,
			&si,
			&pi);
		if (bRet)
		{
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			TRACE("进程id %d\r\n", pi.dwProcessId);
			TRACE("线程id %d\r\n", pi.dwThreadId);
			strCmd += " 2";

			bRet = CreateProcessA(
				NULL,
				(LPSTR)strCmd.c_str(),
				NULL,
				NULL,
				FALSE,
				/*CREATE_NEW_CONSOLE*/0,
				NULL,
				wstDir,
				&si,
				&pi);

			if (bRet)
			{
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				TRACE("进程id %d\r\n", pi.dwProcessId);
				TRACE("线程id %d\r\n", pi.dwThreadId);
				udp_server();
			}
		}
	}
	else if (argc == 2)
	{
		udp_client();
	}
	else
	{
		udp_client(false);
	}
	clearsock();
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

#include "ENetwork.h"

int RecvFromCB(void* arg, const EBuffer& buffer, ESockaddrIn& addr)
{
	EServer* server = (EServer*)arg;
	return server->Sendto(addr, buffer);
}

int SendToCB(void* arg, const ESockaddrIn& addr, int ret)
{
	EServer* server = (EServer*)arg;
	printf("sendto done!%p\r\n", server);
	return 0;
}

void udp_server()
{
	std::list<ESockaddrIn> lstclients;
	printf("%s (%d): %s \r\n", __FILE__, __LINE__, __FUNCTION__);
	EServerParameter param("127.0.0.1", 20000, ETYPE::ETypeUDP, NULL, NULL, NULL, RecvFromCB, SendToCB);
	EServer server(param);
	server.Invoke(&server);
	printf("%s (%d): %s \r\n", __FILE__, __LINE__, __FUNCTION__);
	getchar();
	return;
	
	
}

void udp_client(bool ishost)
{
	Sleep(2000);
	sockaddr_in server, client;
	int ret = 0, len = sizeof client;
	server.sin_family = AF_INET;
	server.sin_port = htons(20000);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
	
	if (sock == INVALID_SOCKET)
	{
		printf("%s (%d): %s ERROR(%d)!!!\r\n", __FILE__, __LINE__,
			__FUNCTION__, WSAGetLastError());
		return;
	}

	if (ishost)
	{
		//主客户端代码
		printf("%s (%d): %s \r\n", __FILE__, __LINE__, __FUNCTION__);
		EBuffer msg = "hello world";
		ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof server);
		printf("%s (%d): %s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);

		if (ret > 0)
		{
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("host %s (%d): %s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			if (ret > 0)
			{
				printf("%s (%d): %s ip %08X port %d \r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));

				printf("%s (%d): %s msg = %d\r\n", __FILE__, __LINE__, __FUNCTION__,
					msg.size());
			}
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("host %s (%d): %s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			if (ret > 0)
			{
				printf("%s (%d): %s ip %08X port %d \r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));

				printf("%s (%d): %s msg = %s\r\n", __FILE__, __LINE__, __FUNCTION__, msg.c_str());
			}
		}
	}
	else
	{
		//从客户端代码
		printf("%s (%d): %s \r\n", __FILE__, __LINE__, __FUNCTION__);
		std::string msg = "hello world";
		ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof server);
		printf("%s (%d): %s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);

		if (ret > 0)
		{
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("client %s (%d): %s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);

			if (ret > 0)
			{
				sockaddr_in addr;
				memcpy(&addr, msg.c_str(), sizeof addr);
				sockaddr_in* paddr = (sockaddr_in*)&addr;

				printf("%s (%d): %s ip %08X port %d \r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));

				printf("%s (%d): %s msg = %d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());

				printf("%s (%d): %s ip %08X port %d \r\n", __FILE__, __LINE__, __FUNCTION__, paddr->sin_addr.s_addr, ntohs(paddr->sin_port));
				msg = "hello I am client\r\n";

				ret = sendto(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)paddr, sizeof sockaddr_in);
				printf("%s (%d): %s ip %08X port %d \r\n", __FILE__, __LINE__, __FUNCTION__, paddr->sin_addr.s_addr, ntohs(paddr->sin_port));

				printf("client %s (%d): %s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			}
		}
	}
	closesocket(sock);
}