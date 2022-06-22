#!/bin/bash

zenity --question --timeout=10 --title="yuzu updater" --text="New update available. Update now?" --icon-name=yuzu --window-icon=yuzu.svg --height=80 --width=400
answer=$?

directory=${APPIMAGE%$ARGV0}

if [ "$answer" -eq 0 ]; then 
	if [ -w $directory ] ; then
		$APPDIR/usr/bin/AppImageUpdate $APPIMAGE && "$directory"yuzu-x86_64.AppImage "$@"
	else
		echo -e "Cannot update in $directory\n\n"
		$APPDIR/AppRun-patched "$@"
	fi
elif [ "$answer" -eq 1 ]; then
	$APPDIR/AppRun-patched "$@"
elif [ "$answer" -eq 5 ]; then
	$APPDIR/AppRun-patched "$@"
fi
exit 0
