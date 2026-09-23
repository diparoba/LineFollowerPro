use std::io::{BufRead, BufReader};
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use tauri::{AppHandle, Emitter, Manager};

use crate::models::FlashResult;

#[cfg(target_os = "windows")]
use std::os::windows::process::CommandExt;
#[cfg(target_os = "windows")]
const CREATE_NO_WINDOW: u32 = 0x08000000;

pub struct FlasherService;

impl FlasherService {
    pub fn resolve_path(app: &AppHandle, relative_subpath: &str) -> Option<PathBuf> {
        // 1. Probar directorio de recursos oficial de Tauri
        if let Ok(resource_dir) = app.path().resource_dir() {
            let candidate = resource_dir.join(relative_subpath);
            if candidate.exists() {
                return Some(candidate);
            }
        }

        // 2. Probar junto al ejecutable actual
        if let Ok(current_exe) = std::env::current_exe() {
            if let Some(parent) = current_exe.parent() {
                let candidate = parent.join("resources").join(relative_subpath);
                if candidate.exists() {
                    return Some(candidate);
                }
                let candidate_direct = parent.join(relative_subpath);
                if candidate_direct.exists() {
                    return Some(candidate_direct);
                }
            }
        }

        // 3. Probar rutas relativas de desarrollo
        let dev_candidates = [
            PathBuf::from("resources").join(relative_subpath),
            PathBuf::from("src-tauri").join("resources").join(relative_subpath),
            PathBuf::from("frontend").join("src-tauri").join("resources").join(relative_subpath),
        ];

        for c in &dev_candidates {
            if c.exists() {
                return Some(c.clone());
            }
        }

        None
    }

    pub fn flash(
        app: &AppHandle,
        port: &str,
        robot_type: &str,
        initial_baud: u32,
        auto_fallback: bool,
    ) -> Result<FlashResult, String> {
        let avrdude_exe = Self::resolve_path(app, "avrdude/avrdude.exe")
            .ok_or_else(|| "No se encontró el ejecutable avrdude.exe en recursos".to_string())?;

        let avrdude_conf = Self::resolve_path(app, "avrdude/avrdude.conf")
            .ok_or_else(|| "No se encontró avrdude.conf en recursos".to_string())?;

        let hex_rel_path = if robot_type.eq_ignore_ascii_case("CODEX_8") || robot_type.contains("8") {
            "firmwares/firmware_codex8.hex"
        } else {
            "firmwares/firmware_im16.hex"
        };

        let hex_file = Self::resolve_path(app, hex_rel_path)
            .ok_or_else(|| format!("No se encontró el binario {} en recursos", hex_rel_path))?;

        let _ = app.emit("flash-log", format!("🔧 Preparando flasheo para: {}", robot_type));
        let _ = app.emit("flash-log", format!("📁 Firmware: {}", hex_file.display()));
        let _ = app.emit("flash-log", format!("🔌 Puerto: {} @ {} baudios", port, initial_baud));

        // Intento 1 con la velocidad inicial (115200 o la seleccionada)
        let attempt1 = Self::run_avrdude(app, &avrdude_exe, &avrdude_conf, &hex_file, port, initial_baud);

        if attempt1.0 {
            let msg = format!("Firmware {} subido con éxito al robot a {} baudios", robot_type, initial_baud);
            let _ = app.emit("flash-log", format!("✅ {}", msg));
            return Ok(FlashResult {
                success: true,
                message: msg,
                log: attempt1.1,
                used_baud: initial_baud,
            });
        }

        // Si falló y auto_fallback está activo y estábamos en 115200, probamos 57600 (Old Bootloader)
        if auto_fallback && initial_baud == 115200 {
            let fallback_baud = 57600;
            let _ = app.emit("flash-log", "⚠️ Falló a 115200 baudios. Activando Auto-Fallback a 57600 baudios (Old Bootloader)...".to_string());

            let attempt2 = Self::run_avrdude(app, &avrdude_exe, &avrdude_conf, &hex_file, port, fallback_baud);
            if attempt2.0 {
                let msg = format!("Firmware {} subido con éxito (Auto-Fallback: Old Bootloader @ 57600 baudios)", robot_type);
                let _ = app.emit("flash-log", format!("✅ {}", msg));
                return Ok(FlashResult {
                    success: true,
                    message: msg,
                    log: format!("--- Intento 115200 ---\n{}\n--- Intento Fallback 57600 ---\n{}", attempt1.1, attempt2.1),
                    used_baud: fallback_baud,
                });
            } else {
                return Ok(FlashResult {
                    success: false,
                    message: "Fallo en flasheo: no respondió ni a 115200 ni a 57600 baudios".to_string(),
                    log: format!("--- Intento 115200 ---\n{}\n--- Intento Fallback 57600 ---\n{}", attempt1.1, attempt2.1),
                    used_baud: fallback_baud,
                });
            }
        }

        Ok(FlashResult {
            success: false,
            message: format!("Error de flasheo a {} baudios", initial_baud),
            log: attempt1.1,
            used_baud: initial_baud,
        })
    }

