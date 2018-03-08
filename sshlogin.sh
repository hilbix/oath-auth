#!/bin/bash
#
# Put this in ~/.ssh/authorized_keys like
#	command=".../sshlogin.sh 3600",no-X11-forwarding KEY description
# the number is the seconds to cache an authentication by the given IP.
# Default is 3600

AUTHFILE=.ssh/sshlogin.tmp

checkauth()
{
NOW="$(date +%s)"
IP="${SSH_CONNECTION%% *}"
have=false
[ -f "$AUTHFILE" ] &&
while read -r stamp ip
do
	let diff=NOW-stamp
	[ $diff -lt "$1" ] || continue
	have=:
	[ ".$ip" = ".$IP" ] && return 0
done < "$AUTHFILE"

$have || rm -f "$AUTHFILE"

read -rp 'auth: ' auth || return 1
google-auth "$auth" || return 1

echo "$NOW $IP" >> "$AUTHFILE"
return 0
}

checkauth "${1:-3600}" && exec -- ${SSH_ORIGINAL_COMMAND:-"$SHELL"}
