#!/bin/sh

. /lib/functions.sh
. /lib/functions/commotion.sh
. /lib/config/uci.sh
. ../netifd-proto.sh
. /lib/firewall/core_interface.sh
init_proto "$@"


WIFI_DEVICE=
TYPE=

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
	local profile type ip netmask dns domain ssid bssid channel mode wpa wpakey announce have_ip lease_zone nolease_zone
	json_get_vars profile type ip netmask dns domain ssid bssid channel mode wpa wpakey announce lease_zone nolease_zone

	commotion_up "$iface" $(uci_get network $config profile)
	logger -t "commotion.proto" -s "Upped"
	type=${type:-$(commotion_get_type $iface)}
	logger -t "commotion.proto" -s "Type: $type"

	if [ "$type" = "plug" ]; then 
		local dhcp_status
		export DHCP_INTERFACE="$config"
		udhcpc -q -i ${iface} -t 2 -T 5 -n -s /lib/netifd/commotion.dhcp.script
		dhcp_status=$?
		export DHCP_INTERFACE=""
		if [ $dhcp_status -eq 0 ]; then
			# we got an IP
			# see commotion.dhcp.script for the rest of
			# the setup code.
			have_ip=1
			uci_set_state network "$config" lease 2

			# get out of here early.
			return
		else
			unset_fwzone "$config"
			uci_set_state network "$config" lease 1
			set_fwzone "$config" "$(uci_get network "$config" nolease_zone "$DEFAULT_NOLEASE_ZONE")"
			uci_commit firewall
		fi
	fi
	proto_init_update "*" 1

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

