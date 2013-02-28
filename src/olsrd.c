/*
 * =====================================================================================
 *
 *       Filename:  olsrd.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/28/2013 01:38:33 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

static olsrd_conf_item_t global_items[];

static olsrd_conf_plugin_t plugins[];

static list_t *ifaces;

static list_t *hna;

static int _co_print_global_item_i
