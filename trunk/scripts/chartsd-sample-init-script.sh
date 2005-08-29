#!/bin/sh

# which config file to use?
CONFIGFILE="/usr/share/chartsd/temp.cpu.conf"

# if you want to run this daemon more than once, 
# then you have to copy this init-script and then
# to increase this number in the new init-script
instance=1


program="chartsd"
PATH=/usr/sbin:/usr/local/bin:${PATH}

# check if it is already running
running=0
if [ -e /var/run/${program}.${instance}.pid ]; then
    PID=`ps ax | grep "${program} -c ${CONFIGFILE}" | head -n 1 | sed -e 's/^\s*\([0-9]*\)\s.*/\1/'`
    status=`ps -p ${PID} -o comm=`
    if [ "${status}" != "" ]; then
	running=1
    fi
fi

# See how we were called.
case "$1" in
  start)
	if [ ${running} -eq 1 ]; then
	    echo "${program} (instance: ${instance}) is already running! Stop it first!"
	else
	    # Start daemon.
	    echo " + Starting ${program} (instance: ${instance})..."
	    ${program} -c ${CONFIGFILE}
	    pid=`ps ax | grep "${program} -c ${CONFIGFILE}" | head -n 1 | sed -e 's/^\s*\([0-9]*\)\s.*/\1/'`
	    rm -f /var/run/${program}.${instance}.pid &>/dev/null
	    echo ${pid} > /var/run/${program}.${instance}.pid
	    echo " > new PID is: ${pid}"
	fi
	;;
  stop)
	if [ ${running} -eq 0 ]; then
	    echo "${program} (instance: ${instance}) is not running! Start it first!"
	else
	    # Stop daemon.
	    echo " - Shutting down ${program}..."
	    kill -9 ${PID}
	    status=`ps -p ${PID} -o comm=`
	    if [ "${status}" != "" ]; then
		echo "${program} could not be stopped!!!"
	    else
		rm -f /var/run/${program}.${instance}.pid &>/dev/null
	    fi
	fi
	;;
  restart)
	$0 stop
	$0 start
	;;
  *)
	echo "Usage: $0 {start|stop|restart}"
	exit 1
esac

exit 0

