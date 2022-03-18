#include <stdio.h>
#include <tchar.h>
#include <SDKDDKVer.h>
#include "Windows.h"
#include "string"
#include "list"
#include <iostream>
using namespace std;

//공유자원을 스레드가 작업할 대상으로만 한정하지 말고 스레드 자체를 공유자원을 보는것
//세마포어를 이용하여 스레드 풀을 구현

typedef std::list<std::string> TP_QUE;
struct THREAD_POOL
{
	HANDLE	m_hMutx;	// 엔큐/데큐 동기화를 위한 뮤텍스
	HANDLE	m_hSema;	// 스레드 풀 구현을 위한 세마포어
	TP_QUE	m_queue;	// 큐 기능을 담당할 STL 리스트
};
typedef THREAD_POOL* PTHREAD_POOL;

HANDLE g_hExit = NULL;	//스레드 종료를 위한 뮤텍스

DWORD WINAPI PoolItemProc(LPVOID pParam)
{
	PTHREAD_POOL pTQ = (PTHREAD_POOL)pParam;

	DWORD dwThId = GetCurrentThreadId();
	HANDLE arhObjs[2] = { g_hExit, pTQ->m_hSema };
	while (true)
	{
		DWORD dwWaitCode = WaitForMultipleObjects(2, arhObjs, FALSE, INFINITE);
		//처음에 모든 스레드는 세마포어가 넌시그널 상태기 때문에 대기 상태로 들어간다.
		//그러다 요청 수신 스레드(여기서는 메인스레드)에서 요청이 들어왔을 때 ReleaseSemaphore를 통해서 자원계수를 1 증가시켜 주면
		//대기 상태에서 풀려 다음 코드를 실행하게 된다.
		//물론 WaitForMultipleObject의 부가적 효과로 대기 상태에서 풀릴 때는 이미 세마포어의 자원계수는 1 감소되어 0이 되어있다.

		if (dwWaitCode == WAIT_FAILED)
		{
			cout << " ~~~ WaitForSingleObject failed : " << GetLastError() << endl;
			break;
		}

		if (dwWaitCode == WAIT_OBJECT_0)
			break;
		//뮤텍스가 시그널 상태가 되면 스레드를 종료하기 때문에 루프를 탈출한다.

		std::string item;
		WaitForSingleObject(pTQ->m_hMutx, INFINITE);
		TP_QUE::iterator it = pTQ->m_queue.begin();
		item = *it;
		pTQ->m_queue.pop_front();
		ReleaseMutex(pTQ->m_hMutx);
		//큐에서 작업 항목을 데큐한다. 뮤테스를 사용해 데큐할 동안 큐에 대한 동시 접근을 막는다.

		printf(" => BEGIN | Thread %d works : %s\n", dwThId, item.c_str());
		Sleep(((rand() % 3) + 1) * 1000);
		printf(" <= END   | Thread %d works : %s\n", dwThId, item.c_str());
		//큐로부터 작업 항목을 휙득하면 이에 대한 처리르 ㄹ하고, 다시 루프로 돌아가 대기 상태로 들어간다.
	}
	ReleaseMutex(g_hExit);
	//뮤텍스를 종료 시그널로 사용하기 때문에 종료하기 전 종료 통지용 뮤텍스의 소유권을 놓아준다.
	printf("......PoolItemProc Thread %d Exit!!!\n", dwThId);

	return 0;
}

void _tmain()
{
	cout << "== 쓰레드풀세마포어 테스트 시작 ==" << endl;

	g_hExit = CreateMutex(NULL, TRUE, NULL);
	//스레드 종료를 위해서 뮤텍스를 생성한다.
	//보통 수동 리셋 이벤트로 처리하지만 아직 이 코드를 작성할 당시에 이벤트에 대해서 배우지 않았기에 뮤텍스를 이용함

	THREAD_POOL tp;
	tp.m_hSema = CreateSemaphore(NULL, 0, INT_MAX, NULL);
	//초기 사용가능 자원계수를 0으로 넘겨주어 넌시그널 상태로 생성
	//사용 가능한 최대 자원수는 지금은 INT_MAX로 구현했지만 큐가 무한정 커지는 것을 막고자 한다면 일정 한계치의 값을 넘겨줌

	tp.m_hMutx = CreateMutex(NULL, FALSE, NULL);
	//엔큐/데큐 시의 동기화를 위해 뮤텍스 생성

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	//풀에 들어갈 스레드수 , 여기서는 시스템의 CPU개수만큼 생성

	PHANDLE parPoolThs = new HANDLE[si.dwNumberOfProcessors];
	for (int i = 0; i < (int)si.dwNumberOfProcessors; i++)
	{
		DWORD dwThreadId;
		parPoolThs[i] = CreateThread(NULL, 0, PoolItemProc, &tp, 0, &dwThreadId);
	}
	//CPU개수만큼 스레드를 생성, 생성된 스레드들은 세마포어가 넌시그널 상태이므로 모두 대기 상태로 존재
	cout << "*** MAIN -> ThreadPool Started, count=" << si.dwNumberOfProcessors << endl;

	char szIn[1024];
	while (true)
	{
		cin >> szIn;
		if (_stricmp(szIn, "quit") == 0)
			break;
		//유저가 메인 콘솔에서 데이터를 입력하면 그 데이터를 대기 스레드가 처리하도록 맡긴다.
		//이 과정은 클라이언트가 서비스 요청을 해 왔음을 시뮬레이션 한것
		//만약 이 부분을 소켓으로 사용한다면 클라이언트에 대해 리슨을 하고 있는것으로 대체할 수 있다.

		WaitForSingleObject(tp.m_hMutx, INFINITE);
		tp.m_queue.push_back(szIn);
		ReleaseMutex(tp.m_hMutx);
		//문자열을 큐에 엔큐한다. 뮤텍스 객체를 이용해 큐에 대한 독점권을 획득한 후 작업 항목을 큐에 추가한다.

		ReleaseSemaphore(tp.m_hSema, 1, NULL);
		//대기중인 스레드에게 작업 항목이 추가되었음을 알리는 방식
		//이 시점에서 CPU수만큼의 스레드들은 전부 m_hSema를 대상으로 대기하고 있다.
		//현재 자원계수가 0이므로 요청에 대해 ReleaseSemaphore를 호출해 1만큼 자원 수를 증가시켜준다.
		//증가시켜줌에 따라 대기 스레드중 임의의 하나를 활성화 시켜 요청을 처리하도록 한다. 
		//앞선 예제와는 다르게 ReleaseSemaphore를 호출하는 스레드는 메인 스레드라는 점을 주의하기 바라다.
		//또한 스레드를 CPU수만큼 생성시켜 주었기 때문에 작업이 계속 추가되더라도 항상 CPU수만큼의 스레드만이 활성화된다는 점을 기억
	}

	ReleaseMutex(g_hExit);
	//종료를 위해 종료통지 뮤텍스를 시그널한다.

	WaitForMultipleObjects(si.dwNumberOfProcessors, parPoolThs, TRUE, INFINITE);
	for (int i = 0; i < (int)si.dwNumberOfProcessors; i++)
		CloseHandle(parPoolThs[i]);
	delete[] parPoolThs;

	CloseHandle(tp.m_hSema);
	CloseHandle(tp.m_hMutx);
	CloseHandle(g_hExit);

	cout << "======= TPSemaphore 테스트 종료 ==========" << endl;
}
