import { invoke } from '@tauri-apps/api/core';
import { listen } from '@tauri-apps/api/event';
import {
  isWebSerialSupported,
  requestWebSerialPort,
  getGrantedWebSerialPorts,
  diagnoseEsp32Web,
  eraseFlashEsp32Web,
  flashEsp32Web,
  getActiveWebSerialPort,
  setActiveWebSerialPort
} from './esp32WebFlasher.js';

export {
  isWebSerialSupported,
  requestWebSerialPort,
  getGrantedWebSerialPorts,
  getActiveWebSerialPort,
  setActiveWebSerialPort
};

let webFlashLogSubscribers = [];

export function emitWebFlashLog(msg) {
  for (const sub of webFlashLogSubscribers) {
    try { sub(msg); } catch (e) { console.error(e); }
  }
}

const isBrowser = typeof window !== 'undefined';
const HTTP_API_URL = isBrowser ? '' : 'http://localhost:5010';
const getWsUrl = () => {
  if (isBrowser) {
    const proto = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    return `${proto}//${window.location.host}/ws/telemetry`;
  }
  return 'ws://localhost:5010/ws/telemetry';
};
const WS_URL = isBrowser ? getWsUrl() : 'ws://localhost:5010/ws/telemetry';

export function isTauri() {
  return typeof window !== 'undefined' && 
    (window.__TAURI_INTERNALS__ !== undefined || window.__TAURI__ !== undefined);
}

export function isRemoteWebHost() {
  if (typeof window === 'undefined') return false;
  if (isTauri()) return false;
  const host = window.location.hostname;
  return host !== 'localhost' && host !== '127.0.0.1' && host !== '0.0.0.0' && host !== '';
}

let reconnectWsManualFn = null;
export function reconnectTelemetry() {
  if (reconnectWsManualFn) {
    reconnectWsManualFn();
  }
}

// Escanear puertos seriales
export async function getPorts() {
  if (isTauri()) {
    try {
      return await invoke('get_ports');
    } catch (e) {
      console.error('Error invocando get_ports en Tauri:', e);
      return { ports: [], connected: false, currentPort: '' };
    }
  } else {
    const webPorts = [];
    if (isWebSerialSupported()) {
      try {
        const granted = await getGrantedWebSerialPorts();
        if (granted.length > 0) {
          webPorts.push('USB Serial (Web Serial Autorizado)');
        }
      } catch {}
    }
    try {
      const res = await fetch(`${HTTP_API_URL}/api/ports`);
      const data = await res.json();
      return {
        ...data,
        ports: Array.from(new Set([...(data.ports || []), ...webPorts])),
      };
    } catch (e) {
      return { ports: webPorts, connected: false, currentPort: '' };
    }
  }
}

// Conectar puerto serial
export async function connectSerial(port, baudRate = 115200) {
  if (isTauri()) {
    const success = await invoke('connect_serial', { port, baudRate });
    return { success, port };
  } else {
    const res = await fetch(`${HTTP_API_URL}/api/connect`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ port, baudRate })
    });
    return await res.json();
  }
}

// Desconectar puerto serial
export async function disconnectSerial() {
  if (isTauri()) {
    const success = await invoke('disconnect_serial');
    return { success };
  } else {
    const res = await fetch(`${HTTP_API_URL}/api/disconnect`, { method: 'POST' });
    return await res.json();
  }
}

// Enviar parámetros de control PID
export async function sendPid(config) {
  if (isTauri()) {
    const success = await invoke('send_pid', {
      kp: Number(config.kp),
      kd: Number(config.kd),
      baseSpeed: Number(config.baseSpeed),
      maxSpeed: Number(config.maxSpeed),
      brakeSpeed: Number(config.brakeSpeed),
      forkMode: Number(config.forkMode),
      lineColor: Number(config.lineColor),
      reflective: Number(config.reflectiveMode || 0)
    });
    return { success };
  } else {
    const res = await fetch(`${HTTP_API_URL}/api/pid`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        kp: config.kp,
        kd: config.kd,
        baseSpeed: config.baseSpeed,
        maxSpeed: config.maxSpeed,
        brakeSpeed: config.brakeSpeed,
        forkMode: config.forkMode,
        lineColor: config.lineColor,
        reflectiveMode: Number(config.reflectiveMode || 0)
      })
    });
    return await res.json();
  }
}

// Enviar comando simple ($CMD,action)
export async function sendCommand(command) {
  if (isTauri()) {
    const success = await invoke('send_command', { command });
    return { success };
  } else {
    const res = await fetch(`${HTTP_API_URL}/api/command`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ command })
    });
    return await res.json();
  }
}

// Guardar en EEPROM del robot
export async function saveEeprom() {
  if (isTauri()) {
    return await invoke('save_eeprom');
  } else {
    const res = await fetch(`${HTTP_API_URL}/api/eeprom/save`, { method: 'POST' });
    return await res.json();
  }
}

// Leer de EEPROM del robot
export async function readEeprom() {
  if (isTauri()) {
    return await invoke('read_eeprom');
  } else {
    const res = await fetch(`${HTTP_API_URL}/api/eeprom/read`);
    return await res.json();
  }
}

