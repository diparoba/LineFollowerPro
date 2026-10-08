import { ESPLoader, Transport } from 'esptool-js';

export const BT_NAME_TOKEN = new TextEncoder().encode("##BT_CAR_CUSTOM_NAME_TOKEN##");
export const BT_NAME_BUFFER_SIZE = 32;

let activeWebSerialPort = null;

export function isWebSerialSupported() {
  return typeof navigator !== 'undefined' && 'serial' in navigator;
}

export function getActiveWebSerialPort() {
  return activeWebSerialPort;
}

export function setActiveWebSerialPort(port) {
  activeWebSerialPort = port;
}

/**
 * Solicita permisos al navegador para conectarse a un puerto COM/USB mediante la Web Serial API.
 */
export async function requestWebSerialPort() {
  if (!isWebSerialSupported()) {
    throw new Error('Tu navegador no soporta la Web Serial API. Por favor utiliza Google Chrome, Microsoft Edge, Opera o Brave.');
  }
  try {
    const port = await navigator.serial.requestPort();
    activeWebSerialPort = port;
    return port;
  } catch (err) {
    if (err.name === 'NotFoundError') {
      throw new Error('No se seleccionó ningún puerto serial.');
    }
    throw err;
  }
}

/**
 * Retorna los puertos seriales previamente autorizados por el usuario en este sitio.
 */
export async function getGrantedWebSerialPorts() {
  if (!isWebSerialSupported()) return [];
  try {
    return await navigator.serial.getPorts();
  } catch {
    return [];
  }
}

/**
 * Parchea en memoria el binario del firmware del ESP32 con el nuevo nombre Bluetooth,
 * recalculando el byte de checksum XOR y el hash SHA-256 de validación.
 */
