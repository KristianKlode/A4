#!/bin/bash

./kudos/util/tfstool create store.file 16384 disk
./kudos/util/tfstool write store.file userland/proc proc
./kudos/util/tfstool write store.file userland/tester tester
./kudos/util/tfstool write store.file userland/sem sem
./run\_qemu.sh proc
./run\_qemu.sh sem
