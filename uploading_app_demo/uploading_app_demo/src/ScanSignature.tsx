import { Html5Qrcode } from 'html5-qrcode';
import { useCallback, useEffect, useRef, useState } from 'react';

const SCANNER_CONTAINER_ID = 'qr-reader-signature';

type ScanSignatureProps = {
  onSignatureScanned: (signatureB64: string) => void;
};

export function ScanSignature({ onSignatureScanned }: ScanSignatureProps) {
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
    if (scannerRef.current) await stopScanner();
    setScanning(true);
  }, [stopScanner]);

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
          let sig = decodedText.trim();
          try {
            const parsed = JSON.parse(decodedText) as { signature?: string; sig?: string };
            if (parsed.signature) sig = parsed.signature;
            else if (parsed.sig) sig = parsed.sig;
          } catch {
            // use raw as signature
          }
          onSignatureScanned(sig);
          stopScanner();
        },
        () => {}
      )
      .catch((err: unknown) => {
        setError(err instanceof Error ? err.message : 'Could not start camera');
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
  }, [scanning, stopScanner, onSignatureScanned]);

  useEffect(() => () => {
    if (scannerRef.current?.isScanning) {
      scannerRef.current.stop().catch(() => {});
      scannerRef.current?.clear();
      scannerRef.current = null;
    }
  }, []);

  return (
    <div style={{ marginTop: '0.5rem' }}>
      {!scanning && (
        <button type="button" onClick={startScanning}>
          Scan signature QR
        </button>
      )}
      {error && <p style={{ color: '#e74c3c', fontSize: '0.9rem', marginTop: '0.25rem' }}>{error}</p>}
      {scanning && (
        <div>
          <div id={SCANNER_CONTAINER_ID} style={{ maxWidth: 320, margin: '0.5rem auto 0' }} />
          <button type="button" onClick={stopScanner} style={{ marginTop: '0.5rem' }}>Cancel</button>
        </div>
      )}
    </div>
  );
}
