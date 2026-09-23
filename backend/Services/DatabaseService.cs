using Microsoft.Data.Sqlite;
using LineFollower.Api.Models;

namespace LineFollower.Api.Services;

public class DatabaseService
{
    private readonly string _connectionString;

    public DatabaseService(IConfiguration config)
    {
        var dbPath = config.GetValue<string>("DatabasePath") ?? "follower.db";
        _connectionString = $"Data Source={dbPath}";
        InitializeDatabase();
    }

    private void InitializeDatabase()
    {
        using var connection = new SqliteConnection(_connectionString);
        connection.Open();

        var sql = @"
            CREATE TABLE IF NOT EXISTS profiles (
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
            );
        ";

        using var command = new SqliteCommand(sql, connection);
        command.ExecuteNonQuery();

        // Migración automática para bases de datos existentes: verificar si existen columnas car_name y car_category
        var columns = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        using (var colCmd = new SqliteCommand("PRAGMA table_info(profiles)", connection))
        using (var reader = colCmd.ExecuteReader())
        {
            while (reader.Read())
            {
                columns.Add(reader.GetString(1));
            }
        }

        if (!columns.Contains("car_name"))
        {
            using var alterCmd = new SqliteCommand("ALTER TABLE profiles ADD COLUMN car_name TEXT NOT NULL DEFAULT 'Carro 1';", connection);
            alterCmd.ExecuteNonQuery();
        }

        if (!columns.Contains("car_category"))
        {
            using var alterCmd = new SqliteCommand("ALTER TABLE profiles ADD COLUMN car_category TEXT NOT NULL DEFAULT 'IM_16';", connection);
            alterCmd.ExecuteNonQuery();
        }

        // Verificar si existen perfiles iniciales
        var countCmd = new SqliteCommand("SELECT COUNT(*) FROM profiles", connection);
        var count = Convert.ToInt64(countCmd.ExecuteScalar());

        if (count == 0)
        {
            InsertDefaultProfiles(connection);
        }
    }

    private void InsertDefaultProfiles(SqliteConnection connection)
    {
        var presets = new[]
        {
            // Perfiles para 16 Líneas (Ingeniero Maker)
            new RobotProfile { Name = "Pista Rápida (16L)", CarName = "Carro IM-1", CarCategory = "IM_16", Kp = 0.20f, Kd = 3.5f, BaseSpeed = 200, MaxSpeed = 255, BrakeSpeed = 120, ForkMode = 0, LineColor = 0 },
            new RobotProfile { Name = "Pista Técnica (16L)", CarName = "Carro IM-1", CarCategory = "IM_16", Kp = 0.28f, Kd = 4.8f, BaseSpeed = 160, MaxSpeed = 220, BrakeSpeed = 150, ForkMode = 0, LineColor = 0 },
            new RobotProfile { Name = "Bifurcación Izq (16L)", CarName = "Carro IM-2", CarCategory = "IM_16", Kp = 0.24f, Kd = 4.2f, BaseSpeed = 170, MaxSpeed = 230, BrakeSpeed = 130, ForkMode = 1, LineColor = 0 },
            
            // Perfiles para 8 Líneas (Codex)
            new RobotProfile { Name = "Pista Rápida (8L Codex)", CarName = "Carro Codex-1", CarCategory = "CODEX_8", Kp = 0.35f, Kd = 5.0f, BaseSpeed = 190, MaxSpeed = 255, BrakeSpeed = 125, ForkMode = 0, LineColor = 0 },
            new RobotProfile { Name = "Pista Técnica (8L Codex)", CarName = "Carro Codex-1", CarCategory = "CODEX_8", Kp = 0.45f, Kd = 6.5f, BaseSpeed = 150, MaxSpeed = 210, BrakeSpeed = 145, ForkMode = 0, LineColor = 0 },
            new RobotProfile { Name = "Bifurcación Der (8L Codex)", CarName = "Carro Codex-2", CarCategory = "CODEX_8", Kp = 0.38f, Kd = 5.2f, BaseSpeed = 170, MaxSpeed = 220, BrakeSpeed = 130, ForkMode = 2, LineColor = 0 }
        };

        foreach (var p in presets)
        {
            SaveProfile(p, connection);
        }
    }

    public List<RobotProfile> GetProfiles(string? category = null, string? carName = null)
    {
        var list = new List<RobotProfile>();
        using var connection = new SqliteConnection(_connectionString);
        connection.Open();

        var sql = "SELECT id, name, car_name, car_category, kp, kd, base_speed, max_speed, brake_speed, fork_mode, line_color, created_at FROM profiles WHERE 1=1";
        
        if (!string.IsNullOrWhiteSpace(category))
        {
            sql += " AND car_category = @category";
        }
        if (!string.IsNullOrWhiteSpace(carName))
        {
            sql += " AND car_name = @carName";
        }
        
        sql += " ORDER BY id DESC";

        using var cmd = new SqliteCommand(sql, connection);
        if (!string.IsNullOrWhiteSpace(category))
        {
            cmd.Parameters.AddWithValue("@category", category);
        }
        if (!string.IsNullOrWhiteSpace(carName))
        {
            cmd.Parameters.AddWithValue("@carName", carName);
        }

        using var reader = cmd.ExecuteReader();

        while (reader.Read())
        {
            list.Add(new RobotProfile
            {
                Id = reader.GetInt32(0),
                Name = reader.GetString(1),
                CarName = reader.GetString(2),
                CarCategory = reader.GetString(3),
                Kp = (float)reader.GetDouble(4),
                Kd = (float)reader.GetDouble(5),
                BaseSpeed = reader.GetInt32(6),
                MaxSpeed = reader.GetInt32(7),
                BrakeSpeed = reader.GetInt32(8),
                ForkMode = reader.GetInt32(9),
                LineColor = reader.GetInt32(10),
                CreatedAt = reader.GetString(11)
            });
        }

        return list;
    }

