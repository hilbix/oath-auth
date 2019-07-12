#!/bin/bash
#
# Put this in ~/.ssh/authorized_keys like
#	command=".../sshlogin.sh 3600 [INFO]",no-X11-forwarding KEY description
# the number is the seconds to cache an authentication by the given IP.
# Default is 3600, use 0 for no caching
# Caching is done by IP.  Use INFO to cache separated keys from the same IP

AUTHFILE=.ssh/sshlogin.tmp

checkauth()
{
local delta="${1:-3600}"
local info="${*:2}"
local NOW="$(date +%s)"
local IP="${SSH_CONNECTION%% *}"
local stamp ip more diff have auth

[ -f "$AUTHFILE" ] &&
{
have=false
while read -r stamp ip more
do
	let diff=NOW-stamp
	[ $diff -lt "$delta" ] || continue
	have=:
	[ ".$ip $more" = ".$IP $info" ] && return 0
done < "$AUTHFILE"
$have || rm -f "$AUTHFILE"
}

read -rp "$(id -un)@$(uname -n) $(date +%Y%m%d-%H%M%S)${info:+ }$info auth: " auth || return 1
oath-auth "$auth" || return 1

echo "$NOW $IP $info" >> "$AUTHFILE"
return 0
}

checkauth "$@" && eval exec -- "${SSH_ORIGINAL_COMMAND:-"$SHELL" -l}"

