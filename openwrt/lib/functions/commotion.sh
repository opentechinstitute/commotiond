#!/bin/sh

. /lib/functions.sh
. /lib/config/uci.sh

#DEBUG=echo
CLIENT="/usr/bin/commotion"
SOCKET="/var/run/commotiond.sock"

DEFAULT_LEASE_ZONE="wan"
DEFAULT_NOLEASE_ZONE="lan"

unset_fwzone() {
  local config="$1"

  reset_cb
  config_load firewall
  config_cb() {
    local type="$1"
    local name="$2"
    [ "$type" = "zone" ] && {
        local networks="$(uci_get firewall "$name" network)"
        uci_remove firewall "$name" network
        for net in $networks; do
          [ "$net" != "$config" ] && uci add_list firewall."$name".network="$net"
        done
    }
  }
  config_load firewall

  return 0
}

set_fwzone() {
  local config="$1"
  local zone="$2"

  reset_cb
  config_load firewall
  config_cb() {
    local type="$1"
    local name="$2"
    [ "$type" = "zone" ] && {
        local fwname=$(uci_get firewall "$name" name)
        [ "$fwname" = "$zone" ] && {
            uci add_list firewall."$name".network="$config"
        }
    }
  }
  config_load firewall

  return 0
}

commotion_get_ip() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface ip 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_get_netmask() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface netmask 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_get_ssid() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface ssid 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_get_bssid() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface bssid 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_get_channel() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface channel 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_get_dns() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface dns 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_get_domain() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface domain 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_get_wpa() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface wpa 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_get_wpakey() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface wpakey 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_get_servald() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface servald 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_get_mode() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface mode 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_get_type() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface type 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_get_nodeid() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET nodeid 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_get_profile() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET status $iface 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_get_announce() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface announce 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_set_nodeid_from_mac() {
  local mac="$1"
  local data=

  data="$($CLIENT -b $SOCKET nodeidset $mac 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_up() {
  local iface="$1"
  local profile="$2"
  local data=

  data="$($CLIENT -b $SOCKET up $iface $profile 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}

commotion_down() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET up $iface 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  
  echo "$data"
  return 0
}
