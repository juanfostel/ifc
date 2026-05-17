#!/usr/bin/env bash
set -euo pipefail

AUTH_BIN="${FUTRONIC_AUTH_BIN:-/usr/local/bin/futronic-auth}"
CHECK_INTERVAL="${FUTRONIC_LOCKWATCH_INTERVAL:-1}"
VERIFY_TIMEOUT="${FUTRONIC_LOCKWATCH_TIMEOUT:-20}"
USER_NAME="${FUTRONIC_USER:-${USER:-$(id -un)}}"

find_session() {
  local uid sid session_uid user seat tty
  uid="$(id -u)"

  while read -r sid session_uid user seat tty; do
    [[ "$user" == "$USER_NAME" ]] || continue
    if [[ "$(loginctl show-session "$sid" -p Remote --value 2>/dev/null)" == "no" ]]; then
      echo "$sid"
      return 0
    fi
  done < <(loginctl list-sessions --no-legend 2>/dev/null)

  while read -r sid session_uid user seat tty; do
    [[ "$session_uid" == "$uid" || "$user" == "$USER_NAME" ]] || continue
    echo "$sid"
    return 0
  done < <(loginctl list-sessions --no-legend 2>/dev/null)

  return 1
}

is_locked() {
  local session="$1"
  [[ "$(loginctl show-session "$session" -p LockedHint --value 2>/dev/null)" == "yes" ]]
}

while true; do
  session="$(find_session || true)"
  if [[ -n "${session:-}" ]] && is_locked "$session"; then
    if timeout "$VERIFY_TIMEOUT" "$AUTH_BIN" verify "$USER_NAME"; then
      loginctl unlock-session "$session" || true
      sleep 2
    fi
  fi

  sleep "$CHECK_INTERVAL"
done
