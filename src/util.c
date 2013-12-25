/* vim: set ts=2 expandtab: */
/**
 *       @file  util.c
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
 * it under the terms of the GNU General Public License as published 
 * by the Free Software Foundation, either version 3 of the License, 
 * or (at your option) any later version.
 * 
 * Commotion is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Commotion.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include "debug.h"
#include "util.h"
#include "extern/md5.h"



size_t strlcpy(char *dest, const char *src, const size_t size)
{
	const size_t length = strlen(src);
	if (size != 0) {
		memmove(dest, src, (length > size - 1) ? size - 1 : length);
		dest[size - 1] = '\0';
	}
	return length;
}

size_t strlcat(char *dst, const char *src, const size_t size)
{
  size_t used, length, copy;

  used = strlen(dst);
  length = strlen(src);
  if (size > 0 && used < size - 1) {
    copy = (length >= size - used) ? size - used - 1 : length;
    memcpy(dst + used, src, copy);
    dst[used + copy] = '\0';
  }
  return used + length;
}

size_t snprintfcat(char *str, size_t size, const char *format, ...) {
  size_t result;
  va_list args;
  size_t len = strnlen(str, size);

  va_start(args, format);
  result = vsnprintf(str + len, size - len, format, args);
  va_end(args);

  return result + len;
}
/*
int strstrip(char *s) {
  size_t len = strlen(s);
  char *ptr = s;

  while(isspace(ptr[len-1])) ptr[--len] = '\0';
  while(*ptr && isspace(*ptr)) {
    ++ptr; 
    --len;
  }

  memmove(s, ptr, len + 1);
  return len;
}
*/

size_t strstrip(const char *s, char *out, size_t outlen) {
  size_t len = strlen(s);
  const char *ptr = s;
  if(len == 0) return 0;

  while(*ptr && isspace(*ptr)) {
    ++ptr; 
    --len;
  }

  if(len <= outlen) {
    strlcpy(out, ptr, len + 1);
    return len;
  } else return -1;
}

int compare_version(const char *aver, const char *bver) {
  char *a, *b, *anext, *bnext;
  long int avalue, bvalue;

  // Grab pointer to first number in each version string.
  a = strpbrk(aver, "0123456789");
  b = strpbrk(bver, "0123456789");

  while(a && b) {
    // Convert characters to number.
    avalue = strtol(a, &anext, 10);
    bvalue = strtol(b, &bnext, 10);

    if(avalue < bvalue) return -1;
    if(avalue > bvalue) return 1;

    a = strpbrk(anext, "0123456789");
    b = strpbrk(bnext, "0123456789");
  }

  // If valid characters remain in 1 string, assume a point release.
  if(b) return -1;
  if(a) return 1;

  // Versions are identical.
  return 0;
}

int process_files(const char *dir_path, file_iter loader) {
  size_t path_size = strlen(dir_path);
  CHECK((path_size > 0) && (path_size <= PATH_MAX), "Invalid path length!");
  DIR *dir_iter = NULL;
  CHECK((dir_iter = opendir(dir_path)), "Could not read directory!");
  struct dirent *dir_entry = NULL;
  DEBUG("Processing files in directory %s", dir_path);
  
  while((dir_entry = readdir(dir_iter)) != NULL) {
    DEBUG("Checking file %s", dir_entry->d_name);
    if(!strcmp(dir_entry->d_name, ".")) continue;
    if(!strcmp(dir_entry->d_name, "..")) continue;
    if(!(*loader)(dir_path, dir_entry->d_name)) break;
  }

  if(dir_iter) closedir(dir_iter);
  return 1;

error:
  return 0;
}

int string_to_argv(const char *input, char **argv, int *argc, const size_t max) {
  int count = 0;
	char *saveptr = NULL;
	char *token = NULL;
  char *input_tmp = strdup(input);
	CHECK_MEM(token = strtok_r(input_tmp, " ", &saveptr));
  while(token && count < max) {
    argv[count++] = token;
	  token = strtok_r(NULL, " ", &saveptr);
  }
  *argc = count;
  return 1; 
error:
  free(input_tmp);
  return 0;
}

int argv_to_string(char **argv, const int argc, char *output, const size_t max) {
  int i;
  for(i = 0; i < argc; i++) {
    strlcat(output, argv[i], max);
    strlcat(output, " ", max);
  }
  if(i < argc) {
    ERROR("Failed to concatenate all of argv.");
    return 0;
  } 
  return 1; 
}

