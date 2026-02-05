o# Arweave Wallet Recipe

This document records the exact formula we use to generate Arweave-compatible wallets (on the device and for future use, e.g. a website). Follow it to reproduce the same wallet format and addresses.

---

## 1. Overview

- **Goal:** Generate an Arweave wallet = RSA 4096-bit key pair in **JWK (JSON Web Key)** form.
- **Compatibility:** The resulting keyfile can be imported into [Wander](https://wander.app), [Arweave.app](https://arweave.app), and any tool that accepts Arweave JWK keyfiles.
- **Determinism:** Same 12-word mnemonic + same recipe → same JWK → same address.

---

## 2. Wallet Generation Formula (Step by Step)

### Step 1: Entropy

- Generate **128 bits (16 bytes)** of cryptographically secure random data.
- This is the only random input; everything else is deterministic from it.

```
entropy = random_bytes(16)
```

### Step 2: BIP39 — Entropy → 12 Words

- Use the standard **BIP39** encoding for 12-word mnemonics (English wordlist, 2048 words).
- **Checksum:** SHA256(entropy), take the **first 4 bits** (high nibble of first byte). Append these 4 bits to the 128 entropy bits → 132 bits total.
- **Words:** Split the 132 bits into 12 groups of **11 bits** each (MSB-first). Each 11-bit value is an index into the BIP39 English wordlist (0–2047). Join the words with single spaces.

**Pseudocode:**

```
hash = SHA256(entropy)
checksum_bits = first 4 bits of hash
bits = entropy_bits || checksum_bits   // 128 + 4 = 132 bits
for i in 0..11:
  idx = bits[i*11 .. i*11+10] as integer
  word[i] = BIP39_ENGLISH_WORDLIST[idx]
mnemonic = word[0] + " " + word[1] + ... + " " + word[11]
```

**Reference:** [BIP39](https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki) (128 bits → 12 words).

### Step 3: Seed from Mnemonic (PBKDF2)

- Derive a **64-byte seed** from the mnemonic using **PBKDF2** with:
  - **Hash:** HMAC-SHA512
  - **Password:** the mnemonic string (UTF-8)
  - **Salt:** the literal string `"mnemonic"` (no trailing null in the salt)
  - **Iterations:** 2048
  - **Output length:** 64 bytes

**Pseudocode:**

```
salt = "mnemonic"   // 9 bytes, no null terminator in the PBKDF2 count
seed = PBKDF2-HMAC-SHA512(password=mnemonic, salt=salt, iterations=2048, key_len=64)
```

This matches BIP39’s standard seed derivation (without optional passphrase).

### Step 4: RSA 4096 Key from Seed

- Use the 64-byte seed as the **only** entropy for a deterministic RNG.
- **RNG:** Use a CSPRNG (e.g. CTR-DRBG or similar) **seeded only** with `seed`. No extra system randomness after that.
- **Key gen:** Generate an **RSA 4096-bit** key pair with:
  - **Public exponent:** 65537 (0x010001)
  - **RNG:** The deterministic RNG above

So: same seed → same RSA key every time.

**Pseudocode:**

```
drbg = CSPRNG_seed(seed)
(rsa_private, rsa_public) = RSA_generate_key(4096, e=65537, rng=drbg)
```

### Step 5: RSA → JWK (JSON Web Key)

- Export the RSA key to **JWK** format.
- All multi-byte integer fields are encoded as **base64url** (see below). No padding; use `-` and `_` instead of `+` and `/`.
- **Fields to include:**

| JWK field | Meaning              | Length (4096-bit RSA)     |
|-----------|----------------------|---------------------------|
| `kty`     | Key type             | `"RSA"`                   |
| `e`       | Public exponent      | Usually 3 bytes → base64url (e.g. `"AQAB"` or `"AAEAAQ"` for 65537) |
| `n`       | Modulus              | 512 bytes → base64url     |
| `d`       | Private exponent     | 512 bytes → base64url     |
| `p`       | Prime 1              | 256 bytes → base64url     |
| `q`       | Prime 2              | 256 bytes → base64url     |
| `dp`      | d mod (p−1)          | CRT exponent → base64url  |
| `dq`      | d mod (q−1)          | CRT exponent → base64url  |
| `qi`      | q⁻¹ mod p            | CRT coefficient → base64url |

- **Base64url:** Alphabet `A–Z`, `a–z`, `0–9`, `-`, `_`. No padding. Standard base64 with `+` → `-`, `/` → `_`, and trailing `=` removed.

**Example JWK shape (values abbreviated):**

```json
{
  "kty": "RSA",
  "e": "AAEAAQ",
  "n": "<base64url of 512-byte modulus>",
  "d": "<base64url>",
  "p": "<base64url>",
  "q": "<base64url>",
  "dp": "<base64url>",
  "dq": "<base64url>",
  "qi": "<base64url>"
}
```

This JWK is the **keyfile** the user can export and import in Wander or other wallets.

---

## 3. Public Key and Arweave Address

### What is the “public key”?

- **In JWK terms:** The public key is the pair **(n, e)**. The value **n** (modulus) is the long base64url string; **e** is the public exponent (usually 65537).
- **What to give when someone asks for “your public key”:** Either the full public JWK `{"kty":"RSA","e":"...","n":"..."}` or just the **n** value, depending on what the other system expects. Never share **d**, **p**, **q**, **dp**, **dq**, **qi**.

### How the Arweave address is derived

- The **wallet address** used on Arweave (and in UIs like Wander) is **not** the raw public key. It is a 43-character string derived from it:

**Formula:**

1. Take the **n** field from the JWK (base64url string).
2. **Decode** n from base64url to raw bytes (512 bytes for 4096-bit RSA).
3. Compute **SHA256** of those bytes → 32 bytes.
4. **Encode** the 32-byte hash as base64url (no padding).
5. The result is exactly **43 characters** — that is the Arweave wallet address.

**Pseudocode:**

```
n_bytes = base64url_decode(jwk.n)
address = base64url(SHA256(n_bytes))
// len(address) == 43
```

**Example:** If your JWK has  
`"n": "rS53EOKY3zrzGK2ptA6na4p5F3_rnAL96n4Ji_B3tiqQ..."`  
then after decode → SHA256 → base64url you get something like  
`rK5PdmW_l-JzlS5JA8ISUlUr2zJqX4uVQJzgGl2r9Mg`  
(43 chars). That is what you use to receive AR and identify the wallet.

---

## 4. How Wander Shows the Public Address

Wander (and the broader Arweave ecosystem) uses two different notions of “public” identifier:

### In the UI (what users see and copy)

- **Account screen, Receive, QR code, “Copy address”:** Wander shows the **43-character Arweave address** (the base64url(SHA256(n)) value).
- So when you import our keyfile into Wander, the “Account 3” line and the copy button show this 43-char string. That is what you give to others to receive AR or tokens.

### In the API (for apps/dApps)

- **`getActivePublicKey` (or equivalent):** Wander returns the **raw n** (the long base64url modulus string), not the 43-char address.
- So in code, “public key” often means **n**; the 43-char address is derived from n as above.

**Summary:**

| Context              | What Wander uses                    |
|----------------------|-------------------------------------|
| UI (receive, QR, copy)| **43-character address**            |
| API (“get public key”)| **n** (long base64url modulus)      |

Our recipe’s **address** matches what Wander shows in the UI. Our **n** matches what Wander’s API returns as the public key.

---

## 5. Constants Summary (for implementation)

| Item              | Value / Convention                          |
|-------------------|---------------------------------------------|
| Entropy size      | 128 bits (16 bytes)                         |
| BIP39 word count  | 12 words                                    |
| BIP39 wordlist    | English, 2048 words                         |
| PBKDF2 salt       | `"mnemonic"` (9 bytes)                     |
| PBKDF2 iterations | 2048                                        |
| PBKDF2 hash       | HMAC-SHA512                                 |
| Seed length       | 64 bytes                                    |
| RSA key size      | 4096 bits                                   |
| RSA exponent e    | 65537                                       |
| Address length    | 43 characters (base64url of SHA256 of n)   |
| Base64url         | A–Z a–z 0–9 `-` `_`, no padding             |

---

## 6. Verification

- **Script:** `jwk_to_address.py` in this repo takes a keyfile (JWK) and computes the public key (n) and the 43-char address. Use it to verify that a generated or imported keyfile yields the same address as Wander.
- **Flow:** Generate wallet on device → export keyfile → import into Wander → compare 43-char address with `python3 jwk_to_address.py keyfile.json`. They must match.

---

## 7. References

- [BIP39 – Mnemonic code for generating deterministic keys](https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki)
- [Arweave Cookbook – Wallets and Keys](https://cookbook.arweave.dev/fundamentals/wallets-and-keyfiles/)
- [RFC 7517 – JSON Web Key (JWK)](https://www.rfc-editor.org/rfc/rfc7517)
- [Arweave JS](https://github.com/ArweaveTeam/arweave-js) (e.g. `jwkToAddress`)
