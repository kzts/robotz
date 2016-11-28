#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>

char filename_ip_robotz[]  = "../data/params/ip_robotz.dat";
char filename_ip_camera1[] = "../data/params/ip_camera1.dat";
char filename_ip_camera2[] = "../data/params/ip_camera2.dat";

#define NUM 99999
//#define NUM_BUFFER 255
#define NUM_BUFFER 1024
#define XY 2

// robotz
#define NUM_OF_PHASE 10
#define NUM_ADC_PORT 8
#define NUM_ADC 2
#define NUM_OF_CHANNELS 16

double data_valves[NUM][NUM_OF_CHANNELS] = {};
unsigned long data_sensors[NUM][NUM_ADC][NUM_ADC_PORT] = {};

int phase_time[NUM_OF_PHASE];
double time_switch[NUM_OF_PHASE] = {};
double value_valves[NUM_OF_CHANNELS] = {};
double value_valves_phase[NUM_OF_PHASE][NUM_OF_CHANNELS] = {};

struct timeval ini_t, now_t;

char buffer[NUM_BUFFER];

char   filename_command_original[NUM] = "../data/robotz/command.dat";
char   filename_ball[NUM];
char   filename_results[NUM];
char   filename_command[NUM];
//int    ball_positions1[NUM][XY];
//int    ball_positions2[NUM][XY];
double ball_positions1[NUM][XY];
double ball_positions2[NUM][XY];
double time0[NUM];
double time1[NUM];
double time2[NUM];

void saveResults(void){
}

void loadCommand(void){
  FILE *fp_cmd;
  char str[NUM_BUFFER];
  unsigned int num_line = 0;
  unsigned int i, j;
  double sum_time;
  unsigned int phase;
  //char s[NUM_OF_CHANNELS + 2][NUM_BUFFER];
  char *tmp;
  double val;
  double deb;

  fp_cmd = fopen( filename_command_original, "r");

  if (fp_cmd == NULL){
    printf( "File open error: %s\n", filename_command_original );
    return;
  }

  fgets( str, NUM_BUFFER, fp_cmd );
  //printf("buffer: %s\n", str );
  for (i = 0; i < NUM_OF_PHASE; i++){
    fgets( str, NUM_BUFFER, fp_cmd);
    //printf("buffer: %s\n", str );
    tmp = strtok( str, " " ); // number
    tmp = strtok( NULL, " " );
    phase_time[i] = atoi( tmp );
    //printf("%s ", tmp );
    for (j = 0; j < NUM_OF_CHANNELS; j++){
      tmp = strtok( NULL, " " );
      //printf("%s ", tmp );
      value_valves_phase[i][j] = atof( tmp );
      //deb = atof( tmp );
      //printf("%4.3f ", atof(tmp) );
      //printf("%4.3f ", deb );
      //printf("%s:%3.2f ", tmp, deb );
    }
    //printf("\n");
  }
  
  for (i = 0; i < NUM_OF_PHASE; i++){
    sum_time = 0.0;
    for (j = 0; j <= i; j++)
      sum_time = sum_time + phase_time[j];
    time_switch[i] = sum_time;
  }
  // debug print
  for (i = 0; i < NUM_OF_PHASE; i++)
    printf( "%d ", phase_time[i]);
    //printf( "%3.2f ", phase_time[i]);
  printf("\n");
  for (i = 0; i < NUM_OF_PHASE; i++)
    printf( "%3.2f ", time_switch[i]);
  printf("\n");
  
  for (i = 0; i < NUM_OF_PHASE; i++){
    for (j = 0; j < NUM_OF_CHANNELS; j++)
      printf( "%3.2f ", value_valves_phase[i][j]);
    printf("\n");
  }
  
  fclose(fp_cmd);
}

void setCommandBuffer(int phase){
  int j;
  char tmp[9];
  for ( j = 0; j < NUM_OF_CHANNELS; j++ ){
    sprintf( tmp, "%4.3f ", value_valves_phase[phase][j] );
    strcat( buffer, tmp );
  }
}

void setSensorValue(int i){
  int j, k;
  char *tmp;

  tmp = strtok( buffer, " " ); 
  for (j = 0; j< NUM_ADC; j++){
    for (k = 0; k< NUM_ADC_PORT; k++){
      tmp = strtok( NULL, " " ); 
      data_sensors[i][j][k] = atof( tmp );
    }
  }
}

