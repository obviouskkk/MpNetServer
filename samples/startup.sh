#! /bin/bash 
#如果文件夹不存在，创建文件夹
if [ ! -d "log"  ]; then
     mkdir log
fi
kill -TERM `ps -ef | grep "echoSrv "  | awk '{print $2}'`
sleep 1
./echoSrv ./bench.ini
