# Script para ejecutar LineFollower Pro en modo de desarrollo Desktop Nativo (Tauri)
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "   Iniciando LineFollower Pro - Desktop Suite (Tauri)     " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$env:PATH = "$env:USERPROFILE\.cargo\bin;$env:PATH"

# Verificar que cargo esté disponible
if (-not (Get-Command cargo -ErrorAction SilentlyContinue)) {
    Write-Host "[ERROR] El compilador de Rust (cargo) no se encuentra en el PATH." -ForegroundColor Red
    exit 1
}

# Iniciar la aplicación nativa en ventana de escritorio
Set-Location "$PSScriptRoot\frontend"
pnpm tauri dev
