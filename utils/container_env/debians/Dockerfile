ARG RELEASE=bullseye
ARG DISTRO=debian

FROM ${DISTRO}:${RELEASE}
ARG RELEASE
ENV RELEASE=${RELEASE}
ENV CMAKE_GENERATOR=Ninja
ENV DOCKER_BUILD_THREAD_NUM=6
ENV DEBIAN_FRONTEND=noninteractive
ENV GCC_NAME="16.1.0"
ENV CLANG_VERSION=22
ENV TZ=Etc/UTC
ENV PATH="/root/.local/bin:${PATH}"
ENV ROOT_VERSION="v6-40-02"


RUN apt-get update &&\
        apt-get install -y cmake wget ninja-build git binutils dpkg-dev g++ gcc libssl-dev libx11-dev libxext-dev libxft-dev libxpm-dev python3 python3-pip python3-dev python3-venv libtbb-dev libjpeg-dev liblz4-dev liblzma-dev libgif-dev libgmp-dev libmpfr-dev libmpc-dev libisl-dev libzstd-dev curl file wget &&\
        python3 -m pip install --user --upgrade cmake conan &&\
        mkdir -p /download && cd /download &&\
        mkdir -p root-build&&\
        git clone -b ${ROOT_VERSION} --depth=1 https://github.com/root-project/root.git root-src &&\
        cmake -B root-build -S root-src -GNinja -Dminimal=ON -Ddataframe=ON -DPython3_EXECUTABLE="$(command -v python3)" &&\
        cmake --build root-build --target install -- -j${DOCKER_BUILD_THREAD_NUM} &&\
        # install gcc\
        mkdir gcc-source gcc-build&&\
        wget https://ftp.gnu.org/gnu/gcc/gcc-${GCC_NAME}/gcc-${GCC_NAME}.tar.gz && tar -xf gcc-${GCC_NAME}.tar.gz -C gcc-source --strip-components=1 &&\
        cd gcc-build &&\
        ../gcc-source/configure -v --prefix=/usr/local/ --enable-checking=release --enable-languages=c,c++ --disable-multilib &&\
        make -j${DOCKER_BUILD_THREAD_NUM} && make install && cd .. &&\
        # add keys for llvm, nodejs, gh aptly\
        mkdir -p -m 755 /etc/apt/keyrings &&\
        echo "deb http://apt.llvm.org/${RELEASE}/ llvm-toolchain-${RELEASE}-${CLANG_VERSION} main" > /etc/apt/sources.list.d/llvm.list &&\
        wget -qO /etc/apt/trusted.gpg.d/llvm.asc https://apt.llvm.org/llvm-snapshot.gpg.key &&\
        curl -fsSL https://deb.nodesource.com/setup_24.x -o nodesource_setup.sh &&\
        bash nodesource_setup.sh &&\
        wget -qO- https://cli.github.com/packages/githubcli-archive-keyring.gpg |  tee /etc/apt/keyrings/githubcli-archive-keyring.gpg > /dev/null &&\
        chmod go+r /etc/apt/keyrings/githubcli-archive-keyring.gpg &&\
        echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" |  tee /etc/apt/sources.list.d/github-cli.list > /dev/null &&\
        apt-get -qq update &&\
        # install packages\
        apt-get install -qqy -t llvm-toolchain-${RELEASE}-${CLANG_VERSION} clang-${CLANG_VERSION} nodejs gh &&\
        # clean up\
        apt clean && apt autoremove &&\
        cd / && rm -rf /download

ENV CC="/usr/lib/llvm-${CLANG_VERSION}/bin/clang"
ENV CFLAGS="--gcc-toolchain=/usr/local"
ENV CXX="/usr/lib/llvm-${CLANG_VERSION}/bin/clang++"
ENV CXXFLAGS="--gcc-toolchain=/usr/local"
