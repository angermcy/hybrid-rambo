#!/bin/bash

for k in 100 200 400 600 800
do
	for (( r=2; r<=8; r+=2))
	do
		for (( b=5; b<=20; b+=5))
		do
			for (( t = 1; t<=10; t++ ))
			do
				./build/rambo 0 -deserCPU -testCPUtime -logfile -K=$k -R=$r -B=$b -lfsuff="_iter_"$t 
				./build/rambo 0 -deserGPU -testGPUtime -logfile -K=$k -R=$r -B=$b -lfsuff="_iter_"$t 
			done
		done
	done

done

