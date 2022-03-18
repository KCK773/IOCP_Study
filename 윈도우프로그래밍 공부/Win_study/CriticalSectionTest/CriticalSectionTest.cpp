#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include "Windows.h"
#include "iostream"
using namespace std;

//크리티컬 섹션(Critical Section : 직역시 임계구역)
//임계구역의 출입을 위해 사용되는 수단을 의미하는 동기화 객체
//뮤텍스와 거의 동일하며 , 뮤텍스는 커널 객체이고, 크리티컬 섹션은 유저모드에서 작동한다는 차이점이 있다.
//장점 : 사용 방법이 단순하다, 작은 단위의 모듈별 동기화에 적용하기 쉽다. 커널 객체를 이용한 동기화 보다 속도가 빠르다.
//단점 : IPC에서 사용불가(다른 프로세스간 사용 불가) , WaitForMultiObjects 등을 사용한 동시 대기 사용 불가
//       공유 리소스에 대한 배타적 접근을 목적으로 주로 사용되기 때문에 복잡한 상황에 적용하기에는 한계가 있음

//크리티컬 섹션은 공유 데이터의 보호에 있어서 그 사용이 간편하고 빠른 속도를 자랑하기 때문에 많이 애용되고 있다.
//특히 컨테이너 클래스(리스트 , 트리 , 큐 , 스택 , 해시테이블 등)를 구현할 때 스레드에 안전한 코드를 사용하는데 자주 사용

//아래 코드는 Part02에 있는 뮤텍스 예재에서 뮤텍스 대신 크리티컬 섹션의 사용으로 바꾼 것이다.
//뮤텍스와도 크게 차이가 없음을 알 수 있다.

//

DWORD WINAPI ThreadProc(LPVOID pParam)
{
	PCRITICAL_SECTION pCS = (PCRITICAL_SECTION)pParam;
	//뮤텍스의 경우 HANLDE hMutex = (HANDLE)pParam;

	EnterCriticalSection(pCS);
	//EnterCriticalSection함수와 LeaveCriticalSection함수로 보호해야 할 리소스를 사용하는 코드부분을 감싸준다.
	//뮤텍스의 경유 -> WaitForSingleObject(hMutex, INFINITE);
	{
		SYSTEMTIME st;
		GetLocalTime(&st);

		cout << "..." << "SubThread " << GetCurrentThreadId() << " => ";
		cout << st.wYear << '/' << st.wMonth << '/' << st.wDay << ' ';
		cout << st.wHour << ':' << st.wMinute << ':' << st.wSecond << '+';
		cout << st.wMilliseconds << endl;
	}
	LeaveCriticalSection(pCS);

	return 0;
}

void _tmain(void)
{
	cout << "======= Start CriticalSection Test ========" << endl;
	CRITICAL_SECTION cs;
	//CRITICAL_SECTION 구조체 선언 , 여기서는 지역변수든 전역변수든 상관없다.

	InitializeCriticalSection(&cs);
	//크리티컬섹션 구조체 초기화
	//뮤텍스의 경우 HANDLE hMutex = CreateMutex(NULL,FALSE,NULL);
	
	HANDLE arhThreads[10];
	for (int i = 0; i < 10; i++)
	{
		DWORD dwTheaID = 0;
		arhThreads[i] = CreateThread(NULL, 0, ThreadProc, &cs, 0, &dwTheaID);
	}
	WaitForMultipleObjects(10, arhThreads, TRUE, INFINITE);

	DeleteCriticalSection(&cs);
	//최종적으로 크리티컬 섹션 삭제하고자 할때 사용하는 함수

	//뮤텍스의 경우 CloseHandle(hMutex);
	cout << "======= End CriticalSection Test ==========" << endl;

	
}

