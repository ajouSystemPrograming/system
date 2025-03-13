#!/bin/bash

SERVER_IP="192.168.163.8"
PORT="8080"
FILEPATH=~/sysprog/Sub_Pi-1/a.out

while :
do
	$FILEPATH $SERVER_IP $PORT
	sleep 1
done
