using System.IO.Ports;
using System.Text;
using LineFollower.Api.Models;

namespace LineFollower.Api.Services;

public class SerialService : IDisposable
{
    private SerialPort? _serialPort;
    private Thread? _readThread;
    private bool _isRunning = false;

    public event Action<TelemetryData>? OnTelemetryReceived;
    public event Action<string>? OnLogReceived;
    public event Action<string>? OnConfigReceived;

    public bool IsConnected => _serialPort?.IsOpen ?? false;
    public string CurrentPort => _serialPort?.PortName ?? string.Empty;

    public string[] GetAvailablePorts()
    {
        return SerialPort.GetPortNames();
    }

    public bool Connect(string portName, int baudRate = 115200)
    {
        try
        {
            Disconnect();

            _serialPort = new SerialPort(portName, baudRate, Parity.None, 8, StopBits.One)
            {
                ReadTimeout = 1000,
                WriteTimeout = 1000,
                DtrEnable = true, // Reiniciar Arduino al conectar si procede
                RtsEnable = true
            };

            _serialPort.Open();
            _isRunning = true;

            _readThread = new Thread(ReadLoop)
            {
                IsBackground = true,
                Name = "SerialReadThread"
            };
            _readThread.Start();

            OnLogReceived?.Invoke($"Conectado exitosamente al puerto {portName} @ {baudRate} baudios");
            return true;
        }
        catch (Exception ex)
        {
            OnLogReceived?.Invoke($"Error conectando a {portName}: {ex.Message}");
            return false;
        }
    }

    public void Disconnect()
    {
        _isRunning = false;

        if (_serialPort != null)
        {
            try
            {
                if (_serialPort.IsOpen)
                {
                    _serialPort.Close();
                }
            }
            catch { }
            finally
            {
                _serialPort.Dispose();
                _serialPort = null;
            }
        }

        _readThread?.Join(500);
        _readThread = null;
    }

    public bool SendRaw(string message)
    {
        if (!IsConnected || _serialPort == null) return false;

        try
        {
            var line = message.TrimEnd() + "\n";
            _serialPort.Write(line);
            return true;
        }
        catch (Exception ex)
        {
            OnLogReceived?.Invoke($"Error enviando comando: {ex.Message}");
            return false;
        }
    }

    public bool SendPid(float kp, float kd, int baseSpeed, int maxSpeed, int brakeSpeed, int forkMode, int lineColor)
    {
        // Formato: $PID,kp,kd,base,max,brake,fork,color
        var cmd = FormattableString.Invariant($"$PID,{kp:F4},{kd:F3},{baseSpeed},{maxSpeed},{brakeSpeed},{forkMode},{lineColor}");
        return SendRaw(cmd);
    }

    public bool SendCommand(string action)
    {
        // Formato: $CMD,action (START, STOP, CAL_BLACK, CAL_WHITE)
        return SendRaw($"$CMD,{action}");
    }

    private TaskCompletionSource<string>? _eepromTcs;
    private readonly object _tcsLock = new();

    public async Task<(bool success, string message, string? data)> SaveEEPROMAsync(int timeoutMs = 2500)
    {
        if (!IsConnected) return (false, "Puerto serial no conectado", null);

        TaskCompletionSource<string> tcs;
        lock (_tcsLock)
        {
            _eepromTcs = new TaskCompletionSource<string>(TaskCreationOptions.RunContinuationsAsynchronously);
            tcs = _eepromTcs;
        }

        SendRaw("$EEPROM,SAVE");

        var completedTask = await Task.WhenAny(tcs.Task, Task.Delay(timeoutMs));
        if (completedTask == tcs.Task)
        {
            var res = await tcs.Task;
            if (res.StartsWith("$EEPROM_OK"))
            {
                return (true, "Guardado y verificado en la memoria EEPROM física del robot con éxito", res);
            }
            return (false, $"Error reportado por el robot: {res}", null);
        }
        else
        {
            return (false, "Tiempo de espera agotado sin respuesta de verificación del robot", null);
        }
    }

    public async Task<(bool success, string message, string? data)> ReadEEPROMAsync(int timeoutMs = 2500)
    {
        if (!IsConnected) return (false, "Puerto serial no conectado", null);

        TaskCompletionSource<string> tcs;
        lock (_tcsLock)
        {
            _eepromTcs = new TaskCompletionSource<string>(TaskCreationOptions.RunContinuationsAsynchronously);
            tcs = _eepromTcs;
        }

        SendRaw("$EEPROM,READ");

        var completedTask = await Task.WhenAny(tcs.Task, Task.Delay(timeoutMs));
        if (completedTask == tcs.Task)
        {
            var res = await tcs.Task;
            if (res.StartsWith("$EEPROM_DATA"))
            {
                return (true, "Lectura de EEPROM exitosa", res);
            }
            return (false, $"Error reportado por el robot: {res}", null);
        }
        else
        {
            return (false, "Tiempo de espera agotado sin respuesta del robot", null);
        }
    }

    public bool SaveEEPROM()
    {
        return SendRaw("$EEPROM,SAVE");
    }

    public bool LoadEEPROM()
    {
        return SendRaw("$EEPROM,LOAD");
    }

    public bool RequestConfig()
    {
        return SendRaw("$GET");
    }

    private void ReadLoop()
    {
        var buffer = new StringBuilder();

        while (_isRunning && _serialPort != null && _serialPort.IsOpen)
        {
            try
            {
                var line = _serialPort.ReadLine();
                if (string.IsNullOrWhiteSpace(line)) continue;

                line = line.Trim();
                ProcessLine(line);
            }
            catch (TimeoutException)
            {
                // Timeout normal, continuar escuchando
            }
            catch (Exception ex)
            {
                if (_isRunning)
                {
                    OnLogReceived?.Invoke($"Error de lectura serial: {ex.Message}");
                }
                break;
            }
        }
    }

    private void ProcessLine(string line)
    {
        if (line.StartsWith("$TEL,"))
        {
            // Formato: $TEL,raw0..rawN,pos,err,pwmL,pwmR,state
            var parts = line.Substring(5).Split(',');
            if (parts.Length >= 13) // Mínimo 8 sensores + 5 campos de control
            {
                int sensorCount = parts.Length - 5;
                var telemetry = new TelemetryData
                {
                    Channels = sensorCount,
                    Raw = new int[sensorCount]
                };

                for (int i = 0; i < sensorCount; i++)
                {
                    if (int.TryParse(parts[i], out int val))
                    {
                        telemetry.Raw[i] = val;
                    }
                }

                if (int.TryParse(parts[sensorCount], out int pos)) telemetry.Position = pos;
                if (int.TryParse(parts[sensorCount + 1], out int err)) telemetry.Error = err;
                if (int.TryParse(parts[sensorCount + 2], out int left)) telemetry.LeftMotor = left;
                if (int.TryParse(parts[sensorCount + 3], out int right)) telemetry.RightMotor = right;
                if (int.TryParse(parts[sensorCount + 4], out int st)) telemetry.State = st;

                OnTelemetryReceived?.Invoke(telemetry);
            }
        }
        else if (line.StartsWith("$CFG,"))
        {
            OnConfigReceived?.Invoke(line.Substring(5));
        }
        else if (line.StartsWith("$EEPROM_"))
        {
            lock (_tcsLock)
            {
                _eepromTcs?.TrySetResult(line);
            }
            OnLogReceived?.Invoke(line);
        }
        else
        {
            OnLogReceived?.Invoke(line);
        }
    }

    public void Dispose()
    {
        Disconnect();
    }
}
