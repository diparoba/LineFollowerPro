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
