FROM debian:bookworm-slim

LABEL org.opencontainers.image.authors="Fred Massin <fmassin@sed.ethz.ch>, Thomas Planes <thomas.planes@sed.ethz.ch>"

ENV    WORK_DIR /usr/local/src
ENV INSTALL_DIR /opt/seiscomp
ENV   REPO_PATH https://github.com/SeisComP
#ENV         TAG master
ENV         TAG 5.1.1
ENV           D "-DSC_GLOBAL_GUI=ON \
                    #-DSC_IPGPADDONS_GUI_APPS=ON \
                    -DSC_TRUNK_DB_MYSQL=ON \
                    -DSC_TRUNK_DB_POSTGRESQL=ON \
                    -DSC_TRUNK_DB_SQLITE3=ON \
                    #-DSC_DOC_GENERATE=ON \
                    #-DSC_DOC_GENERATE_HTML=ON \
                    #-DSC_DOC_GENERATE_MAN=ON \
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
    && apt-get install -y --no-install-recommends \
    build-essential \
    festival \
    cmake \
    cmake-curses-gui \
    flex \
    g++ \
    libgfortran5 \
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
    libboost-all-dev \
    python3 \
    python3-dbus \
    python3-libxml2 \
    python3-numpy \
    python3-m2crypto \
    python3-dateutil \
    libghc-opengl-dev \
    qtbase5-dev \
    qttools5-dev \
    qttools5-dev-tools \
    libqt5opengl5-dev \
    libqt5svg5-dev \
    libqt5webkit5 \
    default-libmysqlclient-dev \
    default-mysql-server \
    cmake-qt-gui \
    libcdio-dev \
    libpython3-dev \
    libsqlite3-dev \
    sqlite3 \
    libpq5 \
    libpq-dev \
    python3-twisted \
    libxml2 \
    libxml2-dev \
    openssh-server \
    openssl \
    libssl-dev \
    net-tools \
    # misc
    git \
    vim \
    wget \
    # doc
    python3-sphinx \
    python3-pip \
    && python3 -m pip install --break-system-packages --no-cache-dir m2r mistune==0.8.4 \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Install seiscomp
RUN echo "Cloning base repository into $WORK_DIR/seiscomp" \
    && git clone --branch $TAG $REPO_PATH/seiscomp.git $WORK_DIR/seiscomp \
    && echo "Cloning base components" \
    && cd $WORK_DIR/seiscomp/src/base \
    && git clone --branch $TAG $REPO_PATH/common.git \
    && git clone --branch $TAG $REPO_PATH/main.git \
    #&& git clone --branch $TAG $REPO_PATH/seedlink.git \
    && git clone --branch $TAG $REPO_PATH/extras.git \
    && echo "Cloning external base components" \
    #&& git clone --branch $TAG $REPO_PATH/contrib-gns.git \
    #&& git clone --branch $TAG $REPO_PATH/contrib-ipgp.git \
    && git clone --branch $TAG  https://github.com/swiss-seismological-service/sed-SeisComP-contributions.git contrib-sed \
    && echo "Done" 

RUN mkdir -p $WORK_DIR/seiscomp/build \
    && cd $WORK_DIR/seiscomp/build \
    && cmake .. $D \
    && make \ 
    && make install

# Install addons
ADD ./ $WORK_DIR/seiscomp/src/base/sed-addons/
RUN cd $WORK_DIR/seiscomp/build \
    && find $WORK_DIR/seiscomp/src/base/sed-addons/ -name "Icon*" -exec rm {} \; \
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

RUN apt-get install -y coreutils

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

RUN cp -r $WORK_DIR/seiscomp/src/base/sed-addons/test/vs/* /home/sysop/.seiscomp/ \
    && sed -i 's;schema/0.*" version="0.*">;schema/0.9" version="0.9">;'  /home/sysop/.seiscomp/*xml \
    && ls /home/sysop/.seiscomp/seedlink.ini 

RUN mkdir -p $INSTALL_DIR/var/lib/seedlink \
    && sed 's;/opt/seiscomp_test;'$INSTALL_DIR';g' /home/sysop/.seiscomp/seedlink.ini > $INSTALL_DIR/var/lib/seedlink/seedlink.ini

RUN mkdir -p $INSTALL_DIR/var/run/seedlink \
    && mkfifo $INSTALL_DIR/var/run/seedlink/mseedfifo

RUN $INSTALL_DIR/bin/seiscomp enable seedlink scautopick scautoloc scevent sceewenv scvsmag sceewlog

WORKDIR /home/sysop

USER root

RUN apt-get install -y python3-numpy

RUN mysqld_safe --skip-networking & \
    sleep 5 && \
    mysql -u root -e "CREATE DATABASE seiscomp" && \
    mysql -u root -e "CREATE USER 'sysop'@'localhost' IDENTIFIED BY 'sysop'" && \
    mysql -u root -e "GRANT ALL PRIVILEGES ON * . * TO 'sysop'@'localhost'" && \
    mysql -u root -e "FLUSH PRIVILEGES" && \
    mysql -u root seiscomp <  $INSTALL_DIR/share/db/mysql.sql && \ 
    #mysql -u root seiscomp <  $INSTALL_DIR/share/db/vs/mysql.sql && \ 
    #mysql -u root seiscomp <  $INSTALL_DIR/share/db/wfparam/mysql.sql  && \ 
    #su sysop -s /bin/bash -c "source /home/sysop/.profile ;  scdb -i /home/sysop/.seiscomp/inventory.xml -d mysql://sysop:sysop@localhost/seiscomp" &&\
    #su sysop -s /bin/bash -c "source /home/sysop/.profile ;  scdb -i /home/sysop/.seiscomp/bindings.xml -d mysql://sysop:sysop@localhost/seiscomp" &&\
    su sysop -s /bin/bash -c "source /home/sysop/.profile ;  seiscomp start ; timeout -300 msrtsimul /home/sysop/.seiscomp/sorted.mseed ;seiscomp status ;  slinktool -Q : ;  seiscomp stop"

RUN tail /home/sysop/.seiscomp/log/sc*.log
#RUN head /home/sysop/.seiscomp/log/*_reports/*


## Start sshd
RUN passwd -d sysop
RUN sed -i'' -e's/^#PermitRootLogin prohibit-password$/PermitRootLogin yes/' /etc/ssh/sshd_config \
    && sed -i'' -e's/^#PasswordAuthentication yes$/PasswordAuthentication yes/' /etc/ssh/sshd_config \
    && sed -i'' -e's/^#PermitEmptyPasswords no$/PermitEmptyPasswords yes/' /etc/ssh/sshd_config \
    && sed -i'' -e's/^UsePAM yes/UsePAM no/' /etc/ssh/sshd_config
EXPOSE 22
CMD ["/usr/sbin/sshd", "-D"]
