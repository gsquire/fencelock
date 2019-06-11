#!/bin/bash

set -ex

make
cp fencelock.so ci/
docker-compose -f ci/docker-compose.yml run --rm test
