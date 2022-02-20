#!/bin/zsh

mkdir -p S1_B
mkdir -p S2_B

for i in $(seq -w 01 10); do
  # Scenario 1
  cd scenario1-$i
  mv scenario1_nogui_mrm.log S1_B_f$i.log
  mv scenario1_nogui_mrm_dc.log S1_B_f$i-dc.log
  python3 ../../parse-stats.py S1_B_f$i.log >> S1_B_f$i-results.log
  cd ..
  mv scenario1-$i S1_B/S1_B_f$i
  # Scenario 2
  cd scenario2-$i
  mv scenario2_nogui_mrm.log S2_B_f$i.log
  mv scenario2_nogui_mrm_dc.log S2_B_f$i-dc.log
  python3 ../../parse-stats.py S2_B_f$i.log >> S2_B_f$i-results.log
  cd ..
  mv scenario2-$i S2_B/S2_B_f$i
done

cd scenario1-123456
mv scenario1_nogui_mrm.log S1_A.log
mv scenario1_nogui_mrm_dc.log S1_A-dc.log
python3 ../../parse-stats.py S1_A.log >> S1_A-results.log
cd ..
mv scenario1-123456 S1_A

cd scenario2-123456
mv scenario2_nogui_mrm.log S2_A.log
mv scenario2_nogui_mrm_dc.log S2_A-dc.log
python3 ../../parse-stats.py S2_A.log >> S2_A-results.log
cd ..
mv scenario2-123456 S2_A