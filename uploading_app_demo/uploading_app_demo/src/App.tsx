import { useState } from 'react'
import './App.css'
import { ScanPublicKey } from './ScanPublicKey'
import { UploadImage } from './UploadImage'

function App() {
  const [scannedPublicKey, setScannedPublicKey] = useState<string | null>(null)

  return (
    <>
      <h1>Upload with wallet (QR)</h1>
      <p className="read-the-docs">
        Scan your wallet’s public key QR, then sign in browser (JWK) or on hardware (hash QR → sign → scan signature).
      </p>
      <ScanPublicKey onPublicKeyScanned={setScannedPublicKey} />
      <UploadImage scannedPublicKey={scannedPublicKey} />
    </>
  )
}

export default App
