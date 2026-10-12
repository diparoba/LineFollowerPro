using System.Diagnostics;
using System.Net.WebSockets;
using System.Text;
using System.Text.Json;
using LineFollower.Api.Models;
using LineFollower.Api.Services;

var builder = WebApplication.CreateBuilder(args);

// Configurar puerto por defecto a 5010 si no se especifica
var serverUrl = Environment.GetEnvironmentVariable("ASPNETCORE_URLS") ?? "http://0.0.0.0:5010";
builder.WebHost.UseUrls(serverUrl);

// Servicios
builder.Services.AddSingleton<DatabaseService>();
builder.Services.AddSingleton<SerialService>();
builder.Services.AddCors(options =>
{
    options.AddDefaultPolicy(policy =>
    {
        policy.AllowAnyOrigin()
              .AllowAnyHeader()
              .AllowAnyMethod();
    });
});

var app = builder.Build();

app.UseCors();
app.UseDefaultFiles();
app.UseStaticFiles();
app.UseWebSockets();

// Colección de WebSockets activos para streaming
var activeSockets = new List<WebSocket>();
var socketLock = new object();

var serialService = app.Services.GetRequiredService<SerialService>();
var dbService = app.Services.GetRequiredService<DatabaseService>();

// Suscribirse a telemetría serial y retransmitir a WebSockets
serialService.OnTelemetryReceived += async (telemetry) =>
{
    var json = JsonSerializer.Serialize(new { type = "telemetry", data = telemetry });
    var bytes = Encoding.UTF8.GetBytes(json);
    var segment = new ArraySegment<byte>(bytes);

    List<WebSocket> toRemove = new();

    lock (socketLock)
    {
        foreach (var ws in activeSockets)
        {
            if (ws.State == WebSocketState.Open)
            {
                _ = ws.SendAsync(segment, WebSocketMessageType.Text, true, CancellationToken.None);
            }
            else
            {
                toRemove.Add(ws);
            }
        }

        foreach (var dead in toRemove)
        {
            activeSockets.Remove(dead);
        }
    }
};

serialService.OnLogReceived += async (log) =>
{
    var json = JsonSerializer.Serialize(new { type = "log", message = log });
    var bytes = Encoding.UTF8.GetBytes(json);
    var segment = new ArraySegment<byte>(bytes);

    lock (socketLock)
    {
        foreach (var ws in activeSockets.Where(s => s.State == WebSocketState.Open))
        {
            _ = ws.SendAsync(segment, WebSocketMessageType.Text, true, CancellationToken.None);
        }
    }
};

// Endpoint WebSocket para datos en vivo
app.Map("/ws/telemetry", async context =>
{
    if (context.WebSockets.IsWebSocketRequest)
    {
        using var webSocket = await context.WebSockets.AcceptWebSocketAsync();
        lock (socketLock)
        {
            activeSockets.Add(webSocket);
        }

        var buffer = new byte[1024 * 4];
        while (webSocket.State == WebSocketState.Open)
        {
            var result = await webSocket.ReceiveAsync(new ArraySegment<byte>(buffer), CancellationToken.None);
            if (result.MessageType == WebSocketMessageType.Close)
            {
                await webSocket.CloseAsync(WebSocketCloseStatus.NormalClosure, "Cerrado por cliente", CancellationToken.None);
                lock (socketLock)
                {
                    activeSockets.Remove(webSocket);
                }
            }
        }
    }
    else
    {
        context.Response.StatusCode = StatusCodes.Status400BadRequest;
    }
});

// REST Endpoints
app.MapGet("/api/ports", (SerialService serial) =>
{
    return Results.Ok(new
    {
        ports = serial.GetAvailablePorts(),
        connected = serial.IsConnected,
        currentPort = serial.CurrentPort
    });
});

app.MapPost("/api/connect", (ConnectRequest req, SerialService serial) =>
{
    if (string.IsNullOrWhiteSpace(req.Port))
    {
        return Results.BadRequest(new { success = false, message = "Debe especificar un puerto COM" });
    }

    var ok = serial.Connect(req.Port, req.BaudRate);
    return Results.Ok(new { success = ok, port = req.Port });
});

app.MapPost("/api/disconnect", (SerialService serial) =>
{
    serial.Disconnect();
    return Results.Ok(new { success = true });
});

