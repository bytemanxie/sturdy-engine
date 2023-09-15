// RemoteCtrl.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include <conio.h>
#include"CEdoyunQueue.h"

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

enum {
	IocpListEmpty,
	IocpListPush,
	IocpListPop,
};

unsigned int _stdcall func(void* arg)
{
	std::string* pstr = (std::string*)arg;
	if (pstr != NULL)
	{
		printf("pop from list:%s\r\n", pstr->c_str());
		delete pstr;
	}
	else
	{
		printf("list is empty\r\n");
	}
	return 0;
}

typedef struct IocpParam {
	int nOperator;//操作
	std::string strData;//数据
	_beginthreadex_proc_type cbFunc;//回调

	HANDLE hEvent;//pop操作需要的
	IocpParam(int op, const char* sData, _beginthreadex_proc_type cb = NULL)
	{
		nOperator = op;
		strData = sData;
		cbFunc = cb;
	}
	IocpParam()
	{
		nOperator = -1;
	}
}IOCP_PARAM;

void threadmain(HANDLE hIOCP)
{
	std::list<std::string> lstString;

	DWORD dwTransferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* pOverlapped = NULL;

	while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE))
	{
		if (dwTransferred == 0 && (CompletionKey == NULL))
		{
			printf("thread is prepare to exit!\r\n");
			break;
		}
		IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;
		if (pParam->nOperator == IocpListPush)
		{
			lstString.push_back(pParam->strData);
		}
		else if (pParam->nOperator == IocpListPop)
		{
			std::string* pStr = NULL;
			if (lstString.size() > 0)
			{
				pStr = new std::string(lstString.front());
				lstString.pop_front();
			}
			if (pParam->cbFunc)
			{
				pParam->cbFunc(pStr);
			}
		}
		else if (pParam->nOperator == IocpListEmpty)
		{
			lstString.clear();
		}
		delete pParam;
	}
}

void threadQueueEntry(HANDLE hIOCP)
{
	threadmain(hIOCP);
	_endthread();//代码到此为止， 会导致本地对象无法调用析构， 从而使得内存发生泄露
}

int main()
{
	if (!CEdoyunTool::Init()) return 1;
	
	//printf("press any key to exit ...\r\n");
	//HANDLE hIOCP = INVALID_HANDLE_VALUE;//Input/Output Completion Port
	//hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);//epoll的区别点1
	//if (hIOCP == INVALID_HANDLE_VALUE || (hIOCP == NULL))
	//{
	//	printf("create iocp failed!\r\n", GetLastError());
	//	return 1;
	//}

	//HANDLE hThread = (HANDLE)_beginthread(threadQueueEntry, 0, hIOCP);

	////getchar();

	ULONGLONG tick = GetTickCount64();
	ULONGLONG tick0 = GetTickCount64();
	CEdoyunQueue<std::string> lstStrings;

	while (_kbhit() == 0)
	{
		if (GetTickCount64() - tick0 > 1300)
		{
			lstStrings.PushBack("hello world");
			tick0 = GetTickCount64();
		}

		if (GetTickCount64() - tick > 2000)
		{
			std::string str;
			lstStrings.PopFront(str);
			tick = GetTickCount64();
			printf("pop from queue:%s\r\n", str.c_str());
		}
		Sleep(1);
	}
	printf("exit done! size %d\r\n", lstStrings.Size());
	lstStrings.clear();
	printf("exit done! size %d\r\n", lstStrings.Size());
	::exit(0);

	//if (CEdoyunTool::isAdmin())
	//{
	//	if (!CEdoyunTool::Init()) return 1;
	//	//TODO: code your application's behavior here.
	//	
	//	if (ChooseAutoInvoke(INVOKE_PATH))
	//	{
	//		CCommand cmd;
	//		int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);

	//		switch (ret)
	//		{
	//		case -1:
	//			MessageBox(NULL, _T("网络初始化异常！"),
	//				_T("网络初始化失败！"),
	//				MB_OK | MB_ICONERROR);
	//			break;
	//		case 2:
	//			MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"),
	//				_T("接入用户失败！"),
	//				MB_OK | MB_ICONERROR);
	//		default:
	//			break;
	//		}
	//	}
	//	
	//}
	//else
	//{
	//	printf("current is run as normal user!\r\n");

	//	if (CEdoyunTool::RunAsAdmin() == false)
	//	{
	//		CEdoyunTool::ShowError();
	//		return 1;
	//	}
	//}

	return 0;
}
