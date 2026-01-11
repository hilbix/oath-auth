#!/bin/bash

exec qrencode -t ANSI256 -o - "otpauth://totp/$USER?issuer=$(uname -n)&secret=$(head -1 ~/.google_authenticator)"

