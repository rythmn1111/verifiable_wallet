# Hardware QR protocol (Vite app ↔ device)

## Flow

1. **Public key QR (device → app)**  
   User shows a QR on the device. App scans it and uses the value as the Arweave **owner** (512-byte RSA public key), base64url-encoded.

2. **Hash QR (app → device)**  
   App shows a QR. Device scans it, gets the hash to sign.

3. **Signature QR (device → app)**  
   Device signs the hash with the user’s JWK and shows a QR. App scans it and uses the signature to build the signed data item and upload.

---

## 1. Public key QR (device → app)

- **Content:** Base64url-encoded **512-byte** Arweave **owner** (same as ANS-104 data item `owner` field). This is the **full RSA public key**, not the Arweave address.
- **Important:** Do **not** send the Arweave **address** (43-character base64url = 32 bytes decoded). The app needs the full 512-byte owner to build the data item. If your wallet only shows “Address” QR, add a “Public key” / “Owner” menu option that outputs the 512-byte owner (e.g. from the JWK’s `n` + `e` in the format used by ANS-104).
- **Optional JSON:** `{ "publicKey": "<base64url-512-bytes>" }` — app also accepts raw base64url string.

---

## 2. Hash QR (app → device)

- **Content (JSON):**  
  `{ "v": 1, "hash": "<base64url-48-bytes>" }`
- **`hash`:** Base64url-encoded 48-byte SHA-384 deep hash of the data item (ANS-104 deep hash of `["dataitem","1", signatureType, rawOwner, rawTarget, rawAnchor, rawTags, fileStream]`).
- **Device:** Decode `hash` from base64url, compute **SHA-256**(hash) to get 32 bytes, then sign with the Arweave JWK using **RSA-PSS with SHA-256** (to match arbundles/gateway). Result is a **512-byte** signature.

---

## 3. Signature QR (device → app)

- **Content:** Base64url-encoded **512-byte** signature.
- **Optional JSON:**  
  `{ "signature": "<base64url-512-bytes>" }` or `{ "sig": "..." }` — app also accepts raw base64url string.

---

## Summary for device firmware

| Step | Direction | Payload |
|------|-----------|--------|
| Public key | Device → App | Base64url(**512-byte owner**, not the 32-byte address). Optional: `{"publicKey":"..."}` |
| Hash | App → Device | `{"v":1,"hash":"<base64url-48-byte-hash>"}`. Sign the 48 bytes with JWK → 512-byte signature. |
| Signature | Device → App | Base64url(512-byte signature). Optional: `{"signature":"..."}` |
