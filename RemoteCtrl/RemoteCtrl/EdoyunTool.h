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
		//��ȡ����ԱȨ�ޣ� ʹ�ø�Ȩ�޴����߳�
		//���ز����� ����administrator�˻�����ֹ������ֻ�ܵ�¼���ؿ���̨
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
			MessageBox(NULL, _T("��������ʧ�ܣ�"), _T("�������"), 0);
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
	{//ͨ���޸Ŀ��������ļ�����ʵ�ֿ�������
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);

		return CopyFile(sPath, strPath, FALSE);
	}

	static bool WriteRegisterTable(const CString& strPath)
	{//ͨ���޸�ע�����ʵ�ֿ�������
		CString strSubkey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR sPath[MAX_PATH] = _T("");
		char sSys[MAX_PATH] = "";
		std::string strExe = "\\RemoteCtrl.exe ";

		GetModuleFileName(NULL, sPath, MAX_PATH);

		BOOL ret = CopyFile(sPath, strPath, FALSE);
		if (ret == FALSE)
		{
			MessageBox(NULL, _T("�����ļ�ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}

		HKEY hKey = NULL;
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubkey, 0, KEY_WRITE | KEY_WOW64_64KEY, &hKey);
		if (ret != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n��������ʧ�ܣ�"),
				_T("����"),
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
			MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n��������ʧ�ܣ�"),
				_T("����"),
				MB_ICONERROR | MB_TOPMOST
			);
			return false;
		}
		RegCloseKey(hKey);
		return true;
	}

	static bool Init()
	{//����MFCӦ�õĳ�ʼ��
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

