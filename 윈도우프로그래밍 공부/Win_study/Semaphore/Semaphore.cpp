#include <stdio.h>
#include <tchar.h>
#include <SDKDDKVer.h>
#include "Windows.h"
#include <iostream>

using namespace std;

//공유자원의 관점에서 세마포어를 바라보는 예

#define MAX_RES_CNT 3
bool g_bExit = false;
//스레드 종료를 위한 플래그를 정의한다.

BOOL	g_abUsedFlag[MAX_RES_CNT];
INT		g_anSharedRes[MAX_RES_CNT] = { 5, 3, 7 };
//자원을 나타내는 배열로 배열의 원소 개수는 3개다.
HANDLE	g_hShareLock;
//가용 자원의 검색을 위한 뮤텍스 핸들이다.

//세마포어를 통해서 자원을 사용하고자 하는 스레드 엔트리 함수에 대한 정의
DWORD WINAPI SemaphoreProc(LPVOID pParam)
{
    HANDLE hSemaphore = (HANDLE)pParam;
    DWORD dwThreadId = GetCurrentThreadId();

    while (!g_bExit)
    {
        DWORD dwWaitCode = WaitForSingleObject(hSemaphore, INFINITE);
        //세마포어에 대해 대기 상태로 들어간다.
        if (dwWaitCode == WAIT_FAILED)
        {
            cout << " ~~~ WaitForSingleObject failed : " << GetLastError() << endl;
            break;
        }
        //dwWaitCode가 에러가 아닌 경우는 세마포어가 시그널 상태가 되었음을 의미한다.
        //따라서 공유자원을 휙득할 수 있는 상태가 되고, 그것에 대한 작업을 수행한다.

        int nSharedIdx = 0;
        WaitForSingleObject(g_hShareLock, INFINITE);
        for (; nSharedIdx < MAX_RES_CNT; nSharedIdx++)
        {
            if (!g_abUsedFlag[nSharedIdx])
                break;
        }
        g_abUsedFlag[nSharedIdx] = TRUE;
        ReleaseMutex(g_hShareLock);
        //세마포어가 시그널 상태가 되었다는 말은 가용 자원이 있다는 것말을 의미하기 때문에 그 자원에 대한 식별수단은 따로 제공x
        //따라서 사용가능한 자원을 찾기위해 루프를 돌면서 그 자원을 찾는 과정이 필요
        //동시에 여러 스레드가 이 루프를 수행할 수 있기 때문에 가용 자원 검색시의 동기화 역시 필요하므로 뮤텍스를 통해 동기화를 수행


        cout << " ==> Thread " << dwThreadId << " waits " << g_anSharedRes[nSharedIdx] << " seconds..." << endl;
        Sleep((DWORD)g_anSharedRes[nSharedIdx] * 1000);
        cout << " ==> Thread " << dwThreadId << " releases semaphore..." << endl;
        g_abUsedFlag[nSharedIdx] = FALSE;

        ReleaseSemaphore(hSemaphore, 1, NULL);
        //자원을 모두 사용했다면 ReleaseSemaphore를 호출해 자원계수를 1 증가시킨다.
        //대기중인 다른 스레드가 해제된 자원을 사용할수 있게 된다.
    }
    
    cout << "*** Thread " << dwThreadId << "exits...." << endl;
    return 0;
}

void _tmain()
{
    cout << "======= 세마포어 테스트 시작 ========" << endl;
    g_hShareLock = CreateMutex(NULL, FALSE, NULL);
    HANDLE hSemaphore = CreateSemaphore( //세마포어를 생성한다. 이 세마포어는 시그널 상태로 생성된다.
        NULL,              //
        MAX_RES_CNT,        //최초 사용 가능한 자원계수 (3)
        MAX_RES_CNT,        //가용 가능한 최대 자원계수 (3)
        NULL                //
    );

    DWORD dwThrId;
    HANDLE hThreads[MAX_RES_CNT + 2];   
    for (int i = 0; i < MAX_RES_CNT + 2; i++)
    {
        hThreads[i] = CreateThread(NULL, 0, SemaphoreProc, hSemaphore, 0, &dwThrId);
        //스레드 5개 생성
        //처음 3개의 스레드는 자원을 차지하게 되지만 네번째 스레드부터는 대기 상태로 들어갈 것이다.
        //스레드 생성시 세마포어의 핸들을 스레드 엔트리 함수의 매개변수로 넘겨준다.
    }

    getchar();
    g_bExit = true;
    //임의의 키가 입력될때까지 메인스레드를 대기시킨다. 키가 입력되면 스레드 종료 처리를 위해 전역 플래그를 true로 설정한다.
    
    WaitForMultipleObjects(MAX_RES_CNT + 2, hThreads, TRUE, INFINITE);
    //5개의 스레드가 종료될 때까지 대기 상태에서 기다린다. 스레드가 정상적으로 종료되도록 하는 가장 안전한 방법

    for (int i = 0; i < MAX_RES_CNT + 2; i++)
    {
        CloseHandle(hThreads[i]);
    }
        CloseHandle(hSemaphore);
        CloseHandle(g_hShareLock);
    //스레드들과 세마포어, 뮤텍스의 핸들을 닫는다.

    cout << "======= 세마포어 테스트 끝 ========" << endl;
}