// CRUD de Perfiles SQLite
export async function getProfiles(category = null, carName = null) {
  if (isTauri()) {
    return await invoke('get_profiles', {
      category: category || null,
      carName: carName || null
    });
  } else {
    let url = `${HTTP_API_URL}/api/profiles`;
    const params = new URLSearchParams();
    if (category) params.append('category', category);
    if (carName) params.append('carName', carName);
    if (params.toString()) url += `?${params.toString()}`;
    const res = await fetch(url);
    return await res.json();
  }
}

export async function saveProfile(profile) {
  if (isTauri()) {
    return await invoke('save_profile', {
      profile: {
        id: profile.id || null,
        name: profile.name,
        carName: profile.carName || profile.car_name || 'Carro 1',
        carCategory: profile.carCategory || profile.car_category || 'IM_16',
        kp: Number(profile.kp),
        kd: Number(profile.kd),
        baseSpeed: Number(profile.baseSpeed),
        maxSpeed: Number(profile.maxSpeed),
        brakeSpeed: Number(profile.brakeSpeed),
        forkMode: Number(profile.forkMode),
        lineColor: Number(profile.lineColor),
        reflectiveMode: Number(profile.reflectiveMode || 0),
        createdAt: profile.createdAt || null
      }
    });
  } else {
    const res = await fetch(`${HTTP_API_URL}/api/profiles`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(profile)
    });
    return await res.json();
  }
}

export async function deleteProfile(id) {
  if (isTauri()) {
    const success = await invoke('delete_profile', { id: Number(id) });
    return { success };
  } else {
    const res = await fetch(`${HTTP_API_URL}/api/profiles/${id}`, { method: 'DELETE' });
    return await res.json();
  }
}

// Crear copia de respaldo de la base de datos SQLite en la flash USB
export async function backupDatabase(filename) {
  if (isTauri()) {
    try {
      const savedName = await invoke('backup_database', { filename: filename || null });
      return { success: true, filename: savedName };
    } catch (e) {
      return { success: false, message: e.toString() };
    }
  } else {
    return { success: false, message: 'El respaldo local solo está disponible en modo de escritorio nativo.' };
  }
}

// Suscripción a Telemetría en Vivo con Retroceso Exponencial y Estado Offline
export function subscribeTelemetry(onTelemetry, onLog, onStatusChange) {
  if (isTauri()) {
    let unlistenTel = null;
    let unlistenLog = null;

    listen('telemetry', (event) => {
      if (onTelemetry) onTelemetry(event.payload);
    }).then((unlisten) => {
      unlistenTel = unlisten;
    });

    listen('log', (event) => {
      if (onLog) onLog(event.payload);
    }).then((unlisten) => {
      unlistenLog = unlisten;
    });

    if (onStatusChange) onStatusChange({ connected: true, offline: false, retries: 0 });

    return () => {
      if (unlistenTel) unlistenTel();
      if (unlistenLog) unlistenLog();
    };
  } else {
    // Modo Web con WebSocket Resiliente
    let ws = null;
    let timer = null;
    let retryCount = 0;
    const MAX_RETRIES = 5;

    function handleDisconnect() {
      if (ws) {
        try { ws.close(); } catch (_) {}
        ws = null;
      }
      if (retryCount >= MAX_RETRIES) {
        if (onStatusChange) {
          onStatusChange({ 
            connected: false, 
            offline: true, 
            retries: retryCount,
            message: 'Telemetría desconectada. Servidor no disponible o sin proxy WebSocket.' 
          });
        }
        return;
      }

      const delay = Math.min(2000 * Math.pow(2, retryCount), 30000);
      retryCount++;
      if (onStatusChange) {
        onStatusChange({ connected: false, offline: false, retries: retryCount, nextRetryIn: delay });
      }
      timer = setTimeout(connectWs, delay);
    }

    function connectWs() {
      if (timer) {
        clearTimeout(timer);
        timer = null;
      }
      try {
        ws = new WebSocket(getWsUrl());
      } catch (err) {
        handleDisconnect();
        return;
      }

      ws.onopen = () => {
        retryCount = 0;
        if (onStatusChange) {
          onStatusChange({ connected: true, offline: false, retries: 0 });
        }
      };

      ws.onmessage = (e) => {
        try {
          const msg = JSON.parse(e.data);
          if (msg.type === 'telemetry' && onTelemetry) {
            onTelemetry(msg.data);
          } else if (msg.type === 'log' && onLog) {
            onLog(msg.message);
          }
        } catch (err) { }
      };

      ws.onclose = () => {
        handleDisconnect();
      };

      ws.onerror = () => {
        handleDisconnect();
      };
    }

    reconnectWsManualFn = () => {
      retryCount = 0;
      connectWs();
    };

    connectWs();

    return () => {
      if (timer) clearTimeout(timer);
      if (ws) ws.close();
    };
  }
}

