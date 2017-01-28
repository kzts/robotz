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
#define ROBOTZ_PORT 7891
#define BALL_PORT   12345
#define MAX_MARKER_NUM 20
#define DISTANCE_LIMIT 200e-3
#define MAX_DISTANCE 10
#define ENOUGH_SAMPLE 10
#define MS_TO_SEC 1.0e-3
#define SEC_TO_MS 1.0e+3
#define HIT_DISTANCE 20e-3
#define NATNET_FPS 120.0

#define NUM 9999
//#define NUM_BUFFER 255
#define NUM_BUFFER 1024
#define XY 2
//#define NUM_PREDICT 100
#define NUM_PREDICT 300
#define XYZ 3
#define TXY 3
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
#define COMMAND_FILE "../data/robotz/command.dat"
#define HAND_FILE    "../data/20170128/hand.dat"
#define DOTS_FILE    "../data/20170128/dots.dat"
#define HAND_NUM 99999
//#define FUTURE_TIME_NUM 400
#define FUTURE_TIME_NUM 100
//#define TIME_TICK 3e-3
#define TIME_TICK 1e-3

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
int jump_time_list[HAND_NUM] = {};

double marker_position[MAX_MARKER_NUM] = {};
double old_position[XYZ] = {};

double data_valves[NUM][NUM_OF_CHANNELS] = {};
unsigned long data_sensors[NUM][NUM_ADC][NUM_ADC_PORT] = {};

int phase_time[NUM_OF_PHASE];
double time_switch[NUM_OF_PHASE] = {};
double value_valves[NUM_OF_CHANNELS] = {};
double value_valves_phase[NUM_OF_PHASE][NUM_OF_CHANNELS] = {};

struct timeval ini_t, now_t, robotz_ini_t;

char buffer[NUM_BUFFER];

char   filename_ball[NUM];
char   filename_results[NUM];
//char   filename_command[NUM];
//int    ball_positions1[NUM][XY];
//int    ball_positions2[NUM][XY];
double ball_positions1[NUM][XY];
double ball_positions2[NUM][XY];
double time0[NUM];
double time1[NUM];
double time2[NUM];

double robotz_time[NUM];
double ball_time[NUM];
double ball_positions[NUM][XYZ];

int motive_frame = 0;

//void saveResults(void){
//}

Vector2d coeffs1_x;
Vector3d coeffs1_y, coeffs2_x, coeffs2_y;

char coefficients_filename[] = "../data/params/regression_coefficients.dat";
char dummy_filename[]        = "../data/20161128/17_11_38_ball.dat";
char hitposition_filename[]  = "../data/params/hitposition.dat";
char waittime_filename[]     = "../data/params/waittime.dat";

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

//VectorXd future_time1vec(NUM_PREDICT);

int hand_num    = 0;
int dots_num    = 0;
int command_num = 0;

