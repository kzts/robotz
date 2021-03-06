// std
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
// socket
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
// ip
#include <unistd.h>
#include <sys/types.h>
//#include <sys/ioctl.h>
//#include <net/if.h>
// c++
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include "Eigen/Core"
#include "Eigen/Geometry"
#include "Eigen/LU"

using namespace std;
using namespace Eigen;

#define ROBOTZ_IP_FILE "../data/params/ip_robotz.dat"
#define BALL_IP_FILE   "../data/params/ip_connect.dat"
#define COMMAND_FILE   "../data/command.dat"
//#define HAND_FILE      "../data/20170128/hand.dat"
#define HAND_FILE      "../data/20170130/hand.dat"
//#define DOTS_FILE      "../data/20170128/dots.dat"
#define DOTS_FILE      "../data/20170130/dots.dat"

#define ROBOTZ_PORT 7891
#define BALL_PORT   12345
#define MAX_MARKER_NUM 20
#define DISTANCE_LIMIT 200e-3
#define MAX_DISTANCE 10
#define ENOUGH_SAMPLE 10
#define MS_TO_SEC 1.0e-3
#define SEC_TO_MS 1.0e+3
#define HIT_DISTANCE 20e-3
//#define NATNET_FPS 120.0
#define NATNET_FPS 60.0
#define BALL_HEIGHT 1.0

#define NUM 9999
//#define NUM_BUFFER 255
#define NUM_BUFFER 1024
//#define NUM_BUFFER 2048
#define XY 2
//#define NUM_PREDICT 100
//#define NUM_PREDICT 300
#define XYZ 3
//#define TXY 3
#define X 0
#define Y 1
#define Z 2

// robotz
#define NUM_OF_PHASE 10
#define NUM_ADC_PORT 8
#define NUM_ADC 3
#define NUM_OF_CHANNELS 16
#define NUM_ARM 13
#define Arm_pressure 0.3
#define TIME_END 5000
#define TIME_SWING 300
#define HAND_NUM 99999
//#define FUTURE_TIME_NUM 400
#define FUTURE_TIME_NUM 100
//#define TIME_TICK 3e-3
#define TIME_TICK 2e-3
//#define TIME_TICK 1e-3

#define R_KNEE_COL 2
#define L_KNEE_COL 7
#define KNEE_MEAN_PRESSURE 0.4
#define JUMP_PHASE1 1
#define JUMP_PHASE2 2

int min_hand_time = HAND_NUM;
int dots_index[HAND_NUM] = {};
int command_index[HAND_NUM] = {};
double hand_positions[HAND_NUM][XYZ] = {};
int hand_time[HAND_NUM] = {};
double knee_command[HAND_NUM] = {};
int jump_time[HAND_NUM] = {};
int jump_time_data[NUM] = {};

double marker_position[XYZ* MAX_MARKER_NUM] = {};
double old_position[XYZ] = {};

double data_valves[NUM][NUM_OF_CHANNELS] = {};
unsigned long data_sensors[NUM][NUM_ADC][NUM_ADC_PORT] = {};

int phase_time[NUM_OF_PHASE];
double time_switch[NUM_OF_PHASE] = {};
double value_valves_phase[NUM_OF_PHASE][NUM_OF_CHANNELS] = {};

struct timeval ini_t, now_t, robotz_ini_t;

char buffer[NUM_BUFFER];

char   filename_ball[NUM];
char   filename_results[NUM];

double robotz_time[NUM];
double ball_time[NUM];
double ball_positions[NUM][XYZ];

int motive_frame = 0;

//char coefficients_filename[] = "../data/params/regression_coefficients.dat";
//char dummy_filename[]        = "../data/20161128/17_11_38_ball.dat";
//char hitposition_filename[]  = "../data/params/hitposition.dat";
//char waittime_filename[]     = "../data/params/waittime.dat";

Vector2d coeffs_x, coeffs_z;
Vector3d coeffs_y;

