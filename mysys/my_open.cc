/* Copyright (c) 2000, 2018, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   Without limiting anything contained in the foregoing, this file,
   which is part of C Driver for MySQL (Connector/C), is also subject to the
   Universal FOSS Exception, version 1.0, a copy of which can be found at
   http://oss.oracle.com/licenses/universal-foss-exception.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/**
  @file mysys/my_open.cc
*/
#include "mysys/my_static.h"

#include "my_config.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/types.h>
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "my_dbug.h"
#include "my_inttypes.h"
#include "my_io.h"
#include "my_sys.h"
#include "my_thread_local.h"
#include "mysql/psi/mysql_mutex.h"
#include "mysql/service_mysql_alloc.h"
#include "mysys/mysys_priv.h"
#include "mysys_err.h"

/*
  Open a file

  SYNOPSIS
    my_open()
      FileName	Fully qualified file name
      Flags	Read | write
      MyFlags	Special flags

  RETURN VALUE
    File descriptor
*/

File my_open(const char *FileName, int Flags, myf MyFlags)
/* Path-name of file */
/* Read | write .. */
/* Special flags */
{
  File fd;
  DBUG_ENTER("my_open");
  DBUG_PRINT("my",
             ("Name: '%s'  Flags: %d  MyFlags: %d", FileName, Flags, MyFlags));
#if defined(_WIN32)
  fd = my_win_open(FileName, Flags);
#else

#ifdef MULTI_MASTER_ZHANG_LOG
  EasyLoggerWithTrace(path_log_mysys, EasyLogger::info).force_flush() << "my_open::open. try to open file:" << FileName;
#endif // MULTI_MASTER_ZHANG_LOG
#ifdef  MULTI_MASTER_ZHANG_REMOTE
  std::string flag;
  if (0 == file_should_be_local_mysys(FileName)) {
      fd = open(FileName, Flags, my_umask); /* Normal unix */
      flag = "local";
  } else {
      fd = remote_client_mysys->remote_open(FileName, Flags, my_umask);
      map_fd_mysys.insert(std::make_pair(fd, fd));
      std::string remote_path(FileName);
      map_path_mysys.insert(std::make_pair(fd, remote_path));
      flag = "remote";
  }
#ifdef MULTI_MASTER_ZHANG_LOG
  EasyLoggerWithTrace(path_log_mysys, EasyLogger::info).force_flush() << "my_open::open. open " << flag << " file:" << FileName << ", fd:" << fd;
#endif // MULTI_MASTER_ZHANG_LOG
#else // MULTI_MASTER_ZHANG_REMOTE
  fd = open(FileName, Flags, my_umask); /* Normal unix */
#endif // MULTI_MASTER_ZHANG_REMOTE
#ifdef MULTI_MASTER_ZHANG_LOG
  EasyLoggerWithTrace(path_log_mysys, EasyLogger::info).force_flush() << "my_open::open. open " << " file:" << FileName << ", fd:" << fd;
#endif // MULTI_MASTER_ZHANG_LOG
#endif

  fd = my_register_filename(fd, FileName, FILE_BY_OPEN, EE_FILENOTFOUND,
                            MyFlags);
  DBUG_RETURN(fd);
} /* my_open */

/*
  Connect to unix domain socket

  SYNOPSIS
  my_unix_socket_connect()
  FileName  Fully qualified file name
  MyFlags Special flags

  RETURN VALUE
  File descriptor
*/
#ifndef __WIN__
File my_unix_socket_connect(const char *FileName, myf MyFlags) noexcept
/* Path-name of file */
/* Read | write .. */
/* Special flags */
{
  struct sockaddr_un addr;
  DBUG_ENTER("my_unix_socket_connect");
  DBUG_PRINT("my", ("Name: '%s'  MyFlags: %d", FileName, MyFlags));

  if (strlen(FileName) > (sizeof(addr.sun_path) - 1)) {
    if (MyFlags & (MY_FAE | MY_WME))
      my_error(EE_TOOLONGFILENAME, MYF(0), FileName, sizeof(addr.sun_path) - 1);
    DBUG_RETURN(-1);
  }
  my_socket sd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sd < 0) {
    if (MyFlags & (MY_FAE | MY_WME))
      my_error(EE_SOCKET, MYF(0), FileName, errno);
    DBUG_RETURN(-1);
  }
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, FileName);
  if (connect(sd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) ==
      -1) {
#ifdef MULTI_MASTER_ZHANG_LOG
  EasyLoggerWithTrace(path_log_mysys, EasyLogger::info).force_flush() << "my_unix_socket_connect::close. try to close fd:" << sd;
#endif // MULTI_MASTER_ZHANG_LOG
#ifdef MULTI_MASTER_ZHANG_REMOTE
  int remote_fd = get_remote_fd_mysys(sd);
  if (remote_fd > 0) {
      remote_client_mysys->remote_close(remote_fd);
      auto iter = map_path_mysys.find(remote_fd);
      if(iter != map_path_mysys.end()){
          map_path_mysys.erase(remote_fd);
      } else {
        #ifdef MULTI_MASTER_ZHANG_LOG
          EasyLoggerWithTrace(path_log_mysys, EasyLogger::info).force_flush() << "[error] no such file, remote fd:" << remote_fd;
        #endif // MULTI_MASTER_ZHANG_LOG
      }
      map_fd_mysys.erase(remote_fd);
  } else {
      close(sd);
  }
#else
    close(sd);
#endif // MULTI_MASTER_ZHANG_REMOTE
    sd = -1;
  }

  DBUG_RETURN(my_register_filename(static_cast<File>(sd), FileName,
                                   FILE_BY_OPEN, EE_FILENOTFOUND, MyFlags));
} /* my_unix_socket_connect */
#endif

