#!/usr/bin/env bash
# ==============================================================================
# LineFollowerPro — Script de Gestión de Servicios Web Self-Hosted
# ==============================================================================
set -e

ACTION="${1:-status}"

case "$ACTION" in
    start)
        echo "==> Iniciando servicios de LineFollowerPro..."
        sudo systemctl start linefollower-backend.service linefollower-frontend.service
        sudo systemctl status linefollower-backend.service linefollower-frontend.service --no-pager
        ;;
    stop)
        echo "==> Deteniendo servicios de LineFollowerPro..."
        sudo systemctl stop linefollower-frontend.service linefollower-backend.service
        echo "Servicios detenidos."
        ;;
    restart)
        echo "==> Reiniciando servicios de LineFollowerPro..."
        sudo systemctl restart linefollower-backend.service linefollower-frontend.service
        sudo systemctl status linefollower-backend.service linefollower-frontend.service --no-pager
        ;;
    status)
        echo "==> Estado de servicios:"
        sudo systemctl status linefollower-backend.service linefollower-frontend.service --no-pager
        echo ""
        echo "==> Verificando puertos:"
        echo -n "Backend  (5010): " && (curl -s -o /dev/null -w "%{http_code}\n" http://localhost:5010/ || echo "INACTIVO")
        echo -n "Frontend (5011): " && (curl -s -o /dev/null -w "%{http_code}\n" http://localhost:5011/ || echo "INACTIVO")
        ;;
    logs)
        echo "==> Mostrando logs en vivo (Ctrl+C para salir)..."
        sudo journalctl -u linefollower-backend.service -u linefollower-frontend.service -f
        ;;
    logs-backend)
        sudo journalctl -u linefollower-backend.service -f
        ;;
    logs-frontend)
        sudo journalctl -u linefollower-frontend.service -f
        ;;
    *)
        echo "Uso: $0 {start|stop|restart|status|logs|logs-backend|logs-frontend}"
        exit 1
        ;;
esac
