#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include "Windows.h"
#include "iostream"

using namespace std;

class MyThread
{
	static LONG ExpFilter(LPEXCEPTION_POINTERS pEx);	//예외 정보 출력 함수
	static DWORD WINAPI MyThreadProc(PVOID pParam);		//스레드 엔트리 함수를 정적 함수로 선언
	void InnerThreadProc();								//클래스 멤버 변수의 편리한 사용을 위해 엔트리 함수 내부에서 호출해 엔트리 함수의 역할을 대신할 함수

	HANDLE	m_hThread;			//스레드 핸들
	DWORD	m_dwThreadID;		//스레드의 ID를 받아줄 변수
	bool	m_bExit;			//스레드 종료 통지 역할
	DWORD	m_dwDelay;			//딜레이 시간

public:
	MyThread() : m_hThread(NULL), m_dwThreadID(0), m_bExit(false) , m_dwDelay(1000) {}	//클래스 생성자 및 변수 초기화
	~MyThread()
	{
		if (m_hThread != NULL)
			CloseHandle(m_hThread); //스레드 역시 커널 객체이므로 , CloseHandle을 호출해 닫아줘야 커널에 의해 삭제가 가능하다.
	}
public:
	//HRESULT : 오류 조건 및 경고 조건을 나타내는 데 사용되는 데이터 유형
	//함수가 HRESULT 형태를 사용함으로서 이 함수의 상태 정보를 리턴 받을 수 있게 된다.
	//BOOL형의 TRUE/FALSE와 마찬가지로 SUCCEEDED/FAILED를 사용하며 차이점으로 반드시 값을 리턴 받지는 않아도 된다.
	HRESULT Start(DWORD dwDelay = 1000)	
	{
		m_dwDelay = dwDelay;
		m_hThread = CreateThread(	//스레드를 생성한다
			NULL,					//보안 식별자를 NULL로 넘겼다
			0,						//스레드 스택의 크기, 0을 넘기면 디폴트로 1MB가 지정된다.
			MyThreadProc,			//유저가 정의한 스레드 엔트리 함수에 대한 포인터와
			this,					//넘겨줄 매개변수를 지정한다. 매개변수는 this, 즉 본 클래스의 인스턴스 포인터이다.
			0,						//생성과 동시에 실행되도록 지정한다.
			&m_dwThreadID			//m_dwThreadID변수에 스레드의 ID가 지정되어 반환된다.
		);
		if (m_hThread == NULL)
			return HRESULT_FROM_WIN32(GetLastError());
		return S_OK;
	}

	void Stop()	//스레드에게 종료를 통지하는 역할을 한다.
	{
		m_bExit = true;
		while (m_dwThreadID > 0)
		{
			Sleep(100);
			//스레드가 종료될 떄까지 대기하는 코드다. 불완전한 코드지만 스레드가 종료될 때까지 대기하는 처리는 중요하다.
		}
	}
};

//예외 정보 출력 함수 정의
LONG MyThread::ExpFilter(LPEXCEPTION_POINTERS pEx)
{
	PEXCEPTION_RECORD pER = pEx->ExceptionRecord;
	printf("~~~~ Exception : Code = %x, Address = %p", pER->ExceptionCode, pER->ExceptionAddress);
	//예외 관련 정보를 간단하게 출력한다

	return EXCEPTION_EXECUTE_HANDLER;
}

DWORD WINAPI MyThread::MyThreadProc(PVOID pParam)
{
	MyThread* pThis = (MyThread*)pParam;

	__try 
	{
		pThis->InnerThreadProc();	//스레드 엔트리 함수의 정의다. this포인터를 받아 스레드 수행을 대리하는 멤버함수 InnerTrheadProc을 호출한다
	}
	__except (ExpFilter(GetExceptionInformation()))
	{
		pThis->m_dwThreadID = 0;
	}
	return 0;
	//스레드 종료 코드로 0을 반환한다.
/*
스레드의 종료 방법으로 ExitThread함수와 TerminateThread함수가 추가로 있다.
ExitThread 함수는 스레드 엔트리 함수내에서 스레드를 스스로 종료하고 싶을 경우,
TerminateThread 함수는 다른 스레드에서 특정 스레드를 강제 종료시키고 싶은경우에 사용한다
왠만하면 이 두개의 사용은 피하고 위 예제처럼 자연스러운 리턴을 통한 종료를 처리해주자.
*/
}

void MyThread::InnerThreadProc()
{
	while (!m_bExit)	//스레드의 종료 통지를 받기 위해 m_bExit를 사용한다. true면 탈출!
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		printf("Thread %d, delay %d => %5d/%02d/%02d-%02d:%02d:%02d+%03d\n",
			m_dwThreadID, m_dwDelay, st.wYear, st.wMonth, st.wDay,
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		Sleep(m_dwDelay);
		//스레드의 작업처리 부분을 Sleep을 통해 에뮬레이션 한다.
	}
	m_dwThreadID = 0;
	//스레드가 종료되었음을 알리기 위해 ID를 0으로 설정한다. 불완전한 처리지만 중요한 부분이다.
}

void _tmain()
{
	cout << "Main thread creating sub thread..." << endl;
	MyThread mt01, mt02;
	HRESULT hr01 = mt01.Start();
	HRESULT hr02 = mt02.Start();

	if (FAILED(hr01))
	{
		cout << "시작 실패 . error code is" << hr01;
		return;
	}
	if (FAILED(hr02))
	{
		cout << "시작 실패 . error code is" << hr02;
		return;
	}
	
	getchar(); //임의의 키를 입력하면 stop호출
	mt01.Stop();
	mt02.Stop();

	cout << "Main thread creating sub thread..." << endl;
}