#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
# Owned, short-lived certificates for native_http_proxy.py. No OS trust changes.
set -euo pipefail
if [ "$#" != 1 ]; then
    printf 'Usage: %s /absolute/fresh/output-directory\n' "$0" >&2
    exit 2
fi
work=$1
bash "$(dirname "$0")/generate_network_tls.sh" "$work"
cat > "$work/wrong-extensions.txt" <<'CONF'
[server]
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature
extendedKeyUsage=serverAuth
subjectAltName=DNS:other-owned.invalid
CONF
openssl x509 -req -sha512 -days 2 -in "$work/server.csr" \
    -CA "$work/trust/ca.pem" -CAkey "$work/ca.key" -set_serial 3 \
    -extfile "$work/wrong-extensions.txt" -extensions server -out "$work/wrong-server.pem"
openssl verify -CAfile "$work/trust/ca.pem" "$work/wrong-server.pem"
