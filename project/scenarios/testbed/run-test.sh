#!/bin/zsh

python3 $TESTBED_CLIENT schedule experiment.json 2>&1 | tee -a test-backlog.txt
while true; do 
    sleep 540
    tput bel
    break
done;
