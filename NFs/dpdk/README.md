This folder contains examples of network functions implemented as DPDK secondary
processes

===============================================================================

* If you are going to use this network function with the xDPd virtual switch, 
you can skip this part *

DPDK environment for network functions

git clone https://github.com/bisdn/dpdk-dev  
cd dpdk-dev/tools  
sudo ./setupd.sh  
Through this script, you should:  
  * build the environment x86_64-native-linuxapp-gcc
  * Insert KNI module
  * Setup hugepage mappings for non-NUMA systems
        
===============================================================================

Please check individual README's in each sub-package.
