#pragma once
class CEdoyunTool
{
public:
	static void Dump(BYTE* pData, size_t nSize)
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

	static bool isAdmin()
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

	static bool RunAsAdmin()
	{
		//获取管理员权限， 使用该权限创建线程
		//本地策略组 开启administrator账户，禁止空密码只能登录本地控制台
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		TCHAR sPath[MAX_PATH] = _T("");
		//GetCurrentDirectory(MAX_PATH, sPath);
		GetModuleFileName(NULL, sPath, MAX_PATH);

		CString strCmd = sPath;
		strCmd += _T("\\RemoteCtrl.exe");

		/*ret = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL,
			(LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);*/
		BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL,
			LOGON_WITH_PROFILE, NULL,
			(LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT,
			NULL, NULL, &si, &pi);

		if (!ret)
		{
			ShowError();
			MessageBox(NULL, _T("创建进程失败！"), _T("程序错误！"), 0);
			return false;
		}

		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	static void ShowError()
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

	static BOOL WriteStartupDir(const CString& strPath)
	{//通过修改开机启动文件夹来实现开机启动
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);

		return CopyFile(sPath, strPath, FALSE);
	}

	static bool WriteRegisterTable(const CString& strPath)
	{//通过修改注册表来实现开机启动
		CString strSubkey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR sPath[MAX_PATH] = _T("");
		char sSys[MAX_PATH] = "";
		std::string strExe = "\\RemoteCtrl.exe ";

		GetModuleFileName(NULL, sPath, MAX_PATH);

		BOOL ret = CopyFile(sPath, strPath, FALSE);
		if (ret == FALSE)
		{
			MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}

		HKEY hKey = NULL;
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubkey, 0, KEY_WRITE | KEY_WOW64_64KEY, &hKey);
		if (ret != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败！"),
				_T("错误"),
				MB_ICONERROR | MB_TOPMOST
			);
			return false;
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
			return false;
		}
		RegCloseKey(hKey);
		return true;
	}

	static bool Init()
	{//所有MFC应用的初始化
		HMODULE hModule = ::GetModuleHandle(nullptr);
		if (hModule == nullptr)
		{
			wprintf(L"Fatal Error: GetModuleHandle failed\n");
			return false;
		}

		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: code your application's behavior here.
			wprintf(L"Fatal Error: MFC initialization failed\n");
			return false;
		}
	}

};

