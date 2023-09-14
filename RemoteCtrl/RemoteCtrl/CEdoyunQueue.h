#pragma once
#include <list>
template<class T> 
class CEdoyunQueue
{//�̰߳�ȫ�Ķ���(����IOCPʵ��)
public:
	CEdoyunQueue();
	~CEdoyunQueue();
	bool PushBack(const T& data);
	bool PopFront(T& data);
	size_t Size();
	void clear();
private:
	static void threadEntry(void* arg);
	void threadMain();
private:
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
public:
	typedef struct IocpParam {
		int nOperator;//����
		T strData;//����
		
		HANDLE hEvent;//pop������Ҫ��
		IocpParam(int op, const char* sData, _beginthreadex_proc_type cb = NULL)
		{
			nOperator = op;
			strData = sData;
		}
		IocpParam()
		{
			nOperator = -1;
		}
	}PPARAM;//Post Parameter ����Ͷ����Ϣ�Ľṹ��

	enum {
		EQPush,
		EQPop,
		EQSize, 
		EQClear
	};
};

