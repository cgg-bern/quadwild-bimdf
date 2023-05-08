#!/bin/sh
# to run with snakemake/singularity, we need a container registry, ugh.
# start one like this:
# docker run -d -p 5000:5000 --restart=always --name registry registry:2


docker build . -t localhost:5000/quadwild:latest && docker push localhost:5000/quadwild:latest
