// RemoteCtrl.cpp : This file contains the 'main' function. Program execution begins and ends there.
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

void ChooseAutoInvoke()
{
	TCHAR wcsSystem[MAX_PATH] = _T("");
	//GetSystemDirectory(wcsSystem, MAX_PATH);
	
	CString strPath = /*CString(wcsSystem) + */CString(_T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe"));
	std::cout << 1 << (LPCTSTR)strPath << std::endl;
	if (PathFileExists(strPath))
	{
		std::cout << 2 << strPath << std::endl;
		return; 
	}

	CString strSubkey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	CString strInfo = _T("该程序只允许用于合法的用途！\n");
	strInfo += _T("继续运行该程序，将使得这台机器处于被监控的状态！\n");
	strInfo += _T("如果你不希望这样， 请按”取消“按钮，退出程序。\n");
	strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
	strInfo += _T("按下“否”按钮， 该程序只运行一次，不会在系统内留下任何东西！\n");
	int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL|MB_ICONWARNING|MB_TOPMOST);

	if (ret == IDYES)
	{
		char sPath[MAX_PATH] = "";
		char sSys[MAX_PATH] = "";
		std::string strExe = "\\RemoteCtrl.exe ";
		GetCurrentDirectoryA(MAX_PATH, sPath);
		
		GetSystemDirectoryA(sSys, sizeof sSys);
		std::cout << sPath << ' ' << sSys << std::endl;
		std::string strCmd = "mklink " + std::string(sSys)
			+ strExe + std::string(sPath) + strExe;
		std::cout << strCmd << std::endl;
		system(strCmd.c_str());
		HKEY hKey = NULL;
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubkey, 0, KEY_WRITE|KEY_WOW64_64KEY, &hKey);
		if (ret != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败！"), 
				_T("错误"),
				MB_ICONERROR|MB_TOPMOST
			);
			exit(0);
		}

		
		

		ret = RegSetValueEx(
			hKey,
			_T("RemoteCtrl"), 
			0, 
			REG_EXPAND_SZ, 
			(BYTE*)(LPCTSTR)strPath,
			strPath.GetLength() * sizeof(TCHAR)
		);

		if (ret != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败！"),
				_T("错误"),
				MB_ICONERROR | MB_TOPMOST
			);
			exit(0);
		}
		RegCloseKey(hKey);
	}
	else if (ret == IDCANCEL)
	{
		exit(0);
	}
	return;
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
			CCommand cmd;
			ChooseAutoInvoke();
			int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);

			switch (ret)
			{
			case -1:
				MessageBox(NULL, _T("网络初始化异常！"),
					_T("网络初始化失败！"),
					MB_OK | MB_ICONERROR);
				exit(0);
				break;
			case 2:
				MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), 
					_T("接入用户失败！"),
					MB_OK | MB_ICONERROR);
				exit(0);
			default:
				break;
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