/*
  Close a file

  SYNOPSIS
    my_close()
      fd	File sescriptor
      myf	Special Flags

*/

int my_close(File fd, myf MyFlags) {
  int err;
  DBUG_ENTER("my_close");
  DBUG_PRINT("my", ("fd: %d  MyFlags: %d", fd, MyFlags));

  mysql_mutex_lock(&THR_LOCK_open);
#ifndef _WIN32
  do {
#ifdef MULTI_MASTER_ZHANG_LOG
  EasyLoggerWithTrace(path_log_mysys, EasyLogger::info).force_flush() << "my_close::clsoe. try to closed fd:" << fd;
#endif // MULTI_MASTER_ZHANG_LOG
#ifdef MULTI_MASTER_ZHANG_REMOTE
  int remote_fd = get_remote_fd_mysys(fd);
  if (remote_fd > 0) {
      err = remote_client_mysys->remote_close(remote_fd);
      auto iter = map_path_mysys.find(remote_fd);
      if(iter != map_path_mysys.end()){
          map_path_mysys.erase(remote_fd);
      } else {
        #ifdef MULTI_MASTER_ZHANG_LOG
          EasyLoggerWithTrace(path_log_mysys, EasyLogger::info).force_flush() << "[error] no such file, remote fd:" << remote_fd;
        #endif // MULTI_MASTER_ZHANG_LOG
      }
      map_fd_mysys.erase(remote_fd);
  } else {
    err = close(fd);
  }
#else
    err = close(fd);
#endif // MULTI_MASTER_ZHANG_REMOTE
#ifdef MULTI_MASTER_ZHANG_LOG
  EasyLoggerWithTrace(path_log_mysys, EasyLogger::info).force_flush() << "my_close::clsoe. closed fd:" << fd << ", ret:" << err << ", errno:" << errno;
#endif // MULTI_MASTER_ZHANG_LOG
  } while (err == -1 && errno == EINTR);
#else
  err = my_win_close(fd);
#endif
  if (err) {
    DBUG_PRINT("error", ("Got error %d on close", err));
    set_my_errno(errno);
    if (MyFlags & (MY_FAE | MY_WME)) {
      char errbuf[MYSYS_STRERROR_SIZE];
      my_error(EE_BADCLOSE, MYF(0), my_filename(fd), my_errno(),
               my_strerror(errbuf, sizeof(errbuf), my_errno()));
    }
  }
  if ((uint)fd < my_file_limit && my_file_info[fd].type != UNOPEN) {
    my_free(my_file_info[fd].name);
    my_file_info[fd].type = UNOPEN;
  }
  my_file_opened--;
  mysql_mutex_unlock(&THR_LOCK_open);
  DBUG_RETURN(err);
} /* my_close */

/*
  Register file in my_file_info[]

  SYNOPSIS
    my_register_filename()
    fd			   File number opened, -1 if error on open
    FileName		   File name
    type_file_type	   How file was created
    error_message_number   Error message number if caller got error (fd == -1)
    MyFlags		   Flags for my_close()

  RETURN
    -1   error
     #   Filenumber

*/

File my_register_filename(File fd, const char *FileName,
                          enum file_type type_of_file,
                          uint error_message_number, myf MyFlags) {
#ifdef MULTI_MASTER_ZHANG_LOG
  EasyLoggerWithTrace(path_log_mysys, EasyLogger::info).force_flush() << "mysys call my_register_filename()";
#endif // MULTI_MASTER_ZHANG_LOG
  char *dup_filename = NULL;
  DBUG_ENTER("my_register_filename");
  if ((int)fd >= MY_FILE_MIN) {
    if ((uint)fd >= my_file_limit) {
#if defined(_WIN32)
      set_my_errno(EMFILE);
#else
      mysql_mutex_lock(&THR_LOCK_open);
      my_file_opened++;
      mysql_mutex_unlock(&THR_LOCK_open);
      DBUG_RETURN(fd); /* safeguard */
#endif
    } else {
      dup_filename = my_strdup(key_memory_my_file_info, FileName, MyFlags);
      if (dup_filename != NULL) {
        mysql_mutex_lock(&THR_LOCK_open);
        my_file_info[fd].name = dup_filename;
        my_file_opened++;
        my_file_total_opened++;
        my_file_info[fd].type = type_of_file;
        mysql_mutex_unlock(&THR_LOCK_open);
        DBUG_PRINT("exit", ("fd: %d", fd));
        DBUG_RETURN(fd);
      }
      set_my_errno(ENOMEM);
    }
    (void)my_close(fd, MyFlags);
  } else
    set_my_errno(errno);

  DBUG_PRINT("error", ("Got error %d on open", my_errno()));
  if (MyFlags & (MY_FFNF | MY_FAE | MY_WME)) {
    char errbuf[MYSYS_STRERROR_SIZE];
    if (my_errno() == EMFILE) error_message_number = EE_OUT_OF_FILERESOURCES;
    DBUG_PRINT("error", ("print err: %d", error_message_number));
    my_error(error_message_number, MYF(0), FileName, my_errno(),
             my_strerror(errbuf, sizeof(errbuf), my_errno()));
  }
  DBUG_RETURN(-1);
}

#ifdef EXTRA_DEBUG

void my_print_open_files(void) {
  if (my_file_opened | my_stream_opened) {
    uint i;
    for (i = 0; i < my_file_limit; i++) {
      if (my_file_info[i].type != UNOPEN) {
        my_message_local(INFORMATION_LEVEL, EE_FILE_NOT_CLOSED,
                         my_file_info[i].name, i);
      }
    }
  }
}

#endif