double future_ball_positions[FUTURE_TIME_NUM][XYZ] = {};

double min_hand_position_x = + MAX_DISTANCE;
double min_hand_position_y = + MAX_DISTANCE;
double min_hand_position_z = + MAX_DISTANCE;
double max_hand_position_x = - MAX_DISTANCE;
double max_hand_position_y = - MAX_DISTANCE;
double max_hand_position_z = - MAX_DISTANCE;

int ball_hit_time = 0;
int robotz_hit_time = 0;
double hit_knee_command = 0.0;

int is_robotz_move = 0;
int is_ball_found = 0;

int hand_num    = 0;
int dots_num    = 0;
int command_num = 0;

void setJumpTime( int i ){
  jump_time_data[i] = phase_time[JUMP_PHASE2];
}

void changeJumpTime( int hit_jump_time ){
  phase_time[JUMP_PHASE2] = hit_jump_time;

  for (int i = 0; i < NUM_OF_PHASE; i++){
    int sum_time = 0;
    for (int j = 0; j <= i; j++)
      sum_time += phase_time[j];
    time_switch[i] = sum_time;
  }

  cout << "switch time change" << endl;
}

double detectTime(void){
  int hand_index = -1;
  int ball_index = -1;
  double min_distance = MAX_DISTANCE;

  for ( int t = 0; t < FUTURE_TIME_NUM; t++ ){
    if ( future_ball_positions[t][X] > min_hand_position_x && 
	 future_ball_positions[t][Y] > min_hand_position_y && 
	 future_ball_positions[t][Z] > min_hand_position_z && 
	 future_ball_positions[t][X] < max_hand_position_x && 
	 future_ball_positions[t][Y] < max_hand_position_y && 
	 future_ball_positions[t][Z] < max_hand_position_z ){
    //if ( future_ball_positions[t][0] > min_hand_position_x ){
      //for ( int h = 0; h < hand_num; h++ ){
      for ( int d = 0; d < command_num; d++ ){
	int h = command_index[d];
	double sum = 0.0;
	for ( int n = 0; n < XYZ; n++ ){
	  double diff = future_ball_positions[t][n] - hand_positions[h][n];
	  sum += diff* diff;
	}
	double distance = sqrt( sum );
	if ( distance < min_distance ){
	  hand_index   = h;
	  ball_index   = t;
	  min_distance = distance;
	  
	  if ( min_distance < HIT_DISTANCE )
	    break;
	}
      }
    }
  }
  // cout << time_index << " " << hand_index << " " << min_distance << endl;
  if ( min_distance < HIT_DISTANCE ){
    ball_hit_time     = ball_index* TIME_TICK* SEC_TO_MS;
    robotz_hit_time   = hand_time[ hand_index ];
    int hit_jump_time = jump_time[ hand_index ];
  
    cout << "detect time " 
	 << ball_index << " " << hand_index << " " << min_distance << " " 
	 << ball_hit_time << " " << robotz_hit_time << " " << hit_knee_command << endl;

    if ( ball_hit_time < ( robotz_hit_time + ( 1.0/ NATNET_FPS )* SEC_TO_MS )){
    //if ( ball_hit_time < ( robotz_hit_time + ( 1.0/ NATNET_FPS )* MS_TO_SEC )){
    //if ( ball_hit_time < ( robotz_hit_time + TIME_TICK* MS_TO_SEC )){
      cout << "before: " << phase_time[JUMP_PHASE2] << endl;
      changeJumpTime( hit_jump_time );
      cout << "after:  " << phase_time[JUMP_PHASE2] << endl;
    }
  }
}

void getCommandIndex( double R_knee_command ){
  int i = 0;
  for ( int h = 0; h < hand_num; h++ ){
    if ( knee_command[h] == R_knee_command ){
      command_index[i] = h;
      i++;
    }
  }
  command_num = i;
}

