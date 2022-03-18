#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include "Winsock2.h"
#include "iostream"
using namespace std;

#pragma comment(lib, "Ws2_32.lib")

void _tmain()
{
	WSADATA	wsd;
	int nErrCode = WSAStartup(MAKEWORD(2, 2), &wsd);
	if (nErrCode)
	{
		cout << "WSAStartup 실패 , 에러코드 : " << nErrCode << endl;
		return;
	}

	//socket->TCP 소켓 생성
	SOCKET hsoListen = socket(AF_INET, SOCK_STREAM, 0);
	if (hsoListen == INVALID_SOCKET)
	{
		cout << "소켓 연결 실패 : " << WSAGetLastError() << endl;
		return;
	}

	//bind->자신의 로컬 주소를 SOCKADDR_IN 구조체에 채워 리슨 소켓과 바인드 한다.
	SOCKADDR_IN	sa;
	memset(&sa, 0, sizeof(SOCKADDR_IN));		//bind 하기위에 connect함수와는 반대로 자신의 로컬 주소 및 접속할 포트 번호 지정
	sa.sin_family = AF_INET;					//
	sa.sin_port = htons(9001);					//
	sa.sin_addr.s_addr = htonl(INADDR_ANY);		//어떠한 주소라도 받아 들이는 의미

	LONG lSockRet = bind(hsoListen, (PSOCKADDR)&sa, sizeof(SOCKADDR_IN));
	if (lSockRet == SOCKET_ERROR)
	{
		cout << "바인드 실패 : " << WSAGetLastError() << endl;
		return;
	}
	
	//listen->클라이언트 접속을 리슨 상태로 만든다. 백로그(backlog)를 SOMAXCONN으로 설정하고, 접속 가능 백로그를 조절할 수 있다.
	//backlog : 들어오는 연결을 대기열에 둘 최대 숫자
	lSockRet = listen(hsoListen, SOMAXCONN);
	if (lSockRet == SOCKET_ERROR)
	{
		cout << "listen 실패 : " << WSAGetLastError() << endl;
		return;
	}
	cout << " ==> 클라이언트 접속 대기중......" << endl;


	//accept->클라이언트의 접속을 기다린다. 접속 요청이 있으면 수용하여 시스템이 생성해준 자식 소켓을 받아들인다.
	//클라이언트의 connect 접속이 있을 때까지 코드는 블록된다.
	//뒤의 두 매개변수에 SOCKADDR_IN 구조체의 포인터를 넘김으로써 접속된 클라이언트의 주소까지 휙득할 수 있다.
	//실질적인 클라이언트와의 송수신 작업은 자식 소켓이 담당하게 된다.
	SOCKET sock = accept(hsoListen, NULL, NULL);
	if (sock == INVALID_SOCKET)
	{
		cout << "accept 실패 : " << WSAGetLastError() << endl;
		return;
	}
	cout << " ==> 새로운 클라이언트 " << sock << " 가(이) 접속했다..." << endl;


	char szIn[512];
	while (true)
	{
		lSockRet = recv(sock, szIn, sizeof(szIn), 0);
		//recv->클라이언트로부터 전송된 데이터가 수신되기를 기다린다. 역시 클라이언트가 데이터를 send로 전송하기 전까진 블록
		//setsockopt 함수를 이용해서 타임아웃을 지정할 수 있다.
		if (lSockRet == SOCKET_ERROR)
		{
			cout << "수신 실패 : " << WSAGetLastError() << endl;
			break;
		}
		//수신 데이터의 길이가 0 이면 클라이언트가 접속을 끊었다는 의미이다.
		if (lSockRet == 0)
		{
			cout << " ==> 클라이언트 " << sock << " 접속종료..." << endl;
			break;
		}

		szIn[lSockRet] = 0;
		cout << " 받은 데이터 : " << szIn << endl;
		Sleep(10);

		//send->클라이언트에서 받은 문자열을 콘솔에 출력한 후 클라이언트에게 응답으로 받은 문자열을 그대로 송신(에코)
		lSockRet = send(sock, szIn, lSockRet, 0);
		if (lSockRet == SOCKET_ERROR)
		{
			cout << "send failed : " << WSAGetLastError() << endl;
			break;
		}
	}
	closesocket(sock);
	closesocket(hsoListen);
	//closesocket->accept를 통해서 받은 자식 소켓과 리슨을 위해 생성한 리슨 소켓을 프로그램 종료전에 닫아준다

	cout << "==== 서버 종료... ==========================" << endl;

	WSACleanup();
}