void changeJumpTime( int hit_jump_time ){
  time_switch[JUMP_PHASE2] = hit_jump_time;
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
  
    cout << ball_index << " " << hand_index << " " << min_distance << " " 
	 << ball_hit_time << " " << robotz_hit_time << " " << hit_knee_command << endl;

    if ( ball_hit_time < ( robotz_hit_time + ( 1.0/ NATNET_FPS )* MS_TO_SEC )){
    //if ( ball_hit_time < ( robotz_hit_time + TIME_TICK* MS_TO_SEC )){
      changeJumpTime( hit_jump_time );
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
  
    cout << ball_index << " " << hand_index << " " << min_distance << " " 
	 << ball_hit_time << " " << robotz_hit_time << " " << hit_knee_command << endl;

    if ( ball_hit_time < ( robotz_hit_time + ( 1.0/ NATNET_FPS )* MS_TO_SEC )){
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
    //t_vec_1(i) = ball_time[i];
    t_vec_1(i) = MS_TO_SEC* ball_time[i];
    //t_vec_2(i) = ball_time[i]* ball_time[i];
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

/*
void getVisionCoefficients(int NUM1,int NUM2){
  // get vector
  VectorXd t1_1(NUM1);
  VectorXd t1_2(NUM1);
  VectorXd b1_x(NUM1);
  VectorXd b1_y(NUM1);
  VectorXd t2_1(NUM2);
  VectorXd t2_2(NUM2);
  VectorXd b2_x(NUM2);
  VectorXd b2_y(NUM2);
  for ( int i = 0; i < NUM1; i++ ){
    t1_1(i) = time1[i];
    t1_2(i) = time1[i]* time1[i];
    b1_x(i) = ball_positions1[i][0];
    b1_y(i) = ball_positions1[i][1];
  }
  for ( int i = 0; i < NUM2; i++ ){
    t2_1(i) = time2[i];
    t2_2(i) = time2[i]* time2[i];
    b2_x(i) = ball_positions2[i][0];
    b2_y(i) = ball_positions2[i][1];
  }
  // coeffs
  MatrixXd A1_1 = MatrixXd::Ones(NUM1,2);
  MatrixXd A1_2 = MatrixXd::Ones(NUM1,3);
  MatrixXd A2_2 = MatrixXd::Ones(NUM2,3);
  A1_1.block(0,0,NUM1,1) = t1_1;
  A1_2.block(0,0,NUM1,1) = t1_2;
  A1_2.block(0,1,NUM1,1) = t1_1;
  A2_2.block(0,0,NUM2,1) = t2_2;
  A2_2.block(0,1,NUM2,1) = t2_1;

  MatrixXd tmp1_x = A1_1.transpose() * A1_1;
  MatrixXd tmp1_y = A1_2.transpose() * A1_2;
  MatrixXd tmp2_x = A2_2.transpose() * A2_2;
  MatrixXd tmp2_y = A2_2.transpose() * A2_2;
  coeffs1_x = tmp1_x.inverse() * A1_1.transpose() * b1_x;
  coeffs1_y = tmp1_y.inverse() * A1_2.transpose() * b1_y;
  coeffs2_x = tmp2_x.inverse() * A2_2.transpose() * b2_x;
  coeffs2_y = tmp2_y.inverse() * A2_2.transpose() * b2_y;
}

MatrixXd loadRegressionCoefficients(void){
  int row = 4, col = 3;
  ifstream File( coefficients_filename );
  MatrixXd coeffs_reg_(row,col);

  for ( int i = 0; i < row; i++ ){
    for ( int j = 0; j < col; j++ ){
      double tmp;
      File >> tmp;
      coeffs_reg_(i,j) = tmp;    
    }
  }
  return coeffs_reg_;
}

Vector3d loadHitPosition(void){
  ifstream File( hitposition_filename );
  Vector3d hit_position_;
  for ( int i = 0; i < XYZ; i++ ){
      double tmp;
      File >> tmp;
      hit_position_(i) = tmp;
  }
  return hit_position_;
}

int loadOptimalWaitTime(void){
  ifstream File( waittime_filename );
  int wait_time_opt_;
  File >> wait_time_opt_;
  return wait_time_opt_;
}
*/


/*
MatrixXd predictBallTrajectory(int now_time_, MatrixXd coeffs_reg_){
  //MatrixXd coeffs_reg = loadRegressionCoefficients();

  //VectorXd future_time1vec(NUM_PREDICT);
  VectorXd future_time2vec(NUM_PREDICT);
  for ( int i = 0; i < NUM_PREDICT; i++ ){
    int future_time = now_time_ + i;
    future_time1vec(i) = future_time;
    future_time2vec(i) = future_time* future_time;
  }

  MatrixXd future_time1mat = MatrixXd::Ones(NUM_PREDICT,2);
  MatrixXd future_time2mat = MatrixXd::Ones(NUM_PREDICT,3);
  future_time1mat.block(0,0,NUM_PREDICT,1) = future_time1vec;
  future_time2mat.block(0,0,NUM_PREDICT,1) = future_time2vec;
  future_time2mat.block(0,1,NUM_PREDICT,1) = future_time1vec;

  VectorXd ball1_x_predict = future_time1mat* coeffs1_x;
  VectorXd ball1_y_predict = future_time2mat* coeffs1_y;
  VectorXd ball2_x_predict = future_time2mat* coeffs2_x;
  VectorXd ball2_y_predict = future_time2mat* coeffs2_y;

  MatrixXd ball_predict_mat( NUM_PREDICT, 4 ); 
  ball_predict_mat.block(0,0,NUM_PREDICT,1) = ball1_x_predict;
  ball_predict_mat.block(0,1,NUM_PREDICT,1) = ball1_y_predict;
  ball_predict_mat.block(0,2,NUM_PREDICT,1) = ball2_x_predict;
  ball_predict_mat.block(0,3,NUM_PREDICT,1) = ball2_y_predict;

  MatrixXd ball_predict_world_ = ball_predict_mat* coeffs_reg_;
  return ball_predict_world_;
}

int getHitTime(MatrixXd ball_predict_world_, Vector3d hit_position_ ){
  //double hit_position[3] = { 199.89, 1404.8, 803.54 };
  double distance_min = 1e+9;
  double hit_time_ = 1e+9;

  for ( int i = 0; i < NUM_PREDICT; i++ ){  
    double distance = 0;
    for ( int j = 0; j < XYZ; j++ ){
      double tmp = ball_predict_world_(i,j) - hit_position_(j);
      distance = distance + tmp*tmp;
    }
    if ( distance < distance_min ){
      distance_min = distance;
      hit_time_ = future_time1vec(i);
    }
  }
  return hit_time_;
}
*/

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

/*
void setSwingCommand(void){
  value_valves[NUM_ARM] = Arm_pressure;
  memset( buffer, 0, sizeof(buffer) );
  strcpy( buffer, "command: " ); 
  char tmp[9];
  for ( int j = 0; j < NUM_OF_CHANNELS; j++ ){
    sprintf( tmp, "%4.3f ", value_valves[j] );
    strcat( buffer, tmp );
  }
}
*/

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
      tmp = strtok( NULL, " " ); 
      data_sensors[i][j][k] = atof( tmp );
    }
  }
}

