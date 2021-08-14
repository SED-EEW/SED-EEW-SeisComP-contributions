FROM debian:buster-slim

MAINTAINER Fred Massin  <fmassin@sed.ethz.ch>

ENV    WORK_DIR /usr/local/src
ENV INSTALL_DIR /opt/seiscomp
ENV   REPO_PATH https://github.com/SeisComP
ENV         TAG 4.5.0
ENV           D "-DSC_GLOBAL_GUI=ON \
                    -DSC_IPGPADDONS_GUI_APPS=ON \
                    -DSC_TRUNK_DB_MYSQL=ON \
                    -DSC_TRUNK_DB_POSTGRESQL=ON \
                    -DSC_TRUNK_DB_SQLITE3=ON \
                    -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR"

# Fix Debian  env
ENV DEBIAN_FRONTEND noninteractive
ENV INITRD No
ENV FAKE_CHROOT 1

# Setup sysop's user and group id
ENV USER_ID 1000
ENV GROUP_ID 1000

WORKDIR $WORK_DIR

RUN echo 'force-unsafe-io' | tee /etc/dpkg/dpkg.cfg.d/02apt-speedup \
    && echo 'DPkg::Post-Invoke {"/bin/rm -f /var/cache/apt/archives/*.deb || true";};' | tee /etc/apt/apt.conf.d/no-cache \
    && apt-get update \
    && apt-get dist-upgrade -y --no-install-recommends \
    && apt-get install -y \
    build-essential \
    festival \
    cmake \
    cmake-curses-gui \
    flex \
    g++ \
    libgfortran4 \
    libncurses5 \
    libncurses5-dev \
    gfortran \
    libboost-dev \
    libboost-filesystem-dev \
    libboost-iostreams-dev \
    libboost-program-options-dev \
    libboost-regex-dev \
    libboost-thread-dev \
    libboost-system-dev \
    libboost-signals-dev \
    libboost-all-dev \
    python \
    python2.7 \
    python-dbus-dev \
    python-libxml2 \
    python-numpy \
    python-m2crypto \
    libghc-opengl-dev \
    libqt4-opengl-dev \
    libqtwebkit-dev \
    default-libmysqlclient-dev \
    default-mysql-server \
    cmake-qt-gui \
    libcdio-dev \
    libpython2.7 \
    libpython2.7-dev \
    libqt4-dev \
    libsqlite3-dev \
    sqlite3 \
    libpq5 \
    libpq-dev \
    python-twisted \
    libxml2 \
    libxml2-dev \
    openssh-server \
    openssl \
    libssl-dev \
    net-tools \
    # misc
    git \
    vim \
    wget


# Install seiscomp
RUN echo "Cloning base repository into $WORK_DIR/seiscomp" \
    && git clone --branch $TAG $REPO_PATH/seiscomp.git $WORK_DIR/seiscomp \
    && echo "Cloning base components" \
    && cd $WORK_DIR/seiscomp/src/base \
    && git clone --branch $TAG $REPO_PATH/seedlink.git \
    && git clone --branch $TAG $REPO_PATH/common.git \
    && git clone --branch $TAG $REPO_PATH/main.git \
    && git clone --branch $TAG $REPO_PATH/extras.git \
    && echo "Cloning external base components" \
    && git clone --branch $TAG $REPO_PATH/contrib-gns.git \
    && git clone --branch $TAG $REPO_PATH/contrib-ipgp.git \
    && git clone --branch $TAG $REPO_PATH/contrib-sed.git \
    && echo "Done" 

RUN mkdir -p $WORK_DIR/seiscomp/build \
    && cd $WORK_DIR/seiscomp/build \
    && cmake .. $D \
    && make \ 
    && make install

# Install addons
ADD ./ $WORK_DIR/seiscomp/src/base/sed-addons/
RUN rm -r $WORK_DIR/seiscomp/src/base/contrib-sed/apps/scvsmag \
    && sed -i 's/scvsmag//' $WORK_DIR/seiscomp/src/base/contrib-sed/apps/CMakeLists.txt \
    && cd $WORK_DIR/seiscomp/build \
    && cmake .. $D \
    && make -j $(grep -c processor /proc/cpuinfo) \
    && make install

# Setup ssh access
RUN mkdir /var/run/sshd
RUN echo 'root:password' | chpasswd
RUN echo X11Forwarding yes >> /etc/ssh/sshd_config
RUN echo X11UseLocalhost no  >> /etc/ssh/sshd_config
RUN echo AllowAgentForwarding yes >> /etc/ssh/sshd_config
RUN echo PermitRootLogin yes >> /etc/ssh/sshd_config
#
## SSH login fix. Otherwise user is kicked off after login
RUN sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' \
    -i /etc/pam.d/sshd

RUN groupadd --gid $GROUP_ID -r sysop && useradd -m -s /bin/bash --uid $USER_ID -r -g sysop sysop \
    && echo 'sysop:sysop' | chpasswd \
    && chown -R sysop:sysop $INSTALL_DIR

RUN mkdir -p /home/sysop/.seiscomp \
    && chown -R sysop:sysop /home/sysop

USER sysop

#### SeisComp3 settings ###
## Configure
RUN  $INSTALL_DIR/bin/seiscomp print env >> /home/sysop/.profile
#RUN echo 'export SEISCOMP_ROOT="$INSTALL_DIR/"' >> /home/sysop/.profile
#RUN echo 'export LD_LIBRARY_PATH="$SEISCOMP_ROOT/lib:/usr/local/lib/x86_64-linux-gnu/:$LD_LIBRARY_PATH"'>> /home/sysop/.profile
#RUN echo 'export PYTHONPATH="$PYTHONPATH:$SEISCOMP_ROOT/lib/python"' >> /home/sysop/.profile
#RUN echo 'export MANPATH="$SEISCOMP_ROOT/man:$MANPATH"' >> /home/sysop/.profile
#RUN echo 'export PATH="$SEISCOMP_ROOT/bin:$PATH"' >> /home/sysop/.profile
#RUN echo 'source $SEISCOMP_ROOT/share/shell-completion/seiscomp.bash' >> /home/sysop/.profile
RUN echo 'date' >> /home/sysop/.profile
RUN echo 'echo \$SEISCOMP_ROOT is $SEISCOMP_ROOT' >> /home/sysop/.profile
RUN echo 'seiscomp status |grep "is running"' >> /home/sysop/.profile
RUN echo 'seiscomp status |grep "WARNING"' >> /home/sysop/.profile


## machinery for next
#ENV SEISCOMP_ROOT=/opt/seiscomp3 PATH=/opt/seiscomp3/bin:$PATH \
#     LD_LIBRARY_PATH=/opt/seiscomp3/lib:$LD_LIBRARY_PATH \
#     PYTHONPATH=/opt/seiscomp3/lib/python:$PYTHONPATH \
#     MANPATH=/opt/seiscomp3/share/man:$MANPATH \
#     LC_ALL=C
#
## Copy default config
#ADD volumes/SYSTEMCONFIGDIR/ $INSTALL_DIR/etc/

WORKDIR /home/sysop

USER root
    
EXPOSE 22

## Start sshd
CMD ["/usr/sbin/sshd", "-D"]
