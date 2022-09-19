FROM tiagopm/to-experiment:base

ARG source_branch=master
ARG build_branch=build

USER root

# Re-cloning the latest version of the repository
RUN rm -r $HYDRO_HOME
WORKDIR $HYDRO_HOME
RUN git clone https://github.com/tiagopmartins/TotalOrderExperiment.git to-experiment
WORKDIR $HYDRO_HOME/to-experiment

RUN git pull
RUN git remote remove origin && git remote add origin https://github.com/tiagopmartins/TotalOrderExperiment.git
RUN git fetch origin && git checkout -b $build_branch origin/$source_branch

WORKDIR $HYDRO_HOME/to-experiment
# Update the time using NTP, add message delays of 100ms (+- 10ms) and run the server
CMD ntpd -d -q -n -p pool.ntp.org && \
    tc qdisc add dev eth0 root netem delay 100ms 10ms && \
    bash scripts/build_server.sh -j2 -bRelease