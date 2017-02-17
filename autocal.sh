#!/bin/bash
oneWayAnalysor="oneWayAnalysor.cc"
temp_out="entropy.txt"
bench=("adobe" "baiduMap" "jingdong" "k9" "weibo" "wps" "yuedong")
#bench=("jingdong" "k9" "weibo" "wps" "yuedong")
#mtype=("indirect" "cond" "total")
mtype=("total")
interval=(50 60 70 80 90 100 110 120 130)
#interval=(50 60 130)
#tag="-Wno-c++11-extensions"
tag="-std=c++11"

# three params:
# bench/mtype/interval
function threeBP()
{
#echo gshare bench mtype interval
	echo "-------start-"${1}"-"${2}"-"${3}"-----"
	g++ $tag $oneWayAnalysor
#two params: mtype/interval
	#./a.out $2 $3
	./a.out $3
	mv $temp_out ${1}"_new_"${2}"_"${3}"_entropy.txt"
}

length_bench=${#bench[@]}
length_mtype=${#mtype[@]}
length_interval=${#interval[@]}

for ((i = 0; i < $length_bench; i++));do
	cp /mnt/hdd2/wangw/workspace/graduation/Mobybench/${bench[$i]}/branch_record.txt ./
	for ((j = 0; j < $length_mtype; j++));do
		for ((k = 0; k < $length_interval; k++));do
			threeBP ${bench[$i]} ${mtype[$j]} ${interval[$k]}
		done
	done
done
