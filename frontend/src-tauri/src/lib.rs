pub mod db;
pub mod flasher;
pub mod models;
pub mod serial;

use std::sync::Arc;
use tauri::{AppHandle, State};

use crate::db::DatabaseService;
use crate::flasher::FlasherService;
use crate::models::{EepromResponse, FlashResult, RobotProfile, SerialPortInfo};
use crate::serial::SerialService;

pub struct AppState {
    pub db: DatabaseService,
    pub serial: Arc<SerialService>,
}

#[tauri::command]
fn get_ports(state: State<'_, AppState>) -> SerialPortInfo {
    let ports = SerialService::get_available_ports();
    let connected = state.serial.is_connected();
    let current_port = state.serial.current_port();
    SerialPortInfo {
        ports,
        connected,
        current_port,
    }
}

#[tauri::command]
fn connect_serial(
    app: AppHandle,
    state: State<'_, AppState>,
    port: String,
    baud_rate: Option<u32>,
) -> Result<bool, String> {
    let baud = baud_rate.unwrap_or(115200);
    state.serial.connect(app, &port, baud)
}

#[tauri::command]
fn disconnect_serial(state: State<'_, AppState>) -> Result<bool, String> {
    state.serial.disconnect();
    Ok(true)
}

#[tauri::command]
fn send_pid(
    state: State<'_, AppState>,
    kp: f32,
    kd: f32,
    base_speed: i32,
    max_speed: i32,
    brake_speed: i32,
    fork_mode: i32,
    line_color: i32,
) -> Result<bool, String> {
    state.serial.send_pid(kp, kd, base_speed, max_speed, brake_speed, fork_mode, line_color)
}

#[tauri::command]
fn send_command(state: State<'_, AppState>, command: String) -> Result<bool, String> {
    state.serial.send_command(&command)
}

#[tauri::command]
fn save_eeprom(state: State<'_, AppState>) -> Result<EepromResponse, String> {
    state.serial.save_eeprom(2500)
}

#[tauri::command]
fn read_eeprom(state: State<'_, AppState>) -> Result<EepromResponse, String> {
    state.serial.read_eeprom(2500)
}

#[tauri::command]
fn get_profiles(
    state: State<'_, AppState>,
    category: Option<String>,
    car_name: Option<String>,
) -> Result<Vec<RobotProfile>, String> {
    state.db.get_profiles(category, car_name)
}

#[tauri::command]
fn get_profile(state: State<'_, AppState>, id: i64) -> Result<Option<RobotProfile>, String> {
    state.db.get_profile(id)
}

#[tauri::command]
fn save_profile(
    state: State<'_, AppState>,
    profile: RobotProfile,
) -> Result<RobotProfile, String> {
    state.db.save_profile(profile)
}

#[tauri::command]
fn delete_profile(state: State<'_, AppState>, id: i64) -> Result<bool, String> {
    state.db.delete_profile(id)
}

#[tauri::command]
fn flash_firmware(
    app: AppHandle,
    state: State<'_, AppState>,
    port: String,
    robot_type: String,
    baud_rate: Option<u32>,
    auto_fallback: Option<bool>,
) -> Result<FlashResult, String> {
    // Si el puerto serial está abierto en este mismo puerto, desconectar antes de flashear
    if state.serial.is_connected() && state.serial.current_port() == port {
        state.serial.disconnect();
    }

    let baud = baud_rate.unwrap_or(115200);
    let fallback = auto_fallback.unwrap_or(true);

    FlasherService::flash(&app, &port, &robot_type, baud, fallback)
}

#[tauri::command]
fn flash_firmware_esp32(
    app: AppHandle,
    state: State<'_, AppState>,
    port: String,
    bt_name: String,
    baud_rate: Option<u32>,
) -> Result<FlashResult, String> {
    // Si el puerto está abierto por el SerialService, desconectar antes de flashear
    if state.serial.is_connected() {
        state.serial.disconnect();
    }

    let baud = baud_rate.unwrap_or(460800);
    FlasherService::flash_esp32(&app, &port, &bt_name, baud)
}

#[tauri::command]
fn diagnose_esp32(
    app: AppHandle,
    state: State<'_, AppState>,
    port: String,
    baud_rate: Option<u32>,
) -> Result<FlashResult, String> {
    if state.serial.is_connected() {
        state.serial.disconnect();
    }
    let baud = baud_rate.unwrap_or(115200);
    FlasherService::diagnose_esp32(&app, &port, baud)
}

#[tauri::command]
fn erase_flash_esp32(
    app: AppHandle,
    state: State<'_, AppState>,
    port: String,
    baud_rate: Option<u32>,
) -> Result<FlashResult, String> {
    if state.serial.is_connected() {
        state.serial.disconnect();
    }
    let baud = baud_rate.unwrap_or(115200);
    FlasherService::erase_flash_esp32(&app, &port, baud)
}

#[tauri::command]
fn install_driver(app: AppHandle, driver_type: String) -> Result<bool, String> {
    FlasherService::install_driver(&app, &driver_type)
}

#[tauri::command]
fn backup_database(state: State<'_, AppState>, filename: Option<String>) -> Result<String, String> {
    state.db.backup(filename)
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    let exe_dir = std::env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(|p| p.to_path_buf()))
        .unwrap_or_else(|| std::env::current_dir().unwrap_or_default());
    let db_path = exe_dir.join("follower.db");

    let db_service = DatabaseService::new(db_path.to_str().unwrap_or("follower.db"))
        .expect("Fallo crítico inicializando SQLite follower.db");
    let serial_service = Arc::new(SerialService::new());

    let state = AppState {
        db: db_service,
        serial: serial_service,
    };

    tauri::Builder::default()
        .manage(state)
        .plugin(
            tauri_plugin_log::Builder::default()
                .level(log::LevelFilter::Info)
                .build(),
        )
        .invoke_handler(tauri::generate_handler![
            get_ports,
            connect_serial,
            disconnect_serial,
            send_pid,
            send_command,
            save_eeprom,
            read_eeprom,
            get_profiles,
            get_profile,
            save_profile,
            delete_profile,
            backup_database,
            flash_firmware,
            flash_firmware_esp32,
            diagnose_esp32,
            erase_flash_esp32,
            install_driver,
        ])
        .run(tauri::generate_context!())
        .expect("Error ejecutando la aplicación Tauri");
}