app.MapPost("/api/firmware/flash", async (FlashFirmwareRequest req, SerialService serial) =>
{
    if (string.IsNullOrWhiteSpace(req.Port))
    {
        return Results.BadRequest(new FlashFirmwareResult
        {
            Success = false,
            Message = "Debe especificar un puerto COM válido.",
            Log = "Error: Puerto COM vacío.",
            UsedBaud = req.BaudRate
        });
    }

    var avrdudeExe = FindAvrdudeExe();
    if (avrdudeExe == null)
    {
        return Results.BadRequest(new FlashFirmwareResult
        {
            Success = false,
            Message = "No se encontró el ejecutable avrdude.exe en los recursos del backend.",
            Log = "Error: avrdude.exe no encontrado.",
            UsedBaud = req.BaudRate
        });
    }

    var avrdudeConf = FindAvrdudeConf();
    if (avrdudeConf == null)
    {
        return Results.BadRequest(new FlashFirmwareResult
        {
            Success = false,
            Message = "No se encontró avrdude.conf en los recursos del backend.",
            Log = "Error: avrdude.conf no encontrado.",
            UsedBaud = req.BaudRate
        });
    }

    var hexFile = FindHexFile(req.RobotType);
    if (hexFile == null)
    {
        return Results.BadRequest(new FlashFirmwareResult
        {
            Success = false,
            Message = $"No se encontró el archivo hexadecimal para {req.RobotType}.",
            Log = $"Error: Firmware hex no encontrado para {req.RobotType}.",
            UsedBaud = req.BaudRate
        });
    }

    // Desconectar SerialService preventivamente para liberar el puerto COM
    if (serial.IsConnected)
    {
        serial.EmitLog($"[AVRDUDE] Liberando conexión serial previa en {serial.CurrentPort}...");
        serial.Disconnect();
        await Task.Delay(400);
    }

    serial.EmitLog($"🔧 Preparando flasheo para: {req.RobotType}");
    serial.EmitLog($"📁 Firmware: {hexFile}");
    serial.EmitLog($"🔌 Puerto: {req.Port} @ {req.BaudRate} baudios");

    var attempt1 = await RunAvrdudeAsync(avrdudeExe, avrdudeConf, hexFile, req.Port, req.BaudRate, serial);

    if (attempt1.Success)
    {
        var msg = $"Firmware {req.RobotType} subido con éxito al robot a {req.BaudRate} baudios";
        serial.EmitLog($"✅ {msg}");
        return Results.Ok(new FlashFirmwareResult
        {
            Success = true,
            Message = msg,
            Log = attempt1.Log,
            UsedBaud = req.BaudRate
        });
    }

    // Si falló y autoFallback está activo y estábamos en 115200, probamos 57600 (Old Bootloader)
    if (req.AutoFallback && req.BaudRate == 115200)
    {
        int fallbackBaud = 57600;
        serial.EmitLog("⚠️ Falló a 115200 baudios. Activando Auto-Fallback a 57600 baudios (Old Bootloader)...");
        await Task.Delay(500);

        var attempt2 = await RunAvrdudeAsync(avrdudeExe, avrdudeConf, hexFile, req.Port, fallbackBaud, serial);
        var combinedLog = $"--- Intento 115200 ---\n{attempt1.Log}\n--- Intento Fallback 57600 ---\n{attempt2.Log}";

        if (attempt2.Success)
        {
            var msg = $"Firmware {req.RobotType} subido con éxito (Auto-Fallback: Old Bootloader @ 57600 baudios)";
            serial.EmitLog($"✅ {msg}");
            return Results.Ok(new FlashFirmwareResult
            {
                Success = true,
                Message = msg,
                Log = combinedLog,
                UsedBaud = fallbackBaud
            });
        }
        else
        {
            var msg = "Fallo en flasheo: no respondió ni a 115200 ni a 57600 baudios";
            serial.EmitLog($"❌ {msg}");
            return Results.Ok(new FlashFirmwareResult
            {
                Success = false,
                Message = msg,
                Log = combinedLog,
                UsedBaud = fallbackBaud
            });
        }
    }

    serial.EmitLog($"❌ Error de flasheo a {req.BaudRate} baudios");
    return Results.Ok(new FlashFirmwareResult
    {
        Success = false,
        Message = $"Error de flasheo a {req.BaudRate} baudios",
        Log = attempt1.Log,
        UsedBaud = req.BaudRate
    });
});

app.MapPost("/api/pid", (RobotProfile profile, SerialService serial) =>
{
    var ok = serial.SendPid(
        profile.Kp,
        profile.Kd,
        profile.BaseSpeed,
        profile.MaxSpeed,
        profile.BrakeSpeed,
        profile.ForkMode,
        profile.LineColor
    );
    return Results.Ok(new { success = ok });
});

app.MapPost("/api/command", (CommandRequest req, SerialService serial) =>
{
    var ok = serial.SendCommand(req.Command);
    return Results.Ok(new { success = ok });
});

app.MapPost("/api/eeprom/save", async (SerialService serial) =>
{
    var (success, message, data) = await serial.SaveEEPROMAsync();
    return Results.Ok(new { success, message, data });
});

app.MapGet("/api/eeprom/read", async (SerialService serial) =>
{
    var (success, message, data) = await serial.ReadEEPROMAsync();
    return Results.Ok(new { success, message, data });
});

app.MapPost("/api/eeprom/load", (SerialService serial) =>
{
    var ok = serial.LoadEEPROM();
    return Results.Ok(new { success = ok });
});

// CRUD de Perfiles SQLite
app.MapGet("/api/profiles", (string? category, string? carName, DatabaseService db) =>
{
    return Results.Ok(db.GetProfiles(category, carName));
});

