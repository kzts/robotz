
// socket
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#define PITCHING_MACHINE_IP "192.168.2.20"

// bbb
#include <stdlib.h>
#define MAX_str 1024
#define NUM_OF_SAMPLES 99999
#define NUM_OF_PHASE 10
#define NUM_ADC_PORT 8
#define NUM_ADC 2
#define NUM_OF_CHANNELS 16

unsigned int num_ch;
double data_time[NUM_OF_SAMPLES] = {};
double data_valves[NUM_OF_SAMPLES][NUM_OF_CHANNELS] = {};
unsigned long data_sensors[NUM_OF_SAMPLES][NUM_ADC][NUM_ADC_PORT] = {};

double time_switch[NUM_OF_PHASE] = {};
double value_valves[NUM_OF_CHANNELS] = {};
double value_valves_phase[NUM_OF_PHASE][NUM_OF_CHANNELS] = {};
/*
unsigned long Value_sensors[NUM_ADC][NUM_ADC_PORT] = {};
double Time_phase[NUM_OF_PHASE] = {};
*/

void saveResults(void){
  int i,j,k;
  FILE *fp_res;
  char str[MAX_str];

  fp_res = fopen( "data/results.dat", "w");

  if (fp_res == NULL){
    printf("File open error (results)\n");
    return;
  }

  for (i = 0; i < NUM_OF_SAMPLES; i++){
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
}

void loadCommand(void){
  FILE *fp_cmd;
  char str[MAX_str];
  unsigned int num_line = 0;
  unsigned int i, j;
  double sum_time;
  unsigned int phase;
  //double phase_time[NUM_OF_PHASE];
  int phase_time[NUM_OF_PHASE];
  char s[NUM_OF_CHANNELS + 2][MAX_str];
  char *tmp;
  //char *ends;
  double val;


  fp_cmd = fopen( "../data/command.dat", "r");

  if (fp_cmd == NULL){
    printf("File open error (command)\n");
    return;
  }

  //val = atof( "324.566" );
  //printf( "%3.2f\n ", val );
  //printf( "%lf\n ", val );

  fgets( str, MAX_str, fp_cmd);
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



int main(int argc, char* argv[]){

  /*
  if ( argc != 2 ){
    printf("input server ip address.\n");
    return -1;
  }
  char* ip_address = argv[1];
  */

  // socket 
  int clientSocket;
  //char buffer[1024];
  char buffer[99999];
  struct sockaddr_in serverAddr;
  socklen_t addr_size;

  clientSocket = socket(PF_INET, SOCK_STREAM, 0);
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(7891);
  //serverAddr.sin_addr.s_addr = inet_addr(ip_address);
  serverAddr.sin_addr.s_addr = inet_addr(PITCHING_MACHINE_IP);
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

  addr_size = sizeof serverAddr;
  connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);

  // loop
  loadCommand();
  strcpy( buffer, "no" );

  int i = 0;
  while ( strcmp( buffer, "END" ) != 0 ){
    recv( clientSocket, buffer, 1024, 0);
    
    if ( strcmp( buffer, "no" ) != 0 ){
      printf("%s \n", buffer);
    }
    if ( i == 10 )
      send( clientSocket, "command: 0.00 0.450 0.333 0  0 0 0 0  0 0 0 0  0 0 0 0 ", 99, 0);
    else
      send( clientSocket, "no", 13, 0);
    i++;
  }
  saveResults();
  return 0;
}
