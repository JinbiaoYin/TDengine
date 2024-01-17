#!/bin/bash

function usage() {
    echo "$0"
    echo -e "\t -e enterprise edition"
    echo -e "\t -h help"
}

ent=1
while getopts "eh" opt; do
    case $opt in
        e)
            ent=1
            ;;
        h)
            usage
            exit 0
            ;;
        \?)
            echo "Invalid option: -$OPTARG"
            usage
            exit 0
            ;;
    esac
done

script_dir=`dirname $0`
cd ${script_dir}
PWD=`pwd`

if [ $ent -eq 0 ]; then
    cd ../../debug
else
    cd ../../../debug
fi

set -e

pgrep taosd || taosd >> /dev/null 2>&1 &

sleep 10

ctest -E "smlTest|funcTest|profileTest|sdbTest|showTest|geomTest|idxFstUtilUT|idxTest|idxUtilUT|idxFstUT|parserTest|plannerTest|transUT|transUtilUt" -j8

ret=$?
archOs=$(arch)

if [[ $archOs =~ "aarch64" ]]; then
    echo "not check mem leak on arm64"
    ret=0    
else
  echo "os check mem leak"
fi

exit $ret

