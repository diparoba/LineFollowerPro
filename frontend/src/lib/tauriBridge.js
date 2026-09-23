import { invoke } from '@tauri-apps/api/core';
import { listen } from '@tauri-apps/api/event';

const HTTP_API_URL = 'http://localhost:5000';
const WS_URL = 'ws://localhost:5000/ws/telemetry';

export function isTauri() {
  return typeof window !== 'undefined' && 
    (window.__TAURI_INTERNALS__ !== undefined || window.__TAURI__ !== undefined);
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
    try {
      const res = await fetch(`${HTTP_API_URL}/api/ports`);
      return await res.json();
    } catch (e) {
      console.warn('Fallback HTTP no disponible:', e);
      return { ports: [], connected: false, currentPort: '' };
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
      lineColor: Number(config.lineColor)
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
        lineColor: config.lineColor
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

// Suscripción a Telemetría en Vivo
export function subscribeTelemetry(onTelemetry, onLog) {
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

    return () => {
      if (unlistenTel) unlistenTel();
      if (unlistenLog) unlistenLog();
    };
  } else {
    // Modo Web con WebSocket
    let ws = null;
    let timer = null;

    function connectWs() {
      ws = new WebSocket(WS_URL);
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
        timer = setTimeout(connectWs, 2000);
      };
      ws.onerror = () => {
        ws.close();
      };
    }

    connectWs();

    return () => {
      if (timer) clearTimeout(timer);
      if (ws) ws.close();
    };
  }
}

// Flasheador de Firmware y Drivers
export async function flashFirmware(port, robotType, baudRate = 115200, autoFallback = true) {
  if (isTauri()) {
    return await invoke('flash_firmware', {
      port,
      robotType,
      baudRate: Number(baudRate),
      autoFallback: Boolean(autoFallback)
    });
  } else {
    throw new Error('El flasheo de firmware solo está disponible en la versión de escritorio nativa.');
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

export function subscribeFlashLogs(onLog) {
  if (isTauri()) {
    let unlisten = null;
    listen('flash-log', (event) => {
      if (onLog) onLog(event.payload);
    }).then((un) => {
      unlisten = un;
    });

    return () => {
      if (unlisten) unlisten();
    };
  }
  return () => {};
}

