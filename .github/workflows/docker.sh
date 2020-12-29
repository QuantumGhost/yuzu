#!/bin/bash -ex

BRANCH=`echo ${GITHUB_REF##*/}`

title=$(cat /yuzu/readme.md | grep 'early-access [[:digit:]]*' | cut -c 14-17)


yuzupatch=( $(ls -d patches/* ) )
for i in "${yuzupatch[@]}"; do patch -p1 < "$i"; done

mkdir build && cd build 

ls .
