use std::io::{BufRead, BufReader};
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use sha2::{Digest, Sha256};
use tauri::{AppHandle, Emitter, Manager};

use crate::models::FlashResult;

pub const BT_NAME_TOKEN: &[u8] = b"##BT_CAR_CUSTOM_NAME_TOKEN##";
pub const BT_NAME_BUFFER_SIZE: usize = 32;

/// Parchea en memoria el binario del ESP32 sustituyendo el token por el nombre Bluetooth,
/// y recalculando el byte de checksum XOR y el validation hash SHA-256 para que el bootloader
/// de ESP32 lo acepte como válido.
pub fn patch_esp32_binary(input_bytes: &[u8], new_name: &str) -> Result<Vec<u8>, String> {
    let sanitized_name = new_name.trim();
    if sanitized_name.is_empty() {
        return Err("El nombre Bluetooth no puede estar vacío.".to_string());
    }
    if sanitized_name.len() >= BT_NAME_BUFFER_SIZE {
        return Err(format!(
            "El nombre Bluetooth no puede exceder los {} caracteres.",
            BT_NAME_BUFFER_SIZE - 1
        ));
    }

    let pos = input_bytes
        .windows(BT_NAME_TOKEN.len())
        .position(|w| w == BT_NAME_TOKEN)
        .ok_or_else(|| "No se encontró el token de reemplazo en el binario del ESP32.".to_string())?;

    if input_bytes.len() < 33 {
        return Err("El binario del ESP32 es demasiado corto para contener checksum y digest.".to_string());
    }

    let mut patched = input_bytes.to_vec();
    let old_checksum_idx = patched.len() - 33;
    let mut checksum = patched[old_checksum_idx];

    // Reemplazar los 32 bytes con ceros y aplicar el nuevo nombre
    for i in 0..BT_NAME_BUFFER_SIZE {
        let old_b = patched[pos + i];
        let new_b = if i < sanitized_name.len() {
            sanitized_name.as_bytes()[i]
        } else {
            0
        };
        checksum ^= old_b ^ new_b;
        patched[pos + i] = new_b;
    }

    // Actualizar byte de checksum XOR
    patched[old_checksum_idx] = checksum;

    // Recalcular SHA-256 validation digest sobre todo el binario excepto los últimos 32 bytes
    let hash_content = &patched[..patched.len() - 32];
    let new_digest = Sha256::digest(hash_content);
    let digest_start = patched.len() - 32;
    patched[digest_start..].copy_from_slice(&new_digest);

    Ok(patched)
}

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

    pub fn flash_esp32(
        app: &AppHandle,
        port: &str,
        bt_name: &str,
        baud: u32,
    ) -> Result<FlashResult, String> {
        let (esptool_prog, prefix_args) = Self::resolve_esptool(app)?;

        let bootloader_bin = Self::resolve_path(app, "firmwares/esp32/bootloader.bin")
            .ok_or_else(|| "No se encontró bootloader.bin para ESP32 en recursos.".to_string())?;
        let partitions_bin = Self::resolve_path(app, "firmwares/esp32/partitions.bin")
            .ok_or_else(|| "No se encontró partitions.bin para ESP32 en recursos.".to_string())?;
        let boot_app0_bin = Self::resolve_path(app, "firmwares/esp32/boot_app0.bin")
            .ok_or_else(|| "No se encontró boot_app0.bin para ESP32 en recursos.".to_string())?;
        let template_bin = Self::resolve_path(app, "firmwares/esp32/firmware_esp32_bt.bin")
            .ok_or_else(|| "No se encontró firmware_esp32_bt.bin en recursos.".to_string())?;

        let _ = app.emit("flash-log", "🔧 Preparando flasheo para Carro Bluetooth ESP32".to_string());
        let _ = app.emit("flash-log", format!("📡 Nombre Bluetooth personalizado: \"{}\"", bt_name));
        let _ = app.emit("flash-log", format!("🔌 Puerto: {} @ {} baudios", port, baud));

        // 1. Leer y parchear binario en memoria
        let original_bytes = std::fs::read(&template_bin)
            .map_err(|e| format!("Error leyendo firmware template ESP32: {}", e))?;
        let patched_bytes = patch_esp32_binary(&original_bytes, bt_name)?;

        // 2. Guardar archivo temporal parcheado
        let temp_bin_path = std::env::temp_dir().join(format!("esp32_patched_{}.bin", std::process::id()));
        std::fs::write(&temp_bin_path, &patched_bytes)
            .map_err(|e| format!("Error escribiendo binario temporal: {}", e))?;

        let _ = app.emit("flash-log", "🧩 Binario parcheado con éxito (checksum y SHA-256 recalculados)".to_string());
        let _ = app.emit("flash-log", format!("🚀 Ejecutando esptool: {}", esptool_prog.display()));

        // 3. Ejecutar esptool
        let (status, log) = Self::run_esptool(
            app,
            &esptool_prog,
            &prefix_args,
            port,
            baud,
            &bootloader_bin,
            &partitions_bin,
            &boot_app0_bin,
            &temp_bin_path,
        );

        // Limpiar archivo temporal
        let _ = std::fs::remove_file(&temp_bin_path);

        if status {
            let msg = format!("Firmware Carro Bluetooth subido con éxito al ESP32 con nombre '{}'", bt_name);
            let _ = app.emit("flash-log", format!("✅ {}", msg));
            Ok(FlashResult {
                success: true,
                message: msg,
                log,
                used_baud: baud,
            })
        } else {
            let msg = format!("Fallo en el flasheo del ESP32 a {} baudios", baud);
            let _ = app.emit("flash-log", format!("❌ {}", msg));
            let _ = app.emit("flash-log", "💡 Sugerencia: Si queda esperando en 'Connecting...', mantén presionado el botón BOOT en el ESP32.".to_string());
            Ok(FlashResult {
                success: false,
                message: msg,
                log,
                used_baud: baud,
            })
        }
    }

    fn resolve_esptool(app: &AppHandle) -> Result<(PathBuf, Vec<String>), String> {
        // 1. Probar ejecutable local embebido en resources/esptool/esptool.exe
        if let Some(p) = Self::resolve_path(app, "esptool/esptool.exe") {
            if p.exists() {
                return Ok((p, vec![]));
            }
        }

        // 2. Probar en PlatformIO penv Scripts/esptool.exe
        if let Ok(user_profile) = std::env::var("USERPROFILE") {
            let pio_esptool = PathBuf::from(&user_profile)
                .join(".platformio")
                .join("penv")
                .join("Scripts")
                .join("esptool.exe");
            if pio_esptool.exists() {
                return Ok((pio_esptool, vec![]));
            }
        }

        // 3. Probar ejecutable en PATH
        if let Ok(status) = Command::new("esptool").arg("version").stdout(Stdio::null()).stderr(Stdio::null()).status() {
            if status.success() {
                return Ok((PathBuf::from("esptool"), vec![]));
            }
        }

        // 4. Probar python en PlatformIO penv ejecutando esptool.py
        if let Ok(user_profile) = std::env::var("USERPROFILE") {
            let pio_python = PathBuf::from(&user_profile)
                .join(".platformio")
                .join("penv")
                .join("Scripts")
                .join("python.exe");
            if pio_python.exists() {
                if let Some(script) = Self::resolve_path(app, "esptool/esptool.py") {
                    if script.exists() {
                        return Ok((pio_python, vec![script.to_string_lossy().to_string()]));
                    }
                }
                let fallback_script = PathBuf::from(&user_profile)
                    .join(".platformio")
                    .join("packages")
                    .join("tool-esptoolpy")
                    .join("esptool.py");
                if fallback_script.exists() {
                    return Ok((pio_python, vec![fallback_script.to_string_lossy().to_string()]));
                }
            }
        }

        Err("No se encontró el ejecutable esptool en los recursos de la aplicación ni en el sistema.".to_string())
    }

    fn run_esptool(
        app: &AppHandle,
        esptool_prog: &Path,
        prefix_args: &[String],
        port: &str,
        baud: u32,
        bootloader: &Path,
        partitions: &Path,
        boot_app0: &Path,
        app_bin: &Path,
    ) -> (bool, String) {
        let mut cmd = Command::new(esptool_prog);
        for arg in prefix_args {
            cmd.arg(arg);
        }

        cmd.arg("--chip").arg("esp32")
            .arg("--port").arg(port)
            .arg("--baud").arg(baud.to_string())
            .arg("--before").arg("default_reset")
            .arg("--after").arg("hard_reset")
            .arg("write_flash")
            .arg("-z")
            .arg("--flash_mode").arg("dio")
            .arg("--flash_freq").arg("40m")
            .arg("--flash_size").arg("detect")
            .arg("0x1000").arg(bootloader)
            .arg("0x8000").arg(partitions)
            .arg("0xe000").arg(boot_app0)
            .arg("0x10000").arg(app_bin)
            .stdout(Stdio::piped())
            .stderr(Stdio::piped());

        #[cfg(target_os = "windows")]
        cmd.creation_flags(CREATE_NO_WINDOW);

        let mut child = match cmd.spawn() {
            Ok(c) => c,
            Err(e) => {
                let err_msg = format!("Error al iniciar esptool: {}", e);
                let _ = app.emit("flash-log", err_msg.clone());
                return (false, err_msg);
            }
        };

        let mut full_log = String::new();

        if let Some(stdout) = child.stdout.take() {
            let reader = BufReader::new(stdout);
            for line in reader.lines().map_while(Result::ok) {
                let _ = app.emit("flash-log", line.clone());
                full_log.push_str(&line);
                full_log.push('\n');
            }
        }

        if let Some(stderr) = child.stderr.take() {
            let reader = BufReader::new(stderr);
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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_patch_esp32_binary() {
        let mut dummy = vec![0u8; 100];
        let token_pos = 10;
        dummy[token_pos..token_pos + BT_NAME_TOKEN.len()].copy_from_slice(BT_NAME_TOKEN);
        let chk_idx = dummy.len() - 33;
        dummy[chk_idx] = 0xAA;

        let patched = patch_esp32_binary(&dummy, "Mi_Carro_BT").expect("Debe parchear exitosamente");
        assert_eq!(&patched[token_pos..token_pos + 11], b"Mi_Carro_BT");
        assert_eq!(patched[token_pos + 11], 0);
        assert_eq!(patched[token_pos + 31], 0);
        assert_ne!(patched[chk_idx], 0xAA);
    }

    #[test]
    fn test_patch_esp32_binary_empty_name() {
        let dummy = vec![0u8; 100];
        assert!(patch_esp32_binary(&dummy, "   ").is_err());
    }

    #[test]
    fn test_patch_esp32_binary_name_too_long() {
        let dummy = vec![0u8; 100];
        let long_name = "A".repeat(32);
        assert!(patch_esp32_binary(&dummy, &long_name).is_err());
    }
}
