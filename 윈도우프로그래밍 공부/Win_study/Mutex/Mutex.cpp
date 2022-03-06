#include <stdio.h>
#include <tchar.h>
#include <SDKDDKVer.h>
#include "Windows.h"
#include <iostream>

using namespace std;

DWORD WINAPI ThreadProc(LPVOID pParam)
{
    HANDLE hMutex = (HANDLE)pParam;

    //이 경우 WaitForSingleObject함수와 ReleaseMutex함수에 주석처리를 원하는 대로 출력이 되지않을것이다.

    WaitForSingleObject(hMutex, INFINITE);
    //소유권을 휙들할 때까지 대기
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        cout << "..." << "SubThread " << GetCurrentThreadId() << " => ";
        cout << st.wYear << '/' << st.wMonth << '/' << st.wDay << ' ';
        cout << st.wHour << ':' << st.wMinute << ':' << st.wSecond << '+';
        cout << st.wMilliseconds << endl;
    }

    ReleaseMutex(hMutex);
    //휙득한 소유권을 해제한다. 리턴값이 false인 경우는 해제에 실패했음을 의미한다.
    //뮤텍스를 시그널 상태로 만들어 다른 스레드가 소유할 수 있도록 한다.

    return 0;
}


#define MAX_THR_CNT 8

void _tmain(void)
{
    cout << "======= 뮤텍스 테스트 시작 ========" << endl;
    HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);
    //뮤텍스를 생성한다. 두번째 매개변수로 FALSE를 넘겨주어 메인 스레드가 뮤텍스를 소유하지 않음을 명시
    //생성한 뮤텍스의 핸들은 스레드 엔트리 함수의 매개변수로 넘겨준다.

    HANDLE arhThreads[MAX_THR_CNT];
    for (int i = 0; i < MAX_THR_CNT; i++)
    {
        DWORD dwThreaID = 0;
        arhThreads[i] = CreateThread(NULL, 0, ThreadProc, hMutex, 0, &dwThreaID);
    }

    //if (ReleaseMutex(hMutex) == FALSE)
    //    printf("~~~ReleaseMutex error, code=%d/n", GetLastError());
    //이부분의 주석을 지우면 에러 구문이 나온다.
    //주석을 지워도 에러구문이 안뜨게 하고싶다면 CreateMutex()함수의 인자중 FALSE를 TRUE로 바꾸자.
    //그리고 왜 이렇게 되는지 생각하자.
    

    WaitForMultipleObjects(MAX_THR_CNT, arhThreads, TRUE, INFINITE);
    //생성한 모든 스레드가 종료될 때까지 대기한다.

    for (int i = 0; i < MAX_THR_CNT; i++)
        CloseHandle(arhThreads[i]);
    CloseHandle(hMutex);
    cout << "======= End Mutex Test ==========" << endl;
}
