import { Html5Qrcode } from 'html5-qrcode';
import { useCallback, useEffect, useRef, useState } from 'react';

const SCANNER_CONTAINER_ID = 'qr-reader-public-key';

export function ScanPublicKey() {
  const [publicKey, setPublicKey] = useState<string | null>(null);
  const [scanning, setScanning] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const scannerRef = useRef<Html5Qrcode | null>(null);

  const stopScanner = useCallback(async () => {
    if (scannerRef.current?.isScanning) {
      try {
        await scannerRef.current.stop();
      } catch {
        // ignore
      }
      scannerRef.current.clear();
      scannerRef.current = null;
    }
    setScanning(false);
  }, []);

  const startScanning = useCallback(async () => {
    setError(null);
    setPublicKey(null);
    if (scannerRef.current) {
      await stopScanner();
    }
    setScanning(true);
  }, [stopScanner]);

  // Start the scanner only after the container is in the DOM (when scanning becomes true)
  useEffect(() => {
    if (!scanning) return;

    const container = document.getElementById(SCANNER_CONTAINER_ID);
    if (!container) return;

    container.innerHTML = '';
    const html5QrCode = new Html5Qrcode(SCANNER_CONTAINER_ID);
    scannerRef.current = html5QrCode;

    html5QrCode
      .start(
        { facingMode: 'environment' },
        { fps: 10, qrbox: { width: 260, height: 260 } },
        (decodedText) => {
          let key = decodedText.trim();
          try {
            const parsed = JSON.parse(decodedText) as { publicKey?: string };
            if (parsed.publicKey) key = parsed.publicKey;
          } catch {
            // use raw decoded text as public key
          }
          setPublicKey(key);
          stopScanner();
        },
        () => {}
      )
      .catch((err: unknown) => {
        const message = err instanceof Error ? err.message : 'Could not start camera';
        setError(message);
        scannerRef.current = null;
        setScanning(false);
      });

    return () => {
      if (scannerRef.current?.isScanning) {
        scannerRef.current.stop().then(() => {
          scannerRef.current?.clear();
          scannerRef.current = null;
        });
      }
    };
  }, [scanning, stopScanner]);

  useEffect(() => {
    return () => {
      if (scannerRef.current?.isScanning) {
        scannerRef.current.stop().catch(() => {});
        scannerRef.current?.clear();
        scannerRef.current = null;
      }
    };
  }, []);

  return (
    <section className="scan-public-key" style={{ marginTop: '2rem' }}>
      <h2>Public key from wallet</h2>
      <p style={{ color: '#888', marginBottom: '1rem' }}>
        Show the public key QR from your hardware wallet. We’ll scan it and display the key here.
      </p>

      {!scanning && !publicKey && (
        <button type="button" onClick={startScanning}>
          Scan public key QR
        </button>
      )}

      {error && (
        <p className="scan-error" style={{ color: '#e74c3c', marginTop: '1rem' }}>
          {error}
        </p>
      )}

      {scanning && (
        <div style={{ marginTop: '1rem' }}>
          <div id={SCANNER_CONTAINER_ID} style={{ maxWidth: 320, margin: '0 auto' }} />
          <button type="button" onClick={stopScanner} style={{ marginTop: '0.5rem' }}>
            Cancel
          </button>
        </div>
      )}

      {publicKey && (
        <div className="public-key-result" style={{ marginTop: '1.5rem', textAlign: 'left', maxWidth: 480, marginLeft: 'auto', marginRight: 'auto' }}>
          <h3 style={{ marginBottom: '0.5rem' }}>Scanned public key</h3>
          <pre
            style={{
              padding: '1rem',
              background: '#1a1a1a',
              borderRadius: 8,
              overflow: 'auto',
              fontSize: '0.85rem',
              wordBreak: 'break-all',
            }}
          >
            {publicKey}
          </pre>
          <button type="button" onClick={startScanning} style={{ marginTop: '0.5rem' }}>
            Scan again
          </button>
        </div>
      )}
    </section>
  );
}
