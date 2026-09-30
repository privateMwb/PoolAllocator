# ============================================================
# .clusterfuzzlite/Dockerfile
# Base image for building the fuzz harnesses under ClusterFuzzLite.
# ============================================================

FROM gcr.io/oss-fuzz-base/base-builder@sha256:23f294580c94a53c1625f5b98c7c22d7155fb030b877d0c2f937fbc8156b5aca

COPY . $SRC/PoolAllocator
COPY .clusterfuzzlite/build.sh $SRC/

WORKDIR $SRC/PoolAllocator
