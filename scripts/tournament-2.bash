#!/bin/bash

#- ile omijac gier 
skip=$1
shift 1

#- ile gier rozegrac
count=$1
shift 1

#- grajace silniki
engines=($@)
ret=

for i in `seq 0 $((${#engines[@]}-1))` ; do
	for j in `seq $(($i+1)) $((${#engines[@]}-1))` ; do
		if [ $skip -gt 0 ] ; then
			skip=$(($skip-1))
		elif [ $count -eq 0 ] ; then
			break
		else
			#- zapis do pliku SILNIK_2-vs-SILNIK_1.dat
			echo "${engines[$j]} vs ${engines[$i]}, count = $count"
			~/bin/gogui-twogtp -black ./${engines[$j]} -white ./${engines[$i]} -auto -size 9 -komi 6.5 -games 2 -sgffile ${engines[$j]}-vs-${engines[$i]} 
			count=$(($count-1))
		fi
	done
done