void mac_string_to_bytes(char *macstr, unsigned char mac[6]) {
  memset(mac, '\0', 6);
  sscanf(macstr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
  return;
}

void print_mac(unsigned char mac[6]) {
  printf("%02x:%02x:%02x:%02x:%02x:%02x", mac[0] & 0xff, mac[1] & 0xff, mac[2] & 0xff, mac[3] & 0xff, mac[4] & 0xff, mac[5] & 0xff);
  return;
}

int wifi_freq(const int channel) {
  if(channel > 14) {
    return 5000 + 5*channel;
  }
  switch(channel) {
    case 1:
      return 2412;
      break;
    case 2:
      return 2417;
      break;
    case 3:
      return 2422;
      break;
    case 4:
      return 2427;
      break;
    case 5:
      return 2432;
      break;
    case 6:
      return 2437;
      break;
    case 7:
      return 2442;
      break;
    case 8:
      return 2447;
      break;
    case 9:
      return 2452;
      break;
    case 10:
      return 2457;
      break;
    case 11:
      return 2462;
      break;
    case 12:
      return 2467;
      break;
    case 13:
      return 2472;
      break;
    case 14:
      return 2484;
      break;
    default:
      ERROR("Not a valid channel!");
      return 0;
  }
}

int wifi_chan(const int frequency) {
  if(frequency < 5080) {
    return (frequency - 5000)/5;
  }
  switch(frequency) {
    case 2412:
      return 1;
      break;
    case 2417:
      return 2;
      break;
    case 2422:
      return 3;
      break;
    case 2427:
      return 4;
      break;
    case 2432:
      return 5;
      break;
    case 2437:
      return 6;
      break;
    case 2442:
      return 7;
      break;
    case 2447:
      return 8;
      break;
    case 2452:
      return 9;
      break;
    case 2457:
      return 10;
      break;
    case 2462:
      return 11;
      break;
    case 2467:
      return 12;
      break;
    case 2472:
      return 13;
      break;
    case 2484:
      return 14;
      break;
    default:
      ERROR("Not a valid frequency!");
      return 0;
  }

}

void get_bssid(const char *essid, const unsigned int chan, char *bssid) {
  DEBUG("ESSID: %s, Channel: %d", essid, chan);
  unsigned char hash[BSSID_SIZE];
  memset(hash, '\0', sizeof(hash));
  char channel[CHAN_SIZE];
  memset(channel, '\0', sizeof(channel));
  int i;
  MD5_CTX ctx;
  MD5_Init(&ctx);

  MD5_Update(&ctx, essid, strlen(essid));
  MD5_Final(hash, &ctx);
  DEBUG("Hash: %s", (char *)hash);

  for(i = 0; i < BSSID_SIZE - CHAN_SIZE; i++)
    bssid[i] = hash[i];

  snprintfcat(channel, CHAN_SIZE + 1, "%u", htons((uint16_t)chan));

  for(int j = 0; j < CHAN_SIZE; j++)
  {
    bssid[i] = channel[j];
    i++;
  }


  DEBUG("BSSID buffer: %s", (char *)bssid);

  return;
}
/*
void get_bssid(const char *essid, const unsigned int freq, unsigned char bssid[BSSID_SIZE]) {
  DEBUG("ESSID: %s, Frequency: %d", essid, freq);
  unsigned char hash[crypto_hash_BYTES];
  char msg[ESSID_SIZE + FREQ_SIZE];

  snprintfcat(msg, ESSID_SIZE + FREQ_SIZE, "%s%d", essid, freq);

  crypto_hash(hash, msg, ESSID_SIZE + FREQ_SIZE);
  DEBUG("Hash: %s", (char *)hash);

  for(int i=0; i < BSSID_SIZE; i++)
    bssid[i] = hash[i];

  DEBUG("BSSID buffer: %s", (char *)bssid);

  return;
}
*/

/** from http://grapsus.net/blog/post/Hexadecimal-dump-in-C */
#define HEXDUMP_COLS 8
void hexdump(void *mem, unsigned int len)
{
  unsigned int i, j;
  
  for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
  {
    /* print offset */
    if(i % HEXDUMP_COLS == 0)
    {
      printf("0x%06x: ", i);
    }
    
    /* print hex data */
    if(i < len)
    {
      printf("%02x ", 0xFF & ((char*)mem)[i]);
    }
    else /* end of block, just aligning for ASCII dump */
    {
      printf("   ");
    }
    
    /* print ASCII dump */
    if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
    {
      for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
      {
	if(j >= len) /* end of block, not really printing */
	{
	  putchar(' ');
	}
	else if(isprint(((char*)mem)[j])) /* printable char */
	{
	  putchar(0xFF & ((char*)mem)[j]);        
	}
	else /* other char */
	{
	  putchar('.');
	}
      }
      putchar('\n');
    }
  }
}
