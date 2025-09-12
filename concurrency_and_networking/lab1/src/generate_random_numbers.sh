#!/bin/bash

i=0

while [ $i -lt 150 ]
do
	od -An -N2 -d /dev/random >> numbers.txt
	let i=i+1
done
