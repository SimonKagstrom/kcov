#!/usr/bin/env bash

apt-get update
apt-get install schroot debootstrap
install -d /chroots/i386

printf "[mychroot]
type=directory
description=An i386 chroot for building kcov.
directory=/chroots/i386
users=$(whoami)
root-groups=root
preserve-environment=true
personality=linux32" >> /etc/schroot/schroot.conf

debootstrap --variant=buildd --arch i386 focal /chroots/i386 http://archive.ubuntu.com/ubuntu/
