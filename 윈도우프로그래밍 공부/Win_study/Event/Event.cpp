#include <stdio.h>
#include <tchar.h>
#include <SDKDDKVer.h>
#include "Windows.h"
#include <iostream>

using namespace std;

//Event는 쓸수있는 방법이 굉장히 다양하다.
//이 예재말고도 다양한 경우를 쓰진 못할지라도 확인은 꼭 하자.

DWORD WINAPI ThreadProc(LPVOID pParam)
{
	HANDLE hEvent = (HANDLE)pParam;

	DWORD dwWaitCode = WaitForSingleObject(hEvent, INFINITE);
	if (dwWaitCode == WAIT_FAILED)
	{
		cout << "WaitForSingleObject failed : " << GetLastError() << endl;
		return 0;
	}

	SYSTEMTIME st;
	GetLocalTime(&st);

	cout << "..." << "SubThread " << GetCurrentThreadId() << " => ";
	cout << st.wYear << '/' << st.wMonth << '/' << st.wDay << ' ';
	cout << st.wHour << ':' << st.wMinute << ':' << st.wSecond << '+';
	cout << st.wMilliseconds << endl;
	SetEvent(hEvent);
	//SetEvent()매배변수로 넘겨진 이벤트 커널 객체를 시그널 상태로 설정
	//반대로 ResetEvent()함수는 넌시그널 상태로 설정한다.

	return 0;
}

void _tmain(void)
{
	cout << "======= Start Event Test ========" << endl;
	HANDLE hEvent = CreateEvent(
		NULL,	//
		FALSE,	// 해당 이벤트가 자동리셋이벤트 인지(FALSE) , 수동리셋이벤트 인지(TRUE)
		TRUE,	// 생성된 이벤트 객체의 초기상태 TRUE:시그널상태 FALSE:넌시그널 상태
		NULL	//
	);
	//자동 리셋 이벤트를 생성해 스레드의 매개변수로 넘긴다.
	//자동리셋의 경우 시그널 상태가 되었을때 대기 함수에서 탈출하면서 다시 넌시그널상태
	//수동리셋의 경우대기에서 탈출하게 되더라고 시그널 상태로 남아있는다.

	HANDLE arhThreads[10];
	for (int i = 0; i < 10; i++)
	{
		DWORD dwTheaID = 0;
		arhThreads[i] = CreateThread(NULL, 0, ThreadProc, hEvent, 0, &dwTheaID);
	}
	WaitForMultipleObjects(10, arhThreads, TRUE, INFINITE);
	CloseHandle(hEvent);
	cout << "======= End Event Test ==========" << endl;
}
