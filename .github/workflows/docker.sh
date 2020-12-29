#!/bin/bash -ex

BRANCH=`echo ${GITHUB_REF##*/}`



cd pineapple-src

ls .

mkdir build && cd build 

ls .
