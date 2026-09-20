# ============================================================
# .clusterfuzzlite/Dockerfile
# Base image for building the fuzz harnesses under ClusterFuzzLite.
# ============================================================

FROM gcr.io/oss-fuzz-base/base-builder@sha256:df8728a13b864d0db206ba6daee06fcdb812cf82111c41d54d8db4d6e585437e

COPY . $SRC/PoolAllocator
COPY .clusterfuzzlite/build.sh $SRC/

WORKDIR $SRC/PoolAllocator
