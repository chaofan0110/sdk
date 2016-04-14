#!/bin/bash

echo "************************************************************************************************"
echo "System Snapshot"
echo "System Load Average:"
top -n 1 -b 
echo "------------------------------------------------------------------------------------------"


echo "System Disk Used:"
df
echo "------------------------------------------------------------------------------------------"


echo "System Memory Used:"
free
echo "------------------------------------------------------------------------------------------"


echo "Process informations:"
ps aux 
echo "------------------------------------------------------------------------------------------"


echo "BS Threads:"
ps H -eo user,pid,ppid,tid,time,%cpu,cmd | grep bsd | grep -v grep
echo "------------------------------------------------------------------------------------------"


echo "BS Stack:"
pstack `cat /ipcc/var/run/pid/bsd.pid`
echo "------------------------------------------------------------------------------------------"


echo "SC Threads:"
ps H -eo user,pid,ppid,tid,time,%cpu,cmd | grep scd | grep -v grep
echo "------------------------------------------------------------------------------------------"


echo "SC Stack:"
pstack `cat /ipcc/var/run/pid/scd.pid`
echo "------------------------------------------------------------------------------------------"

echo "FS Threads:"
ps H -eo user,pid,ppid,tid,time,%cpu,cmd | grep freeswitch | grep -v grep
echo "------------------------------------------------------------------------------------------"

echo "FS Stack:"
pstack `cat /usr/local/freeswitch/run/freeswitch.pid`
echo "------------------------------------------------------------------------------------------"

echo "Netstat:"
netstat -p
echo "------------------------------------------------------------------------------------------"

echo "************************************************************************************************"