void setValveValue(int i, int phase){
  int j;
  for ( j = 0; j < NUM_OF_CHANNELS; j++ )
    data_valves[i][j] = value_valves[j];
    //data_valves[i][j] = value_valves_phase[phase][j];
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
  // sprintf( filename_command, "../data/%04d%02d%02d/%02d_%02d_%02d_command.dat", 
  //year, month, day, hour, minute, second );
  printf("ball file:  %s\n", filename_ball );
  printf("robot file: %s\n", filename_results );
}

void saveDat(void){
  //printf("save init: %s\n", filename_ball );
  int i,j,k;
  //FILE *fp_bal, *fp_res, *fp_cmd;
  //FILE *fp_bal, *fp_res;
  FILE *fp_res;
  char str[NUM_BUFFER];

  // open
  //fp_bal = fopen( filename_ball,    "w" );
  fp_res = fopen( filename_results, "w" );
  //fp_cmd = fopen( filename_command, "w" );
  //if ( fp_bal == NULL ){
  //printf( "File open error: %s\n", filename_ball );
  //return;
  //}
  if ( fp_res == NULL ){
    printf( "File open error: %s\n", filename_results );
    return;
  }
  //if ( fp_cmd == NULL ){
  //printf( "File open error: %s\n", filename_command );
  //return;
  //}
  /*
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
  */
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
  /*
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
  */
  // close
  //fclose( fp_cmd );
  fclose( fp_res );
  //fclose( fp_bal );

  //printf( "save done: %s\n", filename_command );
  printf( "save done: %s\n", filename_results );
  //printf( "save done: %s\n", filename_ball );
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
  }
  return is_found;
}

