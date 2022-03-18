#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include "Winsock2.h"
#include "iostream"
using namespace std;

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable:4996) // C4996 에러를 무시

void _tmain()
{
	WSADATA	wsd;
	int nErrCode = WSAStartup(MAKEWORD(2, 2), &wsd);
	if (nErrCode)
	{
		cout << "WSAStartup 실패 , 에러코드 : " << nErrCode << endl;
		return;
	}

	//IPv4 , TCP 소켓 생성
	SOCKET sock = socket
	(
		AF_INET,	// IPv4 주소기반
		SOCK_STREAM,// TCP기반의 연결지향 프로토콜
		IPPROTO_TCP	// TCP기반
	); 
	if (sock == INVALID_SOCKET)
	{
		cout << "소켓 생성 실패, 에러코드 : " << WSAGetLastError() << endl;
		return;
	}

	//접속할 서버의 IP 주소와 포트 번호를 지정
	SOCKADDR_IN	sa;
	memset(&sa, 0, sizeof(SOCKADDR_IN));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(9001);						//사용할 포트
	sa.sin_addr.s_addr = inet_addr("127.0.0.1");	//목적지 주소정보 , 테스트를 위해 루프 백 주소 사용, 자신의 컴퓨터를 의미

	int lSockRet = connect(sock, (LPSOCKADDR)&sa, sizeof(sa));	//목적지 주소정보가 담긴 SOCKADDR_IN 구조체를 이용
	if (lSockRet == SOCKET_ERROR)
	{
		cout << "연결 실패 : " << WSAGetLastError() << endl;
		closesocket(sock);
		return;
	}
	cout << "==> 서버 연결 성공!!!" << endl;


	int tranLen;
	char szIn[512];
	while (true)
	{
		cin >> szIn;
		if (_stricmp(szIn, "quit") == 0)
			break;

		tranLen = strlen(szIn);
		//send -> 콘솔로부터 입력받은 문자열을 서버로 전송한다.
		//송신에 성공시 보낸 데이터의 길이를 반환 , 바이트 단위
		tranLen = send
		(
			sock,		//연결된 소켓
			szIn,		//보낼 데이터가 담긴 버퍼
			tranLen,	//전송할 데이터의 바이트 길이
			0			//비트 프래그, 게임 코드에서는 대개 0으로 사용
		);
		if (tranLen == SOCKET_ERROR)
		{
			cout << "전송 실패 : " << WSAGetLastError() << endl;
			break;
		}

		Sleep(10);

		//recv->서버로부터 전송될 에코 응답 데이터의 수신을 기다린다. 서버가 데이터를 송신하기 전까지 코드를 블록된다.
		//수신에 성공시 데이터의 길이를 반환 , 바이트 단위
		tranLen = recv
		(
			sock,			//연결된 소켓
			szIn,			//수신받을 버퍼의 포인터
			sizeof(szIn),	//수신받을 버퍼의 크기
			0				//비트 프래그, 게임 코드에서는 대개 0으로 사용
		);
		if (tranLen <= 0)	//수신 실패시
		{
			cout << "수신 실패 : " << WSAGetLastError() << endl;
			break;
		}
		if (tranLen == 0)				//서버측에서 연결을 끊었을때
		{
			cout << "==> Disconnected from server!!!" << endl;
			break;
		}
		szIn[tranLen] = 0;
		cout << " *** Received : " << szIn << endl;
	}
	//소켓을 닫아 접속을 끊고 소켓 커널 객체를 해제
	closesocket(sock);
	cout << "==> 소켓 닫기, 프로그램 종료..." << endl;


	WSACleanup();
}