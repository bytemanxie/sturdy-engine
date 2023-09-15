#pragma once
#include <list>
#include "pch.h"
#include <mutex>

template<class T>
class CEdoyunQueue
{//线程安全的队列(利用IOCP实现)
public:
	enum {
		EQNone,
		EQPush,
		EQPop,
		EQSize,
		EQClear
	};

	typedef struct IocpParam {
		size_t nOperator;//操作
		T Data;//数据

		HANDLE hEvent;//pop操作需要的
		IocpParam(int op, const T& data, HANDLE hEve = NULL)
		{
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IocpParam()
		{
			nOperator = EQNone;
		}
	}PPARAM;//Post Parameter 用于投递信息的结构体

public:
	CEdoyunQueue() {
		m_lock = false;
		m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompletionPort != NULL)
		{
			m_hThread = (HANDLE)_beginthread(
				&CEdoyunQueue<T>::threadEntry,
				0, m_hCompletionPort);
		}
	}

	~CEdoyunQueue() {
		m_lock = true;
		HANDLE hTemp = m_hCompletionPort;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);
		m_hCompletionPort = NULL;
		CloseHandle(hTemp);
	}

	bool PushBack(const T& data) {
		
		if (m_lock == true) return false;
		IocpParam* pParam = new IocpParam(EQPush, data)
		bool ret = PostQueuedCompletionStatus(
			m_hCompletionPort, 
			sizeof PPARAM, 
			(ULONG_PTR)pParam,
			NULL
			);
		if (ret == false) delete pParam;
		return ret;
	}

	bool PopFront(T& data) {
		if (m_lock == true) return false;
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam pParam(EQPop, data, hEvent);
		if (m_lock) {
			if (hEvent) CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(
				m_hCompletionPort,
				sizeof PPARAM,
				(ULONG_PTR)&pParam,
				NULL
			);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret)
		{
			data = pParam.Data;
		}
		return ret;
	}

	size_t Size() {
		
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(EQSize, T(), hEvent);

		if (m_lock) {
			if (hEvent) CloseHandle(hEvent);
			return -1;
		}

		bool ret = PostQueuedCompletionStatus(
			m_hCompletionPort,
			sizeof PPARAM,
			(ULONG_PTR)&Param,
			NULL
		);
		if (ret == false) {
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret)
		{
			return Param.nOperator;
		}
		return -1
	}
	void clear()
	{
		if (m_lock == true) return false;
		IocpParam* pParam = new IocpParam(EQClear, T())
			bool ret = PostQueuedCompletionStatus(
				m_hCompletionPort,
				sizeof PPARAM,
				(ULONG_PTR)pParam,
				NULL
			);
		if (ret == false) delete pParam;
		return ret;
	}
	
private:
	static void threadEntry(void* arg) {
		CEdoyunQueue<T>* thiz = (CEdoyunQueue<T>*) arg;
		thiz->threadMain();
		_endthread();
	}

	void threadMain()
	{
		DWORD dwTransferred = 0;
		PPARAM* pParam = NULL:
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlapped = NULL;
		while (GetQueuedCompletionStatus(
			m_hCompletionPort, &dwTransferred, 
			&CompletionKey, &pOverlapped, INFINITE))
		{
			if (dwTransferred == 0 && (CompletionKey == NULL))
			{
				printf("thread is prepare to exit!\r\n");
				break;
			}

			pParam = (PPARAM*)CompletionKey;
			switch (pParam->nOperator)
			{
			case EQPush:
			{
				m_lstData.push_back(pParam->strData);
				delete pParam;
			}
			break;
			case EQPop:
			{
				std::string str;
				if (lstString.size() > 0)
				{
					str = lstString.front();
					lstString.pop_front();
				}
				if (pParam->hEvent != NULL)
				{
					SetEvent(pParam->hEvent);
				}
			}
			break;
			case EQSize:
			{
				pParam->nOperator = m_lstData.size();
				if (pParam->hEvent != NULL)
				{
					SetEvent(pParam->hEvent);
				}
			}
			break;
			case EQClear:
			{
				m_lstData.clear();
				delete pParam;
			}
			default:
				OutputDebugString("unknown operator!\r\n");
				break;
			}
		}
		CloseHandle(m_hCompletionPort);
	}
	
private:
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_lock;

};

