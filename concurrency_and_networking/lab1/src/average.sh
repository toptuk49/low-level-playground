#!/bin/bash

average=0
count=0

for argument in $@
do
	let average=average+argument
	let count=count+1
done

let average=average/count

cat <<EOF
Количество аргументов:
EOF

echo $count

cat <<EOF
Среднее арифметическое:
EOF

echo $average
