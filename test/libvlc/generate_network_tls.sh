#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
# Generate short-lived, owned loopback certificates. Never import into an OS store.
set -euo pipefail
if [ "$#" != 1 ]; then
    printf 'Usage: %s /absolute/fresh/output-directory\n' "$0" >&2
    exit 2
fi
work=$1
case "$work" in /*) ;; *) printf 'Use an absolute output directory\n' >&2; exit 2 ;; esac
test ! -e "$work"
mkdir -p "$work/trust" "$work/empty"
exec > "$work/generation.log" 2>&1
openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:secp521r1 -sha512 -nodes -days 2 -subj '/CN=Owned Network Fixture Root' -addext 'basicConstraints=critical,CA:TRUE' -addext 'keyUsage=critical,keyCertSign,cRLSign' -keyout "$work/ca.key" -out "$work/trust/ca.pem"
openssl req -new -newkey ec -pkeyopt ec_paramgen_curve:secp521r1 -nodes -subj '/CN=owned-network.invalid' -keyout "$work/server.key" -out "$work/server.csr"
cat > "$work/extensions.txt" <<'CONF'
[server]
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature
extendedKeyUsage=serverAuth
subjectAltName=DNS:owned-network.invalid
CONF
openssl x509 -req -sha512 -days 2 -in "$work/server.csr" -CA "$work/trust/ca.pem" -CAkey "$work/ca.key" -CAcreateserial -extfile "$work/extensions.txt" -extensions server -out "$work/server.pem"
cat "$work/server.pem" "$work/trust/ca.pem" > "$work/server-chain.pem"
openssl verify -CAfile "$work/trust/ca.pem" "$work/server.pem"
