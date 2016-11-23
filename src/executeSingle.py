import sys
import datetime
import subprocess
import time

# pressure
if len(sys.argv) < 2:
    print 'input: pressure (015, 020, 025, 030).'
    sys.exit()
pressure  = sys.argv[1]

# set file name
d    = datetime.datetime.today()
today = str("{0:04d}".format(d.year)) + str("{0:02d}".format(d.month)) + str("{0:02d}".format(d.day))
now  = str("{0:02d}".format(d.hour)) + str("{0:02d}".format(d.minute)) + str("{0:02d}".format(d.second))

# get ip
proc_ip = subprocess.Popen( ['cat ../data/single/ip.dat'], shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
out = proc_ip.communicate()
ip = out[0]

# command
copy_bk1 = "cp ../data/single/command.dat ../data/single/command.bak"
copy_bk2 = "cp ../data/single/command.bak ../data/single/command.dat"
copy_cmd = "cp ../data/single/command/" + pressure + ".dat ../data/single/command.dat"
copy_res = "cp ../data/single/results.dat ../data/single/" + today + "/" + now + ".dat"
execute  = "./remoteControlSingle " + ip

#print date
#print ip
subprocess.call( copy_bk1, shell=True )
print copy_bk1
subprocess.call( copy_cmd, shell=True )
print copy_cmd
subprocess.call( execute, shell=True )
print execute
subprocess.call( copy_res, shell=True )
print copy_res
subprocess.call( copy_bk2, shell=True )
print copy_bk2

