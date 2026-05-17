# Futronic FS81 Login para Cachy Linux

Este proyecto integra el lector Futronic FS81/FS80H con el login de Cachy Linux usando las librerias Linux de Futronic que ya estan en este workspace:

- `ftrapi.so`: enrolamiento y verificacion de plantillas.
- `libScanAPI.so`: captura USB del lector.
- `pam_exec.so`: puente PAM simple para SDDM/login.

La idea es deliberadamente conservadora: primero se prueba `futronic-auth` en terminal y recien despues PAM lo llama durante el inicio de sesion.

## Requisitos en Cachy

```bash
sudo pacman -S --needed base-devel pam libusb-compat
```

Si tu instalacion no tiene `libusb-compat`, el SDK viejo de Futronic puede fallar al abrir el lector.

## Instalar

Desde la carpeta `futronic-linux-login`:

```bash
sudo ./scripts/install-cachy.sh
```

Por defecto instala en `/opt/futronic-fs81`, crea el enlace `/usr/local/bin/futronic-auth` y agrega una regla marcada en:

- `/etc/pam.d/sddm`
- `/etc/pam.d/login`
- `/etc/pam.d/kde`

Si `/etc/pam.d/kde` no existe pero `/usr/lib/pam.d/kde` si, el instalador crea una copia local antes de agregar la regla. Esto es habitual en sistemas Arch/Cachy donde algunos archivos PAM vienen como configuracion vendor.

Para instalar sin tocar PAM:

```bash
sudo ./scripts/install-cachy.sh --no-pam
```

Para elegir un archivo PAM especifico:

```bash
sudo ./scripts/install-cachy.sh --pam sddm
```

## Enrolar y probar

Mantenga una sesion root o una TTY abierta antes de probar login, por si necesita revertir PAM.

```bash
sudo futronic-auth enroll TU_USUARIO
sudo futronic-auth verify TU_USUARIO
```

La plantilla queda en:

```text
/var/lib/futronic-fs81/TU_USUARIO.tpl
```

con permisos `0600`.

## Como funciona PAM

La linea instalada es:

```text
auth sufficient pam_exec.so quiet /usr/local/bin/futronic-auth verify --pam-user
```

`sufficient` significa que una huella valida permite continuar sin pedir password. Si no hay plantilla, no coincide la huella o el lector falla, PAM sigue con el metodo normal que ya tenia el sistema.

## Revertir PAM

El instalador crea backups con este formato:

```text
/etc/pam.d/sddm.futronic-backup.YYYYMMDDHHMMSS
```

Tambien puede quitar manualmente este bloque:

```text
# futronic-fs81 begin
auth sufficient pam_exec.so quiet /usr/local/bin/futronic-auth verify --pam-user
# futronic-fs81 end
```

## Notas de seguridad

- La huella no reemplaza totalmente una politica de seguridad: deja password como respaldo.
- Proteja `/var/lib/futronic-fs81`; contiene plantillas biometricas, no imagenes crudas, pero siguen siendo datos sensibles.
- Pruebe primero en `login` o en otra TTY antes de cerrar la sesion grafica.
- El binario se instala setuid root para que KDE lockscreen pueda leer plantillas y abrir el lector desde PAM. Por eso `enroll` queda bloqueado salvo para root, y `verify` solo permite verificar al usuario real que lo ejecuta.
