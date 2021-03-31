FROM ubuntu:18.04

RUN apt-get update \
    && apt-get install -y git build-essential

WORKDIR /tmp/geoda

COPY . .

CMD cd BuildTools/ubuntu
CMD pwd
CMD ./install.sh bionic 1.18.2