void changeKneePressure( double R_knee_command ){
  value_valves_phase[JUMP_PHASE1][R_KNEE_COL] = R_knee_command;
  value_valves_phase[JUMP_PHASE2][R_KNEE_COL] = R_knee_command;
  value_valves_phase[JUMP_PHASE1][L_KNEE_COL] = 2.0* KNEE_MEAN_PRESSURE - R_knee_command;
  value_valves_phase[JUMP_PHASE2][L_KNEE_COL] = 2.0* KNEE_MEAN_PRESSURE - R_knee_command;
}

double getElaspedTime(void){
  double now_time_;
  gettimeofday( &now_t, NULL );
  now_time_ =
    + 1000.0*( now_t.tv_sec - ini_t.tv_sec )
    + 0.001*( now_t.tv_usec - ini_t.tv_usec );
  return now_time_;
}

double getRobotzTime(void){
  double now_time_;
  gettimeofday( &now_t, NULL );
  now_time_ =
    + 1000.0*( now_t.tv_sec - robotz_ini_t.tv_sec )
    + 0.001*( now_t.tv_usec - robotz_ini_t.tv_usec );
  return now_time_;
}

//double detectHit( int hand_num ){
double detectHit(void){
  int hand_index = -1;
  int ball_index = -1;
  double min_distance = MAX_DISTANCE;

  for ( int t = 0; t < FUTURE_TIME_NUM; t++ ){
    if ( future_ball_positions[t][X] > min_hand_position_x && 
	 future_ball_positions[t][Y] > min_hand_position_y && 
	 future_ball_positions[t][Z] > min_hand_position_z && 
	 future_ball_positions[t][X] < max_hand_position_x && 
	 future_ball_positions[t][Y] < max_hand_position_y && 
	 future_ball_positions[t][Z] < max_hand_position_z ){
    //if ( future_ball_positions[t][0] > min_hand_position_x ){
      //for ( int h = 0; h < hand_num; h++ ){
      for ( int d = 0; d < dots_num; d++ ){
	int h = dots_index[d];
	double sum = 0.0;
	for ( int n = 0; n < XYZ; n++ ){
	  double diff = future_ball_positions[t][n] - hand_positions[h][n];
	  sum += diff* diff;
	}
	double distance = sqrt( sum );
	if ( distance < min_distance ){
	  hand_index   = h;
	  ball_index   = t;
	  min_distance = distance;
	  
	  if ( min_distance < HIT_DISTANCE )
	    break;
	}
      }
    }
  }
  // cout << time_index << " " << hand_index << " " << min_distance << endl;
  if ( min_distance < HIT_DISTANCE ){
    //ball_hit_time     = ball_index* TIME_TICK* SEC_TO_MS;
    ball_hit_time     = ball_index* TIME_TICK* SEC_TO_MS + min_hand_time;
    robotz_hit_time   = hand_time[ hand_index ];
    hit_knee_command  = knee_command[ hand_index ];
  
    cout << "detect command " 
	 << ball_index << " " << hand_index << " " << min_distance << " " 
	 << ball_hit_time << " " << robotz_hit_time << " " << hit_knee_command << endl;

    if ( ball_hit_time < ( robotz_hit_time + ( 1.0/ NATNET_FPS )* SEC_TO_MS )){
    //if ( ball_hit_time < ( robotz_hit_time + ( 1.0/ NATNET_FPS )* MS_TO_SEC )){
    //if ( ball_hit_time < ( robotz_hit_time + TIME_TICK* MS_TO_SEC )){
      is_robotz_move = 1;
      gettimeofday( &robotz_ini_t, NULL );
      changeKneePressure( hit_knee_command );
      getCommandIndex( hit_knee_command );
    }
  }
}

void predictBallShort(void){
  for ( int t = 0; t < FUTURE_TIME_NUM; t++ ){
    double future_time_sec = MS_TO_SEC* getElaspedTime() + t* TIME_TICK;
    future_ball_positions[t][X] = coeffs_x[0]* future_time_sec + coeffs_x[1];
    future_ball_positions[t][Z] = coeffs_z[0]* future_time_sec + coeffs_z[1];
    future_ball_positions[t][Y] = 
      coeffs_y[0]* future_time_sec* future_time_sec + coeffs_y[1]* future_time_sec + coeffs_y[2];
  }
}

