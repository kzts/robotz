
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define NUM_BUFFER 1024
#define NUM 99999
#define XYXY 4

struct timeval ini_t, now_t;

char buffer1[NUM_BUFFER];
char buffer2[NUM_BUFFER];

char   filename_ball[NUM];
int    ball_positions[NUM][XYXY];
double data_time[NUM];

/*
void setBallPositions(int i){
  sscanf( buffer1, "%d %d", &ball_positions[i][0], &ball_positions[i][1] );
  sscanf( buffer2, "%d %d", &ball_positions[i][2], &ball_positions[i][3] );
}
*/

void getFileName(void){
  time_t timer;
  struct tm *local;
  struct tm *utc;

  timer = time(NULL);
  local = localtime(&timer);

  int year   = local->tm_year + 1900;
  int month  = local->tm_mon + 1;
  int day    = local->tm_mday;
  int hour   = local->tm_hour;
  int minute = local->tm_min;
  int second = local->tm_sec;

  sprintf( filename_ball, "../data/ball/%04d%02d%02d/%02d_%02d_%02d.dat", 
	   year, month, day, hour, minute, second );
  printf("save file: %s\n", filename_ball );
}

void saveDat( int num ){
  printf("save init: %s\n", filename_ball );
  int i,j,k;
  FILE *fp_res;
  char str[NUM_BUFFER];

  fp_res = fopen( filename_ball, "w" );

  if (fp_res == NULL){
    printf("File open error (results)\n");
    return;
  }

  for (i = 0; i < num; i++){
    sprintf( str, "%8.3f ", data_time[i]);
    fputs( str, fp_res);

    for (j = 0; j< XYXY; j++){
      sprintf( str, "%d ", ball_positions[i][j] );
      fputs( str, fp_res);
    }
    sprintf( str, "\n");
    fputs( str, fp_res );
  }

  printf("save done: %s\n", filename_ball );
  fclose(fp_res);
  //ofstream ofs( filename_ball );
  //for( unsigned int n = 0; n < num; n++ )
  //ofs << data_time[n] << endl;
}

double getElaspedTime( int i ){
  double now_time_;
  gettimeofday( &now_t, NULL );
  now_time_ =
    + 1000.0*( now_t.tv_sec - ini_t.tv_sec )
    + 0.001*( now_t.tv_usec - ini_t.tv_usec );

  data_time[i] = now_time_;
  return now_time_;
}

int connectSocket( char* ip_address ){
  int clientSocket;
  //char buffer[1024];
  //char buffer[99999];
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
  connect( clientSocket, (struct sockaddr *) &serverAddr, addr_size);

  return clientSocket;
}

int main(int argc, char* argv[]){
  if ( argc != 3 ){
    printf("input two server ip address.\n");
    return -1;
  }
  char* ip_address1 = argv[1];
  char* ip_address2 = argv[2];

  int clientSocket1 = connectSocket( ip_address1 );
  int clientSocket2 = connectSocket( ip_address2 );

  getFileName();
  gettimeofday( &ini_t, NULL );

  int i = 0;
  while (1){
    // communicate 1
    recv( clientSocket1, buffer1, NUM_BUFFER, 0);
    //printf( "buffer1: %s\n", buffer1 );
    /*
    if( strcmp( buffer1, "END" ) == 0 ){
      saveDat(i);      
      break;
    }else
      if ( strcmp( buffer1, "no" ) != 0 )
	sscanf( buffer1, "%d %d", &ball_positions[i][0], &ball_positions[i][1] );
    */
    if( strcmp( buffer1, "END" ) == 0 )
      break;
    else
      sscanf( buffer1, "%d %d", &ball_positions[i][0], &ball_positions[i][1] );
    //printf("ball1: %s \n", buffer1 );
    //strcpy( buffer1, "no" );
    //send( clientSocket1, "no", NUM_BUFFER, 0);

    // communicate 2
    recv( clientSocket2, buffer2, NUM_BUFFER, 0);
    //printf( "buffer2: %s\n", buffer2 );
    /*
    if( strcmp( buffer2, "END" ) == 0 ){
        saveDat(i); 
	break;
    }else
      if ( strcmp( buffer2, "no" ) != 0 )
	sscanf( buffer2, "%d %d", &ball_positions[i][2], &ball_positions[i][3] );
    */
    if( strcmp( buffer2, "END" ) == 0 )
      break;
    else
      sscanf( buffer2, "%d %d", &ball_positions[i][2], &ball_positions[i][3] );
    
    //printf("ball2: %s \n", buffer2);
    //strcpy( buffer2, "no" );
    //send( clientSocket2, "no", NUM_BUFFER, 0);

    // set data
    getElaspedTime(i);
    //setBallPositions(i);
    i++;
  }
  saveDat(i);

  return 0;
}
