#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

char filename_ip_robotz[]  = "../data/params/ip_robotz.dat";
char filename_ip_camera1[] = "../data/params/ip_camera1.dat";
char filename_ip_camera2[] = "../data/params/ip_camera2.dat";

#define NUM_BUFFER 255

//char ip_address[255];

int connectSocket( char* ip_address ){
  int clientSocket;
  struct sockaddr_in serverAddr;
  socklen_t addr_size;
  int port_num;

  /*---- Create the socket. The three arguments are: ----*/
  /* 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) */
  clientSocket = socket(PF_INET, SOCK_STREAM, 0);
  
  /*---- Configure settings of the server address struct ----*/
  /* Address family = Internet */
  serverAddr.sin_family = AF_INET;
  /* Set port number, using htons function to use proper byte order */
  //serverAddr.sin_port = htons(7891);
  serverAddr.sin_port = htons(port_num);
  /* Set IP address to localhost */
  //serverAddr.sin_addr.s_addr = inet_addr("192.168.2.247");
  serverAddr.sin_addr.s_addr = inet_addr(ip_address);
  /* Set all bits of the padding field to 0 */
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

  /*---- Connect the socket to the server using the address struct ----*/
  addr_size = sizeof serverAddr;
  connect( clientSocket, (struct sockaddr *) &serverAddr, addr_size);

  return clientSocket;
}

void getIPAddress( char* filename, char* ip_address ){
  FILE *fp;
  fp = fopen( filename, "r" );
  if ( fp == NULL ){
    printf( "File open error: %s\n", filename );
    return;
  }
  fgets( ip_address, NUM_BUFFER, fp );
  fclose(fp);
}

int main()
{
  // get IP address
  char ip_robotz[NUM_BUFFER], ip_camera1[NUM_BUFFER], ip_camera2[NUM_BUFFER];
  getIPAddress( filename_ip_robotz,  ip_robotz );   
  getIPAddress( filename_ip_camera1, ip_camera1 );   
  getIPAddress( filename_ip_camera2, ip_camera2 );   

  printf( "ip address: %s (robotz), %s (camera1), %s (camera2)\n", ip_robotz, ip_camera1, ip_camera2 );


  /*

  // server 
  int sock1, sock2;
  struct sockaddr_in addr1, addr2;
  fd_set fds, readfds;
  char buf[2048];

  // ファイルディスクリプタの最大値を計算します。selectで使います 
  int maxfd;

  // 受信ソケットを2つ作ります
  sock1 = socket(AF_INET, SOCK_DGRAM, 0);
  sock2 = socket(AF_INET, SOCK_DGRAM, 0);

  addr1.sin_family = AF_INET;
  addr2.sin_family = AF_INET;

  //addr1.sin_addr.s_addr = inet_addr("127.0.0.1");
  //addr2.sin_addr.s_addr = inet_addr("127.0.0.1");
  addr1.sin_addr.s_addr = inet_addr("192.168.2.204");
  addr2.sin_addr.s_addr = inet_addr("192.168.2.204");

  // 2つの別々のポートで待つために別のポート番号をそれぞれ設定します
  addr1.sin_port = htons(11111);
  addr2.sin_port = htons(22222);

  // 2つの別々のポートで待つようにbindします
  bind(sock1, (struct sockaddr *)&addr1, sizeof(addr1));
  bind(sock2, (struct sockaddr *)&addr2, sizeof(addr2));

  // fd_setの初期化します
  FD_ZERO(&readfds);

  // selectで待つ読み込みソケットとしてsock1を登録します
  FD_SET(sock1, &readfds);
  // selectで待つ読み込みソケットとしてsock2を登録します
  FD_SET(sock2, &readfds);

  // selectで監視するファイルディスクリプタの最大値を計算します
  if (sock1 > sock2) {
    maxfd = sock1;
  } else {
    maxfd = sock2;
  }

  //
  無限ループです
  このサンプルでは、この無限ループを抜けません
  //
  while (1) {
    // 読み込み用fd_setの初期化
  // selectが毎回内容を上書きしてしまうので、毎回初期化します
    memcpy(&fds, &readfds, sizeof(fd_set));

    // fdsに設定されたソケットが読み込み可能になるまで待ちます
    // 一つ目の引数はファイルディスクリプタの最大値＋１にします
    select(maxfd+1, &fds, NULL, NULL, NULL);

    // sock1に読み込み可能データがある場合
    if (FD_ISSET(sock1, &fds)) {
      // sock1からデータを受信して表示します
      memset(buf, 0, sizeof(buf));
      recv(sock1, buf, sizeof(buf), 0);
      printf("%s\n", buf);
    }

    // sock2に読み込み可能データがある場合
    if (FD_ISSET(sock2, &fds)) {
      // sock2からデータを受信して表示します
      memset(buf, 0, sizeof(buf));
      recv(sock2, buf, sizeof(buf), 0);
      printf("%s\n", buf);
    }
  }

  // このサンプルでは、ここへは到達しません
  close(sock1);
  close(sock2);
  */
  return 0;
}



