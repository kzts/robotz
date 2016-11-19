
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include <unistd.h>

int main(int argc, char* argv[]){
  int i;

  if ( argc != 2 ){
    printf("input server ip address.\n");
    return -1;
  }
  char* ip_address = argv[1];

  int clientSocket;
  //char buffer[1024];
  char buffer[99999];
  struct sockaddr_in serverAddr;
  socklen_t addr_size;

  /*---- Create the socket. The three arguments are: ----*/
  /* 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) */
  clientSocket = socket(PF_INET, SOCK_STREAM, 0);
  
  /*---- Configure settings of the server address struct ----*/
  /* Address family = Internet */
  serverAddr.sin_family = AF_INET;
  /* Set port number, using htons function to use proper byte order */
  serverAddr.sin_port = htons(7891);
  /* Set IP address to localhost */
  //serverAddr.sin_addr.s_addr = inet_addr("192.168.2.247");
  serverAddr.sin_addr.s_addr = inet_addr(ip_address);
  /* Set all bits of the padding field to 0 */
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

  /*---- Connect the socket to the server using the address struct ----*/
  addr_size = sizeof serverAddr;
  connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);

  strcpy( buffer, "no" );


  i = 0;
  while ( strcmp( buffer, "END" ) != 0 ){
    /*---- Read the message from the server into the buffer ----*/
    recv( clientSocket, buffer, 1024, 0);
    
    if ( strcmp( buffer, "no" ) != 0 ){
      /*---- Print the received message ----*/
      //printf("Data received: %s",buffer);   
      printf("%s \n", buffer);
    }
    if ( i == 10 )
      send( clientSocket, "command: 0.00 0.450 0.333", 99, 0);
    else
      send( clientSocket, "no", 13, 0);
    i++;
  }
  return 0;
}
