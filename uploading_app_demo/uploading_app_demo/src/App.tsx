import './App.css'
import { ScanPublicKey } from './ScanPublicKey'
import { UploadImage } from './UploadImage'

function App() {
  return (
    <>
      <h1>Upload with wallet (QR)</h1>
      <p className="read-the-docs">
        Scan your wallet’s public key QR, then sign with a JWK and upload an image (under 100KB, free).
      </p>
      <ScanPublicKey />
      <UploadImage />
    </>
  )
}

export default App
