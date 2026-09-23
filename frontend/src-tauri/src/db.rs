use rusqlite::{params, Connection};
use std::sync::Mutex;
use crate::models::RobotProfile;

pub struct DatabaseService {
    conn: Mutex<Connection>,
}

impl DatabaseService {
    pub fn new(db_path: &str) -> Result<Self, String> {
        let conn = Connection::open(db_path).map_err(|e| format!("Error abriendo base SQLite: {}", e))?;
        let service = Self {
            conn: Mutex::new(conn),
        };
        service.init()?;
        Ok(service)
    }

    fn init(&self) -> Result<(), String> {
        let conn = self.conn.lock().map_err(|e| e.to_string())?;

        conn.execute(
            "CREATE TABLE IF NOT EXISTS profiles (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                car_name TEXT NOT NULL DEFAULT 'Carro 1',
                car_category TEXT NOT NULL DEFAULT 'IM_16',
                kp REAL NOT NULL,
                kd REAL NOT NULL,
                base_speed INTEGER NOT NULL,
                max_speed INTEGER NOT NULL,
                brake_speed INTEGER NOT NULL,
                fork_mode INTEGER NOT NULL,
                line_color INTEGER NOT NULL,
                created_at TEXT NOT NULL
            );",
            [],
        ).map_err(|e| format!("Error creando tabla profiles: {}", e))?;

        // Migración automática: verificar si existen columnas car_name y car_category
        let mut pragma_stmt = conn.prepare("PRAGMA table_info(profiles)")
            .map_err(|e| e.to_string())?;
        let columns: Vec<String> = pragma_stmt.query_map([], |row| row.get::<_, String>(1))
            .map_err(|e| e.to_string())?
            .filter_map(Result::ok)
            .collect();

        if !columns.iter().any(|c| c.eq_ignore_ascii_case("car_name")) {
            let _ = conn.execute("ALTER TABLE profiles ADD COLUMN car_name TEXT NOT NULL DEFAULT 'Carro 1';", []);
        }

        if !columns.iter().any(|c| c.eq_ignore_ascii_case("car_category")) {
            let _ = conn.execute("ALTER TABLE profiles ADD COLUMN car_category TEXT NOT NULL DEFAULT 'IM_16';", []);
        }

        // Si la tabla está vacía, insertar perfiles de fábrica
        let count: i64 = conn.query_row("SELECT COUNT(*) FROM profiles", [], |row| row.get(0))
            .unwrap_or(0);

        if count == 0 {
            Self::insert_default_profiles(&conn)?;
        }

