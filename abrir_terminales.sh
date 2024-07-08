#!/bin/bash

# Función para abrir una nueva terminal y ejecutar un comando
abrir_terminal_y_ejecutar() {
    gnome-terminal -- bash -c "$1; exec bash"
}

# Cambiar a la carpeta del script
cd "$(dirname "$0")"

# Abrir las terminales para ejecutar ./neurona.exe con diferentes puertos
for port in {45000..45004}; do
    abrir_terminal_y_ejecutar "./neurona.exe $port"
done

# Esperar unos segundos para que las neuronas se inicien
sleep 3

# Crear un script expect temporal
expect_script=$(mktemp)
cat << EOF > $expect_script
#!/usr/bin/expect -f
spawn ./cli.exe
expect "prompt>"  ; # Ajusta esto al prompt que se muestra en cli.exe
send "tableros\r"
sleep 2
send "entrena\r"
sleep 2
send "entrena\r"
sleep 2
send "entrena\r"
sleep 2
interact
EOF

# Hacer el script expect temporal ejecutable
chmod +x $expect_script

# Ejecutar el script expect
gnome-terminal -- bash -c "$expect_script; rm $expect_script; exec bash"
