#!/bin/bash
# A wrapper for build_html.sh

# update this as per tracking period
ISSUE_STRING="\
2022-09-30 2022-10-14;\
2022-10-13 2022-10-28;\
"

IFS=';' read -ra ISSUE_ARRAY <<< "$ISSUE_STRING"

for ISSUE in "${ISSUE_ARRAY[@]}"; do
#	echo $ISSUE
	ARGS=($ISSUE)
	sh ./build_html.sh ${ARGS[0]} ${ARGS[1]}
done


