# Futronic FS81 login para CachyOS / Linux

Este repositorio contiene una integración práctica para usar un lector de huellas **Futronic FS81/FS80H** en Linux cuando no hay soporte nativo en `fprintd/libfprint`.

El objetivo es simple: poder iniciar sesión, desbloquear KDE Plasma y validar el usuario con la huella usando las librerías Linux oficiales de Futronic (`ftrapi.so` y `libScanAPI.so`).

Fue probado en:

- CachyOS / Omarchy
- KDE Plasma 6
- SDDM
- Futronic FS81

## Qué resuelve

El lector Futronic FS81 suele funcionar con el SDK de Futronic en Linux, pero no aparece como dispositivo soportado por `fprintd`. Eso deja una parte importante sin resolver: integrarlo con el login del sistema.

Este proyecto agrega:

- `futronic-auth`: herramienta CLI para enrolar y verificar huellas.
- Integración PAM para `sddm`, `login` y KDE lockscreen.
- `futronic-lockwatch`: servicio de usuario que desbloquea KDE automáticamente al apoyar el dedo.
- Instalador para CachyOS/Arch.

## Qué no incluye

Por licencia y seguridad, el repo **no incluye** las librerías propietarias de Futronic:

```text
ftrapi.so
libScanAPI.so
```

Debes obtenerlas desde el SDK/demo Linux de Futronic y pasarle la ruta al instalador.

## Instalación rápida

```bash
git clone https://github.com/juanfostel/ifc.git
cd ifc/futronic-linux-login

sudo pacman -S --needed base-devel pam libusb-compat
sudo env SDK_DIR=/ruta/al/Linux_gtk_demo_x64 bash ./scripts/install-cachy.sh
```

Ejemplo real:

```bash
sudo env SDK_DIR=/home/juan/futronic-fs81/gtkdemo/Linux_gtk_demo_x64 bash ./scripts/install-cachy.sh
```

## Enrolar y verificar

```bash
sudo futronic-auth enroll TU_USUARIO
sudo futronic-auth verify TU_USUARIO
```

Si la verificación funciona, PAM ya puede usar la huella.

## Desbloqueo automático de KDE

Para que KDE se desbloquee apenas apoyas el dedo, sin tocar el botón de desbloquear:

```bash
systemctl --user daemon-reload
systemctl --user enable --now futronic-lockwatch.service
```

Ver logs:

```bash
journalctl --user -u futronic-lockwatch.service -f
```

Desactivar:

```bash
systemctl --user disable --now futronic-lockwatch.service
```

## Seguridad

El binario `futronic-auth` se instala como `setuid root` porque KDE lockscreen corre como usuario normal y necesita acceder al lector y a las plantillas guardadas en `/var/lib/futronic-fs81`.

Para reducir riesgo:

- `enroll` sólo funciona como root.
- `verify` sólo permite verificar al usuario real que ejecuta el proceso.
- PAM está configurado como `auth sufficient`, por lo que la contraseña normal queda como respaldo.

Aun así, esto toca autenticación del sistema. Antes de probar login gráfico o lockscreen, mantén una TTY o terminal root abierta.

## Revertir

El instalador crea backups de PAM con formato similar a:

```text
/etc/pam.d/sddm.futronic-backup.YYYYMMDDHHMMSS
```

También puedes desactivar el watcher:

```bash
systemctl --user disable --now futronic-lockwatch.service
```

Y quitar manualmente los bloques marcados:

```text
# futronic-fs81 begin
...
# futronic-fs81 end
```

## Carpeta principal

El proyecto está en:

```text
futronic-linux-login/
```

Ahí está el README técnico con más detalles de instalación y uso.

## Estado

Funciona como integración comunitaria para Futronic FS81 en CachyOS/KDE. No es un driver oficial ni soporte nativo `fprintd`, pero permite usar el lector para login y desbloqueo en sistemas donde el SDK de Futronic ya logra capturar huellas.
