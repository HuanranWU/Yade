# Dockerfile
FROM debian:buster

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get clean && apt-get update

RUN apt-get install -y \
    aptly \
    gnupg1 \
    unzip \
    wget

COPY yadedev_pub.gpg /root/

COPY yadedev_sec.gpg /root/

RUN gpg1 --import /root/yadedev_pub.gpg && gpg1 --import /root/yadedev_sec.gpg

RUN rm /root/yadedev_*

COPY .aptly.conf /root/

COPY create_repos.sh /root/

COPY update_repos_initial.sh /root/

COPY update_repos_next.sh /root/

RUN mkdir -p /root/deb

RUN apt-get autoclean && \
    apt-get autoremove && \
    rm -rf /var/lib/apt/lists/* && \
    rm -rf /var/cache/apt/archives/*deb