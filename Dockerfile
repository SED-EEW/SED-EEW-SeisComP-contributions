FROM debian:buster-slim

MAINTAINER Fred Massin  <fmassin@sed.ethz.ch>

ENV WORK_DIR /usr/local/src/
ENV INSTALL_DIR /opt/seiscomp3

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
    # Doc build
    python-sphinx \
    # FinDer 
    gmt libgmt-dev libopencv-dev \
    # misc
    git vim wget \
    # playback
    libfaketime 



# Install FinDer
## add credentials on build
ARG SSH_PRIVATE_KEY
RUN mkdir /root/.ssh/
# RUN echo "${SSH_PRIVATE_KEY}"|sed 's/NEWLINE/\n/g'
RUN echo "${SSH_PRIVATE_KEY}"|sed 's/NEWLINE/\n/g' > /root/.ssh/id_rsa

## make sure your domain is accepted
RUN touch /root/.ssh/known_hosts
RUN ssh-keyscan github.com >> /root/.ssh/known_hosts

RUN chmod 600  /root/.ssh/id_rsa
RUN chmod 644 /root/.ssh/known_hosts

## compile
RUN git clone git@github.com:FMassin/FinDer.git $WORK_DIR/FinDer \
    && cd $WORK_DIR/FinDer/libsrc \
    && git checkout master \
    && make clean \
    && make \
    && make no_timestamp \
    && make test \
    && make install

RUN cd $WORK_DIR/FinDer/finder_file \
    && make clean \
    && make \
    && make test

## clean FinDer source code
RUN mkdir -p $INSTALL_DIR/share/FinDer \
    && rm $WORK_DIR/FinDer/libsrc/*.cc \
          $WORK_DIR/FinDer/libsrc/*.h \
          $WORK_DIR/FinDer/finder_file/*.cc \
          $WORK_DIR/FinDer/finder_file/*.h \
    && mv $WORK_DIR/FinDer/config \
          $WORK_DIR/FinDer/finder_file \
          $WORK_DIR/FinDer/libsrc \
          $INSTALL_DIR/share/FinDer/ \
    && rm -r $WORK_DIR/FinDer
    
# Install seiscomp
RUN git clone https://github.com/SeisComP3/seiscomp3.git $WORK_DIR/seiscomp3 \
    && mkdir -p $WORK_DIR/seiscomp3/build \
    && cd $WORK_DIR/seiscomp3 \
    && git checkout release/jakarta \
    && git pull \
    && cd $WORK_DIR/seiscomp3/build \
    && cmake .. -DSC_GLOBAL_GUI=ON \
       -DSC_TRUNK_DB_MYSQL=ON \
       -DSC_TRUNK_DB_POSTGRESQL=ON \
       -DSC_TRUNK_DB_SQLITE3=ON \
       -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
    && make -j $(grep -c processor /proc/cpuinfo) \
    && make install

# Install addons
ADD ./ $WORK_DIR/seiscomp3/src/sed-addons/
RUN cd $WORK_DIR/seiscomp3/build \
    && cmake .. \
       -DSC_DOC_GENERATE=ON \
       -DSC_GLOBAL_GUI=ON \
       -DSC_IPGPADDONS_GUI_APPS=ON \
       -DSC_TRUNK_DB_MYSQL=ON \
       -DSC_TRUNK_DB_POSTGRESQL=ON \
       -DSC_TRUNK_DB_SQLITE3=ON \
       -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
       -DFinDer_INCLUDE_DIR=/usr/local/include/finder \
       -DFinDer_LIBRARY=/usr/local/lib/libFinder.a \
    && make -j $(grep -c processor /proc/cpuinfo) \
    && make install

# clean seiscomp source code
RUN rm -r $WORK_DIR/seiscomp3

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

RUN mkdir -p /home/sysop/.seiscomp3 \
    && chown -R sysop:sysop /home/sysop

USER sysop

#### SeisComp3 settings ###
## Configure
RUN /opt/seiscomp3/bin/seiscomp print env >> /home/sysop/.profile
RUN echo 'export LD_LIBRARY_PATH="/usr/local/lib/:$LD_LIBRARY_PATH"'>> /home/sysop/.profile
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
