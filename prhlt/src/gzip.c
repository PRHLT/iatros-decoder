/***************************************************************************
 *   Copyright (C) 2005 by Vicente Alabau Gonzalvo                         *
 *   valabau@dsic.upv.es                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <string.h>
#include <stdio.h>
#include <prhlt/config.h>
#include <prhlt/trace.h>
#include <prhlt/utils.h>
#include <prhlt/gzip.h>

/*gzip open and close*/
int is_gzipped(const char * fname) {
  if (fname != NULL) {
    int len = strlen(fname);
    return len > 3 && strcmp(fname+(len-3),".gz") == 0;
  }
  return 0;
}

int is_dash(const char * fname) {
  if (fname != NULL) {
    return strcmp(fname,"-") == 0;
  }
  return 0;
}

FILE *smart_fopen(const char * fname, const char * mode) {
  if (fname == NULL) return NULL;
  FILE *file = NULL;
  if (is_gzipped(fname)) {

#ifdef HAVE_POPEN
    if (strchr(mode,'r') != NULL) {
      const char * const GUNZIP_COMMAND = "gunzip -c '%s'";
      char cmd[strlen(GUNZIP_COMMAND) + strlen(fname) + 1];
      sprintf(cmd,GUNZIP_COMMAND,fname);
      file = popen(cmd,"r");
    }
    else if (strchr(mode,'w') != NULL) {
      const char * const GUNZIP_COMMAND = "gzip -c > '%s'";
      char cmd[strlen(GUNZIP_COMMAND) + strlen(fname) + 1];
      sprintf(cmd,GUNZIP_COMMAND,fname);
      file = popen(cmd,"w");
    }

#else
    REQUIRE(false, "Compressed file management has been disabled");
#endif
  }
  else if (file == NULL && is_dash(fname)) {
    if (strchr(mode,'w') != NULL) {
      file = stdout;
    }
    else if (strchr(mode,'r') != NULL) {
      file = stdin;
    }
  }
  else {
    file = fopen(fname, mode);
  }
  return file;
}

int smart_fclose(FILE *fd){
  if (fd == stdin || fd == stdout || fd == NULL) {
    return 0;
  }
  else {
    int rc = -1;
#ifdef HAVE_POPEN
    rc = pclose(fd);
#endif
    if (rc == -1) { // stream not associated with a popen()
      rc = fclose(fd);
    }
    return rc;
  }
}
