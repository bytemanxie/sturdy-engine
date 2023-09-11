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



int main()
{
	if (CEdoyunTool::isAdmin())
	{
		if (!CEdoyunTool::Init()) return 1;
		printf("current is run as administrator!\r\n");
		MessageBox(NULL, _T("管理员"), _T("用户状态"), 0);

		//TODO: code your application's behavior here.
		CCommand cmd;
		if (!ChooseAutoInvoke(INVOKE_PATH))
		{
			::exit(0);
		}
		int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);

		switch (ret)
		{
		case -1:
			MessageBox(NULL, _T("网络初始化异常！"),
				_T("网络初始化失败！"),
				MB_OK | MB_ICONERROR);
			break;
		case 2:
			MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"),
				_T("接入用户失败！"),
				MB_OK | MB_ICONERROR);
		default:
			break;
		}
	}
	else
	{
		printf("current is run as normal user!\r\n");

		if (CEdoyunTool::RunAsAdmin() == false)
		{
			CEdoyunTool::ShowError();
		}
		return 0;
	}

	return 0;
}
