# libpam-google-authenticator compatible shell tool to verify OATH

Verify OATH from shell level by calling `libpam-google-authenticator` PAM module.

- [google-authenticator](https://github.com/google/google-authenticator-libpam) is a tool to protect your server with OATH tokens (based on RFC4226 HOTP and RFC6238 TOTP).
  - You can install it on Debian with `sudo apt-get install libpam-google-authenticator`
- However this only installs a commandline tool to setup OATH, but not to use OATH from your shell scripts or other services.
- There is `oathtool`, but this is not directly compatible to `libpam-google-authenticator`
  - To check a token in `bash`, like in `checktotp XXXXXX && echo valid token || echo invalid token` following can be used:

        checktotp()
        {
        local KEY window="${2:-7}" interval="${3:-30}";
        read KEY < "$HOME/.google_authenticator";
        oathtool -bw$[window-1] --totp --now="now-$[(window-1)*15]s' "$KEY" |
        fgrep -qx "$1";
        }

    Usage is `checktotp TOKEN [window [interval]]` where

    - `interval` is `30`s by default
    - `window` is 7 by default (which means `+-90s` with the default interval of `30`)

    Notes:

    - The defaults are not parsed out of `$HOME/.google_authenticator` yet.
    - No other options of `$HOME/.google_authenticator` are supported
    - Emergency fallback tokens are not supported as well.
    - HOTP is not supported.

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

