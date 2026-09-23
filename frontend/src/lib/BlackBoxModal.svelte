<script>
  import { createEventDispatcher, onMount, onDestroy } from 'svelte';

  export let isOpen = false;
  export let telemetry = {
    channels: 16,
    raw: [],
    position: 7500,
    error: 0,
    leftMotor: 0,
    rightMotor: 0,
    state: 0
  };
  export let isConnected = false;
  export let activeCarName = 'Carro 1';

  const dispatch = createEventDispatcher();

  // Estados de grabación
  let autoRecordEnabled = true;
  let isRecording = false;
  let recordingStartTime = 0;
  let liveLapTime = 0;
  let liveTimerInterval = null;
  let currentLapSamples = [];
  let lastRobotState = 0;

  // Búfer de las últimas 5 vueltas
  let lapHistory = [];
  let nextLapNumber = 1;

  // Selección de vueltas para comparación
  let selectedLapAId = null;
  let selectedLapBId = null;

  // Referencias Canvas y visualización interactiva
  let canvasElem = null;
  let hoverX = null;
  let hoverInfo = null;

  // Reactividad ante telemetría continua para detección de vueltas
  $: handleTelemetryChange(telemetry);

  function handleTelemetryChange(tel) {
    if (!tel) return;
    const currentState = tel.state;

    // 1. Detección automática al entrar a STATE_RUNNING (5)
    if (autoRecordEnabled && isConnected) {
      if (lastRobotState !== 5 && currentState === 5) {
        // Inicio de carrera detectado automáticamente
        startRecording(true);
      } else if (lastRobotState === 5 && currentState !== 5) {
        // Fin de carrera / frenada detectada automáticamente
        stopRecording();
      }
    }

    lastRobotState = currentState;

    // 2. Si se está grabando activamente, capturar muestra
    if (isRecording) {
      const relTime = Math.round(performance.now() - recordingStartTime);
      liveLapTime = relTime;
      currentLapSamples.push({
        t: relTime,
        timestamp: Date.now(),
        err: tel.error || 0,
        pos: tel.position || 0,
        left: tel.leftMotor || 0,
        right: tel.rightMotor || 0,
        state: tel.state || 0,
        raw: Array.isArray(tel.raw) ? [...tel.raw] : []
      });
    }
  }

  function startRecording(isAuto = false) {
    if (isRecording) return;
    isRecording = true;
    recordingStartTime = performance.now();
    liveLapTime = 0;
    currentLapSamples = [];

    if (liveTimerInterval) clearInterval(liveTimerInterval);
    liveTimerInterval = setInterval(() => {
      if (isRecording) {
        liveLapTime = Math.round(performance.now() - recordingStartTime);
      }
    }, 30);
  }

  function stopRecording() {
    if (!isRecording) return;
    isRecording = false;
    if (liveTimerInterval) {
      clearInterval(liveTimerInterval);
      liveTimerInterval = null;
    }

    // Filtrar sesiones triviales (< 10 muestras / ~400ms)
    if (currentLapSamples.length < 10) {
      currentLapSamples = [];
      return;
    }

    const duration = currentLapSamples[currentLapSamples.length - 1].t;
    const n = currentLapSamples.length;

    // Cálculo riguroso de Error Cuadrático Medio (RMSE)
    const sumSqErr = currentLapSamples.reduce((acc, s) => acc + (s.err * s.err), 0);
    const rmse = Math.round(Math.sqrt(sumSqErr / n));

    // Error absoluto máximo y promedio
    const maxErr = Math.max(...currentLapSamples.map(s => Math.abs(s.err)));
    const avgErr = Math.round(currentLapSamples.reduce((acc, s) => acc + Math.abs(s.err), 0) / n);

    // Velocidades
    const peakSpeed = Math.max(...currentLapSamples.map(s => Math.max(Math.abs(s.left), Math.abs(s.right))));
    const avgSpeed = Math.round(currentLapSamples.reduce((acc, s) => acc + (Math.abs(s.left) + Math.abs(s.right)) / 2, 0) / n);

    // Detección de frenadas bruscas (PWM por debajo de 60 durante la carrera)
    const hardBrakes = currentLapSamples.filter(s => s.left < 60 || s.right < 60).length;

    const newLap = {
      id: nextLapNumber++,
      carName: activeCarName,
      date: new Date().toLocaleTimeString(),
      durationMs: duration,
      formattedDuration: formatTime(duration),
      samplesCount: n,
      rmse,
      maxErr,
      avgErr,
      peakSpeed,
      avgSpeed,
      hardBrakes,
      samples: [...currentLapSamples]
    };

    // Mantener búfer de máximo 5 vueltas (la más reciente al inicio)
    lapHistory = [newLap, ...lapHistory.slice(0, 4)];

    // Selección automática: Lap A = vuelta recién completada
    selectedLapAId = newLap.id;
    // Si ya había una previa, seleccionarla como Lap B para comparación inmediata
    if (lapHistory.length > 1) {
      selectedLapBId = lapHistory[1].id;
    }

    currentLapSamples = [];
  }

  function formatTime(ms) {
    const totalSec = Math.floor(ms / 1000);
    const mins = Math.floor(totalSec / 60);
    const secs = totalSec % 60;
    const millis = ms % 1000;
    return `${String(mins).padStart(2, '0')}:${String(secs).padStart(2, '0')}.${String(millis).padStart(3, '0')}`;
  }

  function clearHistory() {
    if (confirm('¿Deseas vaciar el historial de las últimas vueltas?')) {
      lapHistory = [];
      selectedLapAId = null;
      selectedLapBId = null;
    }
  }

  // Exportar vuelta individual a CSV
  function exportLapCSV(lap) {
    if (!lap || !lap.samples || lap.samples.length === 0) return;

    const numSensors = lap.samples[0].raw ? lap.samples[0].raw.length : 16;
    let header = 'Timestamp_ms,Relative_ms,Error,Position,LeftMotor,RightMotor,State';
    for (let i = 0; i < numSensors; i++) {
      header += `,Sensor_${i}`;
    }
    header += '\r\n';

    let rows = '';
    for (let s of lap.samples) {
      let r = `${s.timestamp},${s.t},${s.err},${s.pos},${s.left},${s.right},${s.state}`;
      if (s.raw && s.raw.length > 0) {
        r += ',' + s.raw.join(',');
      }
      rows += r + '\r\n';
    }

    const blob = new Blob([header + rows], { type: 'text/csv;charset=utf-8;' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `CajaNegra_${activeCarName.replace(/\s+/g, '_')}_Vuelta_${lap.id}_${Date.now()}.csv`;
    a.click();
    URL.revokeObjectURL(url);
  }

  // Exportar todas las vueltas del búfer a CSV
  function exportAllLapsCSV() {
    if (lapHistory.length === 0) return;
    let content = '';

    for (let lap of lapHistory) {
      content += `=== VUELTA #${lap.id} | Duración: ${lap.formattedDuration} | RMSE: ${lap.rmse} | Auto: ${lap.carName} ===\r\n`;
      const numSensors = lap.samples[0].raw ? lap.samples[0].raw.length : 16;
      let header = 'Timestamp_ms,Relative_ms,Error,Position,LeftMotor,RightMotor,State';
      for (let i = 0; i < numSensors; i++) {
        header += `,Sensor_${i}`;
      }
      header += '\r\n';
      content += header;

      for (let s of lap.samples) {
        let r = `${s.timestamp},${s.t},${s.err},${s.pos},${s.left},${s.right},${s.state}`;
        if (s.raw && s.raw.length > 0) {
          r += ',' + s.raw.join(',');
        }
        content += r + '\r\n';
      }
      content += '\r\n\r\n';
    }

    const blob = new Blob([content], { type: 'text/csv;charset=utf-8;' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `CajaNegra_Completa_${lapHistory.length}_Vueltas_${Date.now()}.csv`;
    a.click();
    URL.revokeObjectURL(url);
  }

  // Referencias a los datos de Lap A y Lap B
  $: lapA = lapHistory.find(l => l.id === selectedLapAId) || null;
  $: lapB = lapHistory.find(l => l.id === selectedLapBId) || null;

  // Redibujado en Canvas cuando cambie selección o dimensiones
  $: if (isOpen && canvasElem && (lapA || lapB)) {
    renderCanvas();
  }

  function renderCanvas() {
    if (!canvasElem) return;
    const ctx = canvasElem.getContext('2d');
    const width = canvasElem.width;
    const height = canvasElem.height;

    // Limpieza
    ctx.clearRect(0, 0, width, height);

    // Fondo sutil del área de gráfica
    ctx.fillStyle = '#0f172a';
    ctx.fillRect(0, 0, width, height);

    const padLeft = 55;
    const padRight = 20;
    const padTop = 25;
    const padBottom = 35;
    const plotW = width - padLeft - padRight;
    const plotH = height - padTop - padBottom;
    const midY = padTop + plotH / 2;

    // Calcular escala de tiempo y escala de error
    const maxDuration = Math.max(
      lapA ? lapA.durationMs : 1000,
      lapB ? lapB.durationMs : 1000
    );

    let maxAbsErr = 1000;
    if (lapA) {
      for (let s of lapA.samples) {
        if (Math.abs(s.err) > maxAbsErr) maxAbsErr = Math.abs(s.err);
      }
    }
    if (lapB) {
      for (let s of lapB.samples) {
        if (Math.abs(s.err) > maxAbsErr) maxAbsErr = Math.abs(s.err);
      }
    }
    maxAbsErr = Math.ceil(maxAbsErr * 1.15); // Margen superior del 15%

    // 1. Dibujar líneas de cuadrícula y etiquetas
    ctx.strokeStyle = '#1e293b';
    ctx.lineWidth = 1;

    // Cuadrícula horizontal (Error)
    const errSteps = [-maxAbsErr, -Math.round(maxAbsErr/2), 0, Math.round(maxAbsErr/2), maxAbsErr];
    ctx.font = '10px JetBrains Mono, monospace';
    ctx.fillStyle = '#64748b';
    ctx.textAlign = 'right';

    for (let errVal of errSteps) {
      const y = midY - (errVal / maxAbsErr) * (plotH / 2);
      ctx.beginPath();
      ctx.moveTo(padLeft, y);
      ctx.lineTo(width - padRight, y);
      ctx.stroke();
      ctx.fillText(`${errVal > 0 ? '+' : ''}${errVal}`, padLeft - 6, y + 3);
    }

    // Línea central de cero error (referencia de trayectoria ideal)
    ctx.strokeStyle = 'rgba(56, 189, 248, 0.35)';
    ctx.setLineDash([4, 4]);
    ctx.beginPath();
    ctx.moveTo(padLeft, midY);
    ctx.lineTo(width - padRight, midY);
    ctx.stroke();
    ctx.setLineDash([]);

    // Cuadrícula vertical (Tiempo en segundos)
    const numTimeSteps = 6;
    ctx.textAlign = 'center';
    for (let i = 0; i <= numTimeSteps; i++) {
      const tVal = (maxDuration / numTimeSteps) * i;
      const x = padLeft + (tVal / maxDuration) * plotW;
      ctx.strokeStyle = '#1e293b';
      ctx.beginPath();
      ctx.moveTo(x, padTop);
      ctx.lineTo(x, height - padBottom);
      ctx.stroke();

      ctx.fillText(`${(tVal / 1000).toFixed(1)}s`, x, height - padBottom + 16);
    }

    // 2. Trazar curva de Lap B (Comparativa - Ámbar) primero si existe
    if (lapB && lapB.samples.length > 0) {
      drawCurve(ctx, lapB.samples, maxDuration, maxAbsErr, padLeft, plotW, midY, plotH, '#f59e0b', 'rgba(245, 158, 11, 0.12)');
    }

    // 3. Trazar curva de Lap A (Principal - Cian/Azul Eléctrico)
    if (lapA && lapA.samples.length > 0) {
      drawCurve(ctx, lapA.samples, maxDuration, maxAbsErr, padLeft, plotW, midY, plotH, '#38bdf8', 'rgba(56, 189, 248, 0.15)');
    }

    // 4. Cursor interactivo hover si el ratón está sobre la gráfica
    if (hoverX !== null && hoverX >= padLeft && hoverX <= width - padRight) {
      ctx.strokeStyle = 'rgba(255, 255, 255, 0.6)';
      ctx.lineWidth = 1;
      ctx.setLineDash([2, 2]);
      ctx.beginPath();
      ctx.moveTo(hoverX, padTop);
      ctx.lineTo(hoverX, height - padBottom);
      ctx.stroke();
      ctx.setLineDash([]);
    }
  }

  function drawCurve(ctx, samples, maxDuration, maxAbsErr, padLeft, plotW, midY, plotH, strokeColor, fillColor) {
    if (samples.length < 2) return;

    ctx.strokeStyle = strokeColor;
    ctx.lineWidth = 2.2;
    ctx.beginPath();

    for (let i = 0; i < samples.length; i++) {
      const s = samples[i];
      const x = padLeft + (s.t / maxDuration) * plotW;
      const clampedErr = Math.max(-maxAbsErr, Math.min(maxAbsErr, s.err));
      const y = midY - (clampedErr / maxAbsErr) * (plotH / 2);

      if (i === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    }
    ctx.stroke();
  }

  function handleCanvasMouseMove(e) {
    if (!canvasElem) return;
    const rect = canvasElem.getBoundingClientRect();
    const scaleX = canvasElem.width / rect.width;
    const x = (e.clientX - rect.left) * scaleX;

    const padLeft = 55;
    const padRight = 20;
    const plotW = canvasElem.width - padLeft - padRight;

    if (x >= padLeft && x <= canvasElem.width - padRight) {
      hoverX = x;
      const maxDuration = Math.max(
        lapA ? lapA.durationMs : 1000,
        lapB ? lapB.durationMs : 1000
      );
      const targetTime = ((x - padLeft) / plotW) * maxDuration;

      // Buscar muestras más cercanas
      let sampleA = null;
      if (lapA && lapA.samples.length > 0) {
        sampleA = lapA.samples.reduce((prev, curr) => 
          Math.abs(curr.t - targetTime) < Math.abs(prev.t - targetTime) ? curr : prev
        );
      }

      let sampleB = null;
      if (lapB && lapB.samples.length > 0) {
        sampleB = lapB.samples.reduce((prev, curr) => 
          Math.abs(curr.t - targetTime) < Math.abs(prev.t - targetTime) ? curr : prev
        );
      }

      hoverInfo = {
        timeSec: (targetTime / 1000).toFixed(2),
        sampleA,
        sampleB
      };

      renderCanvas();
    } else {
      hoverX = null;
      hoverInfo = null;
      renderCanvas();
    }
  }

  function handleCanvasMouseLeave() {
    hoverX = null;
    hoverInfo = null;
    renderCanvas();
  }

  function close() {
    dispatch('close');
  }

  onDestroy(() => {
    if (liveTimerInterval) clearInterval(liveTimerInterval);
  });
</script>

{#if isOpen}
  <div 
    class="modal-backdrop" 
    on:click|self={close}
    on:keydown={(e) => { if (e.key === 'Escape') close(); }}
    role="dialog"
    aria-modal="true"
    tabindex="-1"
  >
    <div class="modal-container precision-card">
      <!-- Encabezado Modal -->
      <div class="modal-header">
        <div class="header-brand">
          <div class="blackbox-icon {isRecording ? 'pulse-rec' : ''}">🔴</div>
          <div>
            <h2>Caja Negra & Telemetría de Alta Frecuencia</h2>
            <p class="subtitle">
              Grabación reactiva de carreras, historial de 5 vueltas, análisis comparativo de RMSE y exportación CSV
            </p>
          </div>
        </div>
        <div class="header-actions">
          {#if isRecording}
            <div class="rec-badge precision-chip">
              <span class="live-dot active"></span>
              REC EN VIVO: {formatTime(liveLapTime)}
            </div>
          {:else if autoRecordEnabled}
            <div class="auto-badge precision-chip">
              <span class="live-dot standby"></span>
              AUTO-GRABAR ACTIVO
            </div>
          {/if}
          <button class="close-btn" on:click={close} title="Cerrar modal">✕</button>
        </div>
      </div>

      <!-- Barra de Control de Grabación -->
      <div class="blackbox-toolbar">
        <div class="toolbar-left">
          <label class="toggle-control" title="Inicia la grabación automáticamente cuando el robot cambia a STATE_RUNNING y se detiene al frenar">
            <input type="checkbox" bind:checked={autoRecordEnabled} />
            <span class="toggle-label">Auto-grabación al iniciar carrera</span>
          </label>

          {#if !isRecording}
            <button class="precision-btn btn-record" on:click={() => startRecording(false)}>
              ⏺ Iniciar Manual
            </button>
          {:else}
            <button class="precision-btn btn-stop-rec" on:click={stopRecording}>
              ⏹ Finalizar Vuelta
            </button>
          {/if}
        </div>

        <div class="toolbar-right">
          {#if lapHistory.length > 0}
            <button class="precision-btn btn-csv" on:click={exportAllLapsCSV} title="Descargar todas las vueltas en un archivo CSV consolidado">
              📥 Exportar Todo a CSV ({lapHistory.length})
            </button>
            <button class="precision-btn btn-clear" on:click={clearHistory} title="Vaciar las vueltas registradas">
              🗑 Limpiar
            </button>
          {/if}
        </div>
      </div>

      <!-- Métricas en Vivo durante Grabación -->
      {#if isRecording}
        <div class="live-recording-panel precision-card">
          <div class="live-stat">
            <span class="stat-title">CRONÓMETRO VUELTA</span>
            <span class="stat-value timer-val">{formatTime(liveLapTime)}</span>
          </div>
          <div class="live-stat">
            <span class="stat-title">MUESTRAS REGISTRADAS</span>
            <span class="stat-value">{currentLapSamples.length} pts</span>
          </div>
          <div class="live-stat">
            <span class="stat-title">ERROR EN PISTA</span>
            <span class="stat-value {Math.abs(telemetry.error) > 200 ? 'warn' : 'good'}">
              {telemetry.error > 0 ? `+${telemetry.error}` : telemetry.error}
            </span>
          </div>
          <div class="live-stat">
            <span class="stat-title">POTENCIA MOTORES</span>
            <span class="stat-value">L:{telemetry.leftMotor} | R:{telemetry.rightMotor}</span>
          </div>
        </div>
      {/if}

      <!-- Contenedor Principal: Historial de 5 Vueltas + Gráfica Comparativa -->
      <div class="blackbox-body">
        <!-- Columna Izquierda: Tarjetas de Historial de Vueltas -->
        <div class="laps-column">
          <div class="section-title-row">
            <h3>Historial de Vueltas (Últimas 5)</h3>
            <span class="lap-count-chip">{lapHistory.length} / 5</span>
          </div>

          {#if lapHistory.length === 0}
            <div class="empty-state">
              <div class="empty-icon">⏱</div>
              <p class="empty-title">Sin vueltas registradas</p>
              <p class="empty-desc">
                Inicia una carrera con el robot en pista o pulsa "Iniciar Manual" para registrar la primera vuelta.
              </p>
            </div>
          {:else}
            <div class="laps-list">
              {#each lapHistory as lap}
                <div class="lap-card {lap.id === selectedLapAId ? 'selected-a' : ''} {lap.id === selectedLapBId ? 'selected-b' : ''}">
                  <div class="lap-card-header">
                    <div class="lap-title">
                      <span class="lap-number">Vuelta #{lap.id}</span>
                      <span class="lap-time-tag">{lap.date}</span>
                    </div>
                    <div class="lap-duration precision-mono">
                      {lap.formattedDuration}
                    </div>
                  </div>

                  <div class="lap-metrics-grid">
                    <div class="metric-item">
                      <span class="m-label">RMSE:</span>
                      <span class="m-val {lap.rmse < 250 ? 'good' : 'warn'}">±{lap.rmse}</span>
                    </div>
                    <div class="metric-item">
                      <span class="m-label">Pico Vel:</span>
                      <span class="m-val">{lap.peakSpeed} PWM</span>
                    </div>
                    <div class="metric-item">
                      <span class="m-label">Muestras:</span>
                      <span class="m-val">{lap.samplesCount}</span>
                    </div>
                    <div class="metric-item">
                      <span class="m-label">Frenadas:</span>
                      <span class="m-val">{lap.hardBrakes}</span>
                    </div>
                  </div>

                  <div class="lap-card-actions">
                    <div class="compare-radios">
                      <label class="radio-label {lap.id === selectedLapAId ? 'active-a' : ''}">
                        <input 
                          type="radio" 
                          name="lapA" 
                          checked={lap.id === selectedLapAId} 
                          on:change={() => selectedLapAId = lap.id}
                        />
                        <span>Vuelta A (Principal)</span>
                      </label>
                      <label class="radio-label {lap.id === selectedLapBId ? 'active-b' : ''}">
                        <input 
                          type="radio" 
                          name="lapB" 
                          checked={lap.id === selectedLapBId} 
                          on:change={() => selectedLapBId = lap.id}
                        />
                        <span>Vuelta B (Comparar)</span>
                      </label>
                    </div>

                    <button class="btn-micro-csv" on:click={() => exportLapCSV(lap)} title="Descargar CSV de esta vuelta">
                      📥 CSV
                    </button>
                  </div>
                </div>
              {/each}
            </div>
          {/if}
        </div>

        <!-- Columna Derecha: Gráfica Comparativa y Deltas -->
        <div class="chart-column">
          <!-- Tarjeta de Deltas de Rendimiento -->
          {#if lapA && lapB && lapA.id !== lapB.id}
            <div class="deltas-card precision-card">
              <div class="delta-header">
                <span class="delta-badge">COMPARATIVA DIRECTA</span>
                <span class="delta-vs">Vuelta #{lapA.id} (A) vs Vuelta #{lapB.id} (B)</span>
              </div>
              <div class="delta-metrics-row">
                <div class="delta-metric">
                  <span class="d-label">Δ TIEMPO DE VUELTA:</span>
                  {#if lapB.durationMs < lapA.durationMs}
                    <span class="d-val good">B es {( (lapA.durationMs - lapB.durationMs)/1000 ).toFixed(3)}s MÁS RÁPIDA ⚡</span>
                  {:else if lapB.durationMs > lapA.durationMs}
                    <span class="d-val warn">A es {( (lapB.durationMs - lapA.durationMs)/1000 ).toFixed(3)}s MÁS RÁPIDA ⚡</span>
                  {:else}
                    <span class="d-val">Tiempos idénticos</span>
                  {/if}
                </div>

                <div class="delta-metric">
                  <span class="d-label">Δ PRECISIÓN (RMSE):</span>
                  {#if lapB.rmse < lapA.rmse}
                    <span class="d-val good">B tiene menor error (-{lapA.rmse - lapB.rmse} pts)</span>
                  {:else if lapB.rmse > lapA.rmse}
                    <span class="d-val warn">A tiene menor error (-{lapB.rmse - lapA.rmse} pts)</span>
                  {:else}
                    <span class="d-val">Mismo RMSE</span>
                  {/if}
                </div>
              </div>
            </div>
          {/if}

          <!-- Canvas Interactivo con Gráfica de Error -->
          <div class="canvas-wrapper precision-card">
            <div class="canvas-header">
              <span class="canvas-title">Curva Temporal de Error de Posición (Trayectoria)</span>
              <div class="chart-legend">
                {#if lapA}
                  <span class="legend-chip chip-a">● Vuelta #{lapA.id} ({lapA.formattedDuration})</span>
                {/if}
                {#if lapB && lapB.id !== lapA?.id}
                  <span class="legend-chip chip-b">● Vuelta #{lapB.id} ({lapB.formattedDuration})</span>
                {/if}
              </div>
            </div>

            <!-- Canvas HTML5 nativo -->
            <div class="canvas-container">
              <canvas 
                bind:this={canvasElem}
                width={720}
                height={280}
                on:mousemove={handleCanvasMouseMove}
                on:mouseleave={handleCanvasMouseLeave}
              ></canvas>

              {#if hoverInfo}
                <div class="canvas-tooltip">
                  <div class="tooltip-time">⏱ {hoverInfo.timeSec}s</div>
                  {#if hoverInfo.sampleA}
                    <div class="tooltip-row a">
                      A (#{lapA.id}): Err={hoverInfo.sampleA.err} | L:{hoverInfo.sampleA.left} R:{hoverInfo.sampleA.right}
                    </div>
                  {/if}
                  {#if hoverInfo.sampleB && lapB && lapB.id !== lapA?.id}
                    <div class="tooltip-row b">
                      B (#{lapB.id}): Err={hoverInfo.sampleB.err} | L:{hoverInfo.sampleB.left} R:{hoverInfo.sampleB.right}
                    </div>
                  {/if}
                </div>
              {/if}
            </div>

            <div class="canvas-footer">
              <span class="footer-tip">
                💡 Pasa el ratón sobre la gráfica para inspeccionar los milisegundos exactos, el error y la potencia de los motores en cualquier curva.
              </span>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
{/if}

<style>
  .modal-backdrop {
    position: fixed;
    top: 0;
    left: 0;
    width: 100vw;
    height: 100vh;
    background: rgba(0, 0, 0, 0.78);
    backdrop-filter: blur(5px);
    display: flex;
    justify-content: center;
    align-items: center;
    z-index: 9999;
    padding: 1rem;
    animation: fadeIn 0.18s ease-out;
  }

  @keyframes fadeIn {
    from { opacity: 0; transform: scale(0.98); }
    to { opacity: 1; transform: scale(1); }
  }

  .modal-container {
    width: 100%;
    max-width: 1140px;
    max-height: 92vh;
    background: var(--bg-surface);
    border: 1px solid var(--border-medium);
    border-radius: 12px;
    display: flex;
    flex-direction: column;
    overflow-y: auto;
    box-shadow: 0 20px 50px rgba(0, 0, 0, 0.55);
  }

  .modal-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 1rem 1.25rem;
    border-bottom: 1px solid var(--border-subtle);
    background: var(--bg-subtle);
  }

  .header-brand {
    display: flex;
    align-items: center;
    gap: 0.75rem;
  }

  .blackbox-icon {
    font-size: 1.5rem;
    line-height: 1;
  }

  .pulse-rec {
    animation: pulse 1s infinite alternate;
  }

  @keyframes pulse {
    0% { transform: scale(1); filter: drop-shadow(0 0 2px #ef4444); }
    100% { transform: scale(1.2); filter: drop-shadow(0 0 10px #ef4444); }
  }

  h2 {
    font-size: 1.05rem;
    font-weight: 700;
    margin: 0;
    color: var(--text-primary);
  }

  .subtitle {
    font-size: 0.76rem;
    color: var(--text-secondary);
    margin: 0.15rem 0 0 0;
  }

  .header-actions {
    display: flex;
    align-items: center;
    gap: 0.75rem;
  }

  .rec-badge {
    background: rgba(239, 68, 68, 0.2);
    border: 1px solid #ef4444;
    color: #ef4444;
    font-weight: 700;
    font-size: 0.75rem;
    padding: 0.25rem 0.6rem;
  }

  .auto-badge {
    background: rgba(34, 197, 94, 0.12);
    border: 1px solid rgba(34, 197, 94, 0.4);
    color: #22c55e;
    font-size: 0.72rem;
    padding: 0.25rem 0.5rem;
  }

  .live-dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    display: inline-block;
  }

  .live-dot.active {
    background: #ef4444;
    box-shadow: 0 0 8px #ef4444;
  }

  .live-dot.standby {
    background: #22c55e;
  }

  .close-btn {
    background: transparent;
    border: none;
    color: var(--text-secondary);
    font-size: 1.1rem;
    cursor: pointer;
    padding: 0.25rem 0.5rem;
    border-radius: 4px;
  }

  .close-btn:hover {
    color: var(--text-primary);
    background: var(--bg-hover);
  }

  /* Toolbar */
  .blackbox-toolbar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 0.65rem 1.25rem;
    background: var(--bg-card);
    border-bottom: 1px solid var(--border-subtle);
    flex-wrap: wrap;
    gap: 0.75rem;
  }

  .toolbar-left, .toolbar-right {
    display: flex;
    align-items: center;
    gap: 0.65rem;
    flex-wrap: wrap;
  }

  .toggle-control {
    display: flex;
    align-items: center;
    gap: 0.45rem;
    font-size: 0.78rem;
    color: var(--text-primary);
    font-weight: 500;
    cursor: pointer;
  }

  .btn-record {
    background: #0284c7;
    color: #fff;
    font-size: 0.76rem;
    font-weight: 600;
    padding: 0.35rem 0.75rem;
  }

  .btn-stop-rec {
    background: #dc2626;
    color: #fff;
    font-size: 0.76rem;
    font-weight: 600;
    padding: 0.35rem 0.75rem;
    animation: pulse 1s infinite alternate;
  }

  .btn-csv {
    background: #059669;
    color: #fff;
    font-size: 0.76rem;
    font-weight: 600;
    padding: 0.35rem 0.75rem;
  }

  .btn-clear {
    background: var(--bg-subtle);
    border: 1px solid var(--border-subtle);
    color: var(--text-secondary);
    font-size: 0.76rem;
    padding: 0.35rem 0.6rem;
  }

  .btn-clear:hover {
    color: #ef4444;
  }

  /* Live Recording Panel */
  .live-recording-panel {
    display: flex;
    justify-content: space-around;
    padding: 0.75rem 1rem;
    margin: 0.75rem 1.25rem 0 1.25rem;
    background: rgba(239, 68, 68, 0.08);
    border: 1px solid rgba(239, 68, 68, 0.3);
    border-radius: 8px;
    flex-wrap: wrap;
    gap: 0.5rem;
  }

  .live-stat {
    display: flex;
    flex-direction: column;
    align-items: center;
  }

  .stat-title {
    font-size: 0.65rem;
    font-weight: 700;
    color: var(--text-secondary);
    letter-spacing: 0.5px;
  }

  .stat-value {
    font-family: 'JetBrains Mono', monospace;
    font-size: 1rem;
    font-weight: 700;
    color: var(--text-primary);
  }

  .timer-val {
    color: #ef4444;
    font-size: 1.15rem;
  }

  .stat-value.good { color: #22c55e; }
  .stat-value.warn { color: #f59e0b; }

  /* Body */
  .blackbox-body {
    display: grid;
    grid-template-columns: 340px 1fr;
    gap: 1rem;
    padding: 1rem 1.25rem;
  }

  @media (max-width: 900px) {
    .blackbox-body {
      grid-template-columns: 1fr;
    }
  }

  /* Laps Column */
  .laps-column {
    display: flex;
    flex-direction: column;
    gap: 0.65rem;
  }

  .section-title-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .section-title-row h3 {
    font-size: 0.82rem;
    font-weight: 700;
    margin: 0;
    color: var(--text-secondary);
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }

  .lap-count-chip {
    font-size: 0.7rem;
    background: var(--bg-subtle);
    border: 1px solid var(--border-subtle);
    padding: 0.15rem 0.45rem;
    border-radius: 10px;
    color: var(--text-secondary);
    font-family: 'JetBrains Mono', monospace;
  }

  .empty-state {
    padding: 2.5rem 1rem;
    text-align: center;
    background: var(--bg-card);
    border: 1px dashed var(--border-subtle);
    border-radius: 8px;
  }

  .empty-icon {
    font-size: 2rem;
    margin-bottom: 0.5rem;
    opacity: 0.5;
  }

  .empty-title {
    font-size: 0.85rem;
    font-weight: 700;
    color: var(--text-primary);
    margin: 0;
  }

  .empty-desc {
    font-size: 0.74rem;
    color: var(--text-secondary);
    margin: 0.35rem 0 0 0;
    line-height: 1.4;
  }

  .laps-list {
    display: flex;
    flex-direction: column;
    gap: 0.55rem;
    max-height: 480px;
    overflow-y: auto;
  }

  .lap-card {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    border-radius: 8px;
    padding: 0.65rem 0.75rem;
    display: flex;
    flex-direction: column;
    gap: 0.45rem;
    transition: all 0.15s ease;
  }

  .lap-card:hover {
    border-color: var(--border-medium);
  }

  .lap-card.selected-a {
    border-color: #38bdf8;
    background: rgba(56, 189, 248, 0.05);
    box-shadow: 0 0 0 1px #38bdf8;
  }

  .lap-card.selected-b {
    border-color: #f59e0b;
    background: rgba(245, 158, 11, 0.05);
    box-shadow: 0 0 0 1px #f59e0b;
  }

  .lap-card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .lap-title {
    display: flex;
    align-items: center;
    gap: 0.45rem;
  }

  .lap-number {
    font-size: 0.82rem;
    font-weight: 700;
    color: var(--text-primary);
  }

  .lap-time-tag {
    font-size: 0.68rem;
    color: var(--text-secondary);
  }

  .lap-duration {
    font-size: 0.95rem;
    font-weight: 700;
    color: #38bdf8;
  }

  .lap-metrics-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.25rem 0.5rem;
    font-size: 0.72rem;
    background: var(--bg-subtle);
    padding: 0.35rem 0.5rem;
    border-radius: 5px;
  }

  .metric-item {
    display: flex;
    justify-content: space-between;
  }

  .m-label { color: var(--text-secondary); }
  .m-val { font-family: 'JetBrains Mono', monospace; font-weight: 600; color: var(--text-primary); }
  .m-val.good { color: #22c55e; }
  .m-val.warn { color: #f59e0b; }

  .lap-card-actions {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-top: 0.2rem;
    padding-top: 0.35rem;
    border-top: 1px solid var(--border-subtle);
  }

  .compare-radios {
    display: flex;
    gap: 0.55rem;
    font-size: 0.68rem;
  }

  .radio-label {
    display: flex;
    align-items: center;
    gap: 0.25rem;
    cursor: pointer;
    color: var(--text-secondary);
  }

  .radio-label.active-a { color: #38bdf8; font-weight: 700; }
  .radio-label.active-b { color: #f59e0b; font-weight: 700; }

  .btn-micro-csv {
    background: var(--bg-subtle);
    border: 1px solid var(--border-subtle);
    color: var(--text-secondary);
    font-size: 0.66rem;
    font-weight: 600;
    padding: 0.15rem 0.45rem;
    border-radius: 4px;
    cursor: pointer;
  }

  .btn-micro-csv:hover {
    background: #059669;
    color: #fff;
    border-color: #059669;
  }

  /* Chart Column */
  .chart-column {
    display: flex;
    flex-direction: column;
    gap: 0.75rem;
  }

  /* Deltas Card */
  .deltas-card {
    background: var(--bg-card);
    border: 1px solid var(--border-medium);
    border-radius: 8px;
    padding: 0.65rem 0.85rem;
  }

  .delta-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 0.45rem;
  }

  .delta-badge {
    font-size: 0.65rem;
    font-weight: 800;
    background: #6366f1;
    color: #fff;
    padding: 0.15rem 0.45rem;
    border-radius: 4px;
    letter-spacing: 0.5px;
  }

  .delta-vs {
    font-size: 0.75rem;
    font-weight: 600;
    color: var(--text-secondary);
  }

  .delta-metrics-row {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.5rem;
  }

  .delta-metric {
    display: flex;
    flex-direction: column;
    gap: 0.15rem;
  }

  .d-label {
    font-size: 0.66rem;
    color: var(--text-secondary);
    font-weight: 700;
  }

  .d-val {
    font-size: 0.78rem;
    font-weight: 700;
    font-family: 'JetBrains Mono', monospace;
  }

  .d-val.good { color: #22c55e; }
  .d-val.warn { color: #f59e0b; }

  /* Canvas Wrapper */
  .canvas-wrapper {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    border-radius: 8px;
    padding: 0.75rem;
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
  }

  .canvas-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 0.5rem;
  }

  .canvas-title {
    font-size: 0.78rem;
    font-weight: 700;
    color: var(--text-primary);
  }

  .chart-legend {
    display: flex;
    gap: 0.5rem;
  }

  .legend-chip {
    font-size: 0.7rem;
    padding: 0.15rem 0.5rem;
    border-radius: 4px;
    font-family: 'JetBrains Mono', monospace;
    font-weight: 600;
  }

  .chip-a {
    background: rgba(56, 189, 248, 0.15);
    color: #38bdf8;
    border: 1px solid rgba(56, 189, 248, 0.3);
  }

  .chip-b {
    background: rgba(245, 158, 11, 0.15);
    color: #f59e0b;
    border: 1px solid rgba(245, 158, 11, 0.3);
  }

  .canvas-container {
    position: relative;
    width: 100%;
    background: #0f172a;
    border-radius: 6px;
    overflow: hidden;
  }

  canvas {
    width: 100%;
    height: auto;
    display: block;
    cursor: crosshair;
  }

  .canvas-tooltip {
    position: absolute;
    top: 10px;
    right: 10px;
    background: rgba(15, 23, 42, 0.92);
    border: 1px solid #334155;
    border-radius: 6px;
    padding: 0.35rem 0.6rem;
    font-size: 0.72rem;
    font-family: 'JetBrains Mono', monospace;
    pointer-events: none;
    box-shadow: 0 4px 12px rgba(0,0,0,0.4);
  }

  .tooltip-time {
    color: #94a3b8;
    font-weight: 700;
    margin-bottom: 0.15rem;
  }

  .tooltip-row.a { color: #38bdf8; font-weight: 600; }
  .tooltip-row.b { color: #f59e0b; font-weight: 600; }

  .canvas-footer {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .footer-tip {
    font-size: 0.7rem;
    color: var(--text-secondary);
  }
</style>
