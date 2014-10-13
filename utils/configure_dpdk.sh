#! /bin/bash

# Author: Ivano Cerrato
# Date: 17/04/2014
#
# Brief: this script configures the DPDK on the machine. In other words, it loads
#    the proper kernel modules, associates some interfaces to the DPDK, and finally
#    creates a number of huge pages. The inetrafeces to be assocaited with the DPDP,
#    as well as the number of huge pages to be created can be easily changed by
#    modifying this script.
#    Note that the script requires that DPDK have been properly compiled.
#    Instead of this script, you can use the interactive one provided with the DPDK.

#Set properly the next row, to point to the DPDK folder
cd ~/Scrivania/dpdk-1.6.0r2

sudo modprobe uio
sudo insmod build/kmod/igb_uio.ko
sudo insmod build/kmod/rte_kni.ko

sudo ifconfig eth0 down
sudo tools/igb_uio_bind.py --bind=igb_uio eth0

sudo ifconfig eth1 down
sudo tools/igb_uio_bind.py --bind=igb_uio eth1

#Add here other interfaces to be managed with DPDK

#Set properly the next row, to point to the DPDK folder
export RTE_SDK=~/Scrivania/dpdk-1.6.0r2/build

sudo -u root  mkdir -p /mnt/huge
sudo -u root  mount -t hugetlbfs nodev /mnt/huge
#Change the next row to allocate a different number of huge pages
sudo -u root  echo 500 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages

