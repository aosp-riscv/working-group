#!/bin/bash
# Currently only support running this under the same dir as this script

# Update these vars as per tracking period
ISSUE_NO="11"
ISSUE_STRING="2023-02-16 ~ 2023-03-03"

# Update files in unicornx.github.io
GITHUB_IO=../../../unicornx.github.io
ARGS=($ISSUE_STRING)
#echo ${ARGS[0]} ${ARGS[1]} ${ARGS[2]}

OUTPUT_FILENAME=aosp-riscv-${ARGS[2]}.html
OUTPUT_PATH=$GITHUB_IO/android-review/$OUTPUT_FILENAME
#echo $OUTPUT_PATH
rm -f $OUTPUT_PATH
touch $OUTPUT_PATH
sh ./build_html.sh ${ARGS[0]} ${ARGS[2]} $OUTPUT_PATH

echo -e -n "\n- [Issue $ISSUE_NO ($ISSUE_STRING)](./android-review/$OUTPUT_FILENAME)" >> $GITHUB_IO/README.md

# Update files in working-group
NEWFILE="./${ARGS[2]}.md"
touch $NEWFILE
echo -n "本周期（" >> $NEWFILE
echo -n $ISSUE_STRING >> $NEWFILE
echo -e "）RISCV 相关 merge PR 汇总参考 [这里][1]。\n" >> $NEWFILE
cat ./template.md >> $NEWFILE
