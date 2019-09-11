FROM gitlab-registry.cern.ch/corryvreckan/corryvreckan/corryvreckan-deps:latest
MAINTAINER Simon Spannagel <simon.spannagel@cern.ch>

# Copy currently checked out git revision:
COPY . /opt/corryvreckan

ENV PATH="/opt/corryvreckan/bin:${PATH}"
WORKDIR "/data"

# Compile Corryvreckan
RUN source scl_source enable devtoolset-7 && \
    mkdir -p /opt/corryvreckan/build && \
    cd /opt/corryvreckan/build && \
    cmake3 -DBUILD_EventLoaderEUDAQ2=ON \
           -DCMAKE_INSTALL_PREFIX=../ \
           -DCMAKE_MODULE_PATH="/usr/share/cmake3/Modules/;/usr/share/cmake/Modules/" \
          .. && \
    make -j`grep -c processor /proc/cpuinfo` && \
    make install && \
    rm -rf /opt/corryvreckan/build

# Default command for starting the container, executed after the ENTRYPOINT
CMD ["bash"]
