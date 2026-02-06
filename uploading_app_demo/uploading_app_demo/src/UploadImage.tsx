import {
  ArweaveSigner,
  TurboAuthenticatedClient,
  TurboFactory,
} from '@ardrive/turbo-sdk/web';
import Arweave from 'arweave';
import type { JWKInterface } from 'arweave/node/lib/wallet';
import { useCallback, useEffect, useState } from 'react';

const arweave = new Arweave({ host: 'arweave.net', port: 443, protocol: 'https' });

const MAX_IMAGE_BYTES = 100 * 1024; // 100KB
const ACCEPT_IMAGE = 'image/jpeg,image/png,image/gif,image/webp';

type UploadResult = {
  id: string;
  owner?: string;
  [key: string]: unknown;
} | null;

export function UploadImage() {
  const [jwkRaw, setJwkRaw] = useState('');
  const [turbo, setTurbo] = useState<TurboAuthenticatedClient | null>(null);
  const [jwkError, setJwkError] = useState<string | null>(null);
  const [selectedFile, setSelectedFile] = useState<File | null>(null);
  const [uploadStatus, setUploadStatus] = useState('');
  const [uploadResult, setUploadResult] = useState<UploadResult>(null);
  const [uploading, setUploading] = useState(false);

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

  return (
    <section className="upload-image" style={{ marginTop: '2rem', textAlign: 'left', maxWidth: 520, marginLeft: 'auto', marginRight: 'auto' }}>
      <h2>Upload image (under 100KB)</h2>

      <div style={{ marginBottom: '1rem' }}>
        <label style={{ display: 'block', marginBottom: '0.35rem', fontWeight: 500 }}>
          JWK (to sign in browser for now)
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

      <button
        type="button"
        onClick={handleUpload}
        disabled={!selectedFile || !turbo || uploading}
        style={{ marginBottom: '1rem' }}
      >
        {uploading ? 'Uploading…' : 'Upload image'}
      </button>

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
