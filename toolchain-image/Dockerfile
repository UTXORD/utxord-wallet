FROM emscripten/emsdk:3.1.42

LABEL authors="Dmitry Prokhorov <dip@arrowlabs.com>"

ARG VOLTA_HOME="/opt/volta"
ARG NODEJS_VERSION="18.16.1"

RUN apt-get update && apt-get install -y \
    autotools-dev \
    autoconf \
    libtool \
    pkg-config \
    \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

ENV VOLTA_HOME=${VOLTA_HOME}
ENV PATH="${VOLTA_HOME}/bin:${PATH}"

RUN curl https://get.volta.sh | bash \
    && volta install node@${NODEJS_VERSION} \
    && volta install yarn

RUN echo 'export VOLTA_HOME=/opt/volta' >> ${HOME}/.bashrc \
    && echo 'export PATH=${PATH}:/emsdk/upstream/emscripten/tools' >> ${HOME}/.bashrc
