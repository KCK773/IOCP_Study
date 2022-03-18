#include <stdio.h>
#include <tchar.h>
#include <SDKDDKVer.h>
#include "Windows.h"
#include <iostream>

using namespace std;

//이벤트는 통지를 위한 대표적인 커널 객체다.
//보통의 예로 스레드 종료를 위해 이벤트를 사용한다거나 
//뮤텍스나 세마포어처럼 공유자원을 위한 경쟁 조정의 경우 역시 통지의 일환으로 볼 수 있다.
// 
//여기서의 예제는 "스레드 간 데이터 전달"을 위한 이벤트의 사용 예를 보여준다.
//시나리오는 메인 함수에서 콘솔로부터 입력받아 공유 버퍼를 통해 서브 스레드에게 그 내용을 전달해 주는 것이다.

#define CONSUMER_CNT    4    //공유버퍼를 읽는 스레드의 갯수
#define CMD_NONE    0
#define CMD_STR     1   //문자열
#define CMD_POINT   2   //POINT 구조체
#define CMD_TIME    3   //SYSTEMTIME 구조체
#define CMD_EXIT    100 //종료 명령

struct WAIT_ENV
{
    HANDLE _hevSend;        //쓰기 완료 통지용 이벤트
    HANDLE _hevResp;        //읽기 완료 통지용 이벤트
    LONG    _readCnt;       //사용계수 관리를 위한 멤버 필드
    BYTE    _arBuff[256];   //공유 버퍼
};

DWORD WINAPI WorkerProc(LPVOID pParam)
{
    WAIT_ENV* pwe = (WAIT_ENV*)pParam;
    DWORD dwThrId = GetCurrentThreadId();

    while (true)
    {
        DWORD dwWaitCode = WaitForSingleObject(pwe->_hevSend, INFINITE);
        //쓰기 완료 통지용 이벤트에 대해 대기한다. 종료 통지는 CMD_EXIT 명령으로 별도로 전달된다.

        if (dwWaitCode == WAIT_FAILED)
        {
            cout << "~~~ WaitForSingleObject failed : " << GetLastError() << endl;
            break;
        }

        PBYTE pIter = pwe->_arBuff;
        LONG ICmd = *((PLONG)pIter);
        pIter += sizeof(LONG);
        if (ICmd == CMD_EXIT)
            break;
        //공유 버퍼에서 명령을 읽어들여 종료 명령이면 바로 루프를 탈출한다.

        LONG ISize = *((PLONG)pIter);
        pIter += sizeof(LONG);
        PBYTE pData = new BYTE[ISize + 1];
        memcpy(pData, pIter, ISize);
        //공유버퍼에서 데이터 크기를 얻고 그 크기만큼 임시 버퍼를 할당한 후 데이터 부분을 복사한다.
        //본 코드에서 ISize+1 만큼의 버퍼를 할당한 이유는 문자열일 경우 마지막 NULL 문자를 지정하기 위한 처리다.

        ResetEvent(pwe->_hevSend);
        if (InterlockedIncrement(&pwe->_readCnt) == CONSUMER_CNT)
        {
            //InterlockedIncrement함수는 _readCnt++ 연산을 원자적으로 수행해주는 컴파일러 내장함수
            pwe->_readCnt = 0;
            SetEvent(pwe->_hevResp);
        }
        //_hevSend(쓰기완료통지) 이벤트를 리셋해주어 넌시그널 상태로 바꿔준다.
        //공유 버퍼에서 데이터를 모두 읽어오면 메인 스레드로 하여금 다음 명령을 수신할 수 있도록 읽기 완료 통지용 이벤트를 시그널링 한다.
        //그리고 명령 처리 스레드는 읽어들인 데이터에 대해 명령 종류별로 처리를 수행한다.이 시점부터 메인 스레드의 명령 수신 처리와
        //본 스레드의 이미 수신된 명령에 대한 작업 처리가 동시에 수행된다.

        switch (ICmd)
        {
            case CMD_STR:
            {
                //문자열 데이터에 대한 명령 처리
                pData[ISize] = 0;
                 printf(" <==R-TH %d read STR : %s \n", dwThrId, pData);
            }
            break;

            case CMD_POINT:
            {
                 //POINT 구조체 형에 대한 명령 처리
                 PPOINT ppt = (PPOINT)pData;
                 printf(" <==R-TH %d read POINT : (%d, %d)\n", dwThrId, ppt->x, ppt->y);
            }
            break;

            case CMD_TIME:
            {
                //SYSTEMTIME 구조체 형에 대한 명령 처리
                PSYSTEMTIME pst = (PSYSTEMTIME)pData;
                printf("  <== R-TH %d read TIME : %04d-%02d-%02d %02d:%02d:%02d+%03d\n",
                    dwThrId, pst->wYear, pst->wMonth, pst->wDay, pst->wHour,
                    pst->wMinute, pst->wSecond, pst->wMilliseconds);
            }
            break;
        }
        delete[] pData;
        //공유 버퍼로부터 데이터를 복사한 임시 버퍼를 삭제한다.
    }
    cout << "***WorkerProc Threadexits..." << endl;

    return 0;
}

