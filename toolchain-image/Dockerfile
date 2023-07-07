FROM emscripten/emsdk:3.1.42

LABEL authors="Dmitry Prokhorov <dip@arrowlabs.com>"

ARG VOLTA_HOME="/opt/volta"
ARG NODEJS_VERSION="18.16.1"

USER root

RUN apt-get update && apt-get install -y \
    autotools-dev \
    autoconf \
    libtool \
    pkg-config \
    \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

ENV VOLTA_HOME=${VOLTA_HOME}
ENV PATH=${VOLTA_HOME}/bin:${PATH}:/emsdk/upstream/emscripten/tools

RUN curl https://get.volta.sh | bash \
    && volta install node@${NODEJS_VERSION} \
    && volta install yarn

# I have to add it because /emsdk/emsdk_env.sh completely re-define PATH value at some moment in interactive mode
RUN echo 'export PATH=${PATH}:/emsdk/upstream/emscripten/tools' >> ${HOME}/.bashrc