    fn run_avrdude(
        app: &AppHandle,
        avrdude_exe: &Path,
        avrdude_conf: &Path,
        hex_file: &Path,
        port: &str,
        baud: u32,
    ) -> (bool, String) {
        let flash_arg = format!("flash:w:{}:i", hex_file.display());

        let mut cmd = Command::new(avrdude_exe);
        cmd.arg("-C").arg(avrdude_conf)
            .arg("-v")
            .arg("-p").arg("atmega328p")
            .arg("-c").arg("arduino")
            .arg("-P").arg(port)
            .arg("-b").arg(baud.to_string())
            .arg("-D")
            .arg("-U").arg(&flash_arg)
            .stdout(Stdio::piped())
            .stderr(Stdio::piped());

        #[cfg(target_os = "windows")]
        cmd.creation_flags(CREATE_NO_WINDOW);

        let mut child = match cmd.spawn() {
            Ok(c) => c,
            Err(e) => {
                let err_msg = format!("Error al iniciar avrdude: {}", e);
                let _ = app.emit("flash-log", err_msg.clone());
                return (false, err_msg);
            }
        };

        let mut full_log = String::new();

        // avrdude envía su información de progreso a stderr
        if let Some(stderr) = child.stderr.take() {
            let reader = BufReader::new(stderr);
            for line in reader.lines().map_while(Result::ok) {
                let _ = app.emit("flash-log", line.clone());
                full_log.push_str(&line);
                full_log.push('\n');
            }
        }

        if let Some(stdout) = child.stdout.take() {
            let reader = BufReader::new(stdout);
            for line in reader.lines().map_while(Result::ok) {
                let _ = app.emit("flash-log", line.clone());
                full_log.push_str(&line);
                full_log.push('\n');
            }
        }

        let status = child.wait().map(|s| s.success()).unwrap_or(false);
        (status, full_log)
    }

    pub fn install_driver(app: &AppHandle, driver_type: &str) -> Result<bool, String> {
        match driver_type {
            "ch340" => {
                // 1. Priorizar ejecutable local embebido con elevación de Administrador
                if let Some(driver_path) = Self::resolve_path(app, "drivers/CH341SER.EXE") {
                    #[cfg(target_os = "windows")]
                    {
                        let script = format!("Start-Process -FilePath '{}' -Verb RunAs", driver_path.to_string_lossy());
                        Command::new("powershell")
                            .args(["-NoProfile", "-Command", &script])
                            .spawn()
                            .map_err(|e| format!("Error ejecutando instalador CH340: {}", e))?;
                        return Ok(true);
                    }
                    #[cfg(not(target_os = "windows"))]
                    {
                        return Err("El instalador CH341SER.EXE es exclusivo para Windows.".to_string());
                    }
                }
                // Fallback a página web oficial si no se encontrara el archivo local
                let url = "https://www.wch-ic.com/downloads/CH341SER_EXE.html";
                let _ = open_url(url);
                Ok(true)
            }
            "cp2102" => {
                let url = "https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers";
                let _ = open_url(url);
                Ok(true)
            }
            "ftdi" => {
                let url = "https://ftdichip.com/drivers/vcp-drivers/";
                let _ = open_url(url);
                Ok(true)
            }
            _ => Err(format!("Tipo de driver desconocido: {}", driver_type)),
        }
    }
}

fn open_url(url: &str) -> Result<(), String> {
    #[cfg(target_os = "windows")]
    {
        Command::new("rundll32")
            .args(["url.dll,FileProtocolHandler", url])
            .spawn()
            .map_err(|e| e.to_string())?;
        Ok(())
    }
    #[cfg(not(target_os = "windows"))]
    {
        Command::new("xdg-open")
            .arg(url)
            .spawn()
            .map_err(|e| e.to_string())?;
        Ok(())
    }
}
