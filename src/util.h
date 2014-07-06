/* vim: set ts=2 expandtab: */
/**
 *       @file  util.h
 *      @brief  utility functions for the Commotion daemon
 *
 *     @author  Josh King (jheretic), jking@chambana.net
 *
 *   @internal
 *     Created  03/07/2013
 *    Revision  $Id: doxygen.commotion.templates,v 0.1 2013/01/01 09:00:00 jheretic Exp $
 *    Compiler  gcc/g++
 *     Company  The Open Technology Institute
 *   Copyright  Copyright (c) 2013, Josh King
 *
 * This file is part of Commotion, Copyright (c) 2013, Josh King 
 * 
 * Commotion is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * Commotion is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Commotion.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================================
 */

#ifndef _UTIL_H
#define _UTIL_H
#include <stdlib.h>
#include <stdint.h>

#define NEW(O, T) O##_create(sizeof(T##_t), T##_proto)
#define _(N) proto.N

#define LOWERCASE(N) for ( ; *N; ++N) *N = tolower(*N);
#define UPPERCASE(N) for ( ; *N; ++N) *N = toupper(*N);

#define MAX_ARGS 16

#define BSSID_SIZE 6
#define BSSID_STR_SIZE 18
#define ESSID_SIZE 32
#define FREQ_SIZE 4
#define CHAN_SIZE 2

/**
 * @brief concatenates two strings
 * @param dst destination for new string
 * @param src string to be added to
 * @param size size of combined strings
 */
size_t strlcat(char *dst, const char *src, const size_t size);

/**
 * @brief copies a string
 * @param dst destination for new string
 * @param src string to be copied
 * @param size size of string
 */
size_t strlcpy(char *dest, const char *src, const size_t size);

/**
 * @brief prints output from string "str" in a specified format
 * @param str string to be printed
 * @param size size of string
 * @param format output format
 */
size_t snprintfcat(char *str, size_t size, const char *format, ...);

/**
 * @brief prints output from string "str" in a specified format
 * @param str string to be printed
 * @param size size of string
 * @param format output format
 * @param args a va_list
 */
size_t vsnprintfcat(char *str, size_t size, const char *format, va_list args);

/**
 * @brief removes white space from a given string (to parse for arguments)
 * @param s string to parse
 * @param out output from the string (with white space removed)
 * @param outlen length of the output
 */
size_t strstrip(const char *s, char *out, const size_t outlen);

/**
 * @brief compares version numbers of two software releases
 * @param aver version number for software 'a'
 * @param bver version number for software 'b'
 */
int compare_version(const char * aver, const char *bver);

typedef int (*file_iter)(const char* path, const char *filename);

/**
 * @brief processes file paths 
 * @param dir_path string of the directory path
 * @param loader number of directories in the file path
 */
int process_files(const char *dir_path, file_iter loader);

/**
 * @brief parses a string and converts into a set of arguments 
 * @param input the string to be parsed
 * @param argv pointer to argument list
 * @param argc number of arguments read from the string
 * @param max maximum length of string
 */
int string_to_argv(const char *input, char **argv, int *argc, const size_t max);

/**
 * @brief converts argument vectors to a single string
 * @param argv pointer to argument list
 * @param argc number of arguments
 * @param output concatenated string of arguments
 * @param max maximum length of the string
 */
int argv_to_string(char **argv, const int argc, char *output, const size_t max);

/**
 * @brief converts a MAC address from a string to individual bytes
 * @param macstr MAC string
 * @param mac an array for storing the MAC address
 */
void mac_string_to_bytes(char *macstr, unsigned char mac[6]);

/**
 * @brief prints MAC address from MAC array
 * @param mac array storing MAC address
  */
void print_mac(unsigned char mac[6]);

/**
 * @brief sets Wi-Fi frequency corresponding to specified channel
 * @param channel specified channel
  */
int wifi_freq(const int channel);

/**
 * @brief sets Wi-Fi channel corresponding to specified freuency
 * @param frequency specified frequency
  */
int wifi_chan(const int frequency);

/**
* @brief generates a BSSID from hash of ESSID and channel
* @param essid The ESSID to hash
* @param channel an integer of the channel
* @param bbsid The returned 6-byte BSSID
*/
void get_bssid(const char *essid, const unsigned int channel, char *bssid);

/**
 * @brief prints a raw byte array in hex and ascii output
 * @param mem the byte array to print
 * @param len length of the byte array
 */
void hexdump(void *mem, unsigned int len);

#endif
