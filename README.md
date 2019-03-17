[![Build Status](https://api.cirrus-ci.com/github/hilbix/google-auth.svg)](https://cirrus-ci.com/github/hilbix/google-auth) [![Test Status](https://api.cirrus-ci.com/github/hilbix/google-auth.svg?task=test&script=test)](https://cirrus-ci.com/github/hilbix/google-auth/master) (The important one is "Ci".  For unknown reason, "test" fails on CI.)


# libpam-google-authenticator compatible shell tool to verify OATH

Verify OATH from shell level by calling `libpam-google-authenticator` PAM module.

- [google-authenticator](https://github.com/google/google-authenticator-libpam) is a tool to protect your server with OATH tokens (based on RFC4226 HOTP and RFC6238 TOTP).
  - You can install it on Debian with `sudo apt-get install libpam-google-authenticator`
- However this only installs a commandline tool to setup OATH, but not to use OATH from your shell scripts or other services.

This here is a wrapper to call `/lib/security/pam_google_authenticator.so` from shell level.
Actually you can call each PAM security module this way.


## Usage

    sudo apt-get install libpam-google-authenticator build-essential libpam0g-dev    # for example: Debian 8.9

    git clone https://github.com/hilbix/google-auth.git
    cd google-auth
    make
    sudo make install

To initialize `google-authenticator` (do not forget to scan the QR-Code!):

	yes | google-authenticator

To check a token:

    /usr/local/bin/google-auth "$TOKEN"

To use the authenticator for ssh-login in `~/.ssh/authorized_keys` just use it as `command`-argument to `ssh`-public-keys like this:

	command="/usr/local/bin/sshlogin.sh",no-port-forwarding,no-X11-forwarding,no-agent-forwarding,no-user-rc ssh-..

`ssh..` is what usually stands on the line.


## Notes on `google-auth`

You can try to use another similar PAM security module like in following:

    /usr/local/bin/google-auth "$PASSWORD" /lib/x86_64-linux-gnu/security/pam_unix.so

However this example fails, because there is something missing I do not know.


## Notes on `sshlogin.sh`:

- It asks for authentication code if you run it.
- The answer is cached for one hour (3600 seconds) by default
- The answer is cached on the connecting IP.  (So if some other key is used with the same IP, this is allowed)
- 1st argument to `sshlogin.sh` is the number of seconds to cache the given key.  Use 0 to disable caching.
- 2nd argument to `sshlogin.sh` is the cache-group (all keys from the same IP and the same cache-group can be used interchangeably).  Just give each key a separate value to only cache one key per IP (this cannot be automated due to a limitation of `ssh`).

Notes:

- Caching is good for things like `scp`.  First just do an interactive connection, then you can re-use the key on the copy-script.
- Caching does not refresh on key-usage.  This is a security feature, so the cache times out, regardless how often it was used.
- `sshlogin.sh` is not race-condition-free.  In worst case caching just fails.
- The cache is at `~/.ssh/sshlogin.tmp`.  It is small, but grow-only as long as there are non-expired entries in it.


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


## FAQ

Why does `sshlogin.sh` print the current time (in ISO format)?

- It is important that both sides, this is the server and your handy, agree to the same time.
- To verify that the server time is not too far off, the current time on login is printed.

How to use different Tokens for different entries of `authorized_keys`?

- This currently is not supported by `google-authenticator`, hence it is not supported by this tool yet (read: I did not find out how).
- Read: To use different Tokens use different logins.

Where do the Tokens come from?

- `~/.google-authenticator` is the file in question
- It has mode `400` (readonly for user, no access for others) which is important
- Keep this file secret, as in the first line there is the shared secret, which must not become public.
- The tokens are calculated from this shared secret and the current time
- Be sure your computer has correct time set

`sshlogin.sh` does allow port forwarding without Token

- This is an limitation on `authorized_keys` which cannot be evaded when using `google-auth`.
- If you do not need port forwarding by `ssh`, disable all possibly dangerous things in `authorized_keys`.  Example:

        command="/usr/local/bin/sshlogin.sh",no-port-forwarding,no-X11-forwarding,no-agent-forwarding,no-user-rc ssh-..

- If you need port forwarding depending on a token, do not use this tool here (`google-auth`),
  instead use the normal `pam_google_authenticator`, see `man pam_google_authenticator`

Differences to `pam_google_authenticator`?

- `google-auth` is configured on a per-user ssh-key-level (`$HOME/.ssh/authorized_keys`),
   while `pam_google_authenticator` operates on system-level (`/etc/pam.d/`).
- `google-auth` uses `pam_google_authenticator`, it neither replaces it nor can overcome limitations of `pam_google_authenticator`.

Does it work on other TOTP authenticators?

- Yes, if they are compatible to `google-authenticator` (which nearly all are)

Is it secure?

- `google-auth` does not protect against holes in `ssh`, `bash` or `pam_google_authenticator`
- TOTP (timebased OTP) are inherently insecure, so do not use them as the only authentication thing.
- Insecurity comes from the fact, that TOTP uses something I call "public shared secret".
  A "shared secret" (like a password) usually is stored as salted hash, such that it can only be obtained by brute-force in case you do not know it already.
  However a "public shared secret" is stored fully unencrypted, so it must be considered to be "public", only protected by weak filesystem access rules.
  (Note that "shared secrets" are passed unencrypted most of the time.  TOTP are passed in encrypted form but stored in cleartext.)
- So it is 2FA, not a primary protection.  Primary protection is based on ssh-keys.
- However it protects the destination (where `sshlogin.sh` is used) if something evil happens with the ssh-key used on the source (as you must first overcome 2FA).

Why 2FA?

- On workstations (without third party access) I ususally store my `ssh`-keys without any passphrase.
- This is important, because after a power loss the machine must be able to come up unattended and run cron-jobs before I have a chance to do `ssh-add`.
- Hence I need some way to protect more important `ssh`-keys in case somebody else gains access to the keys unnoticed.
- This setup must be very natural and easy to use, else you will start to work around your own security measures.  (This is how humans are.  You cannot evade!)
- `google-auth` is quite convenient in that sense, nowadays smartphones are ubiquitous.

Not convenient enough!

- Typing in 6 numbers each hour sucks.  My word for this!  But, for now, I do not have something better.
- U2F with USB is even more annoying, as it needs some physical USB connection.
- What I would like to see is something, which is infrastructure-less and interacts with a phone app:
  - The app must use asymmetric encrption, so the secret needs not to be shared.
  - 3rd party can be used for additinal convenience, but the full regular process must work without 3rd party without limitation, securely, completely and trouble-freely.
  - You ssh, you open the app on your phone, on the screen the ongoing login is presented, you press OK to authorize.
  - There should be some 2nd level on 2FA which authorizes via NFC U2F key.
  - There must be some fallback in case the phone has no Internet connectivity.
  - There must be some fallback in case your phone is broken.
  - There must be some fallback in case the U2F key is broken.
- Note that phone-push would be nice.  However it is enough for me if the app is started by the NFC key and then finds out what is to be done.
  - Actually the idea is to place the phone on a power-pad which has NFC attached.
  - So the authentication takes place using this on-seat NFC-tag via the phone.
  - All you need is to look at the phone and press the right button.
  - If the phone is removed from the power-pad, everything using the authentication from the NFC key automagically is frozen.
  - Note that the latter is done by the app, not the pad.

