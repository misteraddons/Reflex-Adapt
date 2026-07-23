#include "auth_msc_assets.h"

#include <stddef.h>

namespace {
constexpr char kWebhidHtml[] = R"HTML(<!doctype html>
<meta charset="utf-8">
<title>Reflex Auth Setup</title>
<style>
body{font:16px/1.4 Segoe UI,Arial,sans-serif;margin:24px;max-width:820px}
h1{margin:0 0 12px}
code,pre{background:#f3f3f3;padding:2px 4px;border-radius:4px}
.card{border:1px solid #ccc;border-radius:10px;padding:16px;margin:14px 0}
button,input{font:inherit}
button{padding:8px 12px;margin-right:8px}
#log{white-space:pre-wrap;min-height:6em}
.row{display:flex;gap:10px;align-items:center;flex-wrap:wrap;margin:8px 0}
.label{width:96px;color:#555}
</style>
<h1>Reflex PS4 Auth</h1>
<div class="card">
<p>This drive is built into the firmware. Use the GP2040-CE split bundle format only: <code>key.pem</code> + <code>serial.txt</code> + <code>sig.bin</code>.</p>
<p>The <code>PS4AUTH</code> folder is for those split files, and this page uploads the same split bundle over WebHID after validating the CRT fields and logging checksums.</p>
</div>
<div class="card">
<p><button id="connect">Connect WebHID</button><span id="state">WebHID idle</span></p>
<p><strong>GP2040-CE split bundle</strong></p>
<div class="row"><span class="label">key.pem</span><input id="pemfile" type="file" accept=".pem,.key" style="flex:1"></div>
<div class="row"><span class="label">serial.txt</span><input id="serialfile" type="file" accept=".txt" style="flex:1"></div>
<div class="row"><span class="label">sig.bin</span><input id="sigfile" type="file" accept=".bin" style="flex:1"></div>
<p><button id="bundle" disabled>Upload Split Bundle</button> <button id="clear">Clear Stored Key</button></p>
<pre id="log"></pre>
</div>
<script>
const MAGIC=0xAD,REPORT_STATUS=0xE4,REPORT_WRITE=0xE5,REPORT_CLEAR=0xE6,KEY_TYPE=0x01,KEY_SIZE=1712,CHUNK=58,REPORT_BYTES=63,REFLEX_VENDOR_ID=0x16D0,REFLEX_PRODUCT_ID=0x1460;
const PS4_KEY_SERIAL_OFF=0,PS4_KEY_SERIAL_SZ=16,PS4_KEY_SIG_OFF=16,PS4_KEY_SIG_SZ=256,PS4_KEY_N_OFF=272,PS4_KEY_N_SZ=256,PS4_KEY_E_OFF=528,PS4_KEY_E_SZ=256,PS4_KEY_D_OFF=784,PS4_KEY_D_SZ=256,PS4_KEY_P_OFF=1040,PS4_KEY_P_SZ=128,PS4_KEY_Q_OFF=1168,PS4_KEY_Q_SZ=128,PS4_KEY_DP_OFF=1296,PS4_KEY_DP_SZ=128,PS4_KEY_DQ_OFF=1424,PS4_KEY_DQ_SZ=128,PS4_KEY_QP_OFF=1552,PS4_KEY_QP_SZ=128;
const STATUS_NONE=0,STATUS_IN_PROGRESS=1,STATUS_OK=2,STATUS_CLEARED=3,STATUS_INCOMPLETE=4,STATUS_INVALID_SERIAL=5,STATUS_INVALID_SIGNATURE=6,STATUS_INVALID_MODULUS=7,STATUS_INVALID_EXPONENT=8,STATUS_INVALID_PRIVATE=9,STATUS_INVALID_CRT=10;
let dev=null;
const logEl=document.getElementById('log');
const stateEl=document.getElementById('state');
const pemEl=document.getElementById('pemfile');
const serialEl=document.getElementById('serialfile');
const sigEl=document.getElementById('sigfile');
const bundleBtn=document.getElementById('bundle');
const log=m=>logEl.textContent+=m+"\n";
function formatCrc32(v){return `0x${(v>>>0).toString(16).toUpperCase().padStart(8,'0')}`;}
function bytesToHex(bytes){return Array.from(bytes,b=>b.toString(16).padStart(2,'0')).join('');}
function crc32(bytes){let crc=0xFFFFFFFF;for(const byte of bytes){crc^=byte;for(let bit=0;bit<8;bit++){crc=(crc>>>1)^((crc&1)?0xEDB88320:0);}}return (~crc)>>>0;}
async function sha256Hex(bytes){const digest=await crypto.subtle.digest('SHA-256',bytes);return bytesToHex(new Uint8Array(digest));}
function uploadStatusLabel(code){switch(code){case STATUS_NONE:return'No upload yet';case STATUS_IN_PROGRESS:return'Upload in progress';case STATUS_OK:return'Accepted';case STATUS_CLEARED:return'Cleared';case STATUS_INCOMPLETE:return'Incomplete upload';case STATUS_INVALID_SERIAL:return'Invalid serial';case STATUS_INVALID_SIGNATURE:return'Invalid signature';case STATUS_INVALID_MODULUS:return'Invalid modulus';case STATUS_INVALID_EXPONENT:return'Invalid exponent';case STATUS_INVALID_PRIVATE:return'Invalid private exponent';case STATUS_INVALID_CRT:return'Invalid CRT parameters';default:return`Unknown (${code})`;}}
function regionLabel(code){switch(code){case 1:return'meaningful';case 2:return'all zero';case 3:return'all FF';default:return'unknown';}}
function pemToDer(pem){const lines=pem.split(/\r?\n/);let base64='';let inKey=false;for(const line of lines){if(line.includes('BEGIN')){inKey=true;continue;}if(line.includes('END'))break;if(inKey)base64+=line.trim();}if(!base64)throw new Error('PEM payload is empty');const binary=atob(base64);const bytes=new Uint8Array(binary.length);for(let i=0;i<binary.length;i++)bytes[i]=binary.charCodeAt(i);return bytes;}
function readDerLength(bytes,offset){if(offset>=bytes.length)throw new Error('Unexpected ASN.1 length');let length=bytes[offset++];if((length&0x80)===0)return{length,offset};const count=length&0x7F;if(count===0||count>4)throw new Error(`Unsupported ASN.1 length width ${count}`);if(offset+count>bytes.length)throw new Error('ASN.1 length overflow');length=0;for(let i=0;i<count;i++)length=(length<<8)|bytes[offset++];return{length,offset};}
function readDerInteger(bytes,offset){if(bytes[offset++]!==0x02)throw new Error('Expected ASN.1 INTEGER');const info=readDerLength(bytes,offset);offset=info.offset;const end=offset+info.length;if(end>bytes.length)throw new Error('ASN.1 INTEGER overflow');let value=bytes.slice(offset,end);while(value.length>1&&value[0]===0x00)value=value.slice(1);return{value,offset:end};}
function parsePkcs1PrivateKey(pem){if(!pem.includes('-----BEGIN RSA PRIVATE KEY-----'))throw new Error('Expected PKCS#1 RSA PRIVATE KEY PEM');const der=pemToDer(pem);let offset=0;if(der[offset++]!==0x30)throw new Error('Expected ASN.1 SEQUENCE');const seq=readDerLength(der,offset);offset=seq.offset;const seqEnd=offset+seq.length;const version=readDerInteger(der,offset);offset=version.offset;const n=readDerInteger(der,offset);offset=n.offset;const e=readDerInteger(der,offset);offset=e.offset;const d=readDerInteger(der,offset);offset=d.offset;const p=readDerInteger(der,offset);offset=p.offset;const q=readDerInteger(der,offset);offset=q.offset;const dp=readDerInteger(der,offset);offset=dp.offset;const dq=readDerInteger(der,offset);offset=dq.offset;const qp=readDerInteger(der,offset);offset=qp.offset;if(offset!==seqEnd)throw new Error('Unexpected trailing RSA data');return{der,modulus:n.value,exponent:e.value,privateExponent:d.value,primeP:p.value,primeQ:q.value,exponentP:dp.value,exponentQ:dq.value,coefficient:qp.value};}
function bigIntFromBytes(bytes){return BigInt(`0x${bytes.length?bytesToHex(bytes):'0'}`);}
function fixedWidthInteger(bytes,size,label){if(bytes.length>size)throw new Error(`${label} is ${bytes.length} bytes, expected at most ${size}`);const out=new Uint8Array(size);out.set(bytes,size-bytes.length);return out;}
function normalizeSerialFile(bytes){const text=new TextDecoder().decode(bytes).trim();if(text.length!==PS4_KEY_SERIAL_SZ)throw new Error(`serial.txt must contain exactly ${PS4_KEY_SERIAL_SZ} characters`);return new TextEncoder().encode(text);}
function validateParsedPrivateKey(parsed){const n=bigIntFromBytes(parsed.modulus),e=bigIntFromBytes(parsed.exponent),d=bigIntFromBytes(parsed.privateExponent),p=bigIntFromBytes(parsed.primeP),q=bigIntFromBytes(parsed.primeQ),dp=bigIntFromBytes(parsed.exponentP),dq=bigIntFromBytes(parsed.exponentQ),qp=bigIntFromBytes(parsed.coefficient);if(e!==65537n)throw new Error(`RSA exponent must be 65537, got ${e.toString()}`);if((p*q)!==n)throw new Error('RSA CRT validation failed: p*q mismatch');if(d%(p-1n)!==dp)throw new Error('RSA CRT validation failed: dp mismatch');if(d%(q-1n)!==dq)throw new Error('RSA CRT validation failed: dq mismatch');if((q*qp)%p!==1n)throw new Error('RSA CRT validation failed: q^-1 mod p mismatch');}
function buildGp2040Blob(parsed,serialBytes,sigBytes){validateParsedPrivateKey(parsed);if(serialBytes.length!==PS4_KEY_SERIAL_SZ)throw new Error(`Serial must be ${PS4_KEY_SERIAL_SZ} bytes, got ${serialBytes.length}`);if(sigBytes.length!==PS4_KEY_SIG_SZ)throw new Error(`Signature must be ${PS4_KEY_SIG_SZ} bytes, got ${sigBytes.length}`);const blob=new Uint8Array(KEY_SIZE);blob.set(serialBytes,PS4_KEY_SERIAL_OFF);blob.set(sigBytes,PS4_KEY_SIG_OFF);blob.set(fixedWidthInteger(parsed.modulus,PS4_KEY_N_SZ,'RSA modulus'),PS4_KEY_N_OFF);blob.set(fixedWidthInteger(parsed.exponent,PS4_KEY_E_SZ,'RSA exponent'),PS4_KEY_E_OFF);blob.set(fixedWidthInteger(parsed.privateExponent,PS4_KEY_D_SZ,'RSA private exponent'),PS4_KEY_D_OFF);blob.set(fixedWidthInteger(parsed.primeP,PS4_KEY_P_SZ,'RSA prime p'),PS4_KEY_P_OFF);blob.set(fixedWidthInteger(parsed.primeQ,PS4_KEY_Q_SZ,'RSA prime q'),PS4_KEY_Q_OFF);blob.set(fixedWidthInteger(parsed.exponentP,PS4_KEY_DP_SZ,'RSA exponent dp'),PS4_KEY_DP_OFF);blob.set(fixedWidthInteger(parsed.exponentQ,PS4_KEY_DQ_SZ,'RSA exponent dq'),PS4_KEY_DQ_OFF);blob.set(fixedWidthInteger(parsed.coefficient,PS4_KEY_QP_SZ,'RSA coefficient'),PS4_KEY_QP_OFF);return blob;}
async function logChecksums(label,bytes){log(`${label} SHA-256: ${await sha256Hex(bytes)}`);log(`${label} CRC32: ${formatCrc32(crc32(bytes))}`);}
function hasFeatureReport(device,reportId){if(!device.collections)return false;for(const col of device.collections){if(!col.featureReports)continue;for(const report of col.featureReports){if(report.reportId===reportId)return true;}}return false;}
function isAuthCapableDevice(device){return hasFeatureReport(device,REPORT_STATUS)&&hasFeatureReport(device,REPORT_WRITE)&&hasFeatureReport(device,REPORT_CLEAR);}
function hidRank(device){return device.vendorId===REFLEX_VENDOR_ID&&device.productId===REFLEX_PRODUCT_ID?0:device.vendorId===REFLEX_VENDOR_ID?1:2;}
async function ensureDevice(){if(dev)return dev;if(!("hid" in navigator))throw new Error("WebHID is unavailable here.");const granted=(await navigator.hid.getDevices()).filter(d=>d.vendorId===REFLEX_VENDOR_ID).sort((a,b)=>hidRank(a)-hidRank(b));dev=granted.find(isAuthCapableDevice)||null;if(!dev){const picks=await navigator.hid.requestDevice({filters:[{vendorId:REFLEX_VENDOR_ID,productId:REFLEX_PRODUCT_ID},{vendorId:REFLEX_VENDOR_ID}]});dev=picks.sort((a,b)=>hidRank(a)-hidRank(b)).find(isAuthCapableDevice)||null;}if(!dev)throw new Error("Selected HID interface has no WebHID auth feature reports. Pick the Reflex WebHID interface.");if(!dev.opened)await dev.open();stateEl.textContent="Connected to "+(dev.productName||"device");return dev;}
async function sendFeature(reportId,payload){const d=await ensureDevice();const report=new Uint8Array(REPORT_BYTES);for(let i=0;i<payload.length&&i<REPORT_BYTES;i++)report[i]=payload[i];await d.sendFeatureReport(reportId,report);}
async function receiveFeature(reportId){const d=await ensureDevice();const report=await d.receiveFeatureReport(reportId);const raw=new Uint8Array(report.buffer);if(raw.length===0)return raw;if(raw[0]===MAGIC)return raw;if(raw[0]===reportId||(raw.length>1&&raw[1]===MAGIC))return raw.slice(1);return raw;}
async function readStatus(){const data=await receiveFeature(REPORT_STATUS);if(data[0]!==MAGIC)throw new Error(`Unexpected key status report: ${Array.from(data.slice(0,8),b=>b.toString(16).padStart(2,'0')).join(' ')||'empty'}`);const loaded=(data[1]&0x01)!==0;const uploadStatus=data[6]||0;const uploadCrc=(data[8]||0)|((data[9]||0)<<8)|((data[10]||0)<<16)|((data[11]||0)<<24);const serialRegion=data[12]||0;const chunks=data[13]||0,totalChunks=data[14]||0;const missing=(data[16]||0)|((data[17]||0)<<8);log(`PS4 key present: ${loaded?'yes':'no'}`);log(`Last upload: ${uploadStatusLabel(uploadStatus)} (${formatCrc32(uploadCrc)})`);log(`Device diag: serial ${regionLabel(serialRegion)}, chunks ${chunks}/${totalChunks}, first missing ${missing===0xFFFF?'none':missing}`);return{loaded,uploadStatus,uploadCrc};}
async function uploadKey(bytes,label){const expectedCrc=crc32(bytes);for(let off=0;off<bytes.length;off+=CHUNK){const len=Math.min(CHUNK,bytes.length-off);const payload=new Uint8Array(REPORT_BYTES);payload[0]=MAGIC;payload[1]=KEY_TYPE;payload[2]=off&0xFF;payload[3]=(off>>8)&0xFF;payload[4]=len;payload.set(bytes.slice(off,off+len),5);await sendFeature(REPORT_WRITE,payload);}const status=await readStatus();if(!status||status.uploadStatus!==STATUS_OK)throw new Error(`${label} rejected by device: ${status?uploadStatusLabel(status.uploadStatus):'no status'}`);if(status.uploadCrc!==expectedCrc)throw new Error(`${label} CRC mismatch: page=${formatCrc32(expectedCrc)} device=${formatCrc32(status.uploadCrc)}`);if(!status.loaded)throw new Error(`${label} uploaded but device still reports not loaded`);log(`${label} uploaded successfully`);}
document.getElementById('connect').onclick=async()=>{try{await readStatus();}catch(err){log(err.message);}};
function updateBundleButton(){bundleBtn.disabled=!(pemEl.files.length&&serialEl.files.length&&sigEl.files.length);}
pemEl.onchange=updateBundleButton;serialEl.onchange=updateBundleButton;sigEl.onchange=updateBundleButton;
bundleBtn.onclick=async()=>{try{const pemText=await pemEl.files[0].text();const parsed=parsePkcs1PrivateKey(pemText);const serialBytes=normalizeSerialFile(new Uint8Array(await serialEl.files[0].arrayBuffer()));const sigBytes=new Uint8Array(await sigEl.files[0].arrayBuffer());const blob=buildGp2040Blob(parsed,serialBytes,sigBytes);await logChecksums('key.pem',new TextEncoder().encode(pemText));await logChecksums('serial.txt',serialBytes);await logChecksums('sig.bin',sigBytes);await logChecksums('Packed PS4 blob',blob);await uploadKey(blob,'GP2040 split bundle');pemEl.value='';serialEl.value='';sigEl.value='';bundleBtn.disabled=true;}catch(err){log(err.message);}};
document.getElementById('clear').onclick=async()=>{try{await sendFeature(REPORT_CLEAR,Uint8Array.of(MAGIC,KEY_TYPE));log("Stored key cleared.");await readStatus();}catch(err){log(err.message);}};
</script>
)HTML";

constexpr char kOledHtml[] = R"HTML(<!doctype html>
<meta charset="utf-8">
<title>Reflex OLED Viewer</title>
<style>
:root{color-scheme:dark;--bg:#101010;--fg:#f5f5f5;--muted:#9a9a9a;--line:#333;--accent:#7fd7ff}
*{box-sizing:border-box}
body{margin:18px;background:var(--bg);color:var(--fg);font:15px/1.4 Segoe UI,Arial,sans-serif}
h1{font-size:20px;margin:0 0 10px}
.row{display:flex;gap:10px;align-items:center;flex-wrap:wrap;margin:10px 0}
button,input{font:inherit}
button{background:#202020;color:var(--fg);border:1px solid var(--line);border-radius:8px;padding:8px 12px}
button:hover{border-color:var(--accent)}
input{width:72px;background:#050505;color:var(--fg);border:1px solid var(--line);border-radius:6px;padding:6px}
#wrap{display:inline-block;background:#000;border:1px solid #444;padding:10px}
canvas{display:block;image-rendering:pixelated;background:#000}
#status{color:var(--muted);white-space:pre-wrap;max-width:720px}
.pad{display:grid;grid-template-columns:repeat(3,58px);gap:6px;align-items:center}
.pad button{min-width:58px}
.wide{grid-column:span 3}
</style>
<h1>Reflex OLED Viewer</h1>
<div class="row">
  <button id="connect">Connect Reflex Adapt Control</button>
  <button id="stream" disabled>Start Stream</button>
  <button id="shot" disabled>Save PNG</button>
  <button id="pop" disabled>Pop Out Preview</button>
  <label>Rate <input id="rate" type="number" min="1" max="60" value="60"> Hz</label>
</div>
<div class="row">
  <div class="pad">
    <button data-ui="MENU" class="wide">Menu</button>
    <span></span><button data-ui="UP">Up</button><span></span>
    <button data-ui="LEFT">Left</button><button data-ui="OK">OK</button><button data-ui="RIGHT">Right</button>
    <span></span><button data-ui="DOWN">Down</button><span></span>
    <button data-ui="BACK">Back</button><button data-ui="RESET" style="grid-column:span 2">Reset</button>
  </div>
</div>
<div id="wrap"><canvas id="oled" width="640" height="320"></canvas></div>
<p id="status">Open this page in Chrome or Edge while the device is in DInput management mode. If multiple ports appear, choose Reflex Adapt Control.</p>
<script>
const W=128,H=64,LEN=1024,SCALE=5,REFLEX_VID=0x16D0,REFLEX_PID=0x1460;
const canvas=document.getElementById('oled'),ctx=canvas.getContext('2d');
const statusEl=document.getElementById('status'),rateEl=document.getElementById('rate');
const connectBtn=document.getElementById('connect'),streamBtn=document.getElementById('stream'),shotBtn=document.getElementById('shot'),popBtn=document.getElementById('pop');
canvas.width=W*SCALE;canvas.height=H*SCALE;ctx.imageSmoothingEnabled=false;
let port,reader,writer,rx=[],need=0,lastSeq=0,frameCount=0,running=false,streaming=false,lastFrame=null,previewWin=null,lastSerialLine='',probeTimer=null,pendingAutoPort=null;
function status(s){statusEl.textContent=s}
function clear(){ctx.fillStyle='#000';ctx.fillRect(0,0,canvas.width,canvas.height)}
clear();
function ascii(bytes){return new TextDecoder().decode(new Uint8Array(bytes))}
function findLf(){for(let i=0;i<rx.length;i++)if(rx[i]===10)return i;return -1}
function framePixels(frame){let n=0;for(const b of frame){let v=b;while(v){v&=v-1;n++;}}return n}
function noteNonOledLine(line){
  if(!line)return;
  lastSerialLine=line;
  if(frameCount===0&&line.startsWith('[DC]')){
    status('This is the Dreamcast debug serial stream, not the OLED control port. Disconnect and choose Reflex Adapt Control.');
  }
}
function armProbeTimeout(){
  if(probeTimer)clearTimeout(probeTimer);
  probeTimer=setTimeout(()=>{
    if(frameCount===0){
      const hint=lastSerialLine?` Last serial line: ${lastSerialLine}`:'';
      status(`No OLED frames received yet. Disconnect and choose Reflex Adapt Control if another Reflex serial port is listed.${hint}`);
    }
  },1500);
}
function portInfo(p){return p&&typeof p.getInfo==='function'?p.getInfo():{}}
function isReflexPort(p){const i=portInfo(p);return i.usbVendorId===REFLEX_VID}
function portRank(p){const i=portInfo(p);return i.usbVendorId===REFLEX_VID&&i.usbProductId===REFLEX_PID?0:i.usbVendorId===REFLEX_VID?1:2}
async function chooseReflexPort(promptUser){
  const granted=await navigator.serial.getPorts();
  const ports=granted.filter(isReflexPort).sort((a,b)=>portRank(a)-portRank(b));
  if(ports.length)return ports[0];
  if(!promptUser)return null;
  return navigator.serial.requestPort({filters:[{usbVendorId:REFLEX_VID,usbProductId:REFLEX_PID},{usbVendorId:REFLEX_VID}]});
}
function openPreview(){
  previewWin=window.open('','reflex-oled-preview','popup=yes,width=640,height=320,menubar=no,toolbar=no,location=no,status=no,scrollbars=no,resizable=yes');
  if(!previewWin){status('Pop-up blocked. Allow pop-ups for this page.');return;}
  previewWin.document.open();
  previewWin.document.write(`<!doctype html><meta charset="utf-8"><title>Reflex OLED Preview</title><style>html,body{margin:0;width:100%;height:100%;overflow:hidden;background:#000}canvas{display:block;width:100vw;height:100vh;image-rendering:pixelated;background:#000}</style><canvas id="oledPop" width="640" height="320"></canvas>`);
  previewWin.document.close();
  if(lastFrame)draw(lastFrame);
}
function draw(frame){
  lastFrame=Array.from(frame);
  frameCount++;
  const img=ctx.createImageData(W,H);
  for(let page=0;page<8;page++)for(let x=0;x<W;x++){
    const b=frame[page*W+x];
    for(let bit=0;bit<8;bit++){
      const y=page*8+bit,p=(y*W+x)*4,on=(b>>bit)&1,v=on?255:0;
      img.data[p]=v;img.data[p+1]=v;img.data[p+2]=v;img.data[p+3]=255;
    }
  }
  const tmp=document.createElement('canvas');tmp.width=W;tmp.height=H;
  tmp.getContext('2d').putImageData(img,0,0);
  ctx.drawImage(tmp,0,0,canvas.width,canvas.height);
  if(previewWin&&!previewWin.closed){
    const pop=previewWin.document.getElementById('oledPop');
    if(pop){
      const popCtx=pop.getContext('2d');
      popCtx.imageSmoothingEnabled=false;
      popCtx.drawImage(tmp,0,0,pop.width,pop.height);
    }
  }
  const pixels=framePixels(frame);
  status(`${streaming?'Streaming':'Connected'}. Frames: ${lastSeq} Pixels: ${pixels}${pixels===0?' (blank mirror)':''}`);
}
function pump(){
  for(;;){
    if(need){
      if(rx.length<need+1)return;
      const frame=rx.splice(0,need);
      if(rx[0]===10)rx.shift();
      need=0;draw(frame);
      continue;
    }
    const lf=findLf();if(lf<0)return;
    const line=ascii(rx.splice(0,lf+1)).trim();
    if(!line.startsWith('OLEDRAW')){noteNonOledLine(line);continue;}
    const m=line.match(/SEQ=(\d+).*LEN=(\d+)/);
    if(!m)continue;
    lastSeq=Number(m[1]);need=Number(m[2]);
    if(need!==LEN){status(`Unexpected OLED frame length ${need}`);need=0;}
  }
}
async function send(line){
  if(!writer)return;
  await writer.write(new TextEncoder().encode(line+'\n'));
}
connectBtn.onclick=async()=>{
  try{
    if(!('serial' in navigator))throw new Error('Web Serial is not available. Use Chrome/Edge, or run the Python viewer.');
    if(!port){
      port=pendingAutoPort||await chooseReflexPort(true);
      pendingAutoPort=null;
      if(!port)return;
      await port.open({baudRate:115200});
      writer=port.writable.getWriter();
      reader=port.readable.getReader();
      frameCount=0;lastSerialLine='';rx=[];need=0;
      running=true;streamBtn.disabled=false;shotBtn.disabled=false;popBtn.disabled=false;connectBtn.textContent='Disconnect';
      await send('OVERLAY OFF');
      await send(`OLED RATE ${Math.max(1,Math.min(60,Number(rateEl.value)||60))}`);
      await send('OLED FRAME');
      status('Connected. Waiting for OLED frames...');
      armProbeTimeout();
      while(running){
        const {value,done}=await reader.read();
        if(done)break;
        if(value){for(const b of value)rx.push(b);pump();}
      }
    }else{
      running=false;await send('OLED OFF');
      if(probeTimer){clearTimeout(probeTimer);probeTimer=null;}
      reader.releaseLock();writer.releaseLock();await port.close();
      streaming=false;
      port=null;reader=null;writer=null;streamBtn.disabled=true;shotBtn.disabled=true;popBtn.disabled=true;connectBtn.textContent='Connect Reflex Adapt Control';streamBtn.textContent='Start Stream';
      status('Disconnected.');
    }
  }catch(e){status(e.message);}
};
streamBtn.onclick=async()=>{
  try{
    streaming=!streaming;
    streamBtn.textContent=streaming?'Stop Stream':'Start Stream';
    await send(`OLED RATE ${Math.max(1,Math.min(60,Number(rateEl.value)||60))}`);
    await send(streaming?'OLED ON':'OLED OFF');
    if(!streaming)await send('OLED FRAME');
  }catch(e){status(e.message);}
};
rateEl.onchange=()=>send(`OLED RATE ${Math.max(1,Math.min(60,Number(rateEl.value)||60))}`).catch(()=>{});
shotBtn.onclick=()=>{
  const a=document.createElement('a');
  a.download=`reflex-oled-${Date.now()}.png`;
  a.href=canvas.toDataURL('image/png');
  a.click();
};
popBtn.onclick=openPreview;
document.querySelectorAll('[data-ui]').forEach(button=>{
  button.onclick=async()=>{
    const action=button.dataset.ui;
    if(action==='RESET'&&!confirm('Reboot Reflex Adapt?'))return;
    try{
      await send(`UI ${action}`);
      if(!streaming)setTimeout(()=>send('OLED FRAME').catch(()=>{}),80);
    }catch(e){status(e.message);}
  };
});
window.addEventListener('load',async()=>{
  if(!('serial' in navigator))return;
  try{
    const p=await chooseReflexPort(false);
    if(p&&!port){
      pendingAutoPort=p;
      status('Found approved Reflex Adapt Control serial. Connecting...');
      connectBtn.click();
    }
  }catch(e){}
});
</script>
)HTML";

constexpr char kReadmeText[] =
  "Reflex Adapt Utility Drive\r\n"
  "==========================\r\n"
  "\r\n"
  "This removable drive is available in DInput management mode.\r\n"
  "\r\n"
  "Files:\r\n"
  "  PS4AUTH.HTM - PS4 key setup page\r\n"
  "  OLED.HTM    - browser OLED capture viewer over Web Serial\r\n"
  "  DEVICE.TXT  - device and PS4 key/import status\r\n"
  "  ADAPTDL.INI - Reflex Adapt Manager Downloader entry\r\n"
  "  PS4AUTH\\    - drop key.pem + serial.txt + sig.bin here\r\n"
  "\r\n"
  "MiSTer setup:\r\n"
  "  1. Copy ADAPTDL.INI to /media/fat/reflex_adapt_downloader.ini.\r\n"
  "  2. Run MiSTer Downloader.\r\n"
  "  3. Run Scripts/reflex_adapt_manager.sh.\r\n"
  "\r\n";

constexpr char kDownloaderIniText[] =
  "[misteraddons/reflex-adapt-manager]\r\n"
  "db_url = https://github.com/misteraddons/Reflex-Adapt/raw/main/reflex-adapt-manager.json.zip\r\n";


}  // namespace

const char* auth_msc_webhid_html() {
  return kWebhidHtml;
}

size_t auth_msc_webhid_html_len() {
  return sizeof(kWebhidHtml) - 1u;
}

const char* auth_msc_oled_html() {
  return kOledHtml;
}

size_t auth_msc_oled_html_len() {
  return sizeof(kOledHtml) - 1u;
}

const char* auth_msc_readme_text() {
  return kReadmeText;
}

size_t auth_msc_readme_text_len() {
  return sizeof(kReadmeText) - 1u;
}

const char* auth_msc_downloader_ini_text() {
  return kDownloaderIniText;
}

size_t auth_msc_downloader_ini_text_len() {
  return sizeof(kDownloaderIniText) - 1u;
}
