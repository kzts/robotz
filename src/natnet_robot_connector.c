
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 256
//#define WINDOWS_PORT 12345
#define PLUTO_PORT 12345
#define LINUX_PORT 12345
//#define END_SEC 3

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

int connectSocket( char* ip_address, int port_num ){
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

int main(int argc, char* argv[]){
  //int i;
  //double time_sec;
  //struct timeval s, e;

  // get IP address
  char ip_pluto[BUFFER_SIZE];
  getIPAddress( filename_ip_windows, ip_pluto );   



  int clientSocket = connectSocket( ip_pluto, PLUTO_PORT );
  //int clientSocket;
  //char buffer[1024];
  char buffer[99999];
  
  //char buffer_send[1];
  //struct sockaddr_in serverAddr;
  //socklen_t addr_size;

  /*---- Create the socket. The three arguments are: ----*/
  /* 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) */
  clientSocket = socket(PF_INET, SOCK_STREAM, 0);
  
  /*---- Configure settings of the server address struct ----*/
  /* Address family = Internet */
  //serverAddr.sin_family = AF_INET;
  /* Set port number, using htons function to use proper byte order */
  //serverAddr.sin_port = htons(WINDOWS_PORT);
  /* Set IP address to localhost */
  //serverAddr.sin_addr.s_addr = inet_addr("192.168.2.247");
  //serverAddr.sin_addr.s_addr = inet_addr(ip_address);
  /* Set all bits of the padding field to 0 */
  //memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

  /*---- Connect the socket to the server using the address struct ----*/
  //addr_size = sizeof serverAddr;
  //connect( clientSocket, (struct sockaddr *) &serverAddr, addr_size);
  

  //gettimeofday( &s, NULL );
  while (1){
    //sprintf( buffer_send, "%d", 1 );
    //send( clientSocket, buffer_send, 1, 0 );

    /*---- Read the message from the server into the buffer ----*/
    //recv( clientSocket, buffer, 1024, 0 );
    recv( clientSocket, buffer, BUFFER_SIZE, 0 );
    
    /*---- Print the received message ----*/
    printf("Data received: %s\n",buffer);

    //gettimeofday( &e, NULL );
    //printf("time = %lf\n", (e.tv_sec - s.tv_sec) + (e.tv_usec - s.tv_usec)*1.0E-6);
    //time_sec = e.tv_sec - s.tv_sec;

    //if( time_sec > END_SEC )
    //break;
  }
  return 0;
}
