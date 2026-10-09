FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential ca-certificates curl git python3 xz-utils unzip && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /turret
COPY .bazelversion /turret/.bazelversion
RUN BAZEL_VERSION="$(cat .bazelversion)" && \
    curl -fL --retry 3 "https://releases.bazel.build/${BAZEL_VERSION}/release/bazel-${BAZEL_VERSION}-linux-x86_64" -o /usr/local/bin/bazel && \
    echo "b4bae524f58e00a69f7c6fa10e62a91f85bfee586105dd480dccb4300c7cbca5  /usr/local/bin/bazel" | sha256sum -c - && \
    chmod +x /usr/local/bin/bazel

RUN useradd --create-home builder && \
    mkdir /firmware && chown builder:builder /turret /firmware
COPY --chown=builder:builder . /turret
USER builder
RUN bazel --batch build //... --lockfile_mode=error --jobs=2 && \
    cp bazel-bin/light_turret.elf bazel-bin/light_turret.bin bazel-bin/light_turret.uf2 /firmware/

CMD ["bash"]
