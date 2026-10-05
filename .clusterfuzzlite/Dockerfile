# ============================================================
# .clusterfuzzlite/Dockerfile
# Base image for building the fuzz harnesses under ClusterFuzzLite.
# ============================================================

FROM gcr.io/oss-fuzz-base/base-builder@sha256:4d95d77396ce573bbfca794166cd2f1e28df5b3445a45132e52ef0e4ff7f566b

COPY . $SRC/PoolAllocator
COPY .clusterfuzzlite/build.sh $SRC/

WORKDIR $SRC/PoolAllocator
