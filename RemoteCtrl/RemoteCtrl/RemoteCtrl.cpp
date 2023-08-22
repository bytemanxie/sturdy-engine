﻿// RemoteCtrl.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "EdoyunTool.h"


// The one and only application object

CWinApp theApp;

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
			CCommand cmd;
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
					ret = cmd.ExcuteCommand(ret);
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