// Flasheador de Firmware y Drivers con Detección de Entorno Web
export async function flashFirmware(port, robotType, baudRate = 115200, autoFallback = true) {
  if (isTauri()) {
    return await invoke('flash_firmware', {
      port,
      robotType,
      baudRate: Number(baudRate),
      autoFallback: Boolean(autoFallback)
    });
  } else {
    ensureSharedWs();
    try {
      const res = await fetch(`${HTTP_API_URL}/api/firmware/flash`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          port,
          robotType,
          baudRate: Number(baudRate),
          autoFallback: Boolean(autoFallback)
        })
      });

      if (!res.ok) {
        let errMsg = `Error ${res.status}: Fallo en el servidor de flasheo`;
        if (res.status === 405) {
          errMsg = 'El servidor web remoto no permite flasheo de puertos USB locales vía avrdude (405 Method Not Allowed). Para flashear Arduino Nano por USB, ejecuta la app nativa de escritorio (LineFollowerPro.exe) o el backend local (http://localhost:5010).';
        } else {
          try {
            const errJson = await res.json();
            if (errJson?.message) errMsg = errJson.message;
          } catch {}
        }
        throw new Error(errMsg);
      }

      const data = await res.json();
      if (data.log) {
        const lines = data.log.split('\n');
        for (const line of lines) {
          if (line.trim()) emitWebFlashLog(line);
        }
      }
      return data;
    } catch (netErr) {
      if (netErr.message && netErr.message.includes('405')) {
        throw netErr;
      }
      throw new Error(`No se pudo conectar con el servicio local de flasheo avrdude (${netErr.message}). Asegúrate de ejecutar el backend local o la versión de escritorio LineFollowerPro.exe.`);
    }
  }
}

export async function flashFirmwareEsp32(port, btName, baudRate = 460800, boardModel = 'esp32') {
  if (isTauri()) {
    return await invoke('flash_firmware_esp32', {
      port: typeof port === 'string' ? port : '',
      btName,
      baudRate: Number(baudRate),
      boardModel
    });
  } else {
    return await flashEsp32Web({
      port: typeof port === 'object' ? port : null,
      btName,
      baudRate: Number(baudRate),
      boardModel,
      onLog: emitWebFlashLog,
    });
  }
}

export async function diagnoseEsp32(port, baudRate = 115200) {
  if (isTauri()) {
    return await invoke('diagnose_esp32', {
      port: typeof port === 'string' ? port : '',
      baudRate: Number(baudRate)
    });
  } else {
    return await diagnoseEsp32Web({
      port: typeof port === 'object' ? port : null,
      baudRate: Number(baudRate),
      onLog: emitWebFlashLog,
    });
  }
}

export async function eraseFlashEsp32(port, baudRate = 115200) {
  if (isTauri()) {
    return await invoke('erase_flash_esp32', {
      port: typeof port === 'string' ? port : '',
      baudRate: Number(baudRate)
    });
  } else {
    return await eraseFlashEsp32Web({
      port: typeof port === 'object' ? port : null,
      baudRate: Number(baudRate),
      onLog: emitWebFlashLog,
    });
  }
}

export async function installDriver(driverType) {
  if (isTauri()) {
    return await invoke('install_driver', { driverType });
  } else {
    // Abrir enlace directamente en la web
    const urls = {
      ch340: 'https://www.wch-ic.com/downloads/CH341SER_EXE.html',
      cp2102: 'https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers',
      ftdi: 'https://ftdichip.com/drivers/vcp-drivers/'
    };
    if (urls[driverType]) {
      window.open(urls[driverType], '_blank');
      return true;
    }
    return false;
  }
}

let sharedWs = null;
let sharedWsTimer = null;

function ensureSharedWs() {
  if (isTauri() || !isBrowser) return;
  if (sharedWs && (sharedWs.readyState === WebSocket.OPEN || sharedWs.readyState === WebSocket.CONNECTING)) {
    return;
  }
  try {
    sharedWs = new WebSocket(getWsUrl());
    sharedWs.onmessage = (e) => {
      try {
        const msg = JSON.parse(e.data);
        if (msg.type === 'log' && msg.message) {
          emitWebFlashLog(msg.message);
        }
      } catch (err) {}
    };
    sharedWs.onclose = () => {
      sharedWs = null;
      if (webFlashLogSubscribers.length > 0) {
        sharedWsTimer = setTimeout(ensureSharedWs, 2000);
      }
    };
    sharedWs.onerror = () => {
      sharedWs?.close();
    };
  } catch (err) {}
}

export function subscribeFlashLogs(onLog) {
  webFlashLogSubscribers.push(onLog);
  ensureSharedWs();

  if (isTauri()) {
    let unlisten = null;
    listen('flash-log', (event) => {
      if (onLog) onLog(event.payload);
    }).then((un) => {
      unlisten = un;
    });

    return () => {
      webFlashLogSubscribers = webFlashLogSubscribers.filter(s => s !== onLog);
      if (unlisten) unlisten();
    };
  }
  return () => {
    webFlashLogSubscribers = webFlashLogSubscribers.filter(s => s !== onLog);
  };
}

