#!/bin/bash
# Currently only support running this under the same dir as this script

# Update these vars as per tracking period
ISSUE_NO="16"
# this Thursday ~ next next Friday
ISSUE_STRING="2023-04-27 ~ 2023-05-12"
ARGS=($ISSUE_STRING)
#echo ${ARGS[0]} ${ARGS[1]} ${ARGS[2]}

# Update files in working-group
NEWFILE="./${ARGS[2]}.md"
touch $NEWFILE
echo -n "第 ${ISSUE_NO} 期（" >> $NEWFILE
echo -n $ISSUE_STRING >> $NEWFILE
echo -e "）RISCV 相关 merge PR 汇总\n" >> $NEWFILE
echo -e "可以通过这个链接过滤 ${ARGS[2]} 之前的 patch: <https://android-review.googlesource.com/q/mergedbefore:+${ARGS[2]}+AND+riscv64>。\n"  >> $NEWFILE

cat ./template.md >> $NEWFILE

echo "Done!"
