# Script para iniciar la suite completa del Seguidor de Linea
# 1. Inicia el Backend en C# (.NET 10) en http://localhost:5000
# 2. Inicia el Frontend en Svelte (Vite) con pnpm en http://localhost:5173

$DOTNET = "$env:USERPROFILE\.dotnet\dotnet.exe"
if (!(Test-Path $DOTNET)) {
    $DOTNET = "dotnet"
}

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "   Iniciando LineFollower Pro 16 Dashboard                " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# Iniciar Backend
Write-Host "[1/2] Levantando Backend C# (.NET 10) en http://localhost:5000..." -ForegroundColor Yellow
$backendJob = Start-Process -FilePath $DOTNET -ArgumentList "run --project backend/LineFollower.Api.csproj --urls=http://localhost:5000" -PassThru -NoNewWindow

Start-Sleep -Seconds 2

# Iniciar Frontend
Write-Host "[2/2] Levantando Frontend Svelte en http://localhost:5173..." -ForegroundColor Yellow
Set-Location frontend
pnpm run dev
