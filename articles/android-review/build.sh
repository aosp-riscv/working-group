#!/bin/bash
# A wrapper for build_html.sh

# update this as per tracking period
ISSUE_STRING="\
2022-09-30 ~ 2022-10-14;\
2022-10-13 ~ 2022-10-28;\
2022-10-27 ~ 2022-11-11;\
2022-11-10 ~ 2022-11-25;\
2022-11-24 ~ 2022-12-09;\
2022-12-08 ~ 2022-12-23;\
2022-12-22 ~ 2023-01-06;\
2023-01-05 ~ 2023-01-20;\
2023-01-19 ~ 2023-02-03;\
"

IFS=';' read -ra ISSUE_ARRAY <<< "$ISSUE_STRING"

for ISSUE in "${ISSUE_ARRAY[@]}"; do
#	echo $ISSUE
	ARGS=($ISSUE)
#	echo ${ARGS[0]} ${ARGS[1]} ${ARGS[2]}
	sh ./build_html.sh ${ARGS[0]} ${ARGS[2]}
done

mv aosp-riscv-*.html ../../../unicornx.github.io/android-review/

cp ./template.md ./${ARGS[2]}.md