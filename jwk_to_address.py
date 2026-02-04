#!/usr/bin/env python3
"""
Arweave JWK → public key and wallet address (for testing).

Paste your keyfile JSON below, or pass a file path as the first argument.
Outputs:
  - Public key (n from JWK, base64url)
  - Arweave address (43 chars): base64url(SHA256(decoded_n))

Usage:
  python3 jwk_to_address.py
  python3 jwk_to_address.py path/to/keyfile.json
"""

import hashlib
import json
import base64
import sys

# Paste your keyfile JSON between the triple quotes (need at least "kty" and "n").
# Or run: python3 jwk_to_address.py path/to/keyfile.json
DEFAULT_KEYFILE_JSON = """
{"kty":"RSA","e":"AAEAAQ","n":"PASTE_YOUR_FULL_JWK_OR_JUST_N_HERE"}
"""


def b64url_decode(s: str) -> bytes:
    """Decode base64url (no padding, - and _) to raw bytes."""
    pad = 4 - (len(s) % 4)
    if pad != 4:
        s = s + ("=" * pad)
    return base64.urlsafe_b64decode(s.replace("-", "+").replace("_", "/"))


def b64url_encode(data: bytes) -> str:
    """Encode raw bytes to base64url (no padding)."""
    s = base64.urlsafe_b64encode(data).decode("ascii").rstrip("=")
    return s.replace("+", "-").replace("/", "_")


def jwk_to_address(jwk: dict) -> tuple[str, str]:
    """
    From Arweave RSA JWK, return (public_key_n, address).
    address = base64url(SHA256(decoded_n))
    """
    n_b64 = jwk.get("n")
    if not n_b64 or "PASTE" in n_b64:
        raise ValueError("JWK missing valid 'n'. Paste your keyfile JSON in this script or pass a file path.")
    n_bytes = b64url_decode(n_b64)
    digest = hashlib.sha256(n_bytes).digest()
    address = b64url_encode(digest)
    return n_b64, address


def main():
    if len(sys.argv) > 1:
        path = sys.argv[1]
        with open(path, "r") as f:
            jwk = json.load(f)
    else:
        raw = DEFAULT_KEYFILE_JSON.strip()
        if not raw:
            print("Paste your keyfile JSON in DEFAULT_KEYFILE_JSON or pass a file path.")
            sys.exit(1)
        jwk = json.loads(raw)

    if jwk.get("kty") != "RSA":
        print("Warning: expected kty 'RSA' for Arweave.")

    public_key_n, address = jwk_to_address(jwk)

    print("Public key (n, base64url):")
    print(public_key_n)
    print()
    print("Arweave address (43 chars):")
    print(address)
    print()
    print(f"Address length: {len(address)} (expected 43)")


if __name__ == "__main__":
    main()
