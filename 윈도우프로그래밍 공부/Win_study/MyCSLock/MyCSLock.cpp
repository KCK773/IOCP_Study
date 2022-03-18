#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include "Windows.h"
#include "iostream"
using namespace std;


//임계구역을 설정하기 위해 크리티컬 섹션을 초기화하고 , 삭제하고 , Enter,Leave 함수를 호출하는 과정을 클래스로 미리 구현해도 ok
//더 직관적이고 사용하기 편해다
class MyLock
{
	CRITICAL_SECTION m_cs;

public:
	MyLock()
	{
		InitializeCriticalSection(&m_cs);
	}
	~MyLock()
	{
		DeleteCriticalSection(&m_cs);
	}

	void Lock()
	{
		EnterCriticalSection(&m_cs);
	}
	void Unlock()
	{
		LeaveCriticalSection(&m_cs);
	}
};

//좀더 다르게 구상한 클래스 이다.
//크리티컬 섹션 자체는 별도로 정의하고, 락을 걸 시점에서 별도의 크리티컬 섹션 포인터를 매개변수로 념겨 락을 걸되,
//함수의 스택 작용을 이용해 명시적인 Enter/Leave 의 호출 없이 클래스의 생성자와 소멸자에서 Enter/Leave 쌍이 저절로 호출되도록 처리
class MyAutoLock
{
	PCRITICAL_SECTION m_pcs;

protected:
	MyAutoLock() { m_pcs = NULL; }
	//매개변수가 없는 MyAutoLock의 변수 선언을 막기위해 디폴드 생성자를 protected로 선언해서 디폴트 생성자 호출을 막아준다

public:
	static void Init(PCRITICAL_SECTION pcs)
	{
		InitializeCriticalSection(pcs);
	}
	static void Delete(PCRITICAL_SECTION pcs)
	{
		DeleteCriticalSection(pcs);
	}
	//편의를 위해 초기화와 삭제 함수를 정적 멤버 함수로 정의한 다음 그 속에서 각각 처리한다.

public:
	MyAutoLock(PCRITICAL_SECTION pcs)
	{
		EnterCriticalSection(pcs);
		m_pcs = pcs;
	}
	~MyAutoLock()
	{
		LeaveCriticalSection(m_pcs);
	}
	//생성자에서는 EnterCriticalSection, 소멸자에서는 LeaveCricalSection을 호출하도록 처리한다.
};


void PrintDateTime(PCRITICAL_SECTION pCS, PSYSTEMTIME pst)
{
	MyAutoLock lock(pCS);
	//오토락 클래스를 함수 정의 초기에 선언하게 되면 그 함수 전체는 임계구역으로 설정된다.
	cout << "..." << "SubThread " << GetCurrentThreadId() << " => ";
	cout << pst->wYear << '/' << pst->wMonth << '/' << pst->wDay << ' ';
	cout << pst->wHour << ':' << pst->wMinute << ':' << pst->wSecond << '+';
	cout << pst->wMilliseconds << endl;
}

DWORD WINAPI ThreadProc3(LPVOID pParam)
{
	PCRITICAL_SECTION pCS = (PCRITICAL_SECTION)pParam;

	SYSTEMTIME st;
	GetLocalTime(&st);

	//스택의 성질을 이용해 소멸자가 자동으로 호출되도록 하기 위해 일부러 브래킷 쌍으로 임계구역의 시작과 끝을 둘러싼다.
	{
		MyAutoLock lock(pCS);	//생성자를 이용한EnterCriticalSection 호출
		cout << "..." << "SubThread " << GetCurrentThreadId() << " => ";
		cout << st.wYear << '/' << st.wMonth << '/' << st.wDay << ' ';
		if (st.wMilliseconds > 500)
		{
			cout << endl;
			return 0;
		}
		cout << st.wHour << ':' << st.wMinute << ':' << st.wSecond << '+';
		cout << st.wMilliseconds << endl;
	}		//소멸자를 이용한 LeaveCriticalSection 호출

	return 0;
}

DWORD WINAPI ThreadProc2(LPVOID pParam)
{
	PCRITICAL_SECTION pCS = (PCRITICAL_SECTION)pParam;

	SYSTEMTIME st;
	GetLocalTime(&st);

	PrintDateTime(pCS, &st);

	return 0;
}

DWORD WINAPI ThreadProc1(LPVOID pParam)
{
	MyLock* pLock = (MyLock*)pParam;

	SYSTEMTIME st;
	GetLocalTime(&st);

	pLock->Lock();
	{
		cout << "..." << "SubThread " << GetCurrentThreadId() << " => ";
		cout << st.wYear << '/' << st.wMonth << '/' << st.wDay << ' ';
		cout << st.wHour << ':' << st.wMinute << ':' << st.wSecond << '+';
		cout << st.wMilliseconds << endl;
	}
	pLock->Unlock();

	return 0;
}

void _tmain(void)
{
	cout << "======= Start MyCSLock Test ========" << endl;
	HANDLE arhThreads[10];

	MyLock lock;
	for (int i = 0; i < 10; i++)
	{
		DWORD dwThrID = 0;
		arhThreads[i] = CreateThread(NULL, 0, ThreadProc1, &lock, 0, &dwThrID);
	}
	WaitForMultipleObjects(10, arhThreads, TRUE, INFINITE);

	int stop;
	cin >> stop;

	CRITICAL_SECTION cs;
	MyAutoLock::Init(&cs);	//크리티컬 섹션 초기화
	for (int i = 0; i < 10; i++)
	{
		DWORD dwThrID = 0;
		arhThreads[i] = CreateThread(NULL, 0, ThreadProc3, &cs, 0, &dwThrID);
	}
	WaitForMultipleObjects(10, arhThreads, TRUE, INFINITE);
	MyAutoLock::Delete(&cs);	//크리티컬 섹션 삭제

	cout << "======= End MyCSLock Test ==========" << endl;
}