void predictBallLong(void){
  for ( int t = 0; t < FUTURE_TIME_NUM; t++ ){
    //double future_time_sec = MS_TO_SEC* getElaspedTime() + t* TIME_TICK;
    double future_time_sec = MS_TO_SEC* getElaspedTime() + t* TIME_TICK + MS_TO_SEC* min_hand_time;
    future_ball_positions[t][X] = coeffs_x[0]* future_time_sec + coeffs_x[1];
    future_ball_positions[t][Z] = coeffs_z[0]* future_time_sec + coeffs_z[1];
    future_ball_positions[t][Y] = 
      coeffs_y[0]* future_time_sec* future_time_sec + coeffs_y[1]* future_time_sec + coeffs_y[2];
  }
}

void getBallCoeffs( int time_num ){
  // time, state vector
  VectorXd t_vec_1( time_num );
  VectorXd t_vec_2( time_num );
  VectorXd b_x( time_num );
  VectorXd b_y( time_num );
  VectorXd b_z( time_num );
  for ( int i = 0; i < time_num; i++ ){
    t_vec_1(i) = MS_TO_SEC* ball_time[i];
    t_vec_2(i) = MS_TO_SEC* ball_time[i]* MS_TO_SEC* ball_time[i];

    b_x(i) = ball_positions[i][X];
    b_y(i) = ball_positions[i][Y];
    b_z(i) = ball_positions[i][Z];
  }
  // time matrix
  MatrixXd t_mat_1 = MatrixXd::Ones( time_num, 1 + 1 );
  MatrixXd t_mat_2 = MatrixXd::Ones( time_num, 2 + 1 );
  t_mat_1.block( 0, 0, time_num, 1 ) = t_vec_1;
  t_mat_2.block( 0, 0, time_num, 1 ) = t_vec_2;
  t_mat_2.block( 0, 1, time_num, 1 ) = t_vec_1;
  // coeffs
  MatrixXd tmp_1 = t_mat_1.transpose() * t_mat_1;
  MatrixXd tmp_2 = t_mat_2.transpose() * t_mat_2;
  coeffs_x = tmp_1.inverse() * t_mat_1.transpose() * b_x;
  coeffs_y = tmp_2.inverse() * t_mat_2.transpose() * b_y;
  coeffs_z = tmp_1.inverse() * t_mat_1.transpose() * b_z;
} 

void loadHandDataFile(void){
  FILE *fp;
  char str[NUM_BUFFER];
  char *tmp;
  int i = 0;

  fp = fopen( HAND_FILE, "r" );

  if (fp == NULL){
    printf( "File open error: %s\n", HAND_FILE );
    return;
  }

  while ( fgets( str, NUM_BUFFER, fp ) != NULL ){
    tmp = strtok( str,  " " ); hand_positions[i][0] = atof( tmp ); //printf( "%s ", tmp );
    tmp = strtok( NULL, " " ); hand_positions[i][1] = atof( tmp ); //printf( "%s ", tmp );
    tmp = strtok( NULL, " " ); hand_positions[i][2] = atof( tmp ); //printf( "%s ", tmp );
    tmp = strtok( NULL, " " ); hand_time[i]         = atoi( tmp ); //printf( "%s ", tmp );
    tmp = strtok( NULL, " " ); knee_command[i]      = atof( tmp ); //printf( "%s ", tmp );
    tmp = strtok( NULL, " " ); jump_time[i]         = atoi( tmp ); //printf( "%s ", tmp );
    //printf( "%d %lf %lf %lf\n", 
    //	    ball_time[i], ball_position[i][0], ball_position[i][1], ball_position[i][2] );
    i++;
  }
  
  for ( int k = 0; k < i; k++ )
    if ( hand_time[k] < min_hand_time )  
      min_hand_time = hand_time[k];

  //for ( int k = 0; k < ( i - 1 ); k++ ){
  for ( int k = 0; k < i; k++ ){
    if ( hand_positions[k][X] < min_hand_position_x )  min_hand_position_x = hand_positions[k][X];
    if ( hand_positions[k][Y] < min_hand_position_y )  min_hand_position_y = hand_positions[k][Y];
    if ( hand_positions[k][Z] < min_hand_position_z )  min_hand_position_z = hand_positions[k][Z];
    if ( hand_positions[k][X] > max_hand_position_x )  max_hand_position_x = hand_positions[k][X];
    if ( hand_positions[k][Y] > max_hand_position_y )  max_hand_position_y = hand_positions[k][Y];
    if ( hand_positions[k][Z] > max_hand_position_z )  max_hand_position_z = hand_positions[k][Z];
  }

  cout << "hand_range: " 
       << min_hand_position_x << ", " << max_hand_position_x << ", "
       << min_hand_position_y << ", " << max_hand_position_y << ", "
       << min_hand_position_z << ", " << max_hand_position_z << ". " << endl;

  fclose( fp );

  hand_num = i;
}

