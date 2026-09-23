use std::io::{BufRead, BufReader, Write};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::mpsc::{channel, Sender};
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;
use tauri::Emitter;

use crate::models::{EepromResponse, TelemetryData};

pub struct SerialService {
    port_writer: Arc<Mutex<Option<Box<dyn serialport::SerialPort>>>>,
    current_port: Arc<Mutex<Option<String>>>,
    is_running: Arc<AtomicBool>,
    eeprom_sender: Arc<Mutex<Option<Sender<String>>>>,
}

impl SerialService {
    pub fn new() -> Self {
        Self {
            port_writer: Arc::new(Mutex::new(None)),
            current_port: Arc::new(Mutex::new(None)),
            is_running: Arc::new(AtomicBool::new(false)),
            eeprom_sender: Arc::new(Mutex::new(None)),
        }
    }

    pub fn get_available_ports() -> Vec<String> {
        match serialport::available_ports() {
            Ok(ports) => ports.into_iter().map(|p| p.port_name).collect(),
            Err(_) => Vec::new(),
        }
    }

    pub fn is_connected(&self) -> bool {
        self.is_running.load(Ordering::SeqCst) && self.current_port.lock().unwrap().is_some()
    }

    pub fn current_port(&self) -> String {
        self.current_port.lock().unwrap().clone().unwrap_or_default()
    }

    pub fn connect(&self, app: tauri::AppHandle, port_name: &str, baud_rate: u32) -> Result<bool, String> {
        self.disconnect();

        let builder = serialport::new(port_name, baud_rate)
            .timeout(Duration::from_millis(1000));

        let mut port = builder.open().map_err(|e| format!("Error abriendo {}: {}", port_name, e))?;

        // Pulso DTR/RTS para resetear Arduino si procede
        let _ = port.write_data_terminal_ready(true);
        let _ = port.write_request_to_send(true);

        let port_reader = port.try_clone().map_err(|e| format!("Error clonando puerto: {}", e))?;

        *self.port_writer.lock().unwrap() = Some(port);
        *self.current_port.lock().unwrap() = Some(port_name.to_string());
        self.is_running.store(true, Ordering::SeqCst);

        let is_running_clone = self.is_running.clone();
        let eeprom_sender_clone = self.eeprom_sender.clone();
        let port_name_str = port_name.to_string();

        thread::spawn(move || {
            let mut reader = BufReader::new(port_reader);
            let mut line = String::new();

            let _ = app.emit("log", format!("Conectado exitosamente a {} @ {} baudios", port_name_str, baud_rate));

            while is_running_clone.load(Ordering::SeqCst) {
                line.clear();
                match reader.read_line(&mut line) {
                    Ok(0) => {
                        // EOF
                        thread::sleep(Duration::from_millis(10));
                    }
                    Ok(_) => {
                        let trimmed = line.trim();
                        if trimmed.is_empty() {
                            continue;
                        }

                        if trimmed.starts_with("$TEL,") {
                            if let Some(telemetry) = parse_telemetry(&trimmed[5..]) {
                                let _ = app.emit("telemetry", &telemetry);
                            }
                        } else if trimmed.starts_with("$EEPROM_") {
                            if let Some(sender) = eeprom_sender_clone.lock().unwrap().take() {
                                let _ = sender.send(trimmed.to_string());
                            }
                            let _ = app.emit("log", trimmed);
                        } else if trimmed.starts_with("$CFG,") {
                            let _ = app.emit("config", &trimmed[5..]);
                        } else {
                            let _ = app.emit("log", trimmed);
                        }
                    }
                    Err(ref e) if e.kind() == std::io::ErrorKind::TimedOut => {
                        // Timeout normal
                        continue;
                    }
                    Err(e) => {
                        if is_running_clone.load(Ordering::SeqCst) {
                            let _ = app.emit("log", format!("Error de lectura serial: {}", e));
                        }
                        break;
                    }
                }
            }
        });

        Ok(true)
    }

    pub fn disconnect(&self) {
        self.is_running.store(false, Ordering::SeqCst);
        *self.current_port.lock().unwrap() = None;
        *self.port_writer.lock().unwrap() = None;
    }

