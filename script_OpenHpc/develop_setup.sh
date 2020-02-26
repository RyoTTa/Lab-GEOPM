#!/bin/bash

yum -y install ohpc-autotools
yum -y install EasyBuild-ohpc
yum -y install hwloc-ohpc
yum -y install spack-ohpc
yum -y install valgrind-ohpc
#Development Tools

yum -y install gnu8-compilers-ohpc
#Compilers

yum -y install openmpi3-gnu8-ohpc mpich-gnu8-ohpc
#MPI Stacks

yum -y install ohpc-gnu8-perf-tools
yum -y install ohpc-gnu8-geopm
#Performance Tools

yum -y install lmod-defaults-gnu8-openmpi3-ohpc
#Setup default development enviroment

yum search petsc-gnu7 ohpc
yum -y install ohpc-gnu8-serial-libs
yum -y install ohpc-gnu8-io-libs
yum -y install ohpc-gnu8-python-libs
yum -y install ohpc-gnu8-runtimes
yum -y install ohpc-gnu8-mpich-parallel-libs
yum -y install ohpc-gnu8-openmpi3-parallel-libs
#3rd party lib/tools

#yum -y install intel-compilers-devel-ohpc
#yum -y install intel-mpi-devel-ohpc
#yum -y install mvapich2-psm2-intel-ohpc
#yum -y install ohpc-intel-serial-libs
#yum -y install ohpc-intel-geopm
#yum -y install ohpc-intel-io-libs
#yum -y install ohpc-intel-perf-tools
#yum -y install ohpc-intel-python-libs
#yum -y install ohpc-intel-runtimes
#yum -y install ohpc-intel-mpich-parallel-libs
#yum -y install ohpc-intel-mvapich2-parallel-libs
#yum -y install ohpc-intel-openmpi3-parallel-libs
#yum -y install ohpc-intel-impi-parallel-libs
#Optional Development Tool

