#!/bin/bash

export CHROOT=/opt/ohpc/admin/images/centos7.6

systemctl disable firewalld
systemctl stop firewalld
systemctl enable ntpd.service
systemctl restart ntpd
systemctl restart xinetd
systemctl enable mariadb.service
systemctl restart mariadb
systemctl enable httpd.service
systemctl restart httpd
systemctl enable dhcpd.service

#restart and enable ganglia services
#systemctl enable gmond
#systemctl enable gmetad
#systemctl start gmond
#systemctl start gmetad
#chroot $CHROOT systemctl enable gmond
systemctl try-restart httpd
systemctl restart dhcpd
wwsh pxe update



