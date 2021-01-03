#!/bin/bash

# usefull for nix-based system
source /applis/site/nix.sh
nix-env --switch-profile $NIX_USER_PROFILE_DIR/yade

DATE='2018_10_28'
DIR='/home/username/results/my_beautifull_simulation'

# Create temp dirs
mkdir -p $DIR/$DATE/__YADE_JOBNO__
cd $DIR/$DATE/__YADE_JOBNO__

# Run yade
__YADE_COMMAND__

# Recover logs
cp -v __YADE_LOGFILE__ output.log
cp -v __YADE_ERRFILE__ output.err

# Recover job running env.
echo $OAR_JOB_ID >> runenv.data
hostname >> runenv.data
echo "__YADE_JOBID__" >> runenv.data

cd $DIR/$DATE/

# Archive to single file
tar -cf __YADE_JOBNO__ __YADE_JOBNO__.tar.gz
