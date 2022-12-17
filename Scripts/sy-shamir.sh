#!/bin/bash

HERE=$(cd `dirname $0`; pwd)
SPDZROOT=$HERE/..

export PLAYERS=${PLAYERS:-5}

. $HERE/run-common.sh

run_player sy-shamir-party.x $* || exit 1
