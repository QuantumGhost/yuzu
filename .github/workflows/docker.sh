#!/bin/bash -ex

BRANCH=`echo ${GITHUB_REF##*/}`

ver=$(cat /yuzu/README.md | grep 'early-access [[:digit:]]*' | cut -c 14-17)
title="yuzu Early Access $ver"


yuzupatch=( $(ls -d patches/* ) )
for i in "${yuzupatch[@]}"; do patch -p1 < "$i"; done

mkdir build && cd build 

cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/lib/ccache/gcc -DCMAKE_CXX_COMPILER=/usr/lib/ccache/g++ -DTITLE_BAR_FORMAT_IDLE="$title" -DTITLE_BAR_FORMAT_RUNNING="$title | {3}" -DENABLE_COMPATIBILITY_LIST_DOWNLOAD=ON -DGIT_BRANCH="HEAD" -DGIT_DESC="$msvc" -DUSE_DISCORD_PRESENCE=ON

ninja


cd /tmp
curl -sLO "https://raw.githubusercontent.com/$GITHUB_REPOSITORY/$BRANCH/.github/workflows/appimage.sh"
chmod a+x appimage.sh
./appimage.sh
