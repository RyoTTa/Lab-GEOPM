#!/bin/bash

compute_prefix=c
# Start munge and slurm controller on master host

systemctl stop munge
systemctl stop slurmctld
systemctl enable munge
systemctl enable slurmctld
systemctl start munge
systemctl start slurmctld

sleep 6

pdsh -w $compute_prefix[1-3] systemctl start slurmd



