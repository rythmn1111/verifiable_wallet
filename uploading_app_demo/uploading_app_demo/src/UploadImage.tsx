import {
  ArweaveSigner,
  TurboFactory,
  TurboAuthenticatedClient,
} from '@ardrive/turbo-sdk/web';
import Arweave from 'arweave';
import type { JWKInterface } from 'arweave/node/lib/wallet';
import QRCode from 'qrcode';
import { useCallback, useEffect, useState } from 'react';
import { buildDataItemHash, buildSignedDataItemStream, fromB64Url } from './hardwareSign';
import { ScanSignature } from './ScanSignature';

const arweave = new Arweave({ host: 'arweave.net', port: 443, protocol: 'https' });

const MAX_IMAGE_BYTES = 100 * 1024; // 100KB
const ACCEPT_IMAGE = 'image/jpeg,image/png,image/gif,image/webp';

type UploadResult = {
  id: string;
  owner?: string;
  [key: string]: unknown;
} | null;

type HardwareHashResult = {
  header: Awaited<ReturnType<typeof buildDataItemHash>>['header'];
  file: File;
};

export function UploadImage({ scannedPublicKey }: { scannedPublicKey: string | null }) {
  const [jwkRaw, setJwkRaw] = useState('');
  const [turbo, setTurbo] = useState<TurboAuthenticatedClient | null>(null);
  const [jwkError, setJwkError] = useState<string | null>(null);
  const [selectedFile, setSelectedFile] = useState<File | null>(null);
  const [uploadStatus, setUploadStatus] = useState('');
  const [uploadResult, setUploadResult] = useState<UploadResult>(null);
  const [uploading, setUploading] = useState(false);

  // Hardware flow
  const [hardwareHashQR, setHardwareHashQR] = useState<string | null>(null);
  const [hardwareHashResult, setHardwareHashResult] = useState<HardwareHashResult | null>(null);
  const [signatureB64, setSignatureB64] = useState<string | null>(null);
  const [hardwareError, setHardwareError] = useState<string | null>(null);
  const [hardwareUploading, setHardwareUploading] = useState(false);
  const [hardwareUploadResult, setHardwareUploadResult] = useState<UploadResult>(null);

  // Build Turbo client when JWK is valid
  useEffect(() => {
    setJwkError(null);
    if (!jwkRaw.trim()) {
      setTurbo(null);
      return;
    }
    try {
      const jwk = JSON.parse(jwkRaw.trim()) as JWKInterface;
      if (!jwk.kty || !jwk.n || !jwk.e) {
        setJwkError('Invalid JWK: missing kty, n, or e');
        setTurbo(null);
        return;
      }
      const signer = new ArweaveSigner(jwk);
      const client = TurboFactory.authenticated({
        signer,
        token: 'arweave',
      });
      setTurbo(client);
    } catch {
      setJwkError('Invalid JSON');
      setTurbo(null);
    }
  }, [jwkRaw]);

  const handleFileChange = useCallback((e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    setUploadResult(null);
    setUploadStatus('');
    if (!file) {
      setSelectedFile(null);
      return;
    }
    if (file.size > MAX_IMAGE_BYTES) {
      setUploadStatus(`File too large. Max ${MAX_IMAGE_BYTES / 1024}KB.`);
      setSelectedFile(null);
      e.target.value = '';
      return;
    }
    setSelectedFile(file);
  }, []);

  const handleUpload = useCallback(async () => {
    if (!selectedFile || !turbo) {
      setUploadStatus(!turbo ? 'Paste a valid JWK first.' : 'Select an image first.');
      return;
    }
    setUploading(true);
    setUploadStatus('Preparing...');
    setUploadResult(null);

    try {
      const result = await turbo.uploadFile({
        fileStreamFactory: () => selectedFile.stream(),
        fileSizeFactory: () => selectedFile.size,
        dataItemOpts: {
          tags: [
            { name: 'Content-Type', value: selectedFile.type || 'application/octet-stream' },
            { name: 'App-Name', value: 'Upload-App-Demo' },
            { name: 'File-Name', value: selectedFile.name },
          ],
        },
        events: {
          onSigningProgress: ({ processedBytes, totalBytes }) => {
            setUploadStatus(`Signing ${Math.round((processedBytes / totalBytes) * 100)}%`);
          },
          onSigningSuccess: () => setUploadStatus('Signed. Uploading...'),
          onSigningError: (err) => setUploadStatus(`Signing error: ${err.message}`),
          onUploadProgress: ({ processedBytes, totalBytes }) => {
            setUploadStatus(`Upload ${Math.round((processedBytes / totalBytes) * 100)}%`);
          },
          onUploadSuccess: () => setUploadStatus('Upload complete.'),
          onUploadError: (err) => setUploadStatus(`Upload error: ${err.message}`),
        },
      });

      setUploadResult(result as UploadResult);
      setUploadStatus('Done.');
    } catch (err) {
      setUploadStatus(err instanceof Error ? err.message : 'Upload failed.');
    } finally {
      setUploading(false);
    }
  }, [selectedFile, turbo]);

  const handleGenerateHashQR = useCallback(async () => {
    if (!selectedFile || !scannedPublicKey) {
      setHardwareError('Need public key (scan above) and an image.');
      return;
    }
    setHardwareError(null);
    setHardwareHashQR(null);
    setHardwareHashResult(null);
    setSignatureB64(null);
    setHardwareUploadResult(null);
    try {
      const tags = [
        { name: 'Content-Type', value: selectedFile.type || 'application/octet-stream' },
        { name: 'App-Name', value: 'Upload-App-Demo' },
        { name: 'File-Name', value: selectedFile.name },
      ];
      const { hashB64, header } = await buildDataItemHash(selectedFile, scannedPublicKey, tags);
      const payload = JSON.stringify({ v: 1, hash: hashB64 });
      const dataUrl = await QRCode.toDataURL(payload, { width: 280, margin: 1 });
      setHardwareHashQR(dataUrl);
      setHardwareHashResult({ header, file: selectedFile });
    } catch (e) {
      setHardwareError(e instanceof Error ? e.message : 'Failed to build hash');
    }
  }, [selectedFile, scannedPublicKey]);

  const handleHardwareUpload = useCallback(async () => {
    if (!hardwareHashResult || !signatureB64) {
      setHardwareError('Need hash generated and signature scanned.');
      return;
    }
    setHardwareUploading(true);
    setHardwareError(null);
    try {
      const sigBytes = fromB64Url(signatureB64);
      const { stream, size } = await buildSignedDataItemStream(
        hardwareHashResult.header,
        sigBytes,
        hardwareHashResult.file
      );
      const turboUnauth = TurboFactory.unauthenticated({ token: 'arweave' });
      const result = await turboUnauth.uploadSignedDataItem({
        dataItemStreamFactory: () => stream,
        dataItemSizeFactory: () => size,
        events: {
          onUploadProgress: ({ processedBytes, totalBytes }) => {
            setUploadStatus(`Upload ${Math.round((processedBytes / totalBytes) * 100)}%`);
          },
          onUploadSuccess: () => setUploadStatus('Upload complete.'),
          onUploadError: (err) => setUploadStatus(`Upload error: ${err.message}`),
        },
      });
      setHardwareUploadResult(result as UploadResult);
      setUploadStatus('Done.');
    } catch (e) {
      setHardwareError(e instanceof Error ? e.message : 'Upload failed');
    } finally {
      setHardwareUploading(false);
    }
  }, [hardwareHashResult, signatureB64]);

  return (
    <section className="upload-image" style={{ marginTop: '2rem', textAlign: 'left', maxWidth: 520, marginLeft: 'auto', marginRight: 'auto' }}>
      <h2>Upload image (under 100KB)</h2>

      <div style={{ marginBottom: '1rem' }}>
        <label style={{ display: 'block', marginBottom: '0.35rem', fontWeight: 500 }}>
          JWK (to sign in browser)
        </label>
        <textarea
          placeholder='Paste Arweave JWK JSON here (e.g. {"kty":"RSA", ...})'
          value={jwkRaw}
          onChange={(e) => setJwkRaw(e.target.value)}
          rows={4}
          style={{
            width: '100%',
            padding: '0.5rem',
            fontFamily: 'monospace',
            fontSize: '0.8rem',
            borderRadius: 6,
            border: '1px solid #444',
            background: '#1a1a1a',
          }}
        />
        <div style={{ marginTop: '0.35rem', display: 'flex', gap: '0.5rem', flexWrap: 'wrap' }}>
          <button
            type="button"
            onClick={async () => {
              try {
                const jwk = await arweave.wallets.generate();
                setJwkRaw(JSON.stringify(jwk, null, 2));
              } catch (e) {
                setJwkError(e instanceof Error ? e.message : 'Generate failed');
              }
            }}
          >
            Generate test JWK
          </button>
        </div>
        {jwkError && <p style={{ color: '#e74c3c', fontSize: '0.9rem', marginTop: '0.25rem' }}>{jwkError}</p>}
        {turbo && !jwkError && <p style={{ color: '#2ecc71', fontSize: '0.9rem', marginTop: '0.25rem' }}>Ready to sign & upload</p>}
      </div>

      <div style={{ marginBottom: '1rem' }}>
        <label style={{ display: 'block', marginBottom: '0.35rem', fontWeight: 500 }}>
          Choose image (max 100KB)
        </label>
        <input
          type="file"
          accept={ACCEPT_IMAGE}
          onChange={handleFileChange}
          style={{ display: 'block' }}
        />
        {selectedFile && (
          <p style={{ color: '#888', fontSize: '0.9rem', marginTop: '0.35rem' }}>
            {selectedFile.name} — {(selectedFile.size / 1024).toFixed(1)} KB
          </p>
        )}
      </div>

      {/* Sign in browser (JWK) */}
      <button
        type="button"
        onClick={handleUpload}
        disabled={!selectedFile || !turbo || uploading}
        style={{ marginBottom: '1.5rem' }}
      >
        {uploading ? 'Uploading…' : 'Upload image (browser sign)'}
      </button>

      {/* Sign on hardware (QR) */}
      <hr style={{ borderColor: '#444', margin: '1.5rem 0' }} />
      <h3 style={{ marginBottom: '0.5rem' }}>Or sign on hardware</h3>
      <p style={{ color: '#888', fontSize: '0.9rem', marginBottom: '0.75rem' }}>
        1. Scan public key QR above. 2. Select image. 3. Show hash QR → sign on device. 4. Scan signature QR. 5. Upload.
      </p>
      {!scannedPublicKey && (
        <p style={{ color: '#f0ad4e', fontSize: '0.9rem' }}>Scan your wallet’s public key QR above first.</p>
      )}
      <div style={{ display: 'flex', gap: '0.75rem', flexWrap: 'wrap', alignItems: 'flex-start', marginBottom: '0.75rem' }}>
        <button
          type="button"
          onClick={handleGenerateHashQR}
          disabled={!selectedFile || !scannedPublicKey}
        >
          Show hash QR
        </button>
        {hardwareHashQR && (
          <div>
            <p style={{ fontSize: '0.85rem', marginBottom: '0.25rem' }}>Scan this on your device to sign:</p>
            <img src={hardwareHashQR} alt="Hash QR" style={{ display: 'block', width: 200, height: 200 }} />
          </div>
        )}
      </div>
      {hardwareHashResult && (
        <div style={{ marginBottom: '0.75rem' }}>
          <p style={{ fontSize: '0.85rem', marginBottom: '0.25rem' }}>Then scan the signature QR from your device:</p>
          <ScanSignature onSignatureScanned={setSignatureB64} />
          {signatureB64 && <p style={{ color: '#2ecc71', fontSize: '0.85rem', marginTop: '0.25rem' }}>Signature received.</p>}
        </div>
      )}
      {hardwareHashResult && signatureB64 && (
        <button
          type="button"
          onClick={handleHardwareUpload}
          disabled={hardwareUploading}
          style={{ marginTop: '0.5rem' }}
        >
          {hardwareUploading ? 'Uploading…' : 'Upload (hardware-signed)'}
        </button>
      )}
      {hardwareError && <p style={{ color: '#e74c3c', fontSize: '0.9rem', marginTop: '0.5rem' }}>{hardwareError}</p>}
      {hardwareUploadResult && (
        <div style={{ padding: '1rem', background: '#1a1a1a', borderRadius: 8, marginTop: '0.75rem' }}>
          <h4 style={{ marginTop: 0 }}>Hardware upload result</h4>
          <pre style={{ margin: 0, fontSize: '0.85rem', overflow: 'auto', wordBreak: 'break-all' }}>
            {JSON.stringify(hardwareUploadResult, null, 2)}
          </pre>
          {hardwareUploadResult.id && (
            <p style={{ marginTop: '0.5rem', marginBottom: 0 }}>
              <a href={`https://arweave.net/${hardwareUploadResult.id}`} target="_blank" rel="noreferrer">View on Arweave</a>
            </p>
          )}
        </div>
      )}

      {uploadStatus && <p style={{ marginBottom: '0.5rem', wordBreak: 'break-word' }}>{uploadStatus}</p>}

      {uploadResult && (
        <div style={{ padding: '1rem', background: '#1a1a1a', borderRadius: 8, marginTop: '0.5rem' }}>
          <h3 style={{ marginTop: 0, marginBottom: '0.5rem' }}>Upload result</h3>
          <pre style={{ margin: 0, fontSize: '0.85rem', overflow: 'auto', wordBreak: 'break-all' }}>
            {JSON.stringify(uploadResult, null, 2)}
          </pre>
          {uploadResult.id && (
            <p style={{ marginTop: '0.5rem', marginBottom: 0 }}>
              <a href={`https://arweave.net/${uploadResult.id}`} target="_blank" rel="noreferrer">
                View on Arweave
              </a>
            </p>
          )}
        </div>
      )}
    </section>
  );
}
