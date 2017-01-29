#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define BUFFER_SIZE 256
#define PLUTO_PORT 12345
#define LINUX_PORT 12345
#define END_SEC 5
#define USEC_SOCKET_TIMEOUT 100 
#define NUM 9999
#define DUMMY_BALL_FILE "../data/20170128/ball.dat"
#define XYZ 3
#define ADDITIONAL_TIME 1000

double ball_position[NUM][XYZ] = {}; 
int ball_time[NUM] ={};

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

  /*
  setsockopt( serverSocket,
	      SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

  socket_timeout.tv_sec  = 0;
  socket_timeout.tv_usec = USEC_SOCKET_TIMEOUT;
  
  setsockopt( serverSocket, 
	      SOL_SOCKET, SO_RCVTIMEO, (char*)&socket_timeout, sizeof( socket_timeout ) );
  */
  bind( serverSocket, (struct sockaddr *)&addr, sizeof(addr) );
  listen( serverSocket, 5 );

  return serverSocket;
}

int loadBall(void){
  FILE *fp;
  char str[BUFFER_SIZE];
  //unsigned int i = 0;
  int i = 0;
  char *tmp;

  fp = fopen( DUMMY_BALL_FILE, "r" );

  if (fp == NULL){
    printf("File open error (command)\n");
    return;
  }

  while ( fgets( str, BUFFER_SIZE, fp ) != NULL ){
    //printf( "%s\n", str );
    //printf( "%d ", i );
    tmp = strtok( str, " " );  ball_time[i]        = atoi( tmp ); //printf( "%s ", tmp );
    tmp = strtok( NULL, " " ); ball_position[i][0] = atof( tmp ); //printf( "%s ", tmp );
    tmp = strtok( NULL, " " ); ball_position[i][1] = atof( tmp ); //printf( "%s ", tmp );
    tmp = strtok( NULL, " " ); ball_position[i][2] = atof( tmp ); //printf( "%s ", tmp );
    //printf( "%d %lf %lf %lf\n", 
    //	    ball_time[i], ball_position[i][0], ball_position[i][1], ball_position[i][2] );
    i++;
  }
  
  fclose( fp );

  return ( i - 1 );
}



int main(int argc, char* argv[]){
  char buffer[BUFFER_SIZE];
  int i = 0;

  int ball_num = loadBall();
  
  /*
  //for ( i = 0; i < ball_num; i++ ){
  for ( i = 0; i < 20; i++ ){
    //if ( ball_time[i] < 1 && i > 1 )
    //break;
    sprintf( buffer, "%d %lf %lf %lf", 
	    ball_time[i], ball_position[i][0], ball_position[i][1], ball_position[i][2] );
    printf( "%d %lf %lf %lf\n", 
	    ball_time[i], ball_position[i][0], ball_position[i][1], ball_position[i][2] );
    printf( "%s\n", buffer );
  }
  */
  // time
  double time_ms;
  //double time_sec;
  struct timeval s, e;
  gettimeofday( &s, NULL );
  gettimeofday( &e, NULL );
  // get ip
  //char ip_pluto[BUFFER_SIZE];
  //getIPAddress( filename_ip_windows, ip_pluto );   
  // sockets
  
 
  struct sockaddr_in client;
  int len;
  //int clientSocket = makeClientSocket( ip_pluto, PLUTO_PORT );
  int serverSocket = makeServerSocket( LINUX_PORT );
  //int robotzSocket = -1;
  // connect to robot repeatedly
  //while (1) {
    // connect to a robot
    //if ( robotzSocket == -1 ){
  len = sizeof( client );
  int robotzSocket = accept( serverSocket, (struct sockaddr *)&client, &len );

  sprintf( buffer, "%s", "ready" );
  send( robotzSocket, buffer, BUFFER_SIZE, 0 );

  gettimeofday( &s, NULL );      

  while ( i < ball_num ){
   gettimeofday( &e, NULL );      
   time_ms = 1.0e+3*( e.tv_sec - s.tv_sec ) + 1.0e-3*( e.tv_usec - s.tv_usec );
   //if ( time_ms > i ){
   
   if ( time_ms > ball_time[i] + ADDITIONAL_TIME ){
     sprintf( buffer, "%d, %lf %lf %lf", 
	      ball_time[i], ball_position[i][0], ball_position[i][1], ball_position[i][2] );
     printf( "%s\n", buffer );
     //printf( "%d ", i );
     send( robotzSocket, buffer, BUFFER_SIZE, 0 );
     i++;
   }
   //if ( ball_time[i] < 1 )
   //break;
  }

  usleep(3e+6);

  //}
  // recieve buffer from NatNet
  //recv( clientSocket, buffer, BUFFER_SIZE, 0 );
  // send buffer to a robot
  //if ( robotzSocket > -1 ){
  //send( robotzSocket, buffer, BUFFER_SIZE, 0 );
  //gettimeofday( &e, NULL );
  // }
  // close connection to a robot
  //if (( e.tv_sec - s.tv_sec ) + 1.0e-6*( e.tv_usec - s.tv_usec ) > END_SEC ){ 
  //close( robotzSocket );
  //robotzSocket = -1;
  //}
  //}
  // close connection to NatNet
  close( robotzSocket );  
  close( serverSocket );
  
  return 0;
}