void findBall(void){
  for ( int j = 0; j < MAX_MARKER_NUM; j++ ){
    if ( marker_position[ XYZ* j ] != 0 ){
      if ( marker_position[ XYZ* j ] < 0 ){
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
  //double (*future_ball_positions_list);
  //future_ball_positions_list = malloc( sizeof( double )* FUTURE_TIME_NUM * NUM * XYZ );


  //vector< vector<double> > future_ball_positions_list;
  //future_ball_positions_list.resize( FUTURE_TIME_NUM* NUM* XYZ );

  //for ( int f = 0; f < FUTURE_TIME_NUM; f++ )
  //for ( int b = 0; b < NUM; b++ )
  //for ( int n = 0; n < XYZ; n++ )
  //future_ball_positions_list[ f*XYZ*NUM + XYZ*b + n ] = 0.0;
	//future_ball_positions_list[f][ XYZ*b + n ] = 0;

  //double future_ball_positions_list[500*3*1000] = {};
  ////double future_ball_positions_list[FUTURE_TIME_NUM*NUM*XYZ] = {};
  //double future_ball_positions_list[FUTURE_TIME_NUM][NUM] = {};
  //double future_ball_positions_list[FUTURE_TIME_NUM][NUM*XYZ] = {};
  // double future_ball_positions_list[00][3000] = {};
  //cout << "a" << endl;
  //gettimeofday( &ini_t, NULL );

  // filename
  getFileNames();
  loadCommand();
  loadDotsIndexFile();
  loadHandDataFile();

  //for ( int i = 0; i < hand_num; i++ )
  //cout << i << " " << hand_positions[i][0] << " " << hand_positions[i][1] << " " << hand_positions[i][2] << endl;

  //Vector3d hit_position = loadHitPosition();
  //MatrixXd coeffs_reg   = loadRegressionCoefficients();
  //int wait_time_opt     = loadOptimalWaitTime();

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
  int now_phase = 0, old_phase = -1;
  int b = 0, r = 0; // counter
  int swing_time = 100000;
  int is_swing = 0;
  
  gettimeofday( &ini_t, NULL );
  while (1){
    //// terminate
    //int now_time = getElaspedTime();
    //if ( now_time > TIME_END || now_time - swing_time > TIME_SWING ){
    //cout << "now: " << now_time << ", swing:" << swing_time << endl;
    //setExhaustCommand();
    //send( robotz_socket, buffer, NUM_BUFFER, 0 );
    //break;
    //}
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
      if ( is_robotz_move > 0 && now_phase == JUMP_PHASE2 ){
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
    //int wait_time = 1000;
    //if ( c1 > 5 && c2 > 5){
    //getVisionCoefficients(c1,c2);
    //int now_time = getElaspedTime();
    //MatrixXd ball_predict_world = predictBallTrajectory( now_time, coeffs_reg );
    //int hit_time = getHitTime( ball_predict_world, hit_position );
    //wait_time = hit_time - now_time;
    //cout << "wait: " << wait_time << endl;
    //}
    // initialize
    memcpy( &fds, &readfds, sizeof(fd_set) ); // ititialize
    select( maxfd + 1, &fds, NULL, NULL, NULL ); // wait 
        
    // ball
    if ( FD_ISSET( ball_socket, &fds )){
      memset( buffer, 0, sizeof(buffer) ); 
      recv( ball_socket, buffer, sizeof(buffer), 0 );

      for ( int i = 0; i < MAX_MARKER_NUM; i++ )
	marker_position[i] = 0;

      setMarkerPosition();

      memset( buffer, 0, sizeof(buffer) ); 
      send( ball_socket, buffer, sizeof(buffer), 0 );
      
      //if ( marker_position[0] != 0 ){
      //for ( int i = 0; i < MAX_MARKER_NUM; i++ )
      //  cout << marker_position[i]<< " ";
      //cout << endl;
      //}

      if ( is_ball_found > 0 ){
	b += setBallPosition(b);
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
      //int now_time = getElaspedTime();
      if ( now_phase >= NUM_OF_PHASE )
      //if ( now_time > TIME_END || now_time - swing_time > TIME_SWING ){
      //cout << "now: " << now_time << ", swing:" << swing_time << endl;
	break;
      // }

      // recieve, set state
      memset( buffer, 0, sizeof(buffer) );
      recv( robotz_socket, buffer, sizeof(buffer), 0); //printf( "robotz: %s\n", buffer );
      if ( strlen( buffer ) > strlen( "sensor: " ))
	setSensorValue(r);
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
  // terminate server
  send( robotz_socket, "END", sizeof("END"), 0 );
  send( ball_socket,   "END", sizeof("END"), 0 );

  //saveDat();
  
  // close
  close( robotz_socket );
  close( ball_socket );
  
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

  //free( future_ball_positions_list );
  */    
  return 0;
}



