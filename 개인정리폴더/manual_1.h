//https://popcorntree.tistory.com/80?category=813524 에서 자료를 찾음
//
//iocp준비
//
//이 헤더는 그냥 메모장에는 큰 개요만 적어둔거라 내가 볼려고 만든 설명서임!
//아니 무슨 IOCP는 내가 가지고 있는 책에 설명이 잘 없넹.....
//
/*
HANDLE CreateIoComplitionPort	커널 객체를 만들어 주는 함수
(
	HANDLE FileHandle, 
	HANDLE ExistingCompletionPort, 
	ULONG_PTR CompletionKey, 
	DWORD NumberOfConcurrentThreads
);
*/
//IOCP 커널 객체 생성 (마지막 0 : CORE 개수 만큼 사용)
/*
HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0};

CreateIoCompletionPort 커널 객체를 만들어주는 함수
파라미터들을 유의하자.

1번째 파라미터
FileHandle 
파일 핸들과 소켓을 같게 취급하니 같다 볼수도 있긴하다.
하지만 특이한 점이 있다.
IOCP 객체만들때 핸들 파라미터에 반드시 INVALID_HANDLE_VALUE를 넣어줘야 한다. 0도 안되고 LISTEN소켓도 안된다. 그래야지 IOCP 객체를 생성한다

2번째 파라미터
ExistingCompletionPort
이것역시 특이하다. 이름만 보면 존재하는 IOCP핸들을 넣어줘야하는것처럼 생겼다. 그렇지만 NULL을 넣어준다.

3번째 파라미터
CompletionKey
마찬가지다. 무슨키를 넣는지에 상관없이 Null을 넣어준다.

4번째 파라미터
NumberOfConcurrentThreads
IOCP 객체가 동시에 컨트롤 할 수 있는 스레드의 개수이다
0을 넣으면, 알아서 모든 코어를 풀로 활용하라는 의미다. 스레드를 하나도 안 쓰겠다는 말이 아니라 코어의 개수만큼 쓰겠다는 것이다.
일부러 코어갯수보다 작은 값을 넣을 것 아니면 0을 넣어주도록 하자
*/



//IOCP 객체와 소켓 연결
/*
HANDLE CreateIoCompletionPort(socket, hIOCP, key, 0};

두번째 해야할 것은 IOCP 객체를 만들면 그걸 통해 SEND RECV 해야 한다. 그러려면 소켓을 IOCP객체에 등록해야 한다.
그런데 함수 이름이 아까 위에서 본 함수랑 같다..... 메모장에서 본 주의사항을 신경쓰자.

1번째 파라미터로 소켓을 받는다. 이 소켓을 IOCP에 연결하겠다는 것이다.
2번째 인자의 IOCP에 1번째 인자로 받은 소켓을 연결하라는 이야기 이다.

3번째 파마미터
key
이 소켓을 어떤 숫자로 , 어떤 이름으로 사용할지 묻는것이다.
IOCP를 통해 I/O을 할때는 소켓을 주는게 아니라 key를 주는 형태다. 등록 하고 난 뒤에 key를 통해 액세스를한다
키 값은 프로그래머가 정하기 나름이며 유의해야 할 점은 반드시 unique하게 정해줘야 한다.

4번째 파라미터
마지막 인자는 무시한다. iocp를 만들때가 아닌 소켓을 연동하는 방식일 때는 마지막 파라미터에 무슨 값을 넣어도 무시된다.
일관성을 위해 0으로 지정해주자.
*/



//Worker Thread 생성
/*
thread { worker_thread };
멀티 스레드로 동작하기 위해서는 스레드를 만들어 IOCP객체에 등록해야 하는데 그 API는 따로 없다.
고로 스레드 생성은 프로그래머가 알아서 하고 나중에 등록하는 과정을 거쳐야 한다.
꼭 멀티스레드 프로그래밍으로 만들 이유는 없지만 고성능 프로그램을 위해서는 멀티스레그 프로그래밍이 필수다.
애초애 IOCP가 성능이 좋은것도 멀티스레드를 사용하기 때문이다.
하지만 처음 실습을 할때는 싱글스레드(메인함수)로 먼저 테스트를 하는것이 좋다.
*/



