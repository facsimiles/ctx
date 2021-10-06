#!/bin/bash

function check_ctx()
{
  echo -ne "\e[?200\$p"  #  DECRQM - request DEC mode 200
  read -s -t 0.01 -n 1 ch >  /dev/null
  tries=10; # for timing out on terminals without DECRQM
  while [ "$ch" != 'y' -a $tries != 0 ];do 
    # append read char until y
    terminal_response="$terminal_response$ch"
    tries=$(($tries+1))
    read -s -t 0.01 -n 1 ch > /dev/null
  done;
  terminal_response="$terminal_response$ch"
  # expected response is "\e?200;2$y", we only check 7th char
  if [ x${terminal_response:7:1} == x2 ];
    then is_ctx=1 ;
    else is_ctx=0 ;
  fi
}

check_ctx

if [ $is_ctx == 1 ];then
  echo "were in ctx"
fi