void _tmain()
{
    cout << "======= EventNotify 테스트 시작! ========" << endl;

    WAIT_ENV we;
    we._hevSend = CreateEvent(NULL, TRUE, FALSE, NULL);
    we._hevResp = CreateEvent(NULL, FALSE, FALSE, NULL);
    //쓰기 완료 이벤트와 읽기 완료 이벤트를 각각 수동,자동 리셋 이벤트로 생성한다.
    we._readCnt = 0;

    DWORD dwThrID;
    HANDLE harThrCsmrs[CONSUMER_CNT];
    for (int i = 0; i < CONSUMER_CNT; i++)
    {
        harThrCsmrs[0] = CreateThread(NULL, 0, WorkerProc, &we, 0, &dwThrID);
    }
    //스레드를 생성하고 WAIT_ENV의 포인터를 매개변수로 넘긴다.
    //공유 버퍼를 읽는 스레드 4개이다

    char szIn[512];
    while (true)
    {
        cin >> szIn;
        if (_stricmp(szIn, "quit") == 0)
            break;
        //콘솔로부터 입력받은 문자열이 "quit"일경우 루프 탈출

        LONG lCmd = CMD_NONE, lSize = 0;
        PBYTE pIter = we._arBuff + sizeof(LONG) * 2;
        //공유 버퍼 구성을 위해서 데이터를 먼저 채우기 위한 포인터를 지정한다.

        if (_stricmp(szIn, "time") == 0)
        {
            SYSTEMTIME st;
            GetLocalTime(&st);
            memcpy(pIter, &st, sizeof(st));
            lCmd = CMD_TIME, lSize = sizeof(st);
            //"time"인 경우 현재 시간을 구해서 공유버퍼에 복사하고, 명령 종류와 SYSTEMTIME 구조체의 크기를 설정한다.
        }
        else if (_stricmp(szIn, "point") == 0)
        {
            POINT pt;
            pt.x = rand() % 1000; pt.y = rand() % 1000;
            *((PPOINT)pIter) = pt;
            lCmd = CMD_POINT, lSize = sizeof(pt);
            //"point"인 경우 현재 난수를 통해 x,y좌표를 구해서 POINT 구조체에 대입한 후 공유 버퍼에 쓴다.
            //또한 명령 종류와 POINT구조체의 크기를 설정한다.
        }
        else
        {
            lSize = strlen(szIn);
            memcpy(pIter, szIn, lSize);
            lCmd = CMD_STR;
            //문자열인 경운 문자열의 길이를 얻어 그 길이만큼 공유 버퍼에 복사한 후 CMD_STR명령을 설정한다
        }
        ((PLONG)we._arBuff)[0] = lCmd;
        ((PLONG)we._arBuff)[1] = lSize;
        //공유 버퍼의 맨 앞 부분에 명령 종류와 데이터의 길이를 설정한다. 
        //그후 공유 버퍼 구성이 마무리되었으므로 완료통지를 한다.

        SignalObjectAndWait(
            we._hevSend,    //해당 객체를 시그널 상태로 만든다.
            we._hevResp,    //이 객체에 대해 시그널 상태가 될 떄까지 대기상태로 들어간다.
            INFINITE,       //타임아웃 지정
            FALSE           //경보가능 대기 상태를 설정할 것인지의 여부
        );
        //쓰기 완료 통지와 더불어 읽기 완료 통지를 위해 대기한다.
        //SignalObjectAndWait()을 사용하지 않는다면 SetEvent와 WaitForSingleObject함수 두개를 연달아 써야한다.
    }
    *((PLONG)we._arBuff) = CMD_EXIT;
    SetEvent(we._hevSend);
    WaitForMultipleObjects(CONSUMER_CNT, harThrCsmrs, TRUE, INFINITE);
    //명령 처리 스레드 자체가 종료되기를 기다림

    CloseHandle(we._hevResp);
    CloseHandle(we._hevSend);
    cout << "======= End EventNotify Test ==========" << endl;
}
