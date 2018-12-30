# libpam-google-authenticator compatible shell tool to verify OATH

Verify OATH from shell level by calling `libpam-google-authenticator` PAM module.

- [google-authenticator](https://github.com/google/google-authenticator-libpam) is a tool to protect your server with OATH tokens (based on RFC4226 HOTP and RFC6238 TOTP).
  - You can install it on Debian with `sudo apt-get install libpam-google-authenticator`
- However this only installs a commandline tool to setup OATH, but not to use OATH from your shell scripts or other services.

This here is a wrapper to call `/lib/security/pam_google_authenticator.so` from shell level.
Actually you can call each PAM security module this way.


## Usage

    sudo apt-get install build-essential libpam0g-dev    # for example: Debian 8.9

    git clone https://github.com/hilbix/google-auth.git
    cd google-auth
    make
    sudo make install

To check a token:

    /usr/local/bin/google-auth "$TOKEN"

To use another PAM security module:

    /usr/local/bin/google-auth "$PASSWORD" /lib/x86_64-linux-gnu/security/pam_unix.so

etc. (actually this example fails, because there is something missing I do not know).


## Setup

	sudo apt-get install libpam-google-authenticator
	google-authenticator
	# Follow the white rabbit
	# Test it with some token (change the number accordingly)
	google-auth 123456; echo $?
	# This should output something like "ok 1 123456" and "0"

Now in `~/.ssh/authorized_keys` prefix the given key with `command="/usr/local/bin/sshlogin.sh" ` (do not forget the space after the closing quote) like in:

	command="/usr/local/bin/sshlogin.sh" ssh-ed25519 ..etc..

This then asks for authentication code if you use this key:

- The answer is cached for one hour (3600 seconds) by default
- The answer is cached on the connecting IP.  (So if some other key is used with the same IP, this is allowed)
- 1st argument to `sshlogin.sh` is the number of seconds to cache the given key.  Use 0 to disable caching.
- 2nd argument to `sshlogin.sh` is the cache-group (all keys from the same IP and the same cache-group can be used interchangeably).  Just give each key a separate value to only cache one key per IP (this cannot be automated due to a limitation of `ssh`).

Notes:

- Caching is good for things like `scp`.  First just do an interactive connection, then you can re-use the key on the copy-script.
- Caching does not refresh on key-usage.  This is a security feature, so the cache times out, regardless how often it was used.
- `sshlogin.sh` is not race-condition-free.  In worst case caching just fails.


## Alternatives

There is `oathtool` (`sudo apt-get install oathtool`), but this is not directly compatible to `libpam-google-authenticator`.  To check a token in `bash` with `oathtool` following can be used:

        checktotp()
        {
        local KEY window="${2:-7}" interval="${3:-30}";
        read KEY < "$HOME/.google_authenticator";
        oathtool -bw$[window-1] --totp --now="now-$[(window-1)*15]s' "$KEY" |
        fgrep -qx "$1";
        }

Example:

	checktotp XXXXXX && echo valid token || echo invalid token

Usage:

	checktotp TOKEN [window [interval]]

- `interval` is `30`s by default
- `window` is 7 by default (which means `+-90s` with the default interval of `30`)

However:

- The defaults are not parsed out of `$HOME/.google_authenticator`
- No other options of `$HOME/.google_authenticator` are supported
- Emergency fallback tokens are not supported as well
- HOTP is not supported

