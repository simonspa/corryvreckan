FROM gitlab-registry.cern.ch/corryvreckan/corryvreckan/corryvreckan-deps:latest
MAINTAINER Simon Spannagel <simon.spannagel@cern.ch>

# Copy currently checked out git revision:
COPY . /opt/corryvreckan

ENV PATH="/opt/corryvreckan/bin:${PATH}"
WORKDIR "/data"

# Compile Corryvreckan
RUN mkdir -p /opt/corryvreckan/build && \
    cd /opt/corryvreckan/build && \
    cmake -DBUILD_EventLoaderEUDAQ2=ON \
          -Deudaq_DIR="/opt/eudaq2/cmake" \
          -DCMAKE_INSTALL_PREFIX=../ \
         .. && \
    make -j`grep -c processor /proc/cpuinfo` && \
    make install && \
    rm -rf /opt/corryvreckan/build

# Default command for starting the container, executed after the ENTRYPOINT
CMD ["bash"]
