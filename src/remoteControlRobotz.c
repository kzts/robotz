// socket
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>

#define NUM_OF_BUFFER 1024

char buffer_recv[NUM_OF_BUFFER];
char buffer_send[NUM_OF_BUFFER];

// bbb
#define MAX_str 1024
#define NUM_OF_SAMPLES 99999
#define NUM_OF_PHASE 10
#define NUM_ADC_PORT 8
#define NUM_ADC 2
#define NUM_OF_CHANNELS 16
#define PORT_NUM 7891

unsigned int num_ch;
double data_time[NUM_OF_SAMPLES] = {};
double data_valves[NUM_OF_SAMPLES][NUM_OF_CHANNELS] = {};
unsigned long data_sensors[NUM_OF_SAMPLES][NUM_ADC][NUM_ADC_PORT] = {};

double time_switch[NUM_OF_PHASE] = {};
double value_valves[NUM_OF_CHANNELS] = {};
double value_valves_phase[NUM_OF_PHASE][NUM_OF_CHANNELS] = {};

char filename_ip_robotz[]  = "../data/params/ip_robotz.dat";
char filename_results[NUM_OF_BUFFER];
struct timeval ini_t, now_t;

/*
unsigned long Value_sensors[NUM_ADC][NUM_ADC_PORT] = {};
double Time_phase[NUM_OF_PHASE] = {};
*/

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

  sprintf( filename_results, 
	   "../data/%04d%02d%02d/%02d_%02d_%02d_results.dat", 
	   year, month, day, hour, minute, second );
  printf( "save file name: %s\n", filename_results );
}


//void saveResults(void){
void saveResults( int time_num ){
  int i,j,k;
  FILE *fp_res;
  char str[NUM_OF_BUFFER];

  fp_res = fopen( filename_results, "w");

  if (fp_res == NULL){
    printf("File open error (results)\n");
    return;
  }

  //for (i = 0; i < NUM_OF_SAMPLES; i++){
  for (i = 0; i < time_num; i++){
    sprintf( str, "%8.3f\t", data_time[i]);
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

  fclose(fp_res);
  printf( "save done: %s\n", filename_results );
}

void loadCommand(void){
  FILE *fp_cmd;
  char str[NUM_OF_BUFFER];
  unsigned int num_line = 0;
  unsigned int i, j;
  double sum_time;
  unsigned int phase;
  //double phase_time[NUM_OF_PHASE];
  int phase_time[NUM_OF_PHASE];
  char s[NUM_OF_CHANNELS + 2][NUM_OF_BUFFER];
  char *tmp;
  //char *ends;
  double val;

  //fp_cmd = fopen( "../data/pitching/command.dat", "r");
  fp_cmd = fopen( "../data/robotz/command.dat", "r");

  if (fp_cmd == NULL){
    printf("File open error (command)\n");
    return;
  }

  //val = atof( "324.566" );
  //printf( "%3.2f\n ", val );
  //printf( "%lf\n ", val );

  fgets( str, NUM_OF_BUFFER, fp_cmd );
  //printf( "%s\n", str );
  for (i = 0; i < NUM_OF_PHASE; i++){
    fgets( str, MAX_str, fp_cmd);
    //printf( "%s\n", str );
    tmp = strtok( str, " " ); // number
    //printf( "%s ", tmp );
    tmp = strtok( NULL, " " );
    //printf( "%s ", tmp );
    //phase_time[i] = atof( tmp );
    phase_time[i] = atoi( tmp );
    //printf( "%d ", phase_time[i] );
    for (j = 0; j < NUM_OF_CHANNELS; j++){
      tmp = strtok( NULL, " " );
      //printf( "%s ", tmp );
      value_valves_phase[i][j] = atof( tmp );
      //val = atof( tmp );
      //value_valves_phase[i][j] = atof( &tmp );
      //value_valves_phase[i][j] = strtod( tmp, &ends );
      //printf( "%lf ", atof(tmp) );
      //printf( "%lf ", value_valves_phase[i][j] );
      //printf( "%3.2f ", val );
    }
    //printf( "\n" );
    /*
    sscanf( str, "%s %s  %s %s %s %s  %s %s %s %s  %s %s %s %s  %s %s %s %s", s[0], s[1],
            s[ 2], s[ 3], s[ 4], s[ 5],  s[ 6], s[ 7], s[ 8], s[9],
            s[10], s[11], s[12], s[13],  s[14], s[15], s[16], s[17]);
    phase_time[i] = atof( s[1]);
    for (j = 0; j < NUM_OF_CHANNELS; j++)
      value_valves_phase[i][j] = atof( s[j+2]);
    */
  }
  
  for (i = 0; i < NUM_OF_PHASE; i++){
    sum_time = 0.0;
    for (j = 0; j <= i; j++)
      sum_time = sum_time + phase_time[j];
    time_switch[i] = sum_time;
  }
  
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
  //printf( "%s ", buffer_send );

  for ( j = 0; j < NUM_OF_CHANNELS; j++ ){
    sprintf( tmp, "%4.3f ", value_valves_phase[phase][j] );
    strcat( buffer_send, tmp );
    //printf( "%s ", buffer_send );
  }
  
  //printf("\n");
}

double getTime(int i){
  double now_time_;
  gettimeofday( &now_t, NULL );
  now_time_ = 
    + 1000.0*( now_t.tv_sec - ini_t.tv_sec ) 
    + 0.001*( now_t.tv_usec - ini_t.tv_usec );

  data_time[i] = now_time_;
  return now_time_;
}

void setSensorValue(int i){
  int j, k;
  char *tmp;

  tmp = strtok( buffer_recv, " " ); 
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
  fgets( ip_address, NUM_OF_BUFFER, fp );
  fclose(fp);
}

int main(int argc, char* argv[])
{
  getFileNames();
  gettimeofday( &ini_t, NULL );

  char ip_robotz[NUM_OF_BUFFER];
  getIPAddress( filename_ip_robotz,  ip_robotz );   
  printf( "ip address: %s (robotz)\n", ip_robotz );

  int clientSocket  = connectSocket( ip_robotz, PORT_NUM );

  // loop
  int i;
  int now_phase = 0, old_phase = -1;
  double now_time;

  loadCommand();
 
  for ( i = 0; i < NUM_OF_SAMPLES; i++ ){
    now_time = getTime(i);
    now_phase = getPhaseNumber( now_time );
    //printf( "%d \t %d \t %8.3f \n", i, now_phase, now_time );

    if ( now_phase >= NUM_OF_PHASE )
      break;

    recv( clientSocket, buffer_recv, NUM_OF_BUFFER, 0);
    
    if ( strlen( buffer_recv ) > strlen( "sensor: " ))
      setSensorValue(i);
   
    setValveValue(i,now_phase);

    if ( now_phase != old_phase ){
      printf( "now phase: %d\n", now_phase );
      strcpy( buffer_send, "command: " ); 
      //printf("buffer in main: %s\n", buffer_send );
      setCommandBuffer( now_phase );
    }else{
      strcpy( buffer_send, "no" );
    }
    //send( clientSocket, buffer, 1024, 0);
    send( clientSocket, buffer_send, NUM_OF_BUFFER, 0 );

    old_phase = now_phase;
  }
  //saveResults();
  saveResults(i);
  return 0;
}
