#!/bin/sh

#SBATCH -J test		# Job 이름

#SBATCH -p normal	# 작업할 Partition 이름

#SBATCH -N 2		# 총 필요한 컴퓨팅 노드 수

#SBATCH -n 2		# 총 필요한 프로세스 수

#SBATCH -o %x.o%j		# stdout 파일 명({작업이름}.o{작업ID})

#SBATCH -e %x.e%j		# stderr 파일 명({작업이름}.e{작업ID})

#SBATCH --comment	# Application별 SBATCH 옵션 (옵션명은 아래 작업스크립트 작성 안내 참고)

#SBATCH --time 00:30:00	# 최대 작업 시간 (Wall Time Clock Limit, 이상 강제 종료)

#SBATCH --gres=gpu:2	# GPU를 사용하기 위한 옵션

srun ./run.x 		# srun 사용

--comment 
    자료 수집의 목적, 아래와 같이 SBATCH 옵션을 통한 사용 프로그램 정보 작성
 PYTHON

 python

 LAMMPS

 lammps 

 Charmm

 charmm

 NAMD

 namd

 Gaussian

 gaussian

 Quantum Espresso

qe

 OpenFoam

 openfoam

 SIESTA

 siesta

 WRF

 wrf

 Tensorflow

 tensorflow

 in-house code

 inhouse

 Caffe

 caffe

 R

 R

 Pytorch

 pytorch

 VASP

 vasp

 Sklearn

 sklearn

 Gromacs

 gromacs

 그 외 applications

etc















