#!/bin/bash
#PBS -l walltime=4:00:00
#PBS -l nodes=1:ppn=2
#PBS -l vmem=32G
#PBS -N WAF_4_1_1_375_5_no_no_no_no
cd /zhome/fc/e/102910/maritime-vrp/build
./maritime_vrp ../data/old_thesis_data/program_params.json ../data/new/WAF_4_1_1_375_5_no_no_no_no.json
