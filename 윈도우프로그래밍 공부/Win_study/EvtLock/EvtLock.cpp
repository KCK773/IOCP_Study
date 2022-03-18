#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include "Windows.h"
#include "iostream"

//이벤트의 사용예시
// 하나의 쓰기 스레드와 다수의 읽기 전용 스레드가 있을때
//1. 읽기 스레드들 사이에서는 잠금없이 자유로이 데이터에 접근 가능 할것
//2. 읽기 스레드들이 데이터에 접근할 때 쓰기 스레드가 데이터 변경 중이면 대기할 것
//3. 쓰기 스레드가 데이터를 변경할 때 읽기 스레드들 중 최소 하나라도 읽기 작업 중이면 대기할 것
//
// 이벤트를 두개를 만든다. 
// WO(WriteOnly):쓰기전용 스레드가 자신이 작업 진행 중임을 알리기 위해 사용 
// RO(ReadOnly)	:읽기 전용 스레드가 읽기 작업을 진행 중임을 알림 
//쓰기전용 스레드
//1.쓰기 작업 시작을 통지하기 위해 WO 이벤트를 리셋한다
//2.읽기 작업과 동기화를 위해 RO 이벤트에 대해 WaitForXXX함수를 호출한다.
//3.데이터를 공유 버퍼에 쓴다
//4.쓰기 작업 완료를 통지하기 위해 WO 이벤트를 시그널링한다.
//
//읽기 전용 스레드
//1.먼저 쓰기 작업과 동기화를 위해 WO 이벤트에 대해 WaitForXXX함수를 호출한다
//2.읽기 작업 시작을 통지하기 위해 RO이벤트를 리셋한다
//3.사용계수를 1 증가시킨다
//4.데이터를 공유 버퍼로부터 읽는다
//5.사용계수를 1 감소시킨다
//6.사용계수가 0이면 읽기 작업 완료를 통지하기 위해 RO 이벤트를 시그널링한다.

using namespace std;

//직관적으로 보기위해 읽기 스레드는 별도로, 쓰기 스레드는 메인 함수에서 처리

struct ERWLOCK
{
	HANDLE	_hevRO;		//읽기 통지 이벤트
	HANDLE	_hevWO;		//쓰기 통지 이벤트
	LONG	_nRoRefs;	//사용 계수

	ERWLOCK()
	{
		_hevRO = _hevWO = NULL;
		_nRoRefs = 0;
	}
	~ERWLOCK()
	{
		if (_hevRO != NULL) CloseHandle(_hevRO);
		if (_hevWO != NULL) CloseHandle(_hevWO);
	}
};

ERWLOCK g_rwl;		//위에서 정의한 ERWLOCK 구조체를 전역적으로 선언한다
int		g_nValue;	//전역 버퍼를 가정한다. 이 변수에 하나의 쓰기 스레드(메인 함수)가 데이터를 쓰고,
					//여러개의 읽기 스레드가 변경된 데이터를 읽고 출력 한다.
LONG	g_bIsExit;	//스레드 종료 플래그

DWORD WINAPI ReaderProc(PVOID pParam)
{
	DWORD dwToVal = *((DWORD*)pParam);
	//스레드 엔트리 함수의 매개변수로 읽기 스레드별 지연 시간을 넘겨준다

	while (g_bIsExit == 0)
	{
		WaitForSingleObject(g_rwl._hevWO, INFINITE);
		//쓰기 스레드가 작업 중이면 쓰는 작업이 끝날 때까지 대기한다.
		{
			ResetEvent(g_rwl._hevRO);
			//RO 이벤트를 리셋하여 쓰기 전용 스레드가 읽기 작업을 수행할 동안 대기하도록 한다.
			InterlockedIncrement(&g_rwl._nRoRefs);
			//사용계수를 1 증가시켜 현재 시점에서 읽기 작업을 수행 중인 스레드의 개수를 파악 가능하도록 한다

			cout << "....TH " << dwToVal << " Read : " << g_nValue << endl;
			//전역 변수에서 읽은 데이터를 콘솔에 출력한다.

			if (InterlockedDecrement(&g_rwl._nRoRefs) == 0)
				SetEvent(g_rwl._hevRO);
			//읽기 작업을 끝냈으므로 사용계수를 1 감소시킨다. 
			//감소한 결과가 0이면 본 스레드가 읽기 작업을 마친 최종 스레드임을 의미하므로.
			//넌시그널 상태의 RO 이벤트를 시그널링 하여 쓰기 스레드가 작업을 진행할 수 있도록 한다.
			//InterlockedIncrement함수와 InterlockedDecrement함수는 추후 프로젝트에서 설명
		}
		Sleep(dwToVal * 200);
	}

	return 0;
}

#define MAX_THR_CNT	5
void _tmain()
{
	g_rwl._hevRO = CreateEvent(NULL, TRUE, TRUE, NULL);
	g_rwl._hevWO = CreateEvent(NULL, TRUE, TRUE, NULL);
	//WO,RO 이벤트를 생성한다. 수동 리셋 이벤트로 생성해야 하는 것에 주의하지 바란다.
	//그리고 어떤 읽기,쓰기, 스레드도 먼저 잠금을 할 수 있도록 하기 위해 이벤트의 초기 상태를 시그널 상태로 지정해야 한다.


	DWORD ardwWaits[] = { 1, 2, 3, 4, 5 };
	HANDLE roThrs[MAX_THR_CNT];
	for (int i = 0; i < MAX_THR_CNT; i++)
	{
		DWORD dwThrId = 0;
		roThrs[i] = CreateThread(NULL, 0, ReaderProc, &ardwWaits[i], 0, &dwThrId);
	}
	//MAX_THR_CNT 만큼의 읽기 전용 스레드를 생선한다.

	char szCmd[512];
	while (true)
	{
		cin >> szCmd;
		if (_stricmp(szCmd, "quit") == 0)
		{
			g_bIsExit = TRUE;
			break;
		}

		int val = atoi(szCmd);
		if (val >= 0)
		{
			ResetEvent(g_rwl._hevWO);
			//읽기 전용 스레드와는 반대로 우선 WO 이벤트를 리셋함으로써, 이 이후부터는 읽기 작업을 위해 대기하도록 설정
			WaitForSingleObject(g_rwl._hevRO, INFINITE);
			//읽기 작업중인 스레드들이 있으면 그 작업이 모두 끝날 때까지 RO 이벤트에 대해 대기한다
			//RO이벤트는 읽기 스레드가 관리하는 사용계수가 최종적으로 0이 될 때 EO 이벤트를 시그널 상태로 만든다.
			{
				g_nValue = val;
				cout << "MAIN TH Change value : " << g_nValue << endl;
				//값을 변경하고 변경된 값을 출력한다.
			}
			SetEvent(g_rwl._hevWO);
			//쓰기 작업이 끝났으므로 읽기 스레드가 읽을 수 있도록 WO이벤트를 시그널링 해준다.
		}
	}

	WaitForMultipleObjects(MAX_THR_CNT, roThrs, TRUE, INFINITE);
}

//만약 1:N 관계가 아닌 N:N 구조를 하고싶다면 쓰기 스레드 사이에서만 공유되는 뮤텍스를 하나 추가하면 된다.