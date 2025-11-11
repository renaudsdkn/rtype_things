#!/bin/bash

# Crée le dossier logs s'il n'existe pas
mkdir -p logs

# Génère un nom de fichier avec timestamp
LOGFILE="logs/server_$(date +%Y%m%d_%H%M%S).log"

echo "🚀 Lancement du serveur R-Type..."
echo "📝 Logs sauvegardés dans: $LOGFILE"

# Lance le serveur avec logs
./build/rtype/server/r-type_server 2>&1 | tee "$LOGFILE"

echo ""
echo "✅ Serveur arrêté. Logs disponibles dans: $LOGFILE"