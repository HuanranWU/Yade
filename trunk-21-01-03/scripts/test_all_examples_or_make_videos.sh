#!/usr/bin/zsh
# INFO: run this script inside ./scripts
#       It can also be used to check if the examples are working (*1*). Only need to modify line the_examples=…… to use a different grep pattern
#       Then when asked '(Press Ctrl-C when recording is complete) Ready? [y/n] :' press n to skip recording. Then a new example is tested.
#       Also can start more examples simultaneously - just press 'n' to start next one without closing the previous one.

# This script automates creation of videos (*2*) from examples. First it greps list_of_examples for pattern "...│...│no │"
# to identify examples which do not have a video uploaded.
# Then it runs a loop over all those examples. Opens two terminals, in first terminal there is yade running,
# ready to start simulation. In second terminal waits until Ready [y/n] to start recording.
# If 'n' is pressed, then it skips recording.


# this command I used to verify that I got a correct regexp for finding the examples.
#grep -E "...│...│no │" ../examples/list_of_examples.txt | grep -E "\<[A-Za-z_0-9-]+\>\.py\>" --color

# get a list of all examples which have "no" uploaded video
#grep -E "...│...│no │" ../examples/list_of_examples.txt | sed -e "s/.*\(\<[A-Za-z_0-9-]\+\>\.py\>\).*/\1/"

# (*1*) Test if they work
the_examples=("${(@f)$(grep -E "[Oo][Kk].│...│...│...│" ../examples/list_of_examples.txt | sed -e "s/.* \([A-Za-z_0-9-]\+\>\.py\>\).*/\1/")}")
# (*2*) Make videos
#the_examples=("${(@f)$(grep -E "...│...│...│no │" ../examples/list_of_examples.txt | sed -e "s/.* \([A-Za-z_0-9-]\+\>\.py\>\).*/\1/")}")

mkdir -p /tmp/video
mkdir -p /tmp/testEx
cp ../examples/yade /tmp/testEx

# sane default for small resolution screens
#XTERM1_POS=102x37+0+0
#XTERM2_POS=102x37+0+720

XTERM1_POS=102x37+2680+2094
XTERM2_POS=102x37+2680+2854

echo "If you can't see any terminals oppened then change \${XTERM1_POS} and \${XTERM2_POS} in this script.\n"

total_size=${#the_examples[*]}
count=0

for example in $the_examples; do
	file_location=("${(@f)$(find ../ -name "${example}" -type f)}")
	print "Working on ${example} in ${file_location}, ${count}/${total_size}"
	count=$((${count}+1))
	DIRPLACE=`dirname ${file_location}`
	PARENT=`basename ${DIRPLACE}`
	cp -a ${DIRPLACE} /tmp/testEx/${PARENT}
	SUCCESS=$?
	if [ ${SUCCESS} -eq 0 ]; then
		BASENAME=`basename ${example} .py`
		xterm -en UTF-8 -b 0 -bg black -fg darkgray -geometry ${XTERM1_POS} -fn "-Misc-Fixed-Medium-R-Normal--20-200-75-75-C-100-ISO10646-1" -e bash -c "cd /tmp/testEx/${PARENT} ; ../yade ./${example}" &!
		xterm -en UTF-8 -b 0 -bg black -fg darkgray -geometry ${XTERM2_POS} -fn "-Misc-Fixed-Medium-R-Normal--20-200-75-75-C-100-ISO10646-1" -e /usr/bin/zsh -c "\
		cd /tmp/video;\
		echo \"working on ${example} as ${BASENAME}. ${count}/${total_size}\";\
		echo \"\";\
		REPLY=\"n\";\
		read -q REPLY\\?\"1. Video is recorded in top left corner with size 1024x768
2. Press 'y' and when recording is complete press Ctrl-C
3. Press 'n' to not record video and launch next example

Ready? [y/n] :\";\
		echo \$REPLY;\
		if test \$REPLY = \"y\" ; then;\
			echo OK, recording ${example};\
			sleep 1;\
			cd /tmp/video
			recordmydesktop -x 0 -y 0 --width 1024 --height 768 -o ${BASENAME}.ogv --no-sound
		fi;
		"
	else
		echo "Error copying file ${example}"
		sleep 1
	fi
done

rm -rf /tmp/testEx

echo "\nRecorded videos are in /tmp/video"

