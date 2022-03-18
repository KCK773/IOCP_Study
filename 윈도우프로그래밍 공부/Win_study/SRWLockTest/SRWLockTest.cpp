#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include "Windows.h"
#include "iostream"
using namespace std;
#pragma warning(disable:4996) // C4996 에러를 무시

//다수의 읽기 전용 스레드와 다수의 쓰기 전용 스레드가 존재 할 때
//이벤트 예제일때 한번 구현해 보았으며 이번에는 SRW-락을 이용하여 해결
// 
// 공유잠금(읽기 스레드)
// AcquireSRWLockShared		잠금
// ...동기화가 요구되는 작업의 코드
// ReleaseSRWLockShared		해제
// 
// 배타적 잠금만을 이용한다면 크리티컬 섹션과 별반 다를게 없어진다.
// 배타적 잠금(쓰기 스레드)
// AcquireSRWLockExclusive	잠금
// ...동기화가 요구되는 작업의 코드
// ReleaseSRWLockExclusive	해제
// 


SRWLOCK	g_srw;
int		g_nValue;
LONG	g_bIsExit;

DWORD WINAPI ThreadProc(PVOID pParam)
{
	DWORD dwToVal = *((DWORD*)pParam);

	while (g_bIsExit == 0)
	{
		AcquireSRWLockShared(&g_srw);	//잠금
		cout << "....TH " << dwToVal << " Read : " << g_nValue << endl;
		ReleaseSRWLockShared(&g_srw);	//해제
		//읽기 전용으로 SRW-락을 사용한다. 쓰기 스레드는 대기해야 하지만, 읽기 스레드들은 대기하지 않고 작업을 계속 수행

		Sleep(dwToVal * 1000);
	}

	return 0;
}

#define MAX_THR_CNT	5
void _tmain()
{
	InitializeSRWLock(&g_srw);
	//SRW-락을 초기화 한다.

	DWORD ardwWaits[] = { 1, 2, 3, 4, 5 };
	HANDLE roThrs[MAX_THR_CNT];
	for (int i = 0; i < MAX_THR_CNT; i++)
	{
		DWORD dwTheaID = 0;
		roThrs[i] = CreateThread(NULL, 0, ThreadProc, &ardwWaits[i], 0, &dwTheaID);
	}

	char szCmd[512];
	while (true)
	{
		cin >> szCmd;
		if (strcmpi(szCmd, "quit") == 0)
		{
			InterlockedExchange(&g_bIsExit, TRUE);
			break;
		}

		int val = atoi(szCmd);
		if (val >= 0)
		{
			AcquireSRWLockExclusive(&g_srw);	//잠금
			g_nValue = val;
			cout << "MAIN TH Change value : " << g_nValue << endl;
			Sleep(2000);
			ReleaseSRWLockExclusive(&g_srw);	//해제
			//쓰기 전용으로 SRW-락을 사용한다. 잠긴 상태에서는 읽기 스레드, 쓰기 스레드 모두 대기 상태가 된다
		}
	}

	//크리티컬 섹션과는 달리 SRW-락을 삭제하는 코드느 존재하지 않는다.
	WaitForMultipleObjects(MAX_THR_CNT, roThrs, TRUE, INFINITE);
}
