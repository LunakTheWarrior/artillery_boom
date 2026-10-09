FROM registry.fedoraproject.org/fedora:latest

RUN dnf -y update && \
    dnf -y install cmake git && \
    dnf clean all

WORKDIR /turret

COPY . /turret

RUN cmake -B docker_build

RUN cmake --build docker_build/

ENTRYPOINT ["bash"]