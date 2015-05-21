This folder contains examples of network functions implemented in Docker 
containers.

===============================================================================

Docker environment for network functions

 On Ubuntu:  
      follow the instruction provided here:  
           http://docs.docker.com/installation/  
      apt-get install lxc -y  
      echo 'DOCKER_OPTS="-e lxc"' >> /etc/default/docker  
      service docker restart

 On Debian:  
      follow the instruction provided here:  
          https://scottlinux.com/2014/05/04/how-to-install-and-run-docker-on-debian-wheezy/  
          apt-get install docker -y  
          echo 'DOCKER_OPTS="-e lxc"' >> /etc/default/docker  
          service docker.io restart  
        
===============================================================================

Please check individual README's in each sub-package.
