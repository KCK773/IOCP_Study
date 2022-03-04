//IOCP API
/*
1.CreateIoCompletionPort();
    1.IOCP 객체 생성
    2.Socket을 IOCP에 연결
2.GetQueuedCompletionStatus();
    1.Thread를 IOCP의 thread-pool에 등록하고 멈춤
    2.IOCP로 부터 I/O 결과를 얻어옴
*/

//IOCP 서버 설계(싱글 쓰레드 버전)
/*
1.초기화
    1.bind , listen
    2.IOCP 핸들 생성

2.Listen 소켓 IOCP에 등록 후 AcceptEx 호출

3.서버 메인 루프
    1.새로운 소켓이 접속하면 IOCP에 연결후 WSARecv 호출
    2.WSARecv가 완료 되면 패킷 처리
    3.WSASend가 완료 되면 자료구조 정리
*/


//Accept 처리
/*
1.AccpetEx 함수를 사용해서 비동기로 처리
    1.Listen Socket을 IOCP에 등록 후 AcceptEx 호출
2.무한 루프를 돌면서
    1.새 클라이언트가 접속햇으면 클라이언트 객체를 만든다
    2.IOCP에 소켓을 등록한다
        1.send/recv가 IOCP를 통해 수행됨
    3.WSARecv()를 호출한다
        1.Overlapped I/O recv상태를 항상 유지해야 한다.
    4.AcceptEx()를 다시 호출한다
*/


//클라이언트 객체
/*
1.서버는 클라이언트의 정보를 갖고 잇는 객체가 필요하다.
    1.최대 동접과 같은 개수가 필요
    2.필요한 정보 : ID, 네트워크 접속 정보(socket , buffer 등) , 상태, 게임정보 (name, HP , x, , y 등등)
2.GetQueuedCompletionStatus 를 받았을 때 클라이언트 객체를 찾을 수 있어야 한다
    1.IOCP에서 오고 가는 것은 completion_key와 overlapped I/O pointer, number of byte 뿐
    2.Completion_key를 클라이언트 객체의 포인터로 하거나 클라이언트 객체의 ID 혹은 index로 한다.
*/


//Overlapped 구조체
/*
1.모든 send recv는 Overlapped 구조체가 필요
2.하나의 구조체를 동시에 여러 호출에서 사용한는 것은 불가능
3.소켓당 recv호출은 무조건 한개
    1.recv 호출용 Overlapped 구조체가 한 개가 있어서 계속 재사용 하는 것이 바람직(new/delete overhead 제거)
4.소켓당 send호출은 동시에 여러개가 될 수 있다.
    1.send 버퍼도 같은 개수가 필요하다
    2.개수의 제한이 없으므로 new/delete 로 사용
        1.send 할때 new, send가 complete되었을 때 delete
    3.성능을 위해서는 공유 pool을 만들어서 관리할 필요가 있다.
*/


//Overlapped I/O pointer를 확장
/*
1.overlapped I/O 구조체 자체는 쓸만한 정보가 없다
2.따라서 정보들을 더 추가할 필요가 있다
    1.뒤에 추가하면 IOCP는 있는지 없는지도 모르고 에러도 나지 않는다(pointer만 왔다갔다 하므로)
3.꼭 필요한 정보
    1.지금 이 I/O가 send인지 recv인지
    2.I/O buffer의 위치 (send 할때 버퍼도 같이 생성되므로)

골치 아픈 구간
우리는 Overlapped I/O구조체가 반드시 필요하다. 근데 이 구조체 자체는 쓸만한 정보가 없다.
이 구조체에 무슨 정보를 넣어서 사용해야하나?
IOCP가 완료됐다고 하자. 그럼 GetQueuedCompletionStatus 함수에서 주는 정보만으로 처리를 해야하는데 이때 key랑 바이트 수가 온다.
key로 어떤 소켓이 I/O를 했는지 알 수 있는데 문제는 send인지 recv인지 뭐가 완료된것인지 알 수 없다.
그래서 그 정보를 추가로 전달해야한다. 그래서 어쩔수 없이 overlapped 구조체를 넣어야하는데,그 안에는 정보를 넣을 수 없다.
그래서 구조체에 정보를 덧붙여서 전달해야 한다. 이걸 확장이라 한다.

꼭 필요한 정보가 send인지 recv인지 구분하는 정보가 필요하고, 또 하나더 버퍼의 위치가 필요하다.
recv : 버퍼가 하나만 존재, 클라이언트 구조체에 recv버퍼를 넣을테니 괜찮음
send : send에 할당한 버퍼의 위치를 알아야 delete든 재사용이든 할수 있으므로 위치를 같이 넣어줌
*/


//완료처리
/*
1.GetQueuedCompletionStatus를 부른다.
    1.에러처리 / 접속 종료 처리를 한다
    2.send recv accept 처리를 한다
        1.확장 Overlapped I/O 구조체를 유용하게 사용한다
        2.Recv
            1.패킷이 다 왔나 검사 후 다 왔으면 패킷 처리
            2.여러 개의 패킷이 한번에 왔을 때 처리
            3.계속 Recv 호출
        3.Send
            1.overlapped 구조체, 버퍼의 free (혹은 재사용)
        4.Accept
            1.새 클라이언트 객체 등록
*/


//버퍼 관리
/*
1.Recv
    1.하나의 소켓에 대헤 Recv호출은 언제나 하나이기 때문에 하나의 버퍼를 계속 사용할 수 있다.
    2.패킷들이 중간에 잘려진 채로 도착할 수 있다
        1.모아 두었다가 다음에 온 데이터와 붗여주어야 한다
        2.남은 데이터를 저장해 두는 버퍼 필요, 또는 Ring Buffer를 사용할 수도 있다
    3.패킷들이 여러 개 한꺼번에 도착할 수 있다
        1.잘라서 처리해야 한다

1.Send
    1.하나의 Socket에 여러 개의 send를 동시에 할 수 있다
        1.다중 접속.broadcasting
        2.overlapper구조체과 wsabuf는 중복 사용 불가능
    2.windows는 send를 실행한 순서대로 내부 버퍼에 넣어놓고 전송한다
    3.내부 버퍼가 차서 send가 중간에 잘렸다면?
        1.나머지를 다시 보내면 된다
        2.다시 보내기 전 다른 쓰레드의 send가 끼어들었다면?
        3.해결책
            1.모아서 차례 차례 보낸다.send 데이터 버퍼 외에 패킷 저장 버퍼를 따로 둔다(성능저하)
            2.또는 이런 일이 벌어진 소켓을 끊어버린다
*/