        Ok(())
    }

    fn insert_default_profiles(conn: &Connection) -> Result<(), String> {
        let presets = vec![
            // 16 Líneas (Ingeniero Maker)
            ("Pista Rápida (16L)", "Carro IM-1", "IM_16", 0.20f32, 3.5f32, 200, 255, 120, 0, 0),
            ("Pista Técnica (16L)", "Carro IM-1", "IM_16", 0.28f32, 4.8f32, 160, 220, 150, 0, 0),
            ("Bifurcación Izq (16L)", "Carro IM-2", "IM_16", 0.24f32, 4.2f32, 170, 230, 130, 1, 0),
            // 8 Líneas (Codex)
            ("Pista Rápida (8L Codex)", "Carro Codex-1", "CODEX_8", 0.35f32, 5.0f32, 190, 255, 125, 0, 0),
            ("Pista Técnica (8L Codex)", "Carro Codex-1", "CODEX_8", 0.45f32, 6.5f32, 150, 210, 145, 0, 0),
            ("Bifurcación Der (8L Codex)", "Carro Codex-2", "CODEX_8", 0.38f32, 5.2f32, 170, 220, 130, 2, 0),
        ];

        let now = chrono_now();

        for (name, car_name, car_cat, kp, kd, base, max, brake, fork, color) in presets {
            conn.execute(
                "INSERT INTO profiles (name, car_name, car_category, kp, kd, base_speed, max_speed, brake_speed, fork_mode, line_color, created_at)
                 VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11)",
                params![name, car_name, car_cat, kp, kd, base, max, brake, fork, color, now],
            ).map_err(|e| format!("Error insertando perfiles por defecto: {}", e))?;
        }

        Ok(())
    }

    pub fn get_profiles(&self, category: Option<String>, car_name: Option<String>) -> Result<Vec<RobotProfile>, String> {
        let conn = self.conn.lock().map_err(|e| e.to_string())?;

        let mut sql = String::from("SELECT id, name, car_name, car_category, kp, kd, base_speed, max_speed, brake_speed, fork_mode, line_color, created_at FROM profiles WHERE 1=1");
        
        if category.is_some() {
            sql.push_str(" AND car_category = ?1");
        }
        if car_name.is_some() {
            if category.is_some() {
                sql.push_str(" AND car_name = ?2");
            } else {
                sql.push_str(" AND car_name = ?1");
            }
        }
        sql.push_str(" ORDER BY id DESC");

        let mut stmt = conn.prepare(&sql).map_err(|e| e.to_string())?;

        let rows = if let (Some(cat), Some(car)) = (&category, &car_name) {
            stmt.query_map(params![cat, car], map_profile_row)
        } else if let Some(cat) = &category {
            stmt.query_map(params![cat], map_profile_row)
        } else if let Some(car) = &car_name {
            stmt.query_map(params![car], map_profile_row)
        } else {
            stmt.query_map([], map_profile_row)
        }.map_err(|e| e.to_string())?;

        let mut list = Vec::new();
        for row in rows {
            if let Ok(profile) = row {
                list.push(profile);
            }
        }

        Ok(list)
    }

    pub fn get_profile(&self, id: i64) -> Result<Option<RobotProfile>, String> {
        let conn = self.conn.lock().map_err(|e| e.to_string())?;
        let mut stmt = conn.prepare(
            "SELECT id, name, car_name, car_category, kp, kd, base_speed, max_speed, brake_speed, fork_mode, line_color, created_at 
             FROM profiles WHERE id = ?1"
        ).map_err(|e| e.to_string())?;

        let mut rows = stmt.query_map(params![id], map_profile_row).map_err(|e| e.to_string())?;
        if let Some(Ok(profile)) = rows.next() {
            Ok(Some(profile))
        } else {
            Ok(None)
        }
    }

    pub fn save_profile(&self, mut profile: RobotProfile) -> Result<RobotProfile, String> {
        let conn = self.conn.lock().map_err(|e| e.to_string())?;

        if profile.car_name.trim().is_empty() {
            profile.car_name = "Carro 1".to_string();
        }
        if profile.car_category.trim().is_empty() {
            profile.car_category = "IM_16".to_string();
        }

        if let Some(id) = profile.id {
            if id > 0 {
                conn.execute(
                    "UPDATE profiles 
                     SET name = ?1, car_name = ?2, car_category = ?3,
                         kp = ?4, kd = ?5, base_speed = ?6, max_speed = ?7,
                         brake_speed = ?8, fork_mode = ?9, line_color = ?10
                     WHERE id = ?11",
                    params![
                        profile.name,
                        profile.car_name,
                        profile.car_category,
                        profile.kp,
                        profile.kd,
                        profile.base_speed,
                        profile.max_speed,
                        profile.brake_speed,
                        profile.fork_mode,
                        profile.line_color,
                        id
                    ],
                ).map_err(|e| format!("Error actualizando perfil: {}", e))?;
                return Ok(profile);
            }
        }

        let now = chrono_now();
        conn.execute(
            "INSERT INTO profiles (name, car_name, car_category, kp, kd, base_speed, max_speed, brake_speed, fork_mode, line_color, created_at)
             VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11)",
            params![
                profile.name,
                profile.car_name,
                profile.car_category,
                profile.kp,
                profile.kd,
                profile.base_speed,
                profile.max_speed,
                profile.brake_speed,
                profile.fork_mode,
                profile.line_color,
                now
            ],
        ).map_err(|e| format!("Error guardando nuevo perfil: {}", e))?;

        let new_id = conn.last_insert_rowid();
        profile.id = Some(new_id);
        profile.created_at = Some(now);

        Ok(profile)
    }

    pub fn delete_profile(&self, id: i64) -> Result<bool, String> {
        let conn = self.conn.lock().map_err(|e| e.to_string())?;
        let affected = conn.execute("DELETE FROM profiles WHERE id = ?1", params![id])
            .map_err(|e| format!("Error eliminando perfil: {}", e))?;
        Ok(affected > 0)
    }

    pub fn backup(&self, dest_filename: Option<String>) -> Result<String, String> {
        let conn = self.conn.lock().map_err(|e| e.to_string())?;

        let exe_dir = std::env::current_exe()
            .ok()
            .and_then(|p| p.parent().map(|p| p.to_path_buf()))
            .unwrap_or_else(|| std::env::current_dir().unwrap_or_default());

        let file_name = dest_filename.unwrap_or_else(|| {
            let secs = std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap_or_default()
                .as_secs();
            format!("backup_follower_{}.db", secs)
        });

        let dest_path = exe_dir.join(&file_name);
        if dest_path.exists() {
            let _ = std::fs::remove_file(&dest_path);
        }

        let dest_str = dest_path.to_string_lossy().to_string();
        conn.execute("VACUUM INTO ?1", params![dest_str])
            .map_err(|e| format!("Error creando backup SQLite: {}", e))?;

        Ok(file_name)
    }
}

fn map_profile_row(row: &rusqlite::Row) -> rusqlite::Result<RobotProfile> {
    Ok(RobotProfile {
        id: Some(row.get(0)?),
        name: row.get(1)?,
        car_name: row.get(2)?,
        car_category: row.get(3)?,
        kp: row.get(4)?,
        kd: row.get(5)?,
        base_speed: row.get(6)?,
        max_speed: row.get(7)?,
        brake_speed: row.get(8)?,
        fork_mode: row.get(9)?,
        line_color: row.get(10)?,
        created_at: Some(row.get(11)?),
    })
}

fn chrono_now() -> String {
    use std::time::{SystemTime, UNIX_EPOCH};
    let secs = SystemTime::now().duration_since(UNIX_EPOCH).unwrap_or_default().as_secs();
    // Formato aproximado o timestamp
    format!("{}", secs)
}
