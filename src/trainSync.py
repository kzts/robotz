import subprocess

proc1 = subprocess.Popen(  './remoteControlPitchingMachine', shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
proc2 = subprocess.Popen(  './jumpHitMachine', shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

