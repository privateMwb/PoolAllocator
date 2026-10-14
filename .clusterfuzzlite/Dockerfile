# ============================================================
# .clusterfuzzlite/Dockerfile
# Base image for building the fuzz harnesses under ClusterFuzzLite.
# ============================================================

FROM gcr.io/oss-fuzz-base/base-builder@sha256:b867812795e2dd4516002f889a9c9f5ac4c549589b9f6b38f3b7a4c6627efaa9

COPY . $SRC/PoolAllocator
COPY .clusterfuzzlite/build.sh $SRC/

WORKDIR $SRC/PoolAllocator
