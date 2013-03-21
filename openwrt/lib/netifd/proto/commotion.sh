#!/bin/sh

. /lib/functions.sh
. /lib/functions/commotion.sh
. /lib/config/uci.sh
. ../netifd-proto.sh
init_proto "$@"


WIFI_DEVICE=
TYPE=
DEFAULT_LEASE_ZONE="wan"
DEFAULT_NOLEASE_ZONE="lan"

configure_wifi_iface() {
	local config="$1"
	local network="$2"
	local ssid="$3"
	local mode="$4"
	local wpakey="$5"
	local wpa="$6"
	#local bssid="$6"
	
	local thisnetwork=
	config_get thisnetwork "$config" network	
	[[ "$thisnetwork" == "$network" ]] && {
		uci_set wireless "$config" ssid "$ssid"
		#uci_set wireless "$config" bssid "$bssid"
		uci_set wireless "$config" mode "$mode"
		[[ "$wpa" == "true" ]] && {
			uci_set wireless "$config" encryption "psk2"
			uci_set wireless "$config" key "$wpakey"
		}
		config_get WIFI_DEVICE "$config" device
	}
}

configure_wifi_device() {
	local config="$1"
	local thisradio="$2"	
	local channel="$3"
	
	[[ "$config" == "$thisradio" ]] && {
		config_set "$config" channel "$channel"
	}
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

proto_commotion_init_config() {
	proto_config_add_string "profile"
	proto_config_add_string "type"
	proto_config_add_string "ip"
	proto_config_add_string "netmask"
	proto_config_add_string "ssid"
	proto_config_add_string "bssid"
	proto_config_add_string "channel"
	proto_config_add_string "dns"
	proto_config_add_string "domain"
	proto_config_add_string "wpa"
	proto_config_add_string "wpakey"
}

proto_commotion_setup() {
	local config="$1"
	local iface="$2"
	local have_ip=0

	logger -s -t commotion.proto "Running protocol handler."
	proto_init_update "*" 1
	local profile type ip netmask dns domain ssid bssid channel mode wpa wpakey announce have_ip lease_zone nolease_zone
	json_get_vars profile type ip netmask dns domain ssid bssid channel mode wpa wpakey announce lease_zone nolease_zone
	
	commotion_up "$iface" $(uci_get network $config profile)
	type=${type:-$(commotion_get_type $iface)}
	logger -t "commotion.proto" -s "Type: $type"


	if [ "$type" = "plug" ]; then 
    		unset_fwzone "$config"
		udhcpc -i ${iface} -t 2 -T 5 -n
		if [ $? -eq 0 ]; then
			# we got an IP
			have_ip=1
			uci_set_state network "$config" lease 0
			set_fwzone "$config" "$(uci_get network "$config" lease_zone "$DEFAULT_LEASE_ZONE")"	
		else
			uci_set_state network "$config" lease 1
			set_fwzone "$config" "$(uci_get network "$config" nolease_zone "$DEFAULT_NOLEASE_ZONE")"	
		fi
	fi

	if [ $have_ip -eq 0 ]; then
		local ip=${ip:-$(commotion_get_ip $iface)} 
		local netmask=${netmask:-$(commotion_get_netmask $iface)}
		proto_add_ipv4_address $ip $netmask
		uci_set_state network "$config" ipaddr "$ip"
		uci_set_state network "$config" netmask "$netmask"
		logger -t "commotion.proto" -s "proto_add_ipv4_address: ${ip:-$(commotion_get_ip $iface)} ${netmask:-$(commotion_get_netmask $iface)}"
		proto_add_dns_server "${dns:-$(commotion_get_dns $iface)}"
		logger -t "commotion.proto" -s "proto_add_dns_server: ${dns:-$(commotion_get_dns $iface)}"
		proto_add_dns_search ${domain:-$(commotion_get_domain $iface)}
		logger -t "commotion.proto" -s "proto_add_dns_search: ${domain:-$(commotion_get_domain $iface)}"
	fi
	
	proto_export "INTERFACE=$config"
	proto_export "TYPE=$type"
	proto_export "MODE=${mode:-$(commotion_get_mode $iface)}"
	proto_export "ANNOUNCE=${announce:-$(commotion_get_announce $iface)}"

	if [ "$type" != "plug" ]; then
		config_load wireless
		#config_foreach configure_wifi_iface wifi-iface $config ${ssid:-$(commotion_get_ssid $iface)} ${bssid:-$(commotion_get_bssid $iface)} ${mode:-$(commotion_get_mode $iface)} ${wpakey:-$(commotion_get_wpakey $iface)}
		config_foreach configure_wifi_iface wifi-iface $config ${ssid:-$(commotion_get_ssid $iface)} ${mode:-$(commotion_get_mode $iface)} ${wpakey:-$(commotion_get_wpakey $iface)} ${wpa:-$(commotion_get_wpa $iface)}
		uci_set wireless $WIFI_DEVICE channel ${channel:-$(commotion_get_channel $iface)}
    		uci_commit wireless
    		wifi up "$config"
	fi
	logger -t "commotion.proto" -s "Sending update for $config"
	proto_send_update "$config"
}

proto_commotion_teardown() {
	local interface="$1"
	proto_kill_command "$interface"
}

add_protocol commotion

