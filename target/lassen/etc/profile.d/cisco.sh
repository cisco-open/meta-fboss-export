#!/bin/bash
#
function ins_env() {
    var=$1
    value=$2
    eval $var=${!var//:${value}:/:}
    eval $var=${!var/%:${value}}
    [[ "${!var}" == ${value} ]] || \
        [[ "${!var}" =~ ${value}:* ]] || \
        eval $var=${value}${!var:+:}${!var}
    export ${var}
}

ins_env PATH /opt/cisco/bin
ins_env LD_LIBRARY_PATH /opt/cisco/lib


unset -f ins_env
