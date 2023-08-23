#pragma once
#include <map>
#include "ServerSocket.h"
#include <direct.h>
#include <atlimage.h>
#include "EdoyunTool.h"
#include <stdio.h>
#include <io.h>
#include <list>
#include "pch.h"
#include "framework.h"
#include "Resource.h"
#include "Packet.h"

#include "LockInfoDialog.h"

class CCommand
{
public:
	CCommand();
	~CCommand();
	int ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket);
	static void RunCommand(void* arg, int status, 
						std::list<CPacket>& lstPacket, CPacket& inPacket) 
	{
		CCommand* thiz = (CCommand*)arg;
		if (status > 0)
		{
			int ret = thiz->ExcuteCommand(status, lstPacket, inPacket);
			if (ret != 0)
			{
				TRACE("ִ������ʧ�ܣ�%d ret = %d\r\n", status, ret);
			}
		}
		else
		{
			MessageBox(NULL, _T("�޷������û��� �Զ����ԣ�"),
				_T("�����û�ʧ�ܣ�"), MB_OK | MB_ICONERROR);
		}
	}
protected:
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket& inPacket);//��Ա����ָ��
	std::map<int, CMDFUNC> m_mapFunction;//������ŵ����ܵ�ӳ��
	CLockInfoDialog dlg;
	unsigned int threadid;

protected:
	int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{//1==>A 2==>B 3==>C ... 26==>Z
	std::string result;
	for (int i = 1; i <= 26; i++) {
		if (_chdrive(i) == 0) {
			if (result.size() > 0)
				result += ',';
			result += 'A' + i - 1;
		}
	}
	lstPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
	return 0;
	}

	

	int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		//std::list<FILEINFO> lstFileInfos;
		
		if (_chdir(strPath.c_str()) != 0)
		{
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof finfo));
			OutputDebugString(_T("û��Ȩ�ޣ�����Ŀ¼����\n"));
			return -2;
		}

		_finddata_t fdata;
		int hfind = 0;
		if ((hfind = _findfirst("*", &fdata)) == -1)
		{
			OutputDebugString(_T("û���ҵ��κ��ļ�����\n"));
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof finfo));
			return -3;
		}

		int count = 0;
		do {
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));

			count++;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof finfo));
		} while (!_findnext(hfind, &fdata));

		TRACE("server count = %d\r\n", count);
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof finfo));
		return 0;
	}

	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData;
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);//���ļ�
		lstPacket.push_back(CPacket(3, NULL, 0));
		return 0;
	}

	//��һ���������ļ��ĳ���
	int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {

		std::string strPath = inPacket.strData;
		long long data = 0;
		FILE* pFile = NULL;
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
		if (err != 0)
		{
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			return -1;
		}
		if (pFile != NULL)
		{
			fseek(pFile, 0, SEEK_END);
			data = _ftelli64(pFile);
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));//����һ���ļ�����
			fseek(pFile, 0, SEEK_SET);

			char buffer[1024] = "";
			size_t rlen = 0;
			do {
				rlen = fread(buffer, 1, 1024, pFile);
				lstPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));//��1K��1K
			} while (rlen >= 1024);//����1024˵�������ļ�β
			fclose(pFile);
		}
		else
		{
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));//�����ٷ�һ����
		}
		return 0;
	}

	int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		MOUSEEV mouse;
		memcpy((void*)&mouse, inPacket.strData.c_str(), sizeof MOUSEEV);
		
		DWORD nFlags = 0;

		switch (mouse.nButton)
		{
		case 0://���
			nFlags = 1;
			break;
		case 1://�Ҽ�
			nFlags = 2;
			break;
		case 2://�м�
			nFlags = 4;
			break;
		case 4://û�а���
			nFlags = 8;
			break;
		}
		if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
		switch (mouse.nAction)
		{
		case 0://����
			nFlags |= 0x10;
			break;
		case 1://˫��
			nFlags |= 0x20;
			break;
		case 2://����
			nFlags |= 0x40;
			break;
		case 3://�ſ�
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
		case 0x21://�������˫��Ч��
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

		case 0x42://�Ҽ�����
			//printf("�Ҽ�����\r\n");

			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82://�Ҽ��ſ�
			//printf("�Ҽ��ſ�\r\n");

			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;

		case 0x24://�м�˫��
			//printf("�м�˫��\r\n");
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
		case 0x08://��������ƶ�
			//printf("Move:%d \r\n", nFlags);
			mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
			break;
		}
		lstPacket.push_back(CPacket(5, NULL, 0));
		
		
		return 0;
	}

	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket)
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
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);//��ָ������Ϊ��ͷ
			PBYTE pData = (PBYTE)GlobalLock(hMem);
			SIZE_T nSize = GlobalSize(hMem);

			
			lstPacket.push_back(CPacket(6, pData, nSize));
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
	static unsigned _stdcall threadLockDlg(void* arg)
	{
		CCommand* thiz = (CCommand*)arg;
		thiz->threadLockDlgMain();
		_endthreadex(0);
		return 0;
	}

	void threadLockDlgMain()
	{
		TRACE("%s %d %d\r\n", __FILE__, __LINE__, GetCurrentThreadId());
		dlg.Create(IDD_DIALOG_INFO, NULL);
		dlg.ShowWindow(SW_SHOW);
		//�ڱκ�̨����
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


		//�����ö�
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		//������깦��
		ShowCursor(false);
		//����������
		::ShowWindow(::FindWindow(_T("Shell_TrayWind"), NULL), SW_HIDE);
		//CRect rect;
		dlg.GetWindowRect(rect);
		//������귶Χ
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
				if (msg.wParam == 0x41 && msg.lParam == 0x1e0001)//��ESC�˳�
				{
					break;
				}
			}
		}
		ClipCursor(NULL);
		//�ָ����
		ShowCursor(false);
		//�ָ�������
		::ShowWindow(::FindWindow(_T("Shell_TrayWind"), NULL), SW_SHOW);
		dlg.DestroyWindow();
	}

	int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE))
		{
			//_beginthread(threadLockDlg, 0, NULL);
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
		}
		
		lstPacket.push_back(CPacket(7, NULL, 0));
		return 0;
	}

	int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		//dlg.SendMessage(WM_KEYDOWN, 0x41, 0x1e0001);
		//::SendMessage(dlg, WM_KEYDOWN, 0x41, 0x1e0001);
		PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0x1e0001);
		lstPacket.push_back(CPacket(8, NULL, 0));
		return 0;
	}

	int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		lstPacket.push_back(CPacket(1981, NULL, 0));
		return 0;
	}

	int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{

		std::string strPath = inPacket.strData;
		TCHAR sPath[MAX_PATH] = _T("");
		mbstowcs(sPath, strPath.c_str(), strPath.size());
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath, sizeof sPath /
			sizeof TCHAR);
		DeleteFileA(strPath.c_str());


		lstPacket.push_back(CPacket(9, NULL, 0));
		return 0;
	}
};