void setValveValue(int i, int phase){
  int j;
  for ( j = 0; j < NUM_OF_CHANNELS; j++ )
    data_valves[i][j] = value_valves_phase[phase][j];
}

int getPhaseNumber( double elasped_t ){
  unsigned int i;
  unsigned int phase = 0;

  for ( i = 0; i < NUM_OF_PHASE; i++)
      if ( elasped_t > time_switch[i] )
        phase = i + 1;

  return phase;
}

void getFileNames(void){
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

  sprintf( filename_ball,    "../data/%04d%02d%02d/%02d_%02d_%02d_ball.dat", 
	   year, month, day, hour, minute, second );
  sprintf( filename_results, "../data/%04d%02d%02d/%02d_%02d_%02d_results.dat", 
	   year, month, day, hour, minute, second );
  sprintf( filename_command, "../data/%04d%02d%02d/%02d_%02d_%02d_command.dat", 
	   year, month, day, hour, minute, second );
  //printf("save file: %s\n", filename_ball );
}

void saveDat(void){
  //printf("save init: %s\n", filename_ball );
  int i,j,k;
  FILE *fp_bal, *fp_res, *fp_cmd;
  char str[NUM_BUFFER];

  // open
  fp_bal = fopen( filename_ball,    "w" );
  fp_res = fopen( filename_results, "w" );
  fp_cmd = fopen( filename_command, "w" );
  if ( fp_bal == NULL ){
    printf( "File open error: %s\n", filename_ball );
    return;
  }
  if ( fp_res == NULL ){
    printf( "File open error: %s\n", filename_results );
    return;
  }
  if ( fp_cmd == NULL ){
    printf( "File open error: %s\n", filename_command );
    return;
  }

  // ball
  for ( i = 0; i < NUM; i++ ){
    // time 1
    sprintf( str, "%8.3f ", time1[i]);
    fputs( str, fp_bal );
    // camera 1
    for ( j = 0; j < XY; j++){
      //sprintf( str, "%d ", ball_positions1[i][j] );
      sprintf( str, "%8.3f ", ball_positions1[i][j] );
      fputs( str, fp_bal );
    }
    // time 2
    sprintf( str, "%8.3f ", time2[i]);
    fputs( str, fp_bal );
    // camera 2
    for ( j = 0; j < XY; j++ ){
      //sprintf( str, "%d ", ball_positions2[i][j] );
      sprintf( str, "%8.3f ", ball_positions2[i][j] );
      fputs( str, fp_bal );
    }
    // time 2
    sprintf( str, "\n");
    fputs( str, fp_bal );
  }
  // sensor
  for (i = 0; i < NUM; i++){
    sprintf( str, "%8.3f\t", time0[i]);
    fputs( str, fp_res);

    for (j = 0; j< NUM_ADC; j++){
      for (k = 0; k< NUM_ADC_PORT; k++){
        sprintf( str, "%10lu\t", data_sensors[i][j][k]);
        fputs( str, fp_res);
      }
    }
    for (j = 0; j < NUM_OF_CHANNELS; j++){
      sprintf( str, "%10lf\t", data_valves[i][j]);
      fputs( str, fp_res);
    }
    sprintf( str, "\n");
    fputs(str, fp_res);
  }
  // command
  for (i = 0; i < NUM_OF_PHASE; i++){
    sprintf( str, "%04d\t", phase_time[i] );
    fputs( str, fp_cmd );
    for (j = 0; j < NUM_OF_CHANNELS; j++){
      sprintf( str, "%4.3f\t", value_valves_phase[i][j] );
      fputs( str, fp_cmd );
    }
    strcpy( str, "\n" );
    fputs( str, fp_cmd );
  }
  // close
  fclose( fp_cmd );
  fclose( fp_res );
  fclose( fp_bal );

  printf( "save done: %s\n", filename_command );
  printf( "save done: %s\n", filename_results );
  printf( "save done: %s\n", filename_ball );
}

