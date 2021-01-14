#!/bin/bash -ex

BRANCH=`echo ${GITHUB_REF##*/}`

ver=$(cat /yuzu/README.md | grep -o 'early-access [[:digit:]]*' | cut -c 14-17)
title="yuzu Early Access $ver"


yuzupatch=( $(ls -d patches/* ) )
for i in "${yuzupatch[@]}"; do patch -p1 < "$i"; done

find . -name "CMakeLists.txt" -exec sed -i 's/^.*-Werror$/-W/g' {} +
find . -name "CMakeLists.txt" -exec sed -i 's/^.*-Werror=.*)$/ )/g' {} +
find . -name "CMakeLists.txt" -exec sed -i 's/^.*-Werror=.*$/ /g' {} +
find . -name "CMakeLists.txt" -exec sed -i 's/-Werror/-W/g' {} +

#Fix freezing when closing the window
sed -i -e '0,/is_thread_exiting.store(true);/! s/is_thread_exiting.store(true);/&\n    cv.notify_all();/' src/video_core/shader/async_shaders.cpp
#Fix Deriving Keys window not being a child of main
sed -i -e 's/QProgressDialog prog;/QProgressDialog prog(this);/g' src/yuzu/main.cpp

mkdir build && cd build 

cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/lib/ccache/gcc -DCMAKE_CXX_COMPILER=/usr/lib/ccache/g++ -DTITLE_BAR_FORMAT_IDLE="$title" -DTITLE_BAR_FORMAT_RUNNING="$title | {3}" -DENABLE_COMPATIBILITY_LIST_DOWNLOAD=ON -DGIT_BRANCH="HEAD" -DGIT_DESC="$msvc" -DUSE_DISCORD_PRESENCE=ON -DENABLE_QT_TRANSLATION=ON

ninja


cd /tmp
curl -sLO "https://raw.githubusercontent.com/$GITHUB_REPOSITORY/$BRANCH/.github/workflows/appimage.sh"
chmod a+x appimage.sh
./appimage.sh
