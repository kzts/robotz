
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#define WINDOWS_PORT 12345

int main(int argc, char* argv[]){
  int i;

  //char* ip_address = "192.168.2.246";
  char* ip_address = "192.168.2.56";

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
  serverAddr.sin_port = htons(WINDOWS_PORT);
  /* Set IP address to localhost */
  //serverAddr.sin_addr.s_addr = inet_addr("192.168.2.247");
  serverAddr.sin_addr.s_addr = inet_addr(ip_address);
  /* Set all bits of the padding field to 0 */
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

  /*---- Connect the socket to the server using the address struct ----*/
  addr_size = sizeof serverAddr;
  connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);

  while (1){
    /*---- Read the message from the server into the buffer ----*/
    recv( clientSocket, buffer, 1024, 0);
    
    /*---- Print the received message ----*/
    printf("Data received: %s\n",buffer);   
  }
  return 0;
}
