#pragma once
#include <atomic>
#include <vector>
#include <mutex>
#include <Windows.h>
#include "pch.h"

class ThreadFuncBase {};
typedef int (ThreadFuncBase::* FUNCTYPE)();

class ThreadWorker {
public:
	ThreadWorker() : thiz(NULL), func(NULL) {}
	ThreadWorker(ThreadFuncBase* obj, FUNCTYPE f) :thiz(obj), func(f) {}

	ThreadWorker(const ThreadWorker& worker)
	{
		thiz = worker.thiz;
		func = worker.func;
	}

	ThreadWorker& operator= (const ThreadWorker& worker)
	{
		if (this != &worker)
		{
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}
	
	int operator()() {
		if (IsValid())
		{
			return (thiz->*func)();
		}
		return -1;
	}

	bool IsValid() const
	{
		return (thiz != NULL) && (func != NULL);
	}

private:
	ThreadFuncBase* thiz;
	int (ThreadFuncBase::* func)();
};

class EdoyunThread
{
public:
	EdoyunThread()
	{
		m_hThread = (HANDLE)_beginthread(&EdoyunThread::ThreadEntry, 0, this);
	}

	~EdoyunThread()
	{
		Stop();
	}

	bool Start()
	{
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&EdoyunThread::ThreadEntry, 0, this);
		if (!IsValid())
		{
			m_bStatus = false;
		}
		return m_bStatus;
	}

	bool IsValid()//返回true表示有效，返回false表示线程异常或终止
	{
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE)) return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}

	bool Stop()
	{
		if (m_bStatus == false) return true;
		m_bStatus = false;

		bool ret = WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;
		UpdateWorker();

		if (m_worker.load() != NULL)
		{
			::ThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		return ret;
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker())
	{
		if (worker.IsValid())
		{
			m_worker.store(NULL);
			return;
		}

		if (m_worker.load() != NULL)
		{
			::ThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		m_worker.store(new ::ThreadWorker(worker));
	}

	//true表示空闲， false表示已分配了工作
	bool IsIdle()
	{
		return !m_worker.load()->IsValid();
	}

protected:
	//返回值小于0 则终止线程循环 大于0 则警告日志 等于0表示正常
	//virtual int each_step() = 0;

private:

	void ThreadWorker()
	{
		while (m_bStatus)
		{
			::ThreadWorker worker = *m_worker.load();
			if (worker.IsValid())
			{
				int ret = worker();
				if (ret != 0)
				{
					CString str;
					str.Format(_T("thread found warning code %d\r\n"), ret);
					OutputDebugString(str);
				}
				else if (ret < 0)
				{
					m_worker.store(NULL);
				}
			}
			else {
				Sleep(1);
			}
		}
	}

	static void ThreadEntry(void* arg)
	{
		EdoyunThread* thiz = (EdoyunThread*)arg;
		if (thiz)
		{
			thiz->ThreadWorker();
		}
		_endthread();
	}

private:
	HANDLE m_hThread;
	bool m_bStatus;//false 表示线程将要关闭 true 表示线程正在运行
	std::atomic<::ThreadWorker*> m_worker;
};

class EdoyunThreadPool
{
public:
	EdoyunThreadPool(size_t size)
	{
		m_threads.resize(size);
		for (size_t i = 0; i < size; ++i)
		{
			m_threads[i] = new EdoyunThread();
		}
	}

	EdoyunThreadPool();
	~EdoyunThreadPool()
	{
		Stop();
		m_threads.clear();
	}

	bool Invoke()
	{
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); ++i)
		{
			if (m_threads[i]->Start() == false)
			{
				ret = false;
				break;
			}

			if (ret == false)
			{
				for (size_t i = 0; i < m_threads.size(); ++i)
				{
					m_threads[i]->Stop();
				}
			}
		}
		return ret;
	}

	void Stop()
	{
		for (size_t i = 0; i < m_threads.size(); ++i)
		{
			m_threads[i]->Stop();
		}
	}

	//返回-1 表示分配失败，所有线程都在忙， 返回n表示第n个线程分配来做这个事情
	int DispatchWorker(const ThreadWorker& worker)
	{
		int index = -1;
		m_lock.lock();
		for (size_t i = 0; i < m_threads.size(); ++i)
		{
			if (m_threads[i]->IsIdle())
			{
				m_threads[i]->UpdateWorker(worker);
				break;
			}
		}
		m_lock.unlock();
		return index;
	}

	bool CheckThreadValid(size_t index)
	{
		if (index < m_threads.size())
		{
			return m_threads[index]->IsValid();
		}
		return false;
	}

private:
	std::mutex m_lock;
	std::vector<EdoyunThread*> m_threads;
};