app.MapGet("/api/profiles/{id:int}", (int id, DatabaseService db) =>
{
    var profile = db.GetProfile(id);
    return profile is not null ? Results.Ok(profile) : Results.NotFound();
});

app.MapPost("/api/profiles", (RobotProfile profile, DatabaseService db) =>
{
    var saved = db.SaveProfile(profile);
    return Results.Ok(saved);
});

app.MapDelete("/api/profiles/{id:int}", (int id, DatabaseService db) =>
{
    var ok = db.DeleteProfile(id);
    return Results.Ok(new { success = ok });
});

// Fallback para SPA en Svelte
app.MapFallbackToFile("index.html");

// Auto-abrir navegador en modo portable de escritorio (solo local interactivo)
if (Environment.GetEnvironmentVariable("ASPNETCORE_ENVIRONMENT") != "Production" &&
    Environment.GetEnvironmentVariable("DISABLE_BROWSER_AUTO_OPEN") != "1")
{
    _ = Task.Run(async () =>
    {
        await Task.Delay(1200);
        try
        {
            var openUrl = serverUrl.Replace("0.0.0.0", "localhost");
            Process.Start(new ProcessStartInfo
            {
                FileName = openUrl,
                UseShellExecute = true
            });
        }
        catch { }
    });
}

app.Run();

// Métodos estáticos de soporte para flasheo con avrdude
static async Task<(bool Success, string Log)> RunAvrdudeAsync(
    string avrdudeExe,
    string avrdudeConf,
    string hexFile,
    string port,
    int baud,
    SerialService serial)
{
    var logBuilder = new StringBuilder();
    var flashArg = $"flash:w:\"{hexFile}\":i";

    var psi = new ProcessStartInfo
    {
        FileName = avrdudeExe,
        Arguments = $"-C \"{avrdudeConf}\" -v -p atmega328p -c arduino -P {port} -b {baud} -D -U {flashArg}",
        RedirectStandardOutput = true,
        RedirectStandardError = true,
        UseShellExecute = false,
        CreateNoWindow = true
    };

    try
    {
        using var process = new Process { StartInfo = psi };
        process.OutputDataReceived += (s, e) =>
        {
            if (!string.IsNullOrEmpty(e.Data))
            {
                lock (logBuilder)
                {
                    logBuilder.AppendLine(e.Data);
                }
                serial.EmitLog(e.Data);
            }
        };
        process.ErrorDataReceived += (s, e) =>
        {
            if (!string.IsNullOrEmpty(e.Data))
            {
                lock (logBuilder)
                {
                    logBuilder.AppendLine(e.Data);
                }
                serial.EmitLog(e.Data);
            }
        };

        process.Start();
        process.BeginOutputReadLine();
        process.BeginErrorReadLine();

        await process.WaitForExitAsync();
        return (process.ExitCode == 0, logBuilder.ToString());
    }
    catch (Exception ex)
    {
        var err = $"Error ejecutando avrdude: {ex.Message}";
        serial.EmitLog(err);
        return (false, err);
    }
}

static string? FindAvrdudeExe()
{
    var candidates = new[]
    {
        Path.Combine(AppContext.BaseDirectory, "resources", "avrdude", "avrdude.exe"),
        Path.Combine(Directory.GetCurrentDirectory(), "resources", "avrdude", "avrdude.exe"),
        Path.Combine(Directory.GetCurrentDirectory(), "backend", "resources", "avrdude", "avrdude.exe"),
        Path.Combine(Directory.GetCurrentDirectory(), "frontend", "src-tauri", "resources", "avrdude", "avrdude.exe")
    };
    return candidates.FirstOrDefault(File.Exists);
}

static string? FindAvrdudeConf()
{
    var candidates = new[]
    {
        Path.Combine(AppContext.BaseDirectory, "resources", "avrdude", "avrdude.conf"),
        Path.Combine(Directory.GetCurrentDirectory(), "resources", "avrdude", "avrdude.conf"),
        Path.Combine(Directory.GetCurrentDirectory(), "backend", "resources", "avrdude", "avrdude.conf"),
        Path.Combine(Directory.GetCurrentDirectory(), "frontend", "src-tauri", "resources", "avrdude", "avrdude.conf")
    };
    return candidates.FirstOrDefault(File.Exists);
}

static string? FindHexFile(string robotType)
{
    string hexName = (robotType.Contains("8", StringComparison.OrdinalIgnoreCase) || robotType.Equals("CODEX_8", StringComparison.OrdinalIgnoreCase))
        ? "firmware_codex8.hex"
        : "firmware_im16.hex";

    var candidates = new[]
    {
        Path.Combine(AppContext.BaseDirectory, "wwwroot", "firmwares", hexName),
        Path.Combine(Directory.GetCurrentDirectory(), "wwwroot", "firmwares", hexName),
        Path.Combine(Directory.GetCurrentDirectory(), "backend", "wwwroot", "firmwares", hexName),
        Path.Combine(Directory.GetCurrentDirectory(), "frontend", "public", "firmwares", hexName)
    };
    return candidates.FirstOrDefault(File.Exists);
}
