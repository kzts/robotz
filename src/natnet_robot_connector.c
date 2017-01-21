#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 256
#define PLUTO_PORT 12345
#define LINUX_PORT 12345
#define END_SEC 5
#define USEC_SOCKET_TIMEOUT 100 

char filename_ip_windows[]  = "../data/params/ip_windows.dat";

void getIPAddress( char* filename, char* ip_address ){
  FILE *fp;
  fp = fopen( filename, "r" );
  if ( fp == NULL ){
    printf( "File open error: %s\n", filename );
    return;
  }
  fgets( ip_address, BUFFER_SIZE, fp );
  fclose(fp);
}

int makeClientSocket( char* ip_address, int port_num ){
  int clientSocket;
  struct sockaddr_in serverAddr;
  socklen_t addr_size;
  // Create the socket. The three arguments are:
  // 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) 
  clientSocket = socket(PF_INET, SOCK_STREAM, 0);
  //---- Configure settings of the server address struct
  serverAddr.sin_family = AF_INET; // Address family = Internet
  serverAddr.sin_port = htons(port_num);   // Set port number, using htons function to use proper byte order
  serverAddr.sin_addr.s_addr = inet_addr(ip_address); // Set IP address to localhost 
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero); // Set all bits of the padding field to 0 
  // Connect the socket to the server using the address struct
  addr_size = sizeof serverAddr;
  connect( clientSocket, (struct sockaddr *) &serverAddr, addr_size);

  return clientSocket;
}

int makeServerSocket( int port_num ){
  int serverSocket;
  struct sockaddr_in addr;
  int yes = 1;
  int ret;
  struct timeval socket_timeout;

  serverSocket         = socket( AF_INET, SOCK_STREAM, 0 );
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons( port_num );
  addr.sin_addr.s_addr = INADDR_ANY;

  setsockopt( serverSocket,
	     SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

  socket_timeout.tv_sec  = 0;
  socket_timeout.tv_usec = USEC_SOCKET_TIMEOUT;
  
  setsockopt( serverSocket, 
	      SOL_SOCKET, SO_RCVTIMEO, (char*)&socket_timeout, sizeof( socket_timeout ) );

  bind( serverSocket, (struct sockaddr *)&addr, sizeof(addr) );
  listen( serverSocket, 5 );

  return serverSocket;
}

int main(int argc, char* argv[]){
  // time
  double time_sec;
  struct timeval s, e;
  gettimeofday( &s, NULL );
  gettimeofday( &e, NULL );
  // get ip
  char ip_pluto[BUFFER_SIZE];
  getIPAddress( filename_ip_windows, ip_pluto );   
  // sockets
  char buffer[BUFFER_SIZE];
  struct sockaddr_in client;
  int len;
  int clientSocket = makeClientSocket( ip_pluto, PLUTO_PORT );
  int serverSocket = makeServerSocket( LINUX_PORT );
  int robotzSocket = -1;
  // connect to robot repeatedly
  while (1) {
    // connect to a robot
    if ( robotzSocket == -1 ){
      len = sizeof( client );
      robotzSocket = accept( serverSocket, (struct sockaddr *)&client, &len );
      gettimeofday( &s, NULL );      
    }
    // recieve buffer from NatNet
    recv( clientSocket, buffer, BUFFER_SIZE, 0 );
    // send buffer to a robot
    if ( robotzSocket > -1 ){
      send( robotzSocket, buffer, BUFFER_SIZE, 0 );
      gettimeofday( &e, NULL );
    }
    // close connection to a robot
    if (( e.tv_sec - s.tv_sec ) + 1.0e-6*( e.tv_usec - s.tv_usec ) > END_SEC ){ 
      close( robotzSocket );
      robotzSocket = -1;
    }
  }
  // close connection to NatNet
  close( serverSocket );
  return 0;
}
