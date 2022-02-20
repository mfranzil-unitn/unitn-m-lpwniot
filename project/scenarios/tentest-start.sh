#!/bin/zsh

# Scenario 1

for i in $(seq 1 10); do
  mkdir -p scenario1-$i
  cd scenario1-$i
  cooja_nogui ../../scenario_1/nogui-random.csc >/dev/null 2>&1 &
  cd ..
done

# Single 123456 test
mkdir -p scenario1-123456
cd scenario1-123456
cooja_nogui ../../scenario_1/nogui.csc >/dev/null 2>&1 &
cd ..

sleep 600

# Scenario 2

for i in $(seq 1 10); do
  mkdir -p scenario2-$i
  cd scenario2-$i
  cooja_nogui ../../scenario_2/nogui-random.csc >/dev/null 2>&1 &
  cd ..
done

# Single 123456 test
mkdir -p scenario2-123456
cd scenario2-123456
cooja_nogui ../../scenario_2/nogui.csc >/dev/null 2>&1 &
cd ..
