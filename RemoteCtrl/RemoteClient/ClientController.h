#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <map>
#include "resource.h"
#include "EdoyunTool.h"

#define WM_SEND_PACK (WM_USER + 1) // ���Ͱ�����
#define WM_SEND_DATA (WM_USER + 2) // ��������
#define WM_SHOW_STATUS (WM_USER + 3) // չʾ״̬
#define WM_SHOW_WATCH (WM_USER + 4) // Զ�̼��
#define WM_SEND_MESSAGE (WM_USER + 0x1000) // �Զ�����Ϣ����

class CClientController
{
public:
	//��ȡȫ��Ψһ����
	static CClientController* getInstance();
	//��ʼ������
	int InitControlller();
	//����
	int Invoke(CWnd*& pMainWnd);
	//������Ϣ
	LRESULT SendMessage(MSG msg);
	//���������������ַ
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
		return true;
	}

	//1 �鿴���̷��� 2 �鿴ָ��Ŀ¼�µ��ļ� 3 ���ļ�
	//4 �����ļ� 5 ������ 6 ������Ļ����
	//7 ���� 8 ���� 9 ɾ���ļ�
	//1981 ��������
	//�ɹ���������ţ� �ڲ�������dealcommand
	//ʧ�ܷ���-1��nLengthΪ�������ݵĳ���
	int SendCommandPacket(int nCmd, bool bAutoClose = true,
		BYTE* pData = NULL, size_t nLength = 0);


	//���ֽ���ת��Ϊͼ��
	int GetImage(CImage& image)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		return CEdoyunTool::Bytes2Image(image, pClient->GetPacket().strData);
	}

	int DownFile(CString strPath);
	
	void StartWatchScreen();
protected:
	void threadWatchScreen();
	static void threadWatchScreen(void* arg);
	void threadDownloadFile();
	static void threadDownloadEntry(void* arg);
	CClientController():
		m_statusDlg(&m_remoteDlg),
		m_watchDlg(&m_remoteDlg)
	{
		m_isClosed = true;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_hThreadDownload = INVALID_HANDLE_VALUE;
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
	HANDLE m_hThreadDownload;
	HANDLE m_hThreadWatch;
	bool m_isFull;//�����Ƿ�������
	bool m_isClosed;//�����Ƿ�ر�

	//�����ļ���Զ��·��
	CString m_strRemote;
	//�����ļ��ı��ر���·��
	CString m_strLocal;

	static CClientController* m_instance;
	class CHelper
	{
	public:
		CHelper()
		{
			//CClientController::getInstance();
		}
		~CHelper()
		{
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};
