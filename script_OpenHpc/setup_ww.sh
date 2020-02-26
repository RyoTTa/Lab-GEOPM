#!/bin/bash

eth_provision="em1"
c_name=("c1"
        "c2"
        "c3"
        )
c_ip=("NODE_IP1"
      "NODE_IP2"
      "NODE_IP3"
     )
c_mac=("NODE_MAC1"
       "NODE_MAC1"
       "NODE_MAC1"
      )
num_computes=3
compute_regex=c*
kargs="intel_pstate=disable"

sms_eth_internal="em1"
sms_ip="MASTER_IP"
eth_provision="em1"
internal_netmask="255.255.255.0"
ifconfig ${sms_eth_internal} ${sms_ip} netmask ${internal_netmask} up

echo "GATEWAYDEV=${eth_provision}" > /tmp/network.$$
wwsh -y file import /tmp/network.$$ --name network
wwsh -y file set network --path /etc/sysconfig/network --mode=0644 --uid=0

for ((i=0; i<$num_computes; i++)) ; do
        wwsh -y node new ${c_name[i]} --ipaddr=${c_ip[i]} --hwaddr=${c_mac[i]} -D ${eth_provision}
        sleep 2
done

#wwsh -y provision set ${compute_regex} --vnfs=centos7.6 --bootstrap=`uname -r \ --files=dynamic_hosts,passwd,group,shadow,slurm.conf,munge.key,network
wwsh -y provision set c[1-3] --vnfs=centos7.6 --bootstrap=`uname -r` \ --files=dynamic_hosts,passwd,group,shadow,slurm.conf,munge.key,network
wwsh -y provision set c[1-3] --fileadd slurm.conf
wwsh -y provision set c[1-3] --kargs="${kargs}"


