#!/bin/sh

. /lib/functions.sh
. /lib/config/uci.sh
. /usr/share/libubox/jshn.sh

#DEBUG=echo
CLIENT="/usr/bin/commotion"
SOCKET="/var/run/commotiond.sock"

DEFAULT_WAN_ZONE="wan"
DEFAULT_CLIENT_BRIDGE="client"

unset_bridge() {
  local bridge="$1"
  local ifname="$2"
  
  json_init
  json_add_string name "$ifname"
  ubus call network.interface.$bridge remove_device "$(json_dump)" 2>/dev/null
  
  return $?
}
  
set_bridge() {
  local bridge="$1"
  local ifname="$2"
  
  json_init
  json_add_string name "$ifname"
  ubus call network.interface.$bridge add_device "$(json_dump)" 2>/dev/null
  
  return $?
}

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

commotion_gen_ip() {
  local subnet="$1"  
  local netmask="$2"     
  local gw="$3"     
             
  data="$("$CLIENT" -b "$SOCKET" genip "$subnet" "$netmask" "$gw"  2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?                                                                        
        
  json_load "$data"
  json_get_var ipaddr address
  echo "$ipaddr"        
  return "$ret" 
} 

commotion_get_ip() {
  local iface="$1"  
  local data=     
             
  data="$("$CLIENT" -b "$SOCKET" state "$iface" ip 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?                                                                        
        
  json_load "$data"
  json_get_var ipaddr ip
  echo "$ipaddr"        
  return "$ret" 
} 

commotion_get_netmask() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface netmask 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
  
  json_load "$data"
  json_get_var nm netmask  
  echo "$nm"
  return "$ret"
}

commotion_get_ssid() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface ssid 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
  
  json_load "$data"
  json_get_var id ssid  
  echo "$id"
  return "$ret"
}

commotion_get_bssid() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface bssid 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
          
  json_load "$data"   
  json_get_var id bssid
  echo "$id"       
  return "$ret"    
}

commotion_get_channel() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface channel 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
          
  json_load "$data"   
  json_get_var chan channel
  echo "$chan"     
  return "$ret"    
}  

commotion_get_dns() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface dns 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
          
  json_load "$data"   
  json_get_var namesrv dns
  echo "$namesrv"  
  return "$ret"    
}   

commotion_get_domain() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface domain 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
          
  json_load "$data"   
  json_get_var dom domain
  echo "$dom"      
  return "$ret"    
}                            
  
commotion_get_wpa() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface wpa 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
          
  json_load "$data"   
  json_get_var w wpa
  echo "$w"            
  return "$ret"    
} 

commotion_get_wpakey() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface wpakey 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
          
  json_load "$data"   
  json_get_var key wpakey
  echo "$key"      
  return "$ret"    
} 

commotion_get_keyring() {
  local iface="$1"
  local data=

  #data="$($CLIENT -b $SOCKET state $iface mdp_keyring 2>/dev/null)"
  data="$($CLIENT -b $SOCKET get global mdp_keyring 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
          
  json_load "$data"   
  json_get_var keyring mdp_keyring
  echo "$keyring"       
  return "$ret"    
}

commotion_get_sid() {
  local iface="$1"
  local data=

  #data="$($CLIENT -b $SOCKET state $iface mdp_sid 2>/dev/null)"
  data="$($CLIENT -b $SOCKET get global mdp_sid 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
          
  json_load "$data"   
  json_get_var sid mdp_sid
   echo "$sid"       
  return "$ret"    
}

commotion_get_servald() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET get global servald 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
          
  json_load "$data"   
  json_get_var sd servald
  echo "$sd"       
  return "$ret"    
}

commotion_get_mode() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface mode 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
          
  json_load "$data"   
  json_get_var md mode
  echo "$md"       
  return "$ret"    
}

commotion_get_class() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface type 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
          
  json_load "$data"   
  json_get_var tp type
  echo "$tp"       
  return "$ret"    
} 

commotion_get_nodeid() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET nodeid 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
          
  json_load "$data"   
  json_get_var nodeid id
  echo "$nodeid"   
  return "$ret"    
} 

commotion_get_profile() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET status $iface 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
          
  json_load "$data"   
  json_get_var st status
  echo "$st"       
  return "$ret"    
} 

commotion_get_announce() {
  local iface="$1"
  local data=

  data="$($CLIENT -b $SOCKET state $iface announce 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
          
  json_load "$data"   
  json_get_var ance announce
  echo "$ance"     
  return "$ret"    
} 

commotion_set_nodeid_from_mac() {
  local mac="$1"
  local data=
                                                               
  data="$($CLIENT -b $SOCKET nodeid mac $mac 2>/dev/null)"                        
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?
                   
  json_load "$data"      
  json_get_var $id nodeid
  echo "$id"       
  return "$ret"    
}  

commotion_up() {            
  local iface="$1"
  local profile="$2"
  local data=              
                                 
  data="$($CLIENT -b $SOCKET up $iface $profile 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?                      

  json_load "$data"
  json_get_var ifc "$iface"
  echo "$ifc"      
  return "$ret"
}          
                                
commotion_down() {                                                            
  local iface="$1"
  local data=
 
  data="$($CLIENT -b $SOCKET down $iface 2>/dev/null)"
  [[ -z "$data" -o "$?" != 0 ]] || echo "$data" | grep -qs "Failed*" && return 1
  ret=$?           
                   
  json_load "$data"
  json_get_var ifc "$iface"
  echo "$ifc"                   
  return "$ret"                                                            
}
