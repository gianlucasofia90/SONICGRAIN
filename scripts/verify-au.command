#!/bin/bash
# Validazione Apple dell'Audio Unit installato. Risultato in auval_log.txt nella cartella del progetto.
cd "$(dirname "$0")/.."
exec > >(tee auval_log.txt) 2>&1
killall -9 AudioComponentRegistrar >/dev/null 2>&1; sleep 2
echo "=== SONIC GRAIN (aufx Sgrn Gsof)"; auval -v aufx Sgrn Gsof | tail -25
echo "=== fine"
