// Copyright 2008, Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// The file contains the implementation of the File methods specific to
// POSIX platforms.

#include "kml/base/file.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace kmlbase {

// Internal to the POSIX File class.
static bool StatFile(const char* path, struct stat* stat_data) {
  struct stat tmp;
  if (stat(path, &tmp) !=0) {
    return false;
  }
  *stat_data = tmp;
  return true;
}

bool File::Exists(const string& full_path) {
  struct stat stat_data;
  if (!StatFile(full_path.c_str(), &stat_data)) {
    return false;
  }
  return S_ISREG(stat_data.st_mode);
}

bool File::Delete(const string& filepath) {
  return unlink(filepath.c_str()) == 0;
}

bool File::CreateNewTempFile(string* path) {
  if (!path) {
    return false;
  }
  char temp_path[] = "/tmp/libkmlXXXXXX";
  int fd = mkstemp(temp_path);
  if (fd == -1) {
    return false;
  }
  close(fd);
  path->assign(temp_path, strlen(temp_path));
  return true;
}

}  // end namespace kmlbase
