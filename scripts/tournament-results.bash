#!/bin/bash

#- grajace silniki
engines=($@)
N=${#engines[@]}

ret=(`for i in \`seq 1 $N\` ; do echo -en " " ; done`)
winB=(`for i in \`seq 1 $N\` ; do echo 0 ; done`)
winW=(`for i in \`seq 1 $N\` ; do echo 0 ; done`)
losB=(`for i in \`seq 1 $N\` ; do echo 0 ; done`)
losW=(`for i in \`seq 1 $N\` ; do echo 0 ; done`)

#- !!! Pliki musza miec postac : !!!
#- !!! SILNIK_1-vs-SILNIK_2.dat  !!!

for i in `seq 0 $(($N-1))` ; do
	for j in `seq 0 $(($i-1))` ; do
		B=`grep -c 'B+R' ${engines[$i]}-vs-${engines[$j]}.dat 2>/dev/null`
		W=`grep -c 'W+R' ${engines[$i]}-vs-${engines[$j]}.dat 2>/dev/null`
		
		if [ -z $B ] || [ -z $W ] ; then
			ret[$i]="${ret[$i]}    "
		else
			ret[$i]="${ret[$i]}$B:$W "
			winB[$i]=$((${winB[$i]}+$B))
			losB[$i]=$((${losB[$i]}+$W))
			losW[$j]=$((${losW[$j]}+$B))
			winW[$j]=$((${winW[$j]}+$W))
		fi
	done

	ret[$i]="${ret[$i]} -  "

	for j in `seq $(($i+1)) $(($N-1))` ; do
		B=`grep -c 'B+R' ${engines[$i]}-vs-${engines[$j]}.dat 2>/dev/null`
		W=`grep -c 'W+R' ${engines[$i]}-vs-${engines[$j]}.dat 2>/dev/null`

		if [ -z $B ] || [ -z $W ] ; then
			ret[$i]="${ret[$i]}    "
		else
			ret[$i]="${ret[$i]}$B:$W "
			winB[$i]=$((${winB[$i]}+$B))
			losB[$i]=$((${losB[$i]}+$W))
			losW[$j]=$((${losW[$j]}+$B))
			winW[$j]=$((${winW[$j]}+$W))
		fi
	done
done

for i in `seq 0 $(($N-1))` ; do
	name=`echo ${engines[$i]} | cut -f 2- -d '-'`

	printf "%10s | ${ret[$i]} = %2d:%2d, %2d:%2d, %2d:%2d\n" "$name" \
		${winB[$i]} ${losB[$i]} ${winW[$i]} ${losW[$i]} \
		$((${winB[$i]}+${winW[$i]})) $((${losB[$i]}+${losW[$i]}))
done

#- Sortowanie wg wynikow
sorted=""

for i in `seq 0 $(($N-1))` ; do
	max=0
	maxid=10000

	for j in `seq 0 $(($N-1))` ; do 
		cur=$((${winB[$j]}+${winW[$j]}))
		if [ $cur -gt $max ] || [ $max -eq 0 ] ; then
			max=$cur
			maxid=$j
		fi
	done

	if [ $maxid -ne 10000 ] ; then
		sorted="$sorted ${engines[$maxid]}"
	fi

	winB[$maxid]=0
	winW[$maxid]=0
done

echo "Check: ./results.bash $sorted"

