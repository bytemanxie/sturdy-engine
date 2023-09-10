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

void WriteRegisterTable(const CString& strPath)
{
	CString strSubkey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	char sPath[MAX_PATH] = "";
	char sSys[MAX_PATH] = "";
	std::string strExe = "\\RemoteCtrl.exe ";
	GetCurrentDirectoryA(MAX_PATH, sPath);

	GetSystemDirectoryA(sSys, sizeof sSys);
	std::cout << sPath << ' ' << sSys << std::endl;
	std::string strCmd = "mklink " + std::string(sSys)
		+ strExe + std::string(sPath) + strExe;
	std::cout << strCmd << std::endl;
	int ret = system(strCmd.c_str());

	HKEY hKey = NULL;
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubkey, 0, KEY_WRITE | KEY_WOW64_64KEY, &hKey);
	if (ret != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败！"),
			_T("错误"),
			MB_ICONERROR | MB_TOPMOST
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

void WriteStartupDir(const CString& strPath)
{
	CString strCmd = GetCommandLine();
	strCmd.Replace(_T("\""), _T(""));
	BOOL ret = CopyFile(strCmd, strPath, FALSE);
	if (ret == FALSE)
	{
		MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
		::exit(0);
	}

}

void ChooseAutoInvoke()
{
	TCHAR wcsSystem[MAX_PATH] = _T("");
	//GetSystemDirectory(wcsSystem, MAX_PATH);
	
	//CString strPath = CString(_T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe"));
	CString strPath = _T("C:\\Users\\edoyun\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe");
	std::cout << 1 << strPath.GetString() << std::endl;
	if (PathFileExists(strPath))
	{
		std::cout << 2 << strPath << std::endl;
		return; 
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
		WriteStartupDir(strPath);
	}
	else if (ret == IDCANCEL)
	{
		exit(0);
	}
	return;
}

void ShowError()
{
	LPWSTR lpMessageBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		lpMessageBuf, 0, NULL
	);
	OutputDebugString(lpMessageBuf);
	LocalFree(lpMessageBuf);
	::exit(0);
}

bool isAdmin()
{
	HANDLE hToken = NULL;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		ShowError();
		return false;
	}
	TOKEN_ELEVATION eve;
	DWORD len = 0;
	if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof eve, &len) == FALSE)
	{
		ShowError();
		return false;
	}

	CloseHandle(hToken);
	if (len == sizeof eve)
	{
		return eve.TokenIsElevated;
	}

	printf("length of token information is %d\r\n", len);
	return false;
}

void RunAsAdmin()
{
	HANDLE hToken = NULL;
	BOOL ret = LogonUser(L"Administrator", NULL, NULL, LOGON32_LOGON_BATCH,
		LOGON32_PROVIDER_DEFAULT, &hToken);
	if (!ret)
	{
		ShowError();
		MessageBox(NULL, _T("登陆错误！"), _T("程序错误"), 0);
		::exit(0);
	}
	OutputDebugString(L"Logon administrator success!\r\n");
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	TCHAR sPath[MAX_PATH] = _T("");
	GetCurrentDirectory(MAX_PATH, sPath);
	CString strCmd = sPath;
	strCmd += _T("\\RemoteCtrl.exe");

	/*ret = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL, 
		(LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);*/
	ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE,
		NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
	CloseHandle(hToken);

	if (!ret)
	{
		ShowError();
		MessageBox(NULL, _T("创建进程失败！"), _T("程序错误！"), 0);
		::exit(0);
	}

	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
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
			if (isAdmin())
			{
				printf("current is run as administrator!\r\n");
				MessageBox(NULL, _T("管理员"), _T("用户状态"), 0);

			}
			else
			{
				printf("current is run as normal user!\r\n");
				
				RunAsAdmin();
				MessageBox(NULL, _T("管理员"), _T("用户状态"), 0);
				return nRetCode;
			}
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
