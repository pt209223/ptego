#!/bin/sh

first=$1
second=$2
name=$3
games=5

shift 3

if [ $# -eq 0 ] ; then
	#- $1 : $2 zapis do pliku *-01.dat
	~/bin/gogui-twogtp -black "$first" -white "$second" -auto -size 9 -komi 6.5 -games $games -sgffile $name-01 
	#- $2 : $1 zapis do pliku *-10.dat
	~/bin/gogui-twogtp -black "$second" -white "$first" -auto -size 9 -komi 6.5 -games $games -sgffile $name-10 
	exit 0
fi

for host in $@ ; do 
	echo -en "Odpalam na $host... "
	
	ssh $host "cd `pwd` ; screen -dmS play-$host ./multi.sh $first $second $name-$host" 

	if [ $? -eq 0 ] ; then 
		echo "OK"
	else 
		echo "PROBLEM"
	fi

done

