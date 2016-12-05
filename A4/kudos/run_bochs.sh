#!/usr/bin/env bash

if [ "$#" -ne 1 ]; then
   echo "./run_qemu [program] \n"
   echo "  program: the program that KUDOS should run"
   exit 1;
fi

set -euo pipefail

iso_path=./qemu/kudos.iso
kudos_disk_path=./store.file
kudos_path=./kudos/kudos-x86_64
grub_set_path=./qemu/grub/iso/boot/grub/grub.cfg

if [ -f "$iso_path" ]; then
  rm "$iso_path"
fi

cp -f "$kudos_path" "./qemu/grub/iso/boot/kudos-x86_64"

grub-mkrescue -o "$iso_path" "./qemu/grub/iso"

rm qemu/grub/iso/boot/grub/grub.cfg

touch qemu/grub/iso/boot/grub/grub.cfg 
if [ -f "$grub_set_path" ]; then
    rm "$grub_set_path"
fi

echo "set timeout=0
set default=0 # Set the default menu entry
 
menuentry \"kudos\" {
   multiboot /boot/kudos-x86_64 initprog=[disk]$1 # The multiboot command replaces the kernel command
   boot
}" >> ./qemu/grub/iso/boot/grub/grub.cfg 

grub-mkrescue -o "$iso_path" "./qemu/grub/iso"

bochs