//IOCP 완료 검사
/*
BOOL GetQueuedCompletionStatus( 
	HANDLE CompletionPort, 
	LPDWORD lpNumberOfByte, 
	PULONG_PTR lpCompletionKey, 
	LPOVERLAPPED *lpOverlapped, 
	DWORD dwMillisocnds 
);

while(true) { 
	GetQueuedCompletionStatus( 
		hIOCP, 
		&dwIOSize, 
		&key, 
		&lpOverlapped, 
		INFINITE;
	); 
		lpOverlapped에 따른 처리; 
}

IOCP로 Overlapped 할때 완료검사를 하는 함수이다.
GetQueuedCompletionStatus 호출을 하면 멈춰있다가 I/O가 완료되면 함수가 종료하고 완료 정보들을 파라미터에 넣어준다. 그후 이걸 가지고 I/O처리를 한다.
게임서버 메인루프가 돌아가면 이 함수를 호출하고 , 기다리다가 정보가 오면 처리를 하고 다시 기다리고, 계속 루프를 돌면서 처리를 하면 된다.
사실 이 함수가 스레드를 IOCP 객체에 등록하는 함수다. 우린 스레드를 등록하지 않았지만 메인 함수에서 이 함수를 호출하면 메인이 스레드에 등록되는 것이다.


1번째 파라미터
IOCP객체 , 소켓과 연동을 마친 IOCP객체 핸들을 넣어주면 된다.

2번째 파라미터
전송된 데이터의 양
이 함수는 SEND RECV ACCEPT의 완료를 알려주는 함수이다. 그래서 완료가 되면 몇바이트가 완료가 됐는지 알려줘야한다. 몇바이트가 전송됐는지 정보를 얻을 수 있다.

3번째 파라미터
미리 정해놓은 ID를 알려준다.
I/O가 완료되면 여러개의 I/O가 중첩되어 실행되고 누군가 완료되었나 이 파라미터로 알 수 있다.

4번째 파라미터
Overlapped I/O 구조체 를 의미한다.
Overlapped I/O 할때 그 구조체를 반드시 써야한다.
구조체로 리턴해주고 이것으로 여러 정보를 얻을 수 있다. 정보가 없는데 여러가지를 얻을수 있냐는 소리는 뒤에서 설명을 한다.

5번째 파라미터
이 함수의 TIMEOUT 이다.
INFINITE를 써주면 IOCP에 등록된 IO가 하나라도 등록될때까지 무한루프에 빠진다. 여기에 적절한 시간을 써주면 그 시간이 지나면 그냥 리턴한다.
그럼 TIMEOUT으로 리턴했다고 에러코드가 뜬다 그러니 리턴값인 BOOL은 에러인가 아닌가를 알려준다. 파라미터가 잘못되었을 때도 에러가 난다.
*/



//IOCP 이벤트 추가
/*
BOOL PostQueuedCompletionStatus(
	HANDLE CompletionPort, 
	DWORD NumberOfByte, 
	ULONG_PTR dwCompletionKey, 
	LPOVERLAPPED lpOverlapped 
);
용도 : IOCP를 사용할 경우 IOCP가 메인 루프가 되기 때문에 socket I/O 이외에도 모든 다른 작업할 내용을 추가 할 때 쓰인다.

IOCP 객체에게 완료를 하나 전달해 주는 것이다.
네트워크 I/O에는 쓰질 않는데 게임서버 돌릴때는 스레드 관리에 유용하게 사용한다.

*/


/*
놀랍게도 iocp와 관련된 API는 위에서 다룬 3개가 전부다. 게다가 마지막에 있는 이벤트 추가함수는 쓰이지 않는 경우도 많다.
iocp가 상당히 복잡한 API라고 했음에도 달랑 2개뿐이다. 그래서 어려운 것이다. 고작 두개로 모든 복잡한 동작을 할려 하니 프로그래밍하기 어려운것이다.
send 나 recv는 기존의 것을 쓴다. 마찬가지로 기존의 overlapped API를 그대로 스며 위의 iocp함수가 추가되는것이다.
*/