FROM ubuntu:22.04
ARG USERNAME=dev
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME
    

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
RUN apt-get install -y vim
RUN pip install -U kaleido && pip install pandas && pip install plotly
