#!/usr/bin/zsh
# This script automatically adds links to youtube videos of examples. First it greps ../examples/list_of_examples for pattern "...│...│YES│"
# to identify examples which have a video uploaded.
# Then it runs a loop over all those examples and appends them to the file.

# http://docutils.sourceforge.net/docs/user/rst/quickref.html
# http://openalea.gforge.inria.fr/doc/openalea/doc/_build/html/source/sphinx/rest_syntax.html
# http://docutils.sourceforge.net/docs/ref/rst/restructuredtext.html#anonymous-hyperlinks
# https://stackoverflow.com/questions/30822880/python-sphinx-anchor-on-arbitrary-line
# section levels: #, *, =, -, ^, ",
the_examples=("${(@f)$(grep -E "...│...│YES│" ../examples/list_of_examples.txt | sed -e "s/.* \([A-Za-z_0-9-]\+\>\.py\>\).*/\1/")}")

PREV_SECTION=""
NEW_SECTION=""

for example in $the_examples; do
	file_location=("${(@f)$(find . -name "${example}" -type f -printf "%P\n")}")
	print "Working on ${example} in ${file_location}"
	if [[ -f ${file_location} ]]; then
		BASENAME=`basename ${example} .py`
		TMP_SECTION=`dirname ${file_location}`
		TMP2_SECTION=`echo "$TMP_SECTION" | cut -d "/" -f1`
		# capitalize first letter in section name
		NEW_SECTION="$(tr '[:lower:]' '[:upper:]' <<< ${TMP2_SECTION:0:1})${TMP2_SECTION:1}"
		YOUTUBEURL=`grep -E " ${example}\>" ../examples/list_of_examples.txt | sed -e "s/.* ${example}\>.*https:\/\/youtu.be\/\([^ ]\+\).*/\1/"`
		# make sure that .rst references do not contain illegal characters
		SANITIZE=`echo ${BASENAME} | sed -e "s/[^A-Za-z0-9]/-/g"`
		Sanitize="$(tr '[:lower:]' '[:upper:]' <<< ${SANITIZE:0:1})${SANITIZE:1}"
		if [[ "${NEW_SECTION}" != "${PREV_SECTION}" ]]; then
			echo "${NEW_SECTION}"                                                                 >> ../doc/sphinx/tutorial-more-examples.rst
			echo '^^^^^^^^^^^^^^^^^^^^^'                                                          >> ../doc/sphinx/tutorial-more-examples.rst
			echo ""                                                                               >> ../doc/sphinx/tutorial-more-examples.rst
			PREV_SECTION=${NEW_SECTION}
		fi
# Note: following line creates "Permalink to this definition" but also breaks the `video link`__, which should be also in .pdf file.
#		echo ".. rst:role:: Example ${BASENAME}\n"                                            >> ../doc/sphinx/tutorial-more-examples.rst
		echo ".. _ref${Sanitize}:\n"                                                                  >> ../doc/sphinx/tutorial-more-examples.rst
		echo "* ref${Sanitize}_, "':ysrc:`source file<examples/'${file_location}'>`, `video`__'".\n"  >> ../doc/sphinx/tutorial-more-examples.rst
		echo "__ https://youtu.be/${YOUTUBEURL}\n"                                                    >> ../doc/sphinx/tutorial-more-examples.rst
		echo ".. youtube:: ${YOUTUBEURL}\n\n"                                                         >> ../doc/sphinx/tutorial-more-examples.rst
	else
		echo "Cannot find file ${example} in LOCATION: ${file_location}"
		sleep 1
	fi
done


