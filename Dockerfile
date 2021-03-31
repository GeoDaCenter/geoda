FROM ubuntu:18.04

RUN apt-get update \
    && apt-get install -y git build-essential

WORKDIR /tmp/geoda

COPY . .

RUN cd BuildTools/ubuntu
RUN chmod +x install.sh
CMD ./install.sh bionic 1.18.2