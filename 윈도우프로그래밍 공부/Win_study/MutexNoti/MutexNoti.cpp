#include <stdio.h>
#include <tchar.h>
#include <SDKDDKVer.h>
#include "Windows.h"
#include <iostream>
//뮤텍스를 임계구역 설정을 통한 데이터 보호의 목적이 아닌 스레드 종료 통지의 목적으로 사용하는 예
//데이터의 보호와 흐름 제어의 구분은 절대적인게 아니다. 통지를 위해 뮤텍스를 사용할 수도 있다.

using namespace std;

DWORD WINAPI ThreadProc(LPVOID pParam)
{
    PHANDLE parWaits = (PHANDLE)pParam;
	DWORD	dwThrId = GetCurrentThreadId();
	while (true)
	{
		DWORD dwWaitCode = WaitForMultipleObjects(2, parWaits, FALSE, INFINITE); 
		//뮤텍스 2개에 대해 동시 대기한다.
		//첫번째 뮤텍스는 종료 통지가 목적이고, 두 번째 뮤텍스는 아래 시간 출력 코드의 동기화를 위해 사용된다.
		//FALSE를 넣었으므로 하나만(시간 출력 뮤텍스) 시그널 상태가 되도 리턴한다.
		if (dwWaitCode == WAIT_FAILED)
		{	//대기하는 동안 오류 발생
			cout  << " ~~~ WaitForSingleObject failed : " << GetLastError() << endl;
			break;
		}
		
		if (dwWaitCode == WAIT_OBJECT_0)
		{
			//메인에서 arWaits[0]에 대한 소유권을 포기한 상태이다.
			ReleaseMutex(parWaits[0]);
			break;
			//첫번째 뮤텍스가 시그널 상태가 되었고, 이는 종료 통지를 의미하므로 루프를 탈출한다.
		}
	
		SYSTEMTIME st;
		GetLocalTime(&st);
		cout << "..." << "SubThread " << dwThrId << " => ";
		cout << st.wYear << '/' << st.wMonth << '/' << st.wDay << ' ';
		cout << st.wHour << ':' << st.wMinute << ':' << st.wSecond << '+';
		cout << st.wMilliseconds << endl;
		ReleaseMutex(parWaits[1]);
		//다른 스레드에서 parWaits[1]을 사용할 수 있게 소유권을 포기한다.

		Sleep(1000);
	}
	printf(" ===> SubThread %d exits...\n", GetCurrentThreadId());
	return 0;
}

#define MAX_THR_CNT 4
void _tmain(void)
{
	cout << " =MutexNoti 테스트 시작= " << endl;
	HANDLE arWaits[2];
	arWaits[0] = CreateMutex(NULL, TRUE, NULL);
	//스레드 종료 통지용 뮤텍스를 생성하기 위해 메인 스레드가 우선 뮤텍스를 소유하도록 한다.
	arWaits[1] = CreateMutex(NULL, FALSE, NULL);
	//임계구역 설정을 위한 또 다른 뮤텍스를 생성한다.

	HANDLE arhThreads[MAX_THR_CNT];
	for (int i = 0; i < MAX_THR_CNT; i++)
	{
		DWORD dwTheaID = 0;
		arhThreads[i] = CreateThread(NULL, 0, ThreadProc, arWaits, 0, &dwTheaID);
	}

	getchar();
	ReleaseMutex(arWaits[0]);
	//키를 입력받으면 실행중인 스레드를 종료하기 위해서 소유한 뮤텍스를 해제한다.

	WaitForMultipleObjects(MAX_THR_CNT, arhThreads, TRUE, INFINITE);
	for (int i = 0; i < MAX_THR_CNT; i++)
		CloseHandle(arhThreads[i]);
	for (int i = 0; i < 2; i++)
		CloseHandle(arWaits[i]);
	cout << "======= End MutexNoti Test ==========" << endl;
}
