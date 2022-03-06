#include <stdio.h>
#include <tchar.h>
#include <SDKDDKVer.h>
#include "Windows.h"
#include <iostream>

using namespace std;

#define MAX_THR_CNT 5

DWORD WINAPI ThreadProc(PVOID pParam)	//스레드 엔트리 함수
{
	DWORD dwDelay = (DWORD)pParam * 1000;

	cout << ">>>>> Thread " << GetCurrentThreadId() << " enter." << endl;
	Sleep(dwDelay);
	cout << "<<<<< Thread " << GetCurrentThreadId() << " leave." << endl;

	return 0;
}


void _tmain()
{
    cout << "Main Trhead creating sub Threads....." << endl;

    HANDLE arhThreads[MAX_THR_CNT];
	for (int i = 0; i < MAX_THR_CNT; i++)
	{
		DWORD dwThreadID = 0;
		arhThreads[i] = CreateThread(NULL, 0, ThreadProc, (PVOID)(i + 3), 0, &dwThreadID);
	}

	//WaitForSingleObject()
	//하나의 커널 객체에 대해 시그널 상태가 될 때까지 스레드의 실행을 정지시키는 함수
	//멀티스레딩 솔루션의 경우에는 대부분 WaitForMultipleObjects()함수를 사용하게 될것

	WaitForMultipleObjects(
		MAX_THR_CNT,	//배열의 원소 갯수 , 즉 동기화 커널 객체의 핸들 수를 지정
		arhThreads,		//동기화 객체들의 핸들을 담고 있는 배열의 시작 포인터
		TRUE,			//동기화 객체들의 상태가 모두 시그널 상태일 경우에만 이 함수에서 리턴할 것인지 아닌지
						//TRUE면 모든 객체들이 시그널 상태일때까지 대시, FALSE면 하나라도 시그널 상태가 되면 리턴
		INFINITE		//타임아웃 값, 시그널 상태가 될 때 까지 기다리는 시간으로 INFINITE사용시 무한히 대기
	);
	//WaitForMultipleObjects의 반환값
	//WAIT_FAILED	: 대기하는 동안 오류가 발생해 리턴 , GetLastError()을 통해서 에러 코드를 얻을 수 있다.
	//WAIT_TIMEOUT	: 객체가 타임아웃에 의해 리턴
	//WAIT_OBJECT_0	: 객체가 시그널 상태가 되어 리턴. 모든 객체가 시그널 상태
	//WAIT_OBJECT_0~: 반환값 -  WAIT_OBJECT_0 의 값은 배열 원소 중에서 시그널 상태가 된 객체를 가리키는 인덱스가 된다.
	//포기된 뮤텍스	: 배열에 담긴 객체중 뮤텍스가 적어도 하나 이상 존재하는 경우에 리턴 가능한 값
	//WAIT_ABANDONED_0 ~ 의 값을 가진다.
	//반환값 - WAIT_ABANDONED_0 의 값은 배열 원소중에서 포기된 뮤텍스의 인덱스

	cout << "All sub threads terminated......" << endl;
	for (int i = 0; i < MAX_THR_CNT; i++)
	{
		CloseHandle(arhThreads[i]);
	}
}
