namespace LineFollower.Api.Models;

public class TelemetryData
{
    public int Channels { get; set; } = 16;
    public int[] Raw { get; set; } = Array.Empty<int>();
    public int Position { get; set; }
    public int Error { get; set; }
    public int LeftMotor { get; set; }
    public int RightMotor { get; set; }
    public int State { get; set; }
    public string Timestamp { get; set; } = DateTime.UtcNow.ToString("o");
}

public class RobotProfile
{
    public int Id { get; set; }
    public string Name { get; set; } = string.Empty;
    public string CarName { get; set; } = "Carro 1";
    public string CarCategory { get; set; } = "IM_16"; // "IM_16" o "CODEX_8"
    public float Kp { get; set; }
    public float Kd { get; set; }
    public int BaseSpeed { get; set; }
    public int MaxSpeed { get; set; }
    public int BrakeSpeed { get; set; }
    public int ForkMode { get; set; }  // 0: Recto, 1: Izquierda, 2: Derecha
    public int LineColor { get; set; } // 0: Negra sobre blanco, 1: Blanca sobre negro
    public string CreatedAt { get; set; } = DateTime.UtcNow.ToString("yyyy-MM-dd HH:mm:ss");
}

public class ConnectRequest
{
    public string Port { get; set; } = string.Empty;
    public int BaudRate { get; set; } = 115200;
}

public class CommandRequest
{
    public string Command { get; set; } = string.Empty;
}
