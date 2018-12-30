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
NOW="$(date +%s)"
IP="${SSH_CONNECTION%% *}"
[ -f "$AUTHFILE" ] &&
{
have=false
while read -r stamp ip more
do
	let diff=NOW-stamp
	[ $diff -lt "$1" ] || continue
	have=:
	[ ".$ip $more" = ".$IP $2" ] && return 0
done < "$AUTHFILE"
$have || rm -f "$AUTHFILE"
}

read -rp "$(date +%Y%m%d-%H%M%S) auth: " auth || return 1
google-auth "$auth" || return 1

echo "$NOW $IP $2" >> "$AUTHFILE"
return 0
}

checkauth "${1:-3600}" "${*:2}" && exec -- ${SSH_ORIGINAL_COMMAND:-"$SHELL"}
