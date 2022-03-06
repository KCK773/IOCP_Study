#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include "Windows.h"
#include "iostream"
//g_nValue라는 전역 변수에 대해 원자성을 보장해주기 위해 직접 스레드의 동기화 루틴을 작성
//정말 무식한 방법이다. 실전에서는 쓰지말도록 하자. 제대로 실행되지 않을때가 많다.

#define OFF 0
#define ON  1
using namespace std;

void AcquireLock(volatile LONG& ISign)
{
    while (ISign == OFF);
    //ISign이 OFF일동안 계속 루프를 돌다가 ON이 되는 순간 빠져나와 코드를 실행

    ISign = OFF;
    //사용하고 있음을 알리기 위해 OFF상태로 만듬
}

void ReleaseLock(LONG& ISign)
{
    ISign = ON;
    //ISign을 ON으로 만들어 다른 스레드가 동유 코드를 실행할 수 있도록 한다.
}

LONG g_ICSSign = ON;
//ON/OFF 표식으로 사용될 전역 변수다. 최초 ON 상태로 설정하여 공유자원을 누구나 사용할 수 있음을 표기해줘야 한다.

int g_nValue = 0;
//원자성을 확보해줘야 할 스레드 사이에서 서로 공유되는 전역 변수

DWORD WINAPI ThreadProc(PVOID pParam)
{
    INT dwCurId = (INT)pParam;

    AcquireLock(g_ICSSign); //공유자원 사용 여부 체크 및 대기

    cout << "Enter" << dwCurId << endl;
    g_nValue++;
    cout << "Leave" << dwCurId << endl;

    ReleaseLock(g_ICSSign); //공유자원 사용 완료 통지

    return 0;
}


#define MAX_THR_CNT 4
void _tmain()
{
    HANDLE arhThrs[MAX_THR_CNT];
    for (int i = 0; i < MAX_THR_CNT; i++)
    {
        DWORD dwTheaID = 0;
        arhThrs[i] = CreateThread(NULL, 0, ThreadProc, (PVOID)i, 0, &dwTheaID);
    }

    getchar();

    for (int i = 0; i < MAX_THR_CNT; i++)
        CloseHandle(arhThrs[i]);
}
