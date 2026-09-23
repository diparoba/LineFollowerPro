use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TelemetryData {
    pub channels: usize,
    pub raw: Vec<i32>,
    pub position: i32,
    pub error: i32,
    #[serde(rename = "leftMotor")]
    pub left_motor: i32,
    #[serde(rename = "rightMotor")]
    pub right_motor: i32,
    pub state: i32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct RobotProfile {
    pub id: Option<i64>,
    pub name: String,
    #[serde(rename = "carName")]
    pub car_name: String,
    #[serde(rename = "carCategory")]
    pub car_category: String,
    pub kp: f32,
    pub kd: f32,
    #[serde(rename = "baseSpeed")]
    pub base_speed: i32,
    #[serde(rename = "maxSpeed")]
    pub max_speed: i32,
    #[serde(rename = "brakeSpeed")]
    pub brake_speed: i32,
    #[serde(rename = "forkMode")]
    pub fork_mode: i32,
    #[serde(rename = "lineColor")]
    pub line_color: i32,
    #[serde(rename = "createdAt")]
    pub created_at: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct EepromResponse {
    pub success: bool,
    pub message: String,
    pub data: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SerialPortInfo {
    pub ports: Vec<String>,
    pub connected: bool,
    #[serde(rename = "currentPort")]
    pub current_port: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FlashResult {
    pub success: bool,
    pub message: String,
    pub log: String,
    #[serde(rename = "usedBaud")]
    pub used_baud: u32,
}

