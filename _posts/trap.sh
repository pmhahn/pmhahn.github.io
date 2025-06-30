#!/bin/bash
cleanup () {
    local rv=$? sig=${1:-0}
    echo "Process $$ received signal $sig after rv=$rv"
    case "$sig" in
    0|'') exit "$rv";;
    *) trap - "$sig"; kill "-$sig" "$$";;
    esac
}
trap 'cleanup 0' EXIT
trap 'cleanup 1' HUP
trap 'cleanup 2' INT
trap 'cleanup 3' QUIT
trap 'cleanup 15' TERM

[ -n "${1:-}" ] && kill "-$1" "$$"
