// std
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <time.h>
//#include <sys/time.h>
// socket
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
// ip
//#include <unistd.h>
//#include <sys/types.h>
// c++
#include <fstream>
#include <string>
#include <iostream>
//#include "Eigen/Core"
//#include "Eigen/Geometry"
//#include "Eigen/LU"

using namespace std;

#define NUM 99999
#define MAX_MARKER_NUM 20
#define XYZ 3
#define BUFFER_NUM 1024

double marker_data[NUM][MAX_MARKER_NUM* XYZ] = {};

char ball_filename[]  = "../data/20170121/12_33_04_natnet.dat";

int getFileLineNum( const char* filePath ){
  // line number
  int i = 0;
  ifstream ifs( filePath );
  if( ifs ){
      string line;
      while( true ){
	  getline( ifs, line );
 	  if( ifs.eof() )
	    break;
	  i++;
      }
  }
  return i;
}
int loadBall( const char* filePath ){
  int time_num = getFileLineNum( filePath );
  cout << time_num << endl;
  
  // marker data
  FILE *fp;
  char buffer[BUFFER_NUM];
  char *tmp;

  fp = fopen( filePath, "r");
  for ( int j = 0; j < time_num; j++ ){
  //for ( int j = 0; j < 10; j++ ){

    fgets( buffer, BUFFER_NUM, fp );
    //cout << buffer << endl;
    
    tmp = strtok( buffer, " " ); 
    //cout << tmp << " ";

    for ( int k = 1; k < MAX_MARKER_NUM* XYZ; k++ ){
      tmp = strtok( NULL, " " ); 
      //cout << tmp << " ";
      marker_data[j][k] = atof( tmp );
    }

    //cout << endl;
    
  }

}

int main(){
  loadBall( ball_filename );
  
  return 0;
}



