#!/bin/bash

# This check is to be called inside xvfb-run, so that it has a working Xserver in which a simple yade GUI session can be started, the
# testGui.py script is only a slightly modified simple-scene-energy-tracking.py example.
# the screenshots have to be taken inside yade session, while the GUI windows are open.

YADE_EXECUTABLE=install/bin/yade-ci

# You can test locally using this script, just change YADE_EXECUTABLE into something that works for you:
#   YADE_EXECUTABLE=./examples/yade
#
# then launch this command:
#   xvfb-run -a -s "-screen 0 1600x1200x16" scripts/checks-and-tests/gui/testGui.sh
#
# or just this command:
#   scripts/checks-and-tests/gui/testGui.sh


testTool () {
	WHICHtool=`which $1`
	echo -e "is $1 present? We found this: ${WHICHtool}"
	ls -la $2
	if [[ ${3} == "ERROR_OK" ]] ; then
		echo "OK: ${1} presence is not obligatory."
	else
		if [[ $2 == "${WHICHtool}" ]] ; then
			echo " OK."
		else
			if [[ $3 == "${WHICHtool}" ]] ; then
				echo " OK (second path)"
			else
				echo "ERROR: $2 is not \"${WHICHtool}\", this script is too stupid for that, aborting."
				exit 1
			fi
		fi
		if [[ ! -f $2 ]] ; then
			echo "ERROR: $2 is missing, aborting."
			exit 1
		fi
	fi
}

testTool "xterm"   "/usr/bin/xterm"
testTool "scrot"   "/usr/bin/scrot"
testTool "xdotool" "/usr/bin/xdotool"
testTool "bash"    "/bin/bash"        "/usr/bin/bash"
testTool "gdb"     "/usr/bin/gdb"     "ERROR_OK"

echo -e "\n\n=== Will now test inside xterm, all usefull output, including gdb crash backtrace, will be on screenshots ===\n\n"

mkdir -p screenshots

# FIXME: this should be deduced automatically from the files matching pattern testGui*.py, see also testGui.py
#        currently these names are written manually inside:
#          *  scripts/checks-and-tests/gui/testGuiEmpty.py
#          *  scripts/checks-and-tests/gui/testGuiSimple.py
declare -a TESTS=( "Empty" "Simple" )

for TestFile in ${TESTS[@]}; do

	LOGFILE="screenshots/testGui_${TestFile}.txt"
	tail -F ${LOGFILE} &
	TAIL_PID=$!

	echo -e "******************************************\n*** Testing file testGui${TestFile}.py ***\n******************************************\nLog in file: ${LOGFILE}\ntail pid:${TAIL_PID}\n"

	/usr/bin/xterm -l -xrm "XTerm*logFile:${LOGFILE}" -geometry 100x48+5+560  -e /bin/bash -c "${YADE_EXECUTABLE} scripts/checks-and-tests/gui/testGui${TestFile}.py"

	# FIXME: the idea is to have a screenshot from outside of yade. But taking a screenshot after it finished (crashed, or by normal exit)
	#        will just produce an empty screenshot. It has to be done in a different way.
	# scrot -z scrBash01.png

	mv scr*.png screenshots
	sleep 0.25
	echo -e "******************************************\n*** Finished file testGui${TestFile}.py ***\n******************************************\n"
	kill -9 ${TAIL_PID}

# FIXME : this number 14 is hardcoded in scripts/checks-and-tests/gui/helper/testGuiHelper.py as self.maxTestNum=14
	if [[ ! -f screenshots/scr_${TestFile}_14.png ]] ; then
	    echo "File screenshots/scr_${TestFile}_14.png is missing, aborting."
	    exit 1
	fi

done

