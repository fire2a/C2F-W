FROM debian:bookworm-slim as base

ENV LANG=en_EN.UTF-8

RUN apt-get update \
    && apt-get install --no-install-recommends --no-install-suggests --allow-unauthenticated -y \
        gnupg \
        ca-certificates \
        wget \
        locales \
    && localedef -i en_US -f UTF-8 en_US.UTF-8 \
    # Add the current key for package downloading
    # Please refer to QGIS install documentation (https://www.qgis.org/fr/site/forusers/alldownloads.html#debian-ubuntu)
    && mkdir -m755 -p /etc/apt/keyrings \
    && wget -O /etc/apt/keyrings/qgis-archive-keyring.gpg https://download.qgis.org/downloads/qgis-archive-keyring.gpg \
    # Add repository for latest version of qgis-server
    # Please refer to QGIS repositories documentation if you want other version (https://qgis.org/en/site/forusers/alldownloads.html#repositories)
    && echo "deb [signed-by=/etc/apt/keyrings/qgis-archive-keyring.gpg] https://qgis.org/debian bookworm main" | tee /etc/apt/sources.list.d/qgis.list \
    && apt-get update \
    && apt-get install --no-install-recommends --no-install-suggests --allow-unauthenticated -y \
        python3-pip qgis python3-qgis python3-qgis-common python3-venv python3-pytest python3-mock qttools5-dev-tools \
        libtiff-dev g++ libboost-all-dev libeigen3-dev \
        build-essential curl \
        nodejs npm \
        xauth \
        xvfb \
        unzip \
    && rm -rf /var/lib/apt/lists/*

RUN curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o "awscliv2.zip" && unzip awscliv2.zip && ./aws/install && rm -rf ./aws awscliv2.zip

RUN npm install --global yarn
COPY Cell2Fire /usr/local/Cell2Fire
WORKDIR /usr/local/Cell2Fire
RUN make clean -f makefile.debian
RUN make install -f makefile.debian

COPY aws /usr/local/Cell2FireWrapper
WORKDIR /usr/local/Cell2FireWrapper
RUN yarn && yarn build

RUN useradd -m qgis

ENV QGIS_PREFIX_PATH /usr
ENV QGIS_SERVER_LOG_STDERR 1
ENV QGIS_SERVER_LOG_LEVEL 2

USER qgis
WORKDIR /home/qgis

ENV QT_QPA_PLATFORM offscreen
RUN mkdir -p ~/.local/share/QGIS/QGIS3/profiles/default/python/plugins/
RUN wget https://github.com/fire2a/fire-analytics-qgis-processing-toolbox-plugin/releases/download/v0.1.24/fireanalyticstoolbox_v0.1.24.zip -O ~/.local/share/QGIS/QGIS3/profiles/default/python/plugins/fire2a.zip && cd ~/.local/share/QGIS/QGIS3/profiles/default/python/plugins && unzip fire2a.zip && rm -f fire2a.zip && mv fireanalyticstoolbox fire2a
RUN pip3 install --break-system-packages -r ~/.local/share/QGIS/QGIS3/profiles/default/python/plugins/fire2a/requirements.txt
RUN qgis_process plugins enable fire2a

FROM base as qgis
ENTRYPOINT ["node", "/usr/local/Cell2FireWrapper/build/wrapper"]
