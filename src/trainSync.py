#import os
import subprocess
#import time

#proc1 = subprocess.call(  './remoteControlPitchingMachine', shell=True )
#proc2 = subprocess.call(  './jumpHitMachine', shell=True )
#proc1 = subprocess.call(  './remoteControlPitchingMachine', shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
#proc2 = subprocess.call(  './jumpHitMachine', shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
proc1 = subprocess.Popen(  './remoteControlPitchingMachine', shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
proc2 = subprocess.Popen(  './jumpHitMachine', shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

#proc = subprocess.Popen( 'cat ../data/ip.dat', shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

#line = proc.stdout.readline()
#line = proc.stdout.readline()
#[ IP_camera1, b ] = line.split( " ", 1 )
#line = proc.stdout.readline()
#[ IP_camera2, b ] = line.split( " ", 1 )
#line = proc.stdout.readline()
#[ IP_pitching, b ] = line.split( " ", 1 )
#line = proc.stdout.readline()
#[ IP_robotz, b ] = line.split( " ", 1 )

#print IP_camera1 + ' ' +  IP_camera2 + ' ' +  IP_pitching + ' ' +  IP_robotz

#os.system( './saveBall ' + IP_camera1 + ' ' + IP_camera2 + ' &' )
#os.system( './saveBall ' + IP_camera1 + ' ' + IP_camera2 )
#proc1 = subprocess.Popen(  './saveBall ' + IP_camera1 + ' ' + IP_camera2, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

#time.sleep(1)
#os.system( './remoteControlPitchingMachine ' + IP_pitching + '&' )
#os.system( './remoteControlPitchingMachine ' + IP_pitching )
#proc2 = subprocess.Popen(  './remoteControlPitchingMachine ' + IP_pitching, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

#proc3= subprocess.Popen(  './remoteControlRobotZ ' + IP_robotz, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
#os.system( './remoteControlRobotZ ' + IP_robotz )