void loadDotsIndexFile(void){
  FILE *fp;
  char str[NUM_BUFFER];
  int i = 0;

  fp = fopen( DOTS_FILE, "r" );

  if (fp == NULL){
    printf( "File open error: %s\n", DOTS_FILE );
    return;
  }

  while ( fgets( str, NUM_BUFFER, fp ) != NULL ){
    dots_index[i] = atoi( str ); //printf( "%s ", tmp );
    i++;
  }
  fclose( fp );

  dots_num = i;
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

  fp_cmd = fopen( COMMAND_FILE, "r");

  if (fp_cmd == NULL){
    printf( "File open error: %s\n", COMMAND_FILE );
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

void setExhaustCommand(void){
  memset( buffer, 0, sizeof(buffer) );
  strcpy( buffer, "command: " ); 
  char tmp[] = "0.00 ";
  for ( int j = 0; j < NUM_OF_CHANNELS; j++ ){
    //sprintf( tmp, "%4.3f ", 0.0 );
    strcat( buffer, tmp );
  }
}

void setSensorValue(int i){
  int j, k;
  char *tmp;

  tmp = strtok( buffer, " " ); 
  for (j = 0; j< NUM_ADC; j++){
    for (k = 0; k< NUM_ADC_PORT; k++){
      //if ( buffer == NULL )
      if ( tmp == NULL )
      	break;
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

  sprintf( filename_ball,    "../data/%04d%02d%02d/%02d_%02d_%02d_ball.dat",   year, month, day, hour, minute, second );
  sprintf( filename_results, "../data/%04d%02d%02d/%02d_%02d_%02d_robotz.dat", year, month, day, hour, minute, second );

  printf("ball file:  %s\n", filename_ball );
  printf("robot file: %s\n", filename_results );
}

void saveRobotz( int time_num ){
  printf("save init: %s\n", filename_results );
  int i,j,k;
  FILE *fp;
  char str[NUM_BUFFER];

  // open
  fp = fopen( filename_results, "w" );
  if ( fp == NULL ){
    printf( "File open error: %s\n", filename_results );
    return;
  }
  // sensor
  for (i = 0; i < time_num; i++ ){
    sprintf( str, "%8.3f\t", robotz_time[i] );
    fputs( str, fp );
    for (j = 0; j< NUM_ADC; j++ ){
      for (k = 0; k< NUM_ADC_PORT; k++ ){
        sprintf( str, "%10lu\t", data_sensors[i][j][k] );
        fputs( str, fp );
      }
    }
    for (j = 0; j < NUM_OF_CHANNELS; j++ ){
      sprintf( str, "%10lf\t", data_valves[i][j] );
      fputs( str, fp );
    }
    sprintf( str, "\n");
    fputs( str, fp );
  }
  // close
  fclose( fp );
  printf( "save done: %s\n", filename_results );
}

void saveBall( int time_num ){
  printf("save init: %s\n", filename_ball );
  int i,j,k;
  FILE *fp;
  char str[NUM_BUFFER];

  // open
  fp = fopen( filename_ball, "w" );
  if ( fp == NULL ){
    printf( "File open error: %s\n", filename_ball );
    return;
  }
  // write
  for ( i = 0; i < time_num; i++ ){
    sprintf( str, "%8.3f\t", ball_time[i] );
    fputs( str, fp );
    for ( j = 0; j< XYZ; j++ ){
      sprintf( str, "%8.3f\t", ball_positions[i][j] );
      fputs( str, fp );
    }
    sprintf( str, "%d\n", jump_time_data[i] );
    fputs( str, fp );
  }
  // close
  fclose( fp );
  printf( "save done: %s\n", filename_ball );
}

int setBallPosition( int i ){
  double diff;
  int near_idx = 0;
  double distance_min = MAX_DISTANCE;
  int is_found = 0;

  for ( int j = 0; j < MAX_MARKER_NUM; j++ ){
    if ( marker_position[ XYZ* j ] != 0.0 ){
      if ( marker_position[ XYZ* j + 0 ] != old_position[0] ||
         marker_position[ XYZ* j + 1 ] != old_position[1] ||
         marker_position[ XYZ* j + 2 ] != old_position[2] ){
	double sum = 0;
	for ( int n = 0; n < XYZ; n++ ){
	  diff = old_position[n] - marker_position[ XYZ*j + n ];
	  sum += diff*diff;
	}
	double distance = sqrt(sum);
	if ( distance < distance_min ){
	  near_idx     = j;
	  distance_min = distance;
	}
      }
    }
  }
  //cout << distance_min << " " << old_position[0] << endl;
  //if ( distance_min < DISTANCE_LIMIT ){
  if ( distance_min < DISTANCE_LIMIT && old_position[0] != marker_position[ XYZ* near_idx ] ){
    //printf( "%d %lf %lf %lf\n", i, old_position[0], old_position[1], old_position[2] );
    //cout << near_idx << " " << distance_min << " " << old_position[0] << endl;
    is_found = 1;
    ball_time[i] = getElaspedTime();  
    
    for ( int n = 0; n < XYZ; n++ ){
      old_position[n]      = marker_position[ XYZ* near_idx + n ];
      ball_positions[i][n] = marker_position[ XYZ* near_idx + n ];
    }

    //if ( marker_position[0] != 0 ){
    //for ( int m = 0; m < MAX_MARKER_NUM; m++ )
    //	cout << marker_position[m]<< " ";
    //cout << endl;
    //}
    //}
  }else{
    is_found = 0;
  }
  return is_found;
}

void findBall(void){
  for ( int j = 0; j < MAX_MARKER_NUM; j++ ){
    if ( marker_position[ XYZ* j + X] != 0 ){
      //if ( marker_position[ XYZ* j + X ] < 0 ){
      if ( marker_position[ XYZ* j + X ] < 0 && marker_position[ XYZ* j + Y ] > BALL_HEIGHT ){
	// find ball
	is_ball_found     = 1;
	for ( int n = 0; n < XYZ; n++ )
	  old_position[n]     = marker_position[ XYZ* j + n ];
	cout << "ball is found." << endl;
	break;
      }
    }
  }
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

void setMarkerPosition(void){
  char* tmp;
  int i = 0;  
  tmp = strtok( buffer, " " ); 
  while( tmp!= NULL ){
    tmp = strtok( NULL, " " ); 
    if ( tmp!= NULL ){
      marker_position[i] = atof( tmp );
      i++;
    }
  }
}

int main(){
  ////double future_ball_positions_list[FUTURE_TIME_NUM*NUM*XYZ] = {};
  //double future_ball_positions_list[FUTURE_TIME_NUM][NUM] = {};
  //double future_ball_positions_list[FUTURE_TIME_NUM][NUM*XYZ] = {};
  //cout << "a" << endl;
  //gettimeofday( &ini_t, NULL );

  // filename
  getFileNames();
  loadCommand();
  loadDotsIndexFile();
  loadHandDataFile();

  //for ( int i = 0; i < hand_num; i++ )
  //cout << i << " " << hand_positions[i][0] << " " << hand_positions[i][1] << " " << hand_positions[i][2] << endl;

  // get IP address
  char robotz_ip[NUM_BUFFER], ball_ip[NUM_BUFFER];
  getIPAddress( (char*) ROBOTZ_IP_FILE, robotz_ip );   
  getIPAddress( (char*) BALL_IP_FILE,   ball_ip );   
  printf( "ip address: %s (robotz), %s (ball)\n", robotz_ip, ball_ip );
  
  // server
  int robotz_socket  = connectSocket( robotz_ip, ROBOTZ_PORT );
  int ball_socket    = connectSocket( ball_ip,   BALL_PORT );

  // fd
  fd_set fds, readfds;
  int maxfd; // max value of file discripter for select 
  FD_ZERO( &readfds ); // initialize fd_set
  FD_SET( robotz_socket, &readfds ); // register 
  FD_SET( ball_socket,   &readfds );
  if ( robotz_socket > ball_socket )
    maxfd = robotz_socket;
  else
    maxfd = ball_socket;

  // check ready
  memset( buffer, 0, sizeof(buffer) ); recv( robotz_socket, buffer, sizeof("ready"), 0 ); printf( "robotz server: %s\n", buffer );
  memset( buffer, 0, sizeof(buffer) ); recv( ball_socket,   buffer, sizeof("ready"), 0 ); printf( "ball server:   %s\n", buffer );

  // loop
  int now_phase = -1, old_phase = -1;
  //int now_phase = 0, old_phase = -1;
  int b = 0, r = 0; // counter
  //int swing_time = 100000;
  //int is_swing = 0;
  
  gettimeofday( &ini_t, NULL );
  while (1){
    //cout << "r: " << r << ". b: " << b << ". phase: " << now_phase << ". time: "  << getElaspedTime() << endl;
      // terminate
      if ( now_phase >= NUM_OF_PHASE || getElaspedTime() > TIME_END ){
	// terminate server
	send( robotz_socket, "END", sizeof("END"), 0 );
	send( ball_socket,   "END", sizeof("END"), 0 );
	// close socket
	close( robotz_socket );
	close( ball_socket );
	// save
	saveRobotz(r);
	saveBall(b);
	break;
      }

    //// predict
    if ( b > ENOUGH_SAMPLE ){
      getBallCoeffs(b); // cout << "get coeffs." << endl;      
      //cout << coeffs_x[0] << " " << coeffs_x[1] << " " << coeffs_y[0] << " " << coeffs_y[1] << " " << coeffs_y[2] << " " << coeffs_z[0] << " " << coeffs_z[1] << endl;
      //predictBall(); // cout << "predict ball." << endl;

      if ( is_robotz_move == 0 ){
	predictBallLong(); // cout << "predict ball." << endl;
	detectHit(); // cout << "detect hit." << endl;
	//detectHit( hand_num ); // cout << "detect hit." << endl;
      }	
      if ( is_robotz_move > 0 && now_phase <= JUMP_PHASE2 ){
      //if ( is_robotz_move > 0 && now_phase == JUMP_PHASE2 ){
	//cout << "adjustment doing." << endl;
	predictBallShort(); // cout << "predict ball." << endl;
	detectTime(); // cout << "detect hit." << endl;
	//detectHit( hand_num ); // cout << "detect hit." << endl;
      }	

      /*
      cout << "in list ";
      for ( int n = 0; n < XYZ; n++ )
	for ( int f = 0; f < FUTURE_TIME_NUM; f++ )
	  future_ball_positions_list[ FUTURE_TIME_NUM* XYZ* b +  FUTURE_TIME_NUM* n + f ] = 
	    future_ball_positions[f][n];
	  //future_ball_positions_list[f][ XYZ*b + n ] = future_ball_positions[f][n];
      cout << "in list end" << endl;
      */
    }
    // initialize
    memcpy( &fds, &readfds, sizeof(fd_set) ); // ititialize
    select( maxfd + 1, &fds, NULL, NULL, NULL ); // wait 
        
    // ball
    if ( FD_ISSET( ball_socket, &fds )){
      memset( buffer, 0, sizeof(buffer) ); 
      recv( ball_socket, buffer, sizeof(buffer), 0 );
      //printf( "ball: %s\n", buffer );
      for ( int i = 0; i < MAX_MARKER_NUM; i++ )
	marker_position[i] = 0;

      //cout << "set marker position." << endl;
      setMarkerPosition();

      //memset( buffer, 0, sizeof(buffer) ); 
      //send( ball_socket, buffer, sizeof(buffer), 0 );
      
      //if ( marker_position[0] != 0 ){
      //for ( int i = 0; i < MAX_MARKER_NUM; i++ )
      //  cout << marker_position[i]<< " ";
      //cout << endl;
      //}

      if ( is_ball_found > 0 ){
	b += setBallPosition(b);
	setJumpTime(b);
	//setBallPosition(b);
	//cout << ball_positions[b-1][0] << " " << ball_positions[b-1][1] << " " << ball_positions[b-1][2] << endl;
	//cout << old_position[0] << " " << old_position[1] << " " << old_position[2] << endl;
	//b++;
      }else{
	findBall();
      }

      //if ( abs( ball_positions[b][0] ) > 0 )
      //cout << ball_positions[b][0] << " " << ball_positions[b][1] << " " << ball_positions[b][2] << endl;
    }
    
    // robotz    
    if ( FD_ISSET( robotz_socket, &fds )){
      if ( is_robotz_move )     
	now_phase = getPhaseNumber( getRobotzTime() );
      //printf( "num: %05d, phase: %02d, time: %9.3f ms \n", r, now_phase, getElaspedTime() );
     
      // terminate
      //if ( now_phase >= NUM_OF_PHASE || getElaspedTime() > TIME_END ){
	//saveRobotz(r);
	//saveBall(b);
	//break;
	//}
      //if ( now_phase >= NUM_OF_PHASE )
      //if ( now_time > TIME_END || now_time - swing_time > TIME_SWING ){
      //cout << "now: " << now_time << ", swing:" << swing_time << endl;
      // }

      // recieve, set state
      memset( buffer, 0, sizeof(buffer) );
      recv( robotz_socket, buffer, sizeof(buffer), 0); 
      if ( strlen( buffer ) > strlen( "sensor: " )){
	//printf( "robotz: %s\n", buffer );
	//cout << "set sensor value." << endl;
	setSensorValue(r);
      }
      //cout << "set valve value." << endl;
      setValveValue( r, now_phase );
      robotz_time[r] = getElaspedTime();

      // send command
      if ( now_phase != old_phase ){
      	printf("now phase: %d\n", now_phase );
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
  //saveRobotz(r);
  //saveBall(b);

  // terminate server
  //send( robotz_socket, "END", sizeof("END"), 0 );
  //send( ball_socket,   "END", sizeof("END"), 0 );
  
  // close
  //close( robotz_socket );
  //close( ball_socket );
  
  // save
  /*
  ofstream ofs( "/home/isi/tanaka/codes/robotz/data/ball.dat", ios::out);

  int o = 0;
  for ( int b_ = 0; b_ < b; b_++ )
    for ( int n = 0; n < XYZ; n++ )
	for ( int f = 0; f < FUTURE_TIME_NUM; f++ ){
	  ofs << future_ball_positions_list[o] << " ";
	  o++;
	//ofs << future_ball_positions_list[f][ XYZ*b_ + n ] << " ";
	  //ofs << endl;
	}
  */    
  return 0;
}



