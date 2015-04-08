#! /bin/bash

# Author: Ivano Cerrato
# Date: 08/05/2015
#
# Brief: this script configures the DPDK on the machine. In other words, it loads
#    the proper kernel modules, associates some interfaces to the DPDK, and finally
#    creates a number of huge pages. The interfaces to be assocaited with the DPDK,
#    as well as the number of huge pages to be created can be easily changed by
#    modifying this script.
#    Note that the script requires that DPDK have been properly compiled, and 
#    refers to the DPDK version available at https://github.com/bisdn/dpdk-dev .
#    Instead of this script, you can use the interactive tool provided with the 
#    DPDK repository.

#Set properly the next row, to point to the DPDK folder
cd ~/Desktop/dpdk-dev

sudo modprobe uio
sudo insmod build/kmod/igb_uio.ko
sudo insmod build/kmod/rte_kni.ko

sudo ifconfig em2 down
sudo tools/dpdk_nic_bind.py --bind=igb_uio em2

sudo ifconfig em3 down
sudo tools/dpdk_nic_bind.py --bind=igb_uio em3

#Add here other interfaces to be managed with DPDK

#Set properly the next row, to point to the DPDK folder
export RTE_SDK=~/Desktop/UNutils/dpdk-dev/x86_64-native-linuxapp-gcc/

sudo -u root  mkdir -p /mnt/huge
sudo -u root  mount -t hugetlbfs nodev /mnt/huge
#Change the next row to allocate a different number of huge pages
sudo -u root  echo 1000 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages

