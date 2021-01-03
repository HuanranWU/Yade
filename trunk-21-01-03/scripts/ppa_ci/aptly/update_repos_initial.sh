#!/bin/bash

# First upload of the packages

PATHDEB="/home/anton/deb/"

set -e

NEW_ID=$(curl -L "https://gitlab.com/api/v4/projects/10133144/repository/commits/master" | jq --raw-output '.short_id')
echo $NEW_ID > ${PATHDEB}/OLD_ID

for i in bionic buster stretch xenial bullseye focal
do
    cd ${PATHDEB}
    wget https://gitlab.com/api/v4/projects/10133144/jobs/artifacts/master/download?job=deb_$i -O yade.zip
    unzip yade.zip
    aptly repo remove yadedaily-$i 'yadedaily'
    aptly repo remove yadedaily-$i 'libyadedaily'
    aptly repo remove yadedaily-$i 'yadedaily-doc'
    aptly repo remove yadedaily-$i 'python3-yadedaily'
    aptly repo add yadedaily-$i deb/*.deb
    aptly repo add yadedaily-$i deb/*.dsc
    rm -rf ${PATHDEB}/*
    aptly publish repo yadedaily-$i filesystem:yadedaily:
done