export async function patchEsp32Binary(inputBuffer, newName) {
  const sanitized = newName.trim();
  if (!sanitized) {
    throw new Error('El nombre Bluetooth no puede estar vacío.');
  }
  if (sanitized.length >= BT_NAME_BUFFER_SIZE) {
    throw new Error(`El nombre Bluetooth no puede exceder ${BT_NAME_BUFFER_SIZE - 1} caracteres.`);
  }

  const bytes = new Uint8Array(inputBuffer);
  let pos = -1;
  for (let i = 0; i <= bytes.length - BT_NAME_TOKEN.length; i++) {
    let match = true;
    for (let j = 0; j < BT_NAME_TOKEN.length; j++) {
      if (bytes[i + j] !== BT_NAME_TOKEN[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      pos = i;
      break;
    }
  }

  if (pos === -1) {
    throw new Error('No se encontró el token de reemplazo en el binario del ESP32.');
  }

  const patched = new Uint8Array(bytes);
  const oldChecksumIdx = patched.length - 33;
  let checksum = patched[oldChecksumIdx];

  const nameBytes = new TextEncoder().encode(sanitized);
  for (let i = 0; i < BT_NAME_BUFFER_SIZE; i++) {
    const oldB = patched[pos + i];
    const newB = i < nameBytes.length ? nameBytes[i] : 0;
    checksum ^= oldB ^ newB;
    patched[pos + i] = newB;
  }
  patched[oldChecksumIdx] = checksum;

  // Recalcular SHA-256 digest sobre todo el binario excepto los últimos 32 bytes
  const hashContent = patched.subarray(0, patched.length - 32);
  const hashBuffer = await crypto.subtle.digest('SHA-256', hashContent);
  const hashArray = new Uint8Array(hashBuffer);
  patched.set(hashArray, patched.length - 32);

  return patched;
}

/**
 * Diagnóstico de Salud ESP32 directamente en la Web mediante Web Serial API.
 */
export async function diagnoseEsp32Web({ port = null, baudRate = 115200, onLog = console.log } = {}) {
  let targetPort = port || activeWebSerialPort;
  if (!targetPort) {
    onLog('🔌 Solicitando permisos de acceso al puerto COM en el navegador...');
    targetPort = await requestWebSerialPort();
  }

  const transport = new Transport(targetPort);
  const terminal = {
    clean: () => onLog('---'),
    writeLine: (data) => onLog(data),
    write: (data) => onLog(data),
  };

  const baud = Number(baudRate) || 115200;
  const loader = new ESPLoader({
    transport,
    baudrate: baud,
    terminal,
  });

  onLog('==================================================');
  onLog('🩺 DIAGNÓSTICO WEB SERIAL ESP32 INICIADO');
  onLog(`⚡ Velocidad configurada: ${baud} baudios`);
  onLog('💡 Sugerencia: Si queda esperando conexión, mantén presionado el botón BOOT en la placa.');
  onLog('==================================================');

  try {
    const chipRom = await loader.main();
    onLog(`🔌 Conexión establecida con procesador: ${chipRom}`);

    // Leer ID de la memoria flash SPI
    const flashId = await loader.readFlashId();
    const mfg = flashId & 0xff;
    const devLo = (flashId >> 16) & 0xff;
    const devHi = (flashId >> 8) & 0xff;
    const devHex = devHi.toString(16).padStart(2, '0') + devLo.toString(16).padStart(2, '0');
    const flashSizeStr = loader.DETECTED_FLASH_SIZES[devLo] || 'Desconocido';

    onLog('--------------------------------------------------');
    onLog(`📊 Modelo de Chip: ${loader.chip ? loader.chip.CHIP_NAME : chipRom}`);
    onLog(`🏭 Fabricante Flash ID: 0x${mfg.toString(16).toUpperCase()}`);
    onLog(`💾 Dispositivo Flash ID: 0x${devHex.toUpperCase()}`);
    onLog(`📦 Capacidad Flash Detectada: ${flashSizeStr}`);
    onLog('--------------------------------------------------');

    const isFlashBad = (mfg === 0xff || mfg === 0x00 || devHex === 'ffff' || devHex === '0000');

    if (isFlashBad) {
      onLog('');
      onLog('❌ ERROR GRAVE DE HARDWARE: Memoria Flash SPI inaccesible (0xFF / 0xFFFF).');
      onLog('📌 CAUSAS IDENTIFICADAS:');
      onLog('  1. 🔌 Pines SPI (GPIO 6 al 11) conectados o en cortocircuito con algún módulo externo.');
      onLog('  2. ⚡ Caída severa de voltaje en la línea 3.3V (voltaje < 2.7V por puerto USB o regulador defectuoso).');
      onLog('  3. 🔥 Si la placa calienta intensamente al tacto: Daño físico irreversible por retorno inductivo de motores (Back-EMF) o inversión de polaridad.');
      onLog('⚠️ RECOMENDACIÓN: Si el ESP32 se calienta, desconéctalo inmediatamente de tu computadora para proteger el puerto USB.');

      await transport.disconnect().catch(() => {});
      return {
        success: false,
        message: 'Fallo crítico: Memoria flash no responde (0xFF / 0xFFFF). El chip puede estar dañado o los pines SPI en cortocircuito.',
      };
    }

    onLog('');
    onLog('✅ ESP32 SALUDABLE: El procesador y la memoria Flash SPI están respondiendo normalmente al 100%.');
    onLog('💡 Si tienes problemas con el Bluetooth tras subir el firmware:');
    onLog('  • Utiliza el botón "🧹 Formateo Total (Erase Flash)" para limpiar particiones y datos NVS corruptos.');
    onLog('  • Asegúrate de que la fuente de energía suministre al menos 500mA estables (el radio Bluetooth consume picos de corriente).');
    onLog('  • Pulsa una vez el botón EN (RST) del ESP32 tras el flasheo para reiniciar el stack de Bluetooth.');

    await transport.disconnect().catch(() => {});
    return {
      success: true,
      message: `ESP32 Saludable (${loader.chip ? loader.chip.CHIP_NAME : chipRom}, Flash ${flashSizeStr}).`,
    };
  } catch (err) {
    await transport.disconnect().catch(() => {});
    onLog('');
    onLog(`❌ Error en diagnóstico Web Serial: ${err.message || err}`);
    onLog('💡 Sugerencias de solución:');
    onLog('  • Mantén presionado el botón BOOT (IO0) en el ESP32 mientras intenta conectar.');
    onLog('  • Prueba con otro cable micro-USB (muchos cables solo son de carga y no transmiten datos).');
    onLog('  • Verifica que no haya otro programa usando el puerto serial.');
    return {
      success: false,
      message: `Fallo de conexión Web Serial: ${err.message || err}`,
    };
  }
}

/**
 * Formateo Total (Erase Flash / Rescate) en la Web mediante Web Serial API.
 */
export async function eraseFlashEsp32Web({ port = null, baudRate = 115200, onLog = console.log } = {}) {
  let targetPort = port || activeWebSerialPort;
  if (!targetPort) {
    onLog('🔌 Solicitando permisos de acceso al puerto COM en el navegador...');
    targetPort = await requestWebSerialPort();
  }

  const transport = new Transport(targetPort);
  const terminal = {
    clean: () => onLog('---'),
    writeLine: (data) => onLog(data),
    write: (data) => onLog(data),
  };

  const baud = Number(baudRate) || 115200;
  const loader = new ESPLoader({
    transport,
    baudrate: baud,
    terminal,
  });

  onLog('==================================================');
  onLog('🧹 INICIANDO FORMATEO TOTAL / RESCATE ESP32 (WEB SERIAL)');
  onLog(`⚡ Velocidad: ${baud} baudios`);
  onLog('⏳ Esto borrará todas las particiones, código previo y sectores NVS corruptos...');
  onLog('==================================================');

  try {
    await loader.main();
    onLog('🧹 Borrando memoria flash por completo (Erase Flash)... Espera unos segundos.');
    await loader.eraseFlash();
    onLog('');
    onLog('✅ Memoria Flash borrada por completo (100% limpia).');
    onLog('🚀 Ya puedes flashear el firmware del Carro Bluetooth; el microcontrolador arrancará con NVS totalmente limpio.');

    await transport.disconnect().catch(() => {});
    return {
      success: true,
      message: 'Memoria Flash borrada por completo (100% limpia). ESP32 listo para flasheo limpio.',
    };
  } catch (err) {
    await transport.disconnect().catch(() => {});
    onLog('');
    onLog(`❌ Error al formatear memoria Flash: ${err.message || err}`);
    onLog('💡 Si queda esperando en "Connecting...", mantén pulsado el botón BOOT en el ESP32.');
    return {
      success: false,
      message: `Error al borrar memoria flash: ${err.message || err}`,
    };
  }
}

/**
 * Flasheo del Firmware Carro Bluetooth directamente desde la Web mediante Web Serial API.
 */
export async function flashEsp32Web({ port = null, btName = 'Carro_BT_ESP32', baudRate = 460800, onLog = console.log } = {}) {
  const cleanName = btName.trim();
  if (!cleanName) {
    throw new Error('El nombre Bluetooth no puede estar vacío.');
  }

  let targetPort = port || activeWebSerialPort;
  if (!targetPort) {
    onLog('🔌 Solicitando permisos de acceso al puerto COM en el navegador...');
    targetPort = await requestWebSerialPort();
  }

  onLog('==================================================');
  onLog('🚀 INICIANDO FLASHEO CARRO BLUETOOTH ESP32 (WEB SERIAL)');
  onLog(`📡 Nombre Bluetooth: "${cleanName}"`);
  onLog(`⚡ Velocidad UART: ${baudRate} baudios`);
  onLog('==================================================');

  onLog('📥 Descargando binarios de firmware desde el servidor web...');
  const [bootloaderBuf, partitionsBuf, bootApp0Buf, firmwareBuf] = await Promise.all([
    fetch('/firmwares/esp32/bootloader.bin').then(r => {
      if (!r.ok) throw new Error('No se pudo descargar bootloader.bin');
      return r.arrayBuffer();
    }),
    fetch('/firmwares/esp32/partitions.bin').then(r => {
      if (!r.ok) throw new Error('No se pudo descargar partitions.bin');
      return r.arrayBuffer();
    }),
    fetch('/firmwares/esp32/boot_app0.bin').then(r => {
      if (!r.ok) throw new Error('No se pudo descargar boot_app0.bin');
      return r.arrayBuffer();
    }),
    fetch('/firmwares/esp32/firmware_esp32_bt.bin').then(r => {
      if (!r.ok) throw new Error('No se pudo descargar firmware_esp32_bt.bin');
      return r.arrayBuffer();
    }),
  ]);

  onLog('🧩 Parcheando nombre Bluetooth y recalculando SHA-256 en el navegador...');
  const patchedFirmware = await patchEsp32Binary(firmwareBuf, cleanName);

  const transport = new Transport(targetPort);
  const terminal = {
    clean: () => onLog('---'),
    writeLine: (data) => onLog(data),
    write: (data) => onLog(data),
  };

  const baud = Number(baudRate) || 460800;
  const loader = new ESPLoader({
    transport,
    baudrate: baud,
    terminal,
  });

  try {
    onLog('🔌 Conectando con el ESP32...');
    await loader.main();

    onLog('📦 Escribiendo particiones y firmware en memoria Flash...');
    await loader.writeFlash({
      fileArray: [
        { data: new Uint8Array(bootloaderBuf), address: 0x1000 },
        { data: new Uint8Array(partitionsBuf), address: 0x8000 },
        { data: new Uint8Array(bootApp0Buf), address: 0xe000 },
        { data: patchedFirmware, address: 0x10000 },
      ],
      flashMode: 'dio',
      flashFreq: '40m',
      flashSize: 'detect',
      eraseAll: false,
      compress: true,
      reportProgress: (fileIndex, written, total) => {
        const pct = Math.round((written / total) * 100);
        onLog(`  ↳ Archivo [${fileIndex + 1}/4]: ${pct}% grabado`);
      },
    });

    onLog('');
    onLog(`✅ Firmware Carro Bluetooth subido con éxito al ESP32 con nombre '${cleanName}'!`);
    onLog('💡 Si el Bluetooth no aparece de inmediato en tu celular, pulsa una vez el botón EN (RST) del ESP32.');

    await loader.after('hard_reset').catch(() => {});
    await transport.disconnect().catch(() => {});

    return {
      success: true,
      message: `Firmware Carro Bluetooth subido con éxito al ESP32 con nombre '${cleanName}'.`,
    };
  } catch (err) {
    await transport.disconnect().catch(() => {});
    onLog('');
    onLog(`❌ Error durante el flasheo: ${err.message || err}`);
    onLog('💡 Si queda en "Connecting...", mantén pulsado el botón BOOT en el ESP32.');
    return {
      success: false,
      message: `Fallo en el flasheo del ESP32: ${err.message || err}`,
    };
  }
}
