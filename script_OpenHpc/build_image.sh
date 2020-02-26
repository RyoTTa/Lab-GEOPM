#!/bin/bash

sms_ip="150.183.250.136"
kargs=""
# Define chroot location
export CHROOT=/opt/ohpc/admin/images/centos7.6
# Build initial chroot image
wwmkchroot centos-7 $CHROOT


yum -y --installroot=$CHROOT install ohpc-base-compute
cp -p /etc/resolv.conf $CHROOT/etc/resolv.conf
yum -y --installroot=$CHROOT install ohpc-slurm-client
yum -y --installroot=$CHROOT install ntp

yum -y --installroot=$CHROOT install kernel
yum -y --installroot=$CHROOT install lmod-ohpc

wwinit database
wwinit ssh_keys

echo "${sms_ip}:/home /home nfs nfsvers=3,nodev,nosuid,noatime 0 0" >> $CHROOT/etc/fstab
echo "${sms_ip}:/opt/ohpc/pub /opt/ohpc/pub nfs nfsvers=3,nodev,noatime 0 0" >> $CHROOT/etc/fstab
#echo "${sms_ip}:/disk1 /disk1 nfs nfsvers=3,nodev,nosuid,noatime 0 0" >> $CHROOT/etc/fstab
echo "/home *(rw,no_subtree_check,fsid=10,no_root_squash)" >> /etc/exports
echo "/opt/ohpc/pub *(ro,no_subtree_check,fsid=11)" >> /etc/exports
#echo "/disk1 *(rw,no_subtree_check,fsid=12,no_root_squash)" >> /etc/exports
exportfs -a

systemctl restart nfs-server
systemctl enable nfs-server

chroot $CHROOT systemctl enable ntpd
echo "server ${sms_ip}" >> $CHROOT/etc/ntp.conf


wwsh file import /etc/passwd
wwsh file import /etc/group
wwsh file import /etc/shadow

wwsh file import /etc/slurm/slurm.conf
#wwsh file import /etc/munge/munge.key
cp /etc/munge/munge.key $CHROOT/etc/munge/munge.key
#mumge.key 직접 작업

sudo  yum -y --installroot=$CHROOT install vim
sudo  yum -y --installroot=$CHROOT install htop
sudo  yum -y --installroot=$CHROOT install libgfortran

export kargs="${kargs} intel_pstate=disable"

yum -y --installroot=$CHROOT install kmod-msr-safe-ohpc
yum -y --installroot=$CHROOT install msr-safe-slurm-ohpc

wwbootstrap `uname -r`
wwvnfs --chroot $CHROOT



