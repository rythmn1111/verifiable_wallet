/**
 * Build deep hash for Arweave data item (ANS-104) for hardware signing.
 * Protocol: hash is shown in QR → device signs hash → app gets signature via QR → build signed item → upload.
 */
import { createData, deepHash, stringToBuffer } from '@dha-team/arbundles';
import { Buffer } from 'buffer';

type DataItemLike = ReturnType<typeof createData>;

const ARWEAVE_OWNER_LENGTH = 512;
const ARWEAVE_SIGNATURE_LENGTH = 512;
const ARWEAVE_SIGNATURE_TYPE = 1;

export function fromB64Url(s: string): Uint8Array {
  const base64 = s.replace(/-/g, '+').replace(/_/g, '/');
  const pad = base64.length % 4;
  const padded = pad ? base64 + '='.repeat(4 - pad) : base64;
  const binary = atob(padded);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) bytes[i] = binary.charCodeAt(i);
  return bytes;
}

export function toB64Url(buf: Uint8Array): string {
  let b64 = '';
  const chunkSize = 0x8000;
  for (let i = 0; i < buf.length; i += chunkSize) {
    b64 += String.fromCharCode(...buf.subarray(i, i + chunkSize));
  }
  b64 = btoa(b64);
  return b64.replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
}

async function* fileToAsyncIterable(file: File): AsyncGenerator<Buffer> {
  const reader = file.stream().getReader();
  while (true) {
    const { done, value } = await reader.read();
    if (done) break;
    yield Buffer.from(value);
  }
}

export function ownerBytesError(byteLength: number): string {
  if (byteLength === 32) {
    return (
      'The QR contains the Arweave address (32 bytes), not the full public key. ' +
      'Use your wallet’s “Public key” or “Owner” QR (512 bytes), not the “Address” QR.'
    );
  }
  return `Arweave owner must be 512 bytes (base64url from wallet QR), got ${byteLength}.`;
}

function makeStubSigner(ownerBytes: Uint8Array) {
  if (ownerBytes.length !== ARWEAVE_OWNER_LENGTH) {
    throw new Error(ownerBytesError(ownerBytes.length));
  }
  return {
    publicKey: Buffer.from(ownerBytes),
    signatureType: ARWEAVE_SIGNATURE_TYPE,
    signatureLength: ARWEAVE_SIGNATURE_LENGTH,
    ownerLength: ARWEAVE_OWNER_LENGTH,
  };
}

export type BuildHashResult = {
  hashB64: string;
  header: DataItemLike;
  hashBytes: Uint8Array;
};

export async function buildDataItemHash(
  file: File,
  publicKeyB64: string,
  tags: { name: string; value: string }[]
): Promise<BuildHashResult> {
  const ownerBytes = fromB64Url(publicKeyB64);
  const stubSigner = makeStubSigner(ownerBytes);
  const header = createData('', stubSigner as never, { tags });
  const fileIter = fileToAsyncIterable(file);
  const parts = [
    stringToBuffer('dataitem'),
    stringToBuffer('1'),
    stringToBuffer(header.signatureType.toString()),
    Uint8Array.from(header.rawOwner),
    Uint8Array.from(header.rawTarget),
    Uint8Array.from(header.rawAnchor),
    Uint8Array.from(header.rawTags),
    fileIter,
  ];
  const hash = await deepHash(parts);
  const hashBytes = new Uint8Array(hash);
  const hashB64 = toB64Url(hashBytes);
  return { hashB64, header, hashBytes };
}

/** Build signed data item stream (header + signature + file) for upload */
export async function buildSignedDataItemStream(
  header: DataItemLike,
  signatureBytes: Uint8Array,
  file: File
): Promise<{ stream: ReadableStream<Uint8Array>; size: number }> {
  if (signatureBytes.length !== ARWEAVE_SIGNATURE_LENGTH) {
    throw new Error(
      `Signature must be exactly ${ARWEAVE_SIGNATURE_LENGTH} bytes (got ${signatureBytes.length}). ` +
        'Ensure the full signature QR was scanned and the device uses RSA-PSS SHA-256.'
    );
  }
  await header.setSignature(Buffer.from(signatureBytes));
  const headerBytes = header.getRaw();
  const headerLen = headerBytes.byteLength;
  const totalSize = headerLen + file.size;

  const stream = new ReadableStream<Uint8Array>({
    async start(controller) {
      controller.enqueue(new Uint8Array(headerBytes));
      const reader = file.stream().getReader();
      while (true) {
        const { done, value } = await reader.read();
        if (done) break;
        controller.enqueue(value);
      }
      controller.close();
    },
  });

  return { stream, size: totalSize };
}
