#!/bin/bash -ex

BRANCH=`echo ${GITHUB_REF##*/}`





ls .
echo $pwd
echo $PWD

yuzupatch=( $(ls -d patches/* ) )
for i in "${yuzupatch[@]}"; do echo "$i"; done

mkdir build && cd build 

ls .
