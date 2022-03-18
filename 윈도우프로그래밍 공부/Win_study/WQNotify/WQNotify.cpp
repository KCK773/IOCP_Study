#include <stdio.h>
#include <tchar.h>
#include <SDKDDKVer.h>
#include "Windows.h"
#include <iostream>
#include "list"
using namespace std;

#pragma warning(disable:4996) // C4996 에러를 무시
//아니 왜 strcpy 부근에서 에러가 나는지를 모르겠네 거참 진짜
//strcoy_s가 없는건 또 무슨 현상이고
//정책이 바꼈나?
//뭐지 진짜?

//대기가능 큐를 이용한 스레드간 매개변수 전달
//이 방식은 이벤트를 이용한 방식보다 상당히 유용하다
// A > B > C 의 작업을 한다고 가정하고 각 과정을 스레드로 처리한다고 해보자.
//기존의 이벤트 방식을 사용하면 B가 상대적으로 느릴때 A는 B의 작업이 끝날때까지 기다려야 하는 순간이 발생하지만
//대기가능 큐를 부여하면 A의 결과를 큐에 추가하고 다음 작업을 수행할 수 있기 때문이다.

#define CMD_NONE	  0
#define CMD_STR		  1
#define CMD_POINT	  2
#define CMD_TIME	  3
#define CMD_EXIT	100

//대기가능 큐 클래스
class WAIT_QUE
{
	struct NOTI_ITEM
	{
		LONG	_cmd, _size;
		PBYTE	_data;

		//명령 식별과 데이터 길이를 위한 변수인 _data에 실제 데이터의 포인터를 담는다.
		NOTI_ITEM()
		{
			_cmd = _size = 0, _data = NULL;
		}
		NOTI_ITEM(LONG cmd, LONG size, PBYTE data)
		{
			_cmd = cmd, _size = size, _data = data;
		}
	};
	typedef std::list<NOTI_ITEM> ITEM_QUE;
	//큐에 추가될 항목에 대한 구조체에와 큐를 정의한다.

	HANDLE	 m_hMutx;	// 항목 엔큐/데큐 시 동기화를 위한 뮤텍스
	HANDLE	 m_hSema;	// 큐 관리를 위한 세마포어
	ITEM_QUE m_queue;	// STL 리스트를 이용한 큐

public:
	WAIT_QUE()
	{
		m_hMutx = m_hSema = NULL;
	}
	~WAIT_QUE()
	{
		if (m_hMutx != NULL) CloseHandle(m_hMutx);
		if (m_hSema != NULL) CloseHandle(m_hSema);
	}

public:
	//Init 함수에서 뮤텍스와 세마포어를 생성한다.
	void Init()
	{
		m_hSema = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
		m_hMutx = CreateMutex(NULL, FALSE, NULL);
	}

	//큐에 항목을 추가하는 엔큐 멤버 함수를 정의한다.
	void Enqueue(LONG cmd, LONG size = 0, PBYTE data = NULL)
	{
		NOTI_ITEM ni(cmd, size, data);

		WaitForSingleObject(m_hMutx, INFINITE);
		m_queue.push_back(ni);
		ReleaseMutex(m_hMutx);

		ReleaseSemaphore(m_hSema, 1, NULL);
	}

	//큐로부터 항목을 추출하는 데큐 멤버 함수를 정의한다.
	PBYTE Dequeue(LONG& cmd, LONG& size)
	{
		DWORD dwWaitCode = WaitForSingleObject(m_hSema, INFINITE);
		if (dwWaitCode == WAIT_FAILED)
		{
			cmd = CMD_NONE, size = HRESULT_FROM_WIN32(GetLastError());
			return NULL;
		}

		NOTI_ITEM ni;
		WaitForSingleObject(m_hMutx, INFINITE);
		ITEM_QUE::iterator it = m_queue.begin();
		ni = *it;
		m_queue.pop_front();
		ReleaseMutex(m_hMutx);

		cmd = ni._cmd, size = ni._size;
		return ni._data;
	}
};


DWORD WINAPI WorkerProc(LPVOID pParam)
{
	WAIT_QUE* pwq = (WAIT_QUE*)pParam;
	DWORD dwThrId = GetCurrentThreadId();

	while (true)
	{
		LONG lCmd, lSize;
		PBYTE pData = pwq->Dequeue(lCmd, lSize);
		//WaitForSingleObject를 통해 쓰기 통지 이벤트에 대해 대기 상태에서 풀려나 공유 버퍼를 읽은 후
		//SetEvent를 통해 읽기 통지 이벤트를 시그널링 해주는 과정을 하나의 데큐 호출로 대신한다

		if (lSize < 0)
		{
			cout << " ~~~ WaitForSingleObject failed : " << GetLastError() << endl;
			break;
		}

		if (lCmd == CMD_EXIT)
			break;

		switch (lCmd)
		{
		case CMD_STR:
		{
			pData[lSize] = 0;
			printf("  <== R-TH %d read STR : %s\n", dwThrId, pData);
		}
		break;

		case CMD_POINT:
		{
			PPOINT ppt = (PPOINT)pData;
			printf("  <== R-TH %d read POINT : (%d, %d)\n", dwThrId, ppt->x, ppt->y);
		}
		break;

		case CMD_TIME:
		{
			PSYSTEMTIME pst = (PSYSTEMTIME)pData;
			printf("  <== R-TH %d read TIME : %04d-%02d-%02d %02d:%02d:%02d+%03d\n",
				dwThrId, pst->wYear, pst->wMonth, pst->wDay, pst->wHour,
				pst->wMinute, pst->wSecond, pst->wMilliseconds);
		}
		break;
		}
		delete[] pData;
	}
	cout << " *** WorkerProc Thread exits..." << endl;

	return 0;
}

void _tmain()
{
	cout << "======= Start WQueNotify Test ========" << endl;

	WAIT_QUE wq;
	wq.Init();
	//WAIT_ENV 구조체 대신에 대기가능 큐 클래스를 선언해 사용한다.

	DWORD  dwThrID;
	HANDLE hThread = CreateThread(NULL, 0, WorkerProc, &wq, 0, &dwThrID);

	char szIn[512];
	while (true)
	{
		cin >> szIn;
		if (_stricmp(szIn, "quit") == 0)
			break;

		LONG lCmd = CMD_NONE, lSize = 0;
		PBYTE pData = NULL;

		if (_stricmp(szIn, "time") == 0)
		{
			lSize = sizeof(SYSTEMTIME), lCmd = CMD_TIME;
			pData = new BYTE[lSize];

			SYSTEMTIME st;
			GetLocalTime(&st);
			memcpy(pData, &st, lSize);
		}
		else if (_stricmp(szIn, "point") == 0)
		{
			lSize = sizeof(POINT), lCmd = CMD_POINT;
			pData = new BYTE[lSize];

			POINT pt;
			pt.x = rand() % 1000; pt.y = rand() % 1000;
			*((PPOINT)pData) = pt;
		}
		else
		{
			lSize = strlen(szIn), lCmd = CMD_STR;
			pData = new BYTE[lSize + 1];
			strcpy((char*)pData, szIn);
		}

		wq.Enqueue(lCmd, lSize, pData);
		//콘솔에서 데이터를 입력받은 명령과 크기, 그리고 데이터 포인터를 엔큐 함수를 통해 큐에 추가한다.
	}

	wq.Enqueue(CMD_EXIT);
	//종료 처리를 위해 종료 명령을 큐에 추가한다.
	WaitForSingleObject(hThread, INFINITE);

	cout << "======= End WQueNotify Test ==========" << endl;
}