double getElaspedTime(void){
  double now_time_;
  gettimeofday( &now_t, NULL );
  now_time_ =
    + 1000.0*( now_t.tv_sec - ini_t.tv_sec )
    + 0.001*( now_t.tv_usec - ini_t.tv_usec );
  return now_time_;
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

int main(){
  gettimeofday( &ini_t, NULL );

  // filename
  getFileNames();
  loadCommand();

  // get IP address
  char ip_robotz[NUM_BUFFER], ip_camera1[NUM_BUFFER], ip_camera2[NUM_BUFFER];
  getIPAddress( filename_ip_robotz,  ip_robotz );   
  getIPAddress( filename_ip_camera1, ip_camera1 );   
  getIPAddress( filename_ip_camera2, ip_camera2 );   

  printf( "ip address: %s (robotz), %s (camera1), %s (camera2)\n", ip_robotz, ip_camera1, ip_camera2 );

  // server
  int port_num = 7891;
  int robotz_socket  = connectSocket( ip_robotz,  port_num );
  //int robotz_socket  = 0;
  int camera1_socket = connectSocket( ip_camera1, port_num );
  int camera2_socket = connectSocket( ip_camera2, port_num );

  fd_set fds, readfds;
  int maxfd; // max value of file discripter for select 

  FD_ZERO(&readfds); // initialize fd_set
  FD_SET( robotz_socket,  &readfds ); // register 
  FD_SET( camera1_socket, &readfds );
  FD_SET( camera2_socket, &readfds );

  // maxfd 
  if ( robotz_socket > camera1_socket && robotz_socket > camera2_socket )
    maxfd = robotz_socket;
  if ( camera1_socket > robotz_socket && camera1_socket > camera2_socket )
    maxfd = camera1_socket;
  if ( camera2_socket > robotz_socket && camera2_socket > camera1_socket )
    maxfd = camera2_socket;

  // loop
  int now_phase = 0, old_phase = -1;
  int c1 = 0, c2 = 0, r = 0; // counter
  while (1){
    memcpy( &fds, &readfds, sizeof(fd_set) ); // ititialize
    select( maxfd + 1, &fds, NULL, NULL, NULL ); // wait 
    // camera1
    if ( FD_ISSET( camera1_socket, &fds )){
      memset( buffer, 0, sizeof(buffer) );
      recv( camera1_socket, buffer, sizeof(buffer), 0 );
      send( camera1_socket, ".",    sizeof(buffer), 0 );
      
      time1[c1] = getElaspedTime();
      //sscanf( buffer, "%d %d", &ball_positions1[c1][0], &ball_positions1[c1][1] );
      sscanf( buffer, "%lf %lf", &ball_positions1[c1][0], &ball_positions1[c1][1] );
      //printf( "camera1: %s\n", buffer );
      c1++;
    }
    // camera2
    if ( FD_ISSET( camera2_socket, &fds )){
      memset( buffer, 0, sizeof(buffer));
      recv( camera2_socket, buffer, sizeof(buffer), 0 );
      send( camera2_socket, ".",    sizeof(buffer), 0 );

      time2[c2] = getElaspedTime();
      //sscanf( buffer, "%d %d", &ball_positions2[c2][0], &ball_positions2[c2][1] );
      sscanf( buffer, "%lf %lf", &ball_positions2[c2][0], &ball_positions2[c2][1] );
      //if ( strlen( buffer ) > 5 )
      //printf( "camera2: %s\n", buffer );
      c2++;
    }
    // robotz    
    if ( FD_ISSET( robotz_socket, &fds )){      
      now_phase = getPhaseNumber( getElaspedTime() );
      //printf( "num: %05d, phase: %02d, time: %9.3f ms \n", r, now_phase, getElaspedTime() );
     
      // terminate
      if ( now_phase >= NUM_OF_PHASE )
	break;

      // recieve, set state
      memset( buffer, 0, sizeof(buffer) );
      recv( robotz_socket, buffer, sizeof(buffer), 0);
      //printf( "robotz: %s\n", buffer );
      if ( strlen( buffer ) > strlen( "sensor: " ))
	setSensorValue(r);
      setValveValue(r,now_phase);
      time0[r] = getElaspedTime();
      
      // send command
      if ( now_phase != old_phase ){
	//printf("now phase: %d\n", now_phase );
	strcpy( buffer, "command: " ); 
	setCommandBuffer( now_phase );
      }else{
	strcpy( buffer, "no" );
      }
      send( robotz_socket, buffer, NUM_BUFFER, 0 );

      // ++
      r++;
      old_phase = now_phase;
    }
  }
  // terminate server
  send( robotz_socket,  "END", sizeof(buffer), 0 );
  send( camera1_socket, "END", sizeof(buffer), 0 );
  send( camera2_socket, "END", sizeof(buffer), 0 );

  saveDat();

  // close
  close( robotz_socket );
  close( camera1_socket );
  close( camera2_socket );

  return 0;
}



