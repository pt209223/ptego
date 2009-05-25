#!/bin/bash

#skip=$1
#shift

#count=$1
#shift

attach=$1
shift

engines=($@)
games=2
ret=

for i in `seq 0 $((${#engines[@]}-1))` ; do
	if ! [ ${engines[$i]} = $attach ] ; then
		#if [ $skip -gt 0 ] ; then skip=$(($skip-1)) ;
		#elif [ $count -eq 0 ] ; then break ; 
		#else
			#count=$(($count-1))
			
			echo "Playing : ${engines[$i]} vs $attach"# ($count)"
			~/bin/gogui-twogtp -black ./${engines[$i]} -white ./$attach -auto -size 9 -komi 6.5 -games $games -sgffile ${engines[$i]}-vs-$attach
			echo "Playing : $attach vs ${engines[$i]}"
			~/bin/gogui-twogtp -black ./$attach -white ./${engines[$i]} -auto -size 9 -komi 6.5 -games $games -sgffile $attach-vs-${engines[$i]}
		#fi
	fi
done
