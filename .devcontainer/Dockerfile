FROM ubuntu:22.04

#RUN echo 'PermitRootLogin yes' >> /etc/ssh/sshd_config && \
#    echo 'PermitEmptyPasswords yes' >> /etc/ssh/sshd_config && \
#    echo 'PasswordAuthentication yes' >> /etc/ssh/sshd_config && \
#    ssh-keygen -A


RUN apt-get upgrade -y
RUN apt-get update -y --fix-missing
#RUN apt install -y software-properties-common
RUN apt-get install -y build-essential uuid-dev git curl python3 python3-pip          
RUN apt-get install -y cmake libgtest-dev clang libc++-dev libc++abi-dev lld lldb clang-format
RUN apt-get install -y libtool lcov libssl-dev libsrtp2-dev libmicrohttpd-dev libopus-dev zip
