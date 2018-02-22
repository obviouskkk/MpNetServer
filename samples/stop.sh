#########################################################################
# File Name: stop.sh
# author: obvious
# mail: 942744575@qq.com
# Created Time: Mon 24 Apr 2017 05:59:22 PM CST
#########################################################################
#!/bin/bash
pid=`ps -ef | grep "echoSrv " | awk '{print $2}'`
if [[ ${pid} ]];then
     kill -TERM  $pid
    sleep 1
fi 
echo "stop $pid"