    public RobotProfile? GetProfile(int id)
    {
        using var connection = new SqliteConnection(_connectionString);
        connection.Open();

        var sql = "SELECT id, name, car_name, car_category, kp, kd, base_speed, max_speed, brake_speed, fork_mode, line_color, created_at FROM profiles WHERE id = @id";
        using var cmd = new SqliteCommand(sql, connection);
        cmd.Parameters.AddWithValue("@id", id);
        using var reader = cmd.ExecuteReader();

        if (reader.Read())
        {
            return new RobotProfile
            {
                Id = reader.GetInt32(0),
                Name = reader.GetString(1),
                CarName = reader.GetString(2),
                CarCategory = reader.GetString(3),
                Kp = (float)reader.GetDouble(4),
                Kd = (float)reader.GetDouble(5),
                BaseSpeed = reader.GetInt32(6),
                MaxSpeed = reader.GetInt32(7),
                BrakeSpeed = reader.GetInt32(8),
                ForkMode = reader.GetInt32(9),
                LineColor = reader.GetInt32(10),
                CreatedAt = reader.GetString(11)
            };
        }

        return null;
    }

    public RobotProfile SaveProfile(RobotProfile profile)
    {
        using var connection = new SqliteConnection(_connectionString);
        connection.Open();
        return SaveProfile(profile, connection);
    }

    private RobotProfile SaveProfile(RobotProfile profile, SqliteConnection connection)
    {
        if (string.IsNullOrWhiteSpace(profile.CarName))
        {
            profile.CarName = "Carro 1";
        }
        if (string.IsNullOrWhiteSpace(profile.CarCategory))
        {
            profile.CarCategory = "IM_16";
        }

        if (profile.Id > 0)
        {
            var updateSql = @"
                UPDATE profiles 
                SET name = @name, car_name = @car_name, car_category = @car_category,
                    kp = @kp, kd = @kd, base_speed = @base_speed, 
                    max_speed = @max_speed, brake_speed = @brake_speed, 
                    fork_mode = @fork_mode, line_color = @line_color
                WHERE id = @id";
            using var cmd = new SqliteCommand(updateSql, connection);
            cmd.Parameters.AddWithValue("@id", profile.Id);
            cmd.Parameters.AddWithValue("@name", profile.Name);
            cmd.Parameters.AddWithValue("@car_name", profile.CarName);
            cmd.Parameters.AddWithValue("@car_category", profile.CarCategory);
            cmd.Parameters.AddWithValue("@kp", profile.Kp);
            cmd.Parameters.AddWithValue("@kd", profile.Kd);
            cmd.Parameters.AddWithValue("@base_speed", profile.BaseSpeed);
            cmd.Parameters.AddWithValue("@max_speed", profile.MaxSpeed);
            cmd.Parameters.AddWithValue("@brake_speed", profile.BrakeSpeed);
            cmd.Parameters.AddWithValue("@fork_mode", profile.ForkMode);
            cmd.Parameters.AddWithValue("@line_color", profile.LineColor);
            cmd.ExecuteNonQuery();
        }
        else
        {
            var insertSql = @"
                INSERT INTO profiles (name, car_name, car_category, kp, kd, base_speed, max_speed, brake_speed, fork_mode, line_color, created_at)
                VALUES (@name, @car_name, @car_category, @kp, @kd, @base_speed, @max_speed, @brake_speed, @fork_mode, @line_color, @created_at);
                SELECT last_insert_rowid();";
            using var cmd = new SqliteCommand(insertSql, connection);
            cmd.Parameters.AddWithValue("@name", profile.Name);
            cmd.Parameters.AddWithValue("@car_name", profile.CarName);
            cmd.Parameters.AddWithValue("@car_category", profile.CarCategory);
            cmd.Parameters.AddWithValue("@kp", profile.Kp);
            cmd.Parameters.AddWithValue("@kd", profile.Kd);
            cmd.Parameters.AddWithValue("@base_speed", profile.BaseSpeed);
            cmd.Parameters.AddWithValue("@max_speed", profile.MaxSpeed);
            cmd.Parameters.AddWithValue("@brake_speed", profile.BrakeSpeed);
            cmd.Parameters.AddWithValue("@fork_mode", profile.ForkMode);
            cmd.Parameters.AddWithValue("@line_color", profile.LineColor);
            cmd.Parameters.AddWithValue("@created_at", DateTime.UtcNow.ToString("yyyy-MM-dd HH:mm:ss"));
            profile.Id = Convert.ToInt32(cmd.ExecuteScalar());
        }

        return profile;
    }

    public bool DeleteProfile(int id)
    {
        using var connection = new SqliteConnection(_connectionString);
        connection.Open();

        var sql = "DELETE FROM profiles WHERE id = @id";
        using var cmd = new SqliteCommand(sql, connection);
        cmd.Parameters.AddWithValue("@id", id);
        return cmd.ExecuteNonQuery() > 0;
    }
}
