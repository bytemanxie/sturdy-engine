#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <map>
#include "resource.h"
#include "EdoyunTool.h"

#define WM_SEND_PACK (WM_USER + 1) // 发送包数据
#define WM_SEND_DATA (WM_USER + 2) // 发送数据
#define WM_SHOW_STATUS (WM_USER + 3) // 展示状态
#define WM_SHOW_WATCH (WM_USER + 4) // 远程监控
#define WM_SEND_MESSAGE (WM_USER + 0x1000) // 自定义消息处理

class CClientController
{
public:
	//获取全局唯一对象
	static CClientController* getInstance();
	//初始化操作
	int InitControlller();
	//启动
	int Invoke(CWnd*& pMainWnd);
	//发送消息
	LRESULT SendMessage(MSG msg);
	//更新网络服务器地址
	void UpdateAddress(int nIP, int nPort)
	{
		CClientSocket::getInstance()->UpdataAddress(nIP, nPort);
	}
	int DealCommand()
	{
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket()
	{
		CClientSocket::getInstance()->CloseSocket();
	}

	bool SendPacket(const CPacket& pack)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		if (pClient->InitSocket() == false) return false;
		pClient->Send(pack);
	}

	//1 查看磁盘分区 2 查看指定目录下的文件 3 打开文件
	//4 下载文件 5 鼠标操作 6 发送屏幕内容
	//7 锁机 8 解锁 9 删除文件
	//1981 测试连接
	//成功返回命令号， 内部调用了dealcommand
	//失败返回-1，nLength为发送数据的长度
	int SendCommandPacket(int nCmd, bool bAutoClose = true, 
		BYTE* pData = NULL, size_t nLength = 0)
	{
		SendPacket(CPacket(nCmd, pData, nLength));
		int cmd = DealCommand();
		TRACE("ack: %d\r\n", cmd);
		if (bAutoClose)
			CloseSocket();
		return cmd;
	}

	int GetImage(CImage& image)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		return CEdoyunTool::Bytes2Image(image, pClient->GetPacket().strData);
		
	}

protected:
	CClientController():
		m_statusDlg(&m_remoteDlg),
		m_watchDlg(&m_remoteDlg)
	{
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
	}
	~CClientController()
	{
		WaitForSingleObject(m_hThread, 100);
	}
	void threadFunc();
	static unsigned __stdcall threadEntry(void* arg);
	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			delete m_instance;
			m_instance = NULL;
		}
	}
	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);
private:
	typedef struct MsgInfo {
		MSG msg;
		LRESULT result;
		MsgInfo(MSG m)
		{
			result = 0;
			memcpy(&msg, &m, sizeof msg);
		}
		
		MsgInfo(const MsgInfo& m)
		{
			result = m.result;
			memcpy(&msg, &m.msg, sizeof MSG);
		}
		MsgInfo& operator= (const MsgInfo& m)
		{
			if (this != &m)
			{
				result = m.result;
				memcpy(&msg, &m.msg, sizeof MSG);
			}
			return *this;
		}
	}MSGINFO;

	typedef LRESULT(CClientController::* MSGFUNC) (UINT nMsg, WPARAM wParam, LPARAM lParam);
	static std::map<UINT, MSGFUNC> m_mapFunc;
	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	unsigned m_nThreadID;

	static CClientController* m_instance;
	class CHelper
	{
	public:
		CHelper()
		{
			CClientController::getInstance();
		}
		~CHelper()
		{
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

