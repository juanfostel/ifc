#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PAM_FILES=("sddm" "login" "kde")
KWALLET_PAM_FILES=("sddm" "kde")
PAM_LINE='auth sufficient pam_exec.so quiet /usr/local/bin/futronic-auth verify --pam-user'
MARKER_BEGIN='# futronic-fs81 begin'
MARKER_END='# futronic-fs81 end'
KWALLET_MARKER_BEGIN='# futronic-kwallet begin'
KWALLET_MARKER_END='# futronic-kwallet end'

usage() {
  cat <<'EOF'
Uso:
  sudo ./scripts/install-cachy.sh [--pam sddm] [--pam login] [--no-pam] [--no-kwallet]

Por defecto instala el binario y agrega la huella como metodo suficiente en:
  /etc/pam.d/sddm
  /etc/pam.d/login
  /etc/pam.d/kde

Tambien agrega pam_kwallet5 en sddm/kde para abrir KDE Wallet cuando el
inicio de sesion use la contrasena normal.
EOF
}

require_root() {
  if [[ "$(id -u)" != "0" ]]; then
    echo "Ejecute con sudo." >&2
    exit 1
  fi
}

parse_args() {
  local custom_pam=0
  while [[ $# -gt 0 ]]; do
    case "$1" in
      --pam)
        [[ $# -ge 2 ]] || { usage; exit 2; }
        if [[ "$custom_pam" == 0 ]]; then
          PAM_FILES=()
          KWALLET_PAM_FILES=()
          custom_pam=1
        fi
        PAM_FILES+=("$2")
        if [[ "$2" == "sddm" || "$2" == "kde" ]]; then
          KWALLET_PAM_FILES+=("$2")
        fi
        shift 2
        ;;
      --no-pam)
        PAM_FILES=()
        KWALLET_PAM_FILES=()
        shift
        ;;
      --no-kwallet)
        KWALLET_PAM_FILES=()
        shift
        ;;
      -h|--help)
        usage
        exit 0
        ;;
      *)
        usage
        exit 2
        ;;
    esac
  done
}

install_binary() {
  make -C "$ROOT" install
}

patch_pam_file() {
  local name="$1"
  local path="/etc/pam.d/$name"
  local vendor_path="/usr/lib/pam.d/$name"

  if [[ ! -f "$path" ]]; then
    if [[ -f "$vendor_path" ]]; then
      install -m 0644 "$vendor_path" "$path"
      echo "Creado $path desde $vendor_path"
    else
      echo "Saltando $path: no existe." >&2
      return
    fi
  fi

  if grep -qF "$MARKER_BEGIN" "$path"; then
    echo "$path ya contiene la configuracion Futronic."
    return
  fi

  cp -a "$path" "$path.futronic-backup.$(date +%Y%m%d%H%M%S)"
  local tmp
  tmp="$(mktemp)"
  {
    echo "$MARKER_BEGIN"
    echo "$PAM_LINE"
    echo "$MARKER_END"
    cat "$path"
  } > "$tmp"
  install -m 0644 "$tmp" "$path"
  rm -f "$tmp"
  echo "Actualizado $path"
}

patch_kwallet_pam_file() {
  local name="$1"
  local path="/etc/pam.d/$name"
  local vendor_path="/usr/lib/pam.d/$name"

  if [[ ! -f "$path" ]]; then
    if [[ -f "$vendor_path" ]]; then
      install -m 0644 "$vendor_path" "$path"
      echo "Creado $path desde $vendor_path"
    else
      echo "Saltando $path: no existe." >&2
      return
    fi
  fi

  if grep -qF "$KWALLET_MARKER_BEGIN" "$path"; then
    echo "$path ya contiene la configuracion KWallet."
    return
  fi

  cp -a "$path" "$path.kwallet-backup.$(date +%Y%m%d%H%M%S)"
  local tmp
  tmp="$(mktemp)"
  {
    cat "$path"
    echo
    echo "$KWALLET_MARKER_BEGIN"
    echo "-auth optional pam_kwallet5.so"
    echo "-session optional pam_kwallet5.so auto_start"
    echo "$KWALLET_MARKER_END"
  } > "$tmp"
  install -m 0644 "$tmp" "$path"
  rm -f "$tmp"
  echo "Actualizado KWallet en $path"
}

main() {
  parse_args "$@"
  require_root
  install_binary

  for pam_file in "${PAM_FILES[@]}"; do
    patch_pam_file "$pam_file"
  done

  for pam_file in "${KWALLET_PAM_FILES[@]}"; do
    patch_kwallet_pam_file "$pam_file"
  done

  cat <<'EOF'

Listo.

Siguiente prueba recomendada:
  sudo futronic-auth enroll TU_USUARIO
  sudo futronic-auth verify TU_USUARIO

Para desbloqueo automatico de KDE:
  systemctl --user daemon-reload
  systemctl --user enable --now futronic-lockwatch.service

Para KWallet:
  sudo pacman -S --needed kwallet-pam

KWallet se abre automaticamente si la contrasena del wallet coincide con la
contrasena de usuario y el inicio de sesion pasa por contrasena. Si entra solo
con huella, PAM no tiene esa contrasena y KWallet todavia puede pedirla.

Abra otra TTY o mantenga una sesion root activa antes de probar el login grafico.
EOF
}

main "$@"
