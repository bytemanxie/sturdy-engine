// RemoteCtrl.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

void Dump(BYTE* pData, size_t nSize)
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

int MakeDriverInfo() {//1==>A 2==>B 3==>C ... 26==>Z
	std::string result;
	for (int i = 1; i <= 26; i++) {
		if (_chdrive(i) == 0) {
			if (result.size() > 0)
				result += ',';
			result += 'A' + i - 1;
		}
	}
    CPacket pack(1, (BYTE*)result.c_str(), result.size());//将result打包
    Dump((BYTE*)pack.Data(), pack.Size());
    //CServerSocket::getInstance()->Send(pack);
	return 0;
}

#include <stdio.h>
#include <io.h>
#include <list>

typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//是否有效
	BOOL IsDirectory;//是否为目录 0 否 1 是
	BOOL HasNext;//是否还有后续 0 没有 1 有
	char szFileName[256];//文件名
}FILEINFO, * PFILEINFO;

int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	std::string strPath = inPacket.strData;
	//std::list<FILEINFO> lstFileInfos;
	if (_chdir(strPath.c_str()) != 0) {
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		OutputDebugString(_T("没有权限访问目录！！"));
		return -2;
	}
	_finddata_t fdata;
	int hfind = 0;
	if ((hfind = _findfirst("*", &fdata)) == -1) {
		OutputDebugString(_T("没有找到任何文件！！"));
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		return -3;
	}
	int count = 0;
	do {
		FILEINFO finfo;
		finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
		memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
		TRACE("%s\r\n", finfo.szFileName);
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		count++;
	} while (!_findnext(hfind, &fdata));
	TRACE("server: count = %d\r\n", count);
	//发送信息到控制端
	FILEINFO finfo;
	finfo.HasNext = FALSE;
	lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
	return 0;
}

int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	std::string strPath = inPacket.strData;
	long long data = 0;
	FILE* pFile = NULL;
	errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
	if (err != 0) {
		lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
		return -1;
	}
	if (pFile != NULL) {
		fseek(pFile, 0, SEEK_END);
		data = _ftelli64(pFile);
		lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
		fseek(pFile, 0, SEEK_SET);
		char buffer[1024] = "";
		size_t rlen = 0;
		do {
			rlen = fread(buffer, 1, 1024, pFile);
			lstPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
		} while (rlen >= 1024);
		fclose(pFile);
	}
	else {
		lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
	}
	return 0;
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
            // TODO: code your application's behavior here.
            //CServerSocket* pserver = CServerSocket::getInstance();
            //int count = 0;
            //if (pserver->InitSocket() == false)
            //{
            //    MessageBox(NULL, _T("网络初始化异常！"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
            //    exit(0);
            //}
            //while (CServerSocket::getInstance())
            //{
            //    if (pserver->AcceptClient() == false)
            //    {
            //        if (count >= 3)
            //        {
            //            MessageBox(NULL, _T("多次无法接入用户， 结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
            //            exit(0);
            //        }
            //        MessageBox(NULL, _T("无法接入用户， 自动重试！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
            //        count++;
            //    }
            //    int ret = pserver->DealCommand();
            //    //TODO
            //}
            int ncmd = 1;
            switch (ncmd)
            {
            case 1:
                MakeDriverInfo();
                break;
			case 2:
				MakeDirectoryInfo();
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