    pub fn send_raw(&self, message: &str) -> Result<bool, String> {
        let mut writer_guard = self.port_writer.lock().unwrap();
        if let Some(ref mut port) = *writer_guard {
            let line = format!("{}\n", message.trim_end());
            port.write_all(line.as_bytes()).map_err(|e| e.to_string())?;
            port.flush().map_err(|e| e.to_string())?;
            Ok(true)
        } else {
            Err("Puerto serial no conectado".to_string())
        }
    }

    pub fn send_pid(&self, kp: f32, kd: f32, base: i32, max: i32, brake: i32, fork: i32, color: i32) -> Result<bool, String> {
        let cmd = format!("$PID,{:.4},{:.3},{},{},{},{},{}", kp, kd, base, max, brake, fork, color);
        self.send_raw(&cmd)
    }

    pub fn send_command(&self, action: &str) -> Result<bool, String> {
        let cmd = format!("$CMD,{}", action);
        self.send_raw(&cmd)
    }

    pub fn save_eeprom(&self, timeout_ms: u64) -> Result<EepromResponse, String> {
        if !self.is_connected() {
            return Ok(EepromResponse {
                success: false,
                message: "Puerto serial no conectado".to_string(),
                data: None,
            });
        }

        let (tx, rx) = channel();
        *self.eeprom_sender.lock().unwrap() = Some(tx);

        self.send_raw("$EEPROM,SAVE")?;

        match rx.recv_timeout(Duration::from_millis(timeout_ms)) {
            Ok(res) => {
                if res.starts_with("$EEPROM_OK") {
                    Ok(EepromResponse {
                        success: true,
                        message: "Guardado y verificado en la memoria EEPROM física del robot con éxito".to_string(),
                        data: Some(res),
                    })
                } else {
                    Ok(EepromResponse {
                        success: false,
                        message: format!("Error reportado por el robot: {}", res),
                        data: None,
                    })
                }
            }
            Err(_) => {
                Ok(EepromResponse {
                    success: false,
                    message: "Tiempo de espera agotado sin respuesta de verificación del robot".to_string(),
                    data: None,
                })
            }
        }
    }

    pub fn read_eeprom(&self, timeout_ms: u64) -> Result<EepromResponse, String> {
        if !self.is_connected() {
            Ok(EepromResponse {
                success: false,
                message: "Puerto serial no conectado".to_string(),
                data: None,
            })
        } else {
            let (tx, rx) = channel();
            *self.eeprom_sender.lock().unwrap() = Some(tx);

            self.send_raw("$EEPROM,READ")?;

            match rx.recv_timeout(Duration::from_millis(timeout_ms)) {
                Ok(res) => {
                    if res.starts_with("$EEPROM_DATA") {
                        Ok(EepromResponse {
                            success: true,
                            message: "Lectura de EEPROM exitosa".to_string(),
                            data: Some(res),
                        })
                    } else {
                        Ok(EepromResponse {
                            success: false,
                            message: format!("Error reportado por el robot: {}", res),
                            data: None,
                        })
                    }
                }
                Err(_) => {
                    Ok(EepromResponse {
                        success: false,
                        message: "Tiempo de espera agotado sin respuesta del robot".to_string(),
                        data: None,
                    })
                }
            }
        }
    }
}

fn parse_telemetry(payload: &str) -> Option<TelemetryData> {
    // Formato: raw0..rawN,pos,err,pwmL,pwmR,state
    let parts: Vec<&str> = payload.split(',').collect();
    if parts.len() < 13 {
        return None;
    }

    let sensor_count = parts.len() - 5;
    let mut raw = Vec::with_capacity(sensor_count);

    for i in 0..sensor_count {
        raw.push(parts[i].trim().parse::<i32>().unwrap_or(0));
    }

    let position = parts[sensor_count].trim().parse::<i32>().unwrap_or(0);
    let error = parts[sensor_count + 1].trim().parse::<i32>().unwrap_or(0);
    let left_motor = parts[sensor_count + 2].trim().parse::<i32>().unwrap_or(0);
    let right_motor = parts[sensor_count + 3].trim().parse::<i32>().unwrap_or(0);
    let state = parts[sensor_count + 4].trim().parse::<i32>().unwrap_or(0);

    Some(TelemetryData {
        channels: sensor_count,
        raw,
        position,
        error,
        left_motor,
        right_motor,
        state,
    })
}
