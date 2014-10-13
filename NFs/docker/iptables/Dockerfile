FROM      ubuntu
MAINTAINER Ivano Cerrato <ivano.cerrato@polito.it>

RUN apt-get update
RUN apt-get install -y iptables bridge-utils

ADD start.sh start.sh
RUN chmod +x start.sh

CMD ./start.sh
