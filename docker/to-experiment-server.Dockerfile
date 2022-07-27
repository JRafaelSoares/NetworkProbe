FROM to-experiment-base:latest

ARG source_branch=master
ARG build_branch=build

USER root

# Re-cloning the latest version of the repository
RUN rm -r $HYDRO_HOME/to-experiment
WORKDIR $HYDRO_HOME
RUN git clone https://github.com/tiagopmartins/TotalOrderExperiment.git to-experiment
WORKDIR $HYDRO_HOME/to-experiment

RUN git pull
RUN git remote remove origin && git remote add origin https://github.com/tiagopmartins/TotalOrderExperiment.git

RUN git fetch origin && git checkout -b $build_branch origin/$source_branch
RUN bash scripts/build_server.sh -j2 -bRelease
WORKDIR /hydro