/*
 * $Id: protocol_sftp.c 2786 2013-07-09 16:42:55 FreeWAF Development Team $
 *
 * (C) 2013-2014 see FreeWAF Development Team
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file LICENSE.GPLv2.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/licenses/gpl-2.0.html
 *
 */
#include "libssh2_config.h"
#include <libssh2.h>
#include <libssh2_sftp.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include <sys/types.h>
#include <sys/socket.h> 
#include <sys/ioctl.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <utime.h>
#include <stdlib.h>
#include <time.h>

#include "protocol_sftp.h"
#include "util.h"

/* sftpЭ���ʼ�� */
static int sftp_init(protocol_data_t *protocol)
{
    int rc;

    rc = libssh2_init(0);
    if (rc != 0) {
        return -1;
    }

    return 0;
}

/* sftpЭ�鷴��ʼ�� */
static void sftp_uninit(protocol_data_t *protocol)
{
    libssh2_exit();
}

static int unblock_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    unsigned long ul; 
    struct timeval tm;
    fd_set set;
    int error, len;
    int rc;

    /* ����Ϊ������ģʽ */
    ul = 1;
    rc = ioctl(sockfd, FIONBIO, &ul); 
    if (rc != 0) {
        return -1;
    }
    
    rc = connect(sockfd, (struct sockaddr*)addr, addrlen);
    if (rc != 0 && errno == EINPROGRESS) {
        tm.tv_sec = 10;
        tm.tv_usec = 0;
        FD_ZERO(&set);
        FD_SET(sockfd, &set);
        if(select(sockfd + 1, NULL, &set, NULL, &tm) > 0) {
            len = sizeof(int);
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
            if(error != 0) {
                return -1;
            }
        } else {
            return -1;
        }
    } else if (rc != 0) {
        return -1;     
    }
    
    /* ����Ϊ����ģʽ */
    ul = 0;
    rc = ioctl(sockfd, FIONBIO, &ul); 
    if (rc != 0) {
        return -1;
    }
    
    return 0;
}

/* sftpЭ������ */
static int sftp_connect(protocol_data_t *protocol, char *host, int port, char *username, 
            char *password)
{
    int rc;
    struct sockaddr_in sin;

    if (protocol == NULL || protocol->protocol_data == NULL || host == NULL
            || username == NULL || password == NULL) {
        return;
    }

    sftp_data_t *data = (sftp_data_t *)protocol->protocol_data;

    data->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (data->sock == -1) {
        return -1;
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = inet_addr(host);

    rc = unblock_connect(data->sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in));
    if (rc != 0) {
        close(data->sock);
        return -1;
    } 
    
    data->session = libssh2_session_init();
    if(!data->session) {
        close(data->sock);
        return -1;
    }

    rc = libssh2_session_handshake(data->session, data->sock);
    if(rc) {
        close(data->sock);
        return -1;
    }

    rc = libssh2_userauth_password(data->session, username, password);
    if (rc) {
        goto shutdown;
    }

    data->sftp_session = libssh2_sftp_init(data->session);
    if (!data->sftp_session) {
        goto shutdown;
    }

    libssh2_session_set_blocking(data->session, 1);
    
    /* TODO: ��ֵ */

    return 0;
        
shutdown:
    libssh2_session_disconnect(data->session, "Normal Shutdown");
    libssh2_session_free(data->session);
    close(data->sock);

    return -1;    
}

/* sftpЭ��Ͽ����� */
static void sftp_disconnect(protocol_data_t *protocol)
{
    if (protocol == NULL || protocol->protocol_data == NULL) {
        return;
    }

    sftp_data_t *data = (sftp_data_t *)protocol->protocol_data;

    libssh2_sftp_shutdown(data->sftp_session);
    libssh2_session_disconnect(data->session, "Normal Shutdown");
    libssh2_session_free(data->session);
    close(data->sock);
}

/* sftp���ļ��� */
static int sftp_open_dir(protocol_data_t *protocol, char *dirpath)
{
    if (protocol == NULL || protocol->protocol_data || dirpath == NULL) {
        return;
    }
    
    sftp_data_t *data = (sftp_data_t *)protocol->protocol_data;

    data->dir_handle = libssh2_sftp_opendir(data->sftp_session, dirpath);
    if (!data->dir_handle) {
        return -1;
    }

    return 0;
}

/* sftp��ȡ�ļ��б� */
static int sftp_get_list(protocol_data_t *protocol, char **filelist, int *filecounts)
{
#define MALLOC_NUM 10
    if (protocol == NULL || protocol->protocol_data == NULL) {
        return;
    }

    int rc;
    int allocnum;
    int blocksize;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    int reallocnum;

    sftp_data_t *data = (sftp_data_t *)protocol->protocol_data;
    
    blocksize = sizeof(file_t);

    *filelist = (char *)malloc(blocksize * MALLOC_NUM);
    if (*filelist == NULL) {
        return -2;
    }

    allocnum = MALLOC_NUM;
    reallocnum = MALLOC_NUM;
    *filecounts = 0;

    do {
         char filename[MAX_FILENAME_LEN] = {0}; /*255���ļ�����󳤶�*/
         /* filename�洢�ļ����ļ����� */
         rc = libssh2_sftp_readdir_ex(data->dir_handle, filename, sizeof(filename), NULL, 0, &attrs);
         if(rc > 0) {
             if (filename[0] != '\0' && filename[0] != '.') {/*��ȥ�����ļ�*/
                 /* �ļ���  ���� ��ͨ�ļ�*/
                 if(LIBSSH2_SFTP_S_ISDIR(attrs.permissions) || LIBSSH2_SFTP_S_ISREG(attrs.permissions)) {
                     if (*filecounts >= allocnum) {
                         reallocnum = reallocnum + MALLOC_NUM;
                         *filelist = (char *)realloc(*filelist, blocksize * reallocnum);
                         if (*filelist == NULL) {
                             return -2;
                         }
                         allocnum = allocnum + MALLOC_NUM;
                     }

                     memcpy(*filelist + *filecounts * blocksize, filename, MAX_FILENAME_LEN);
                     memcpy(*filelist + *filecounts * blocksize + MAX_FILENAME_LEN,
                             (char *)&attrs + sizeof(unsigned long), blocksize - MAX_FILENAME_LEN);
                     *filecounts += 1;
                 }
             } 
         } else if (rc == 0) {
             return 0;
         } else {
             return -1;
         }
     } while (1);

     return 0;
}

/* sftp�ر��ļ��� */
static int sftp_close_dir(protocol_data_t *protocol)
{
    if (protocol == NULL || protocol->protocol_data == NULL) {
        return;
    }
    
    sftp_data_t *data = (sftp_data_t *)protocol->protocol_data;

    return libssh2_sftp_closedir(data->dir_handle);
}

/* sftp���ļ��ӿ�(��) */
static int sftp_open_file_for_read(protocol_data_t *protocol, char *filepath)
{
    if (protocol == NULL || protocol->protocol_data == NULL 
            || filepath == NULL) {
        return -1;
    }
    
    sftp_data_t *data = (sftp_data_t *)protocol->protocol_data;

    data->file_handle = libssh2_sftp_open(data->sftp_session, filepath, LIBSSH2_FXF_READ, 0);

    return 0;
}

/* sftp���ļ��ӿ�(д) */
static int sftp_open_file_for_write(protocol_data_t *protocol, char *filepath)
{
    if (protocol == NULL || protocol->protocol_data == NULL 
            || filepath == NULL) {
        return -1;
    }
    
    sftp_data_t *data = (sftp_data_t *)protocol->protocol_data;

    data->file_handle = libssh2_sftp_open(data->sftp_session, filepath,
            LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT| LIBSSH2_FXF_TRUNC,
            LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR | LIBSSH2_SFTP_S_IRGRP
            | LIBSSH2_SFTP_S_IROTH);

    return 0;
}

/* sftp��ȡ�ļ��ӿ� */
static int sftp_read_file(protocol_data_t *protocol, char *buffer, int len)
{
    if (protocol == NULL || protocol->protocol_data == NULL 
            || buffer == NULL) {
        return -1;
    }
    
    sftp_data_t *data = (sftp_data_t *)protocol->protocol_data;

    return libssh2_sftp_read(data->file_handle, buffer, len);
}

/* sftp�ر��ļ��ӿ� */
static int sftp_close_file(protocol_data_t *protocol)
{
    if (protocol == NULL || protocol->protocol_data == NULL ) {
        return -1;
    }
    
    sftp_data_t *data = (sftp_data_t *)protocol->protocol_data;

    return libssh2_sftp_close(data->file_handle);
}

/* sftpɾ���ļ� */
static int sftp_rm_file(protocol_data_t *protocol, char *filename)
{
    if (protocol == NULL || protocol->protocol_data == NULL 
            || filename == NULL) {
        return -1;
    }
    
    sftp_data_t *data = (sftp_data_t *)protocol->protocol_data;

    return libssh2_sftp_unlink(data->sftp_session, filename);
}

/* sftpɾ���ļ��� */
static int sftp_rm_dir(protocol_data_t *protocol, char *dirname)
{
    int rc;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    LIBSSH2_SFTP_HANDLE *dir_handle;
    char source[MAX_PATH_LEN] = {0};

    if (protocol == NULL || protocol->protocol_data == NULL 
            || dirname == NULL) {
        return -1;
    }
    
    sftp_data_t *data = (sftp_data_t *)protocol->protocol_data;
    strcpy(source, dirname);

    dir_handle = libssh2_sftp_opendir(data->sftp_session, source);
    if (!dir_handle) {
        return -1;
    }

    do {
         char filename[MAX_FILENAME_LEN] = {0}; /*255���ļ�����󳤶�*/
         rc = libssh2_sftp_readdir_ex(dir_handle, filename, sizeof(filename), NULL, 0, &attrs);
         if(rc > 0) {
             if (filename[0] != '\0' && filename[0] != '.') {/*��ȥ�����ļ�*/
                 if (LIBSSH2_SFTP_S_ISREG(attrs.permissions)) {
                     add_lastfilename(source, filename);
                     libssh2_sftp_unlink(data->sftp_session, source);
                     del_lastfilename(source);
                 }
                 if(LIBSSH2_SFTP_S_ISDIR(attrs.permissions)){
                     add_lastdirname(source, filename);
                     sftp_rm_dir(protocol, source);
                     del_lastdirname(source);
                 }
             }
         } else if (rc == 0) {
             break;
         } else {
             continue;
         }
    } while (1);

    libssh2_sftp_closedir(dir_handle);
    libssh2_sftp_rmdir(data->sftp_session, source);

    return 0;
}

/* sftpд�ļ� */
static int sftp_write_file(protocol_data_t *protocol, char *buffer, int len)
{
    if (protocol == NULL || protocol->protocol_data == NULL 
            || buffer == NULL) {
        return -1;
    }
    
    sftp_data_t *data = (sftp_data_t *)protocol->protocol_data;

    return libssh2_sftp_write(data->file_handle, buffer, len);
}

/* sftp�����ļ��� */
static int sftp_write_dir(protocol_data_t *protocol, char *dirname, long mode)
{
    if (protocol == NULL || protocol->protocol_data == NULL 
            || dirname == NULL) {
        return -1;
    }

    sftp_data_t *data = (sftp_data_t *)protocol->protocol_data;

    return libssh2_sftp_mkdir(data->sftp_session, dirname, mode);
}

/* sftp�����ļ����� */
static int sftp_set_stat(protocol_data_t *protocol, char *filename, file_t *fileinfo)
{
    LIBSSH2_SFTP_ATTRIBUTES attrs;

    if (protocol == NULL || protocol->protocol_data == NULL 
            || filename == NULL || fileinfo == NULL) {
        return -1;
    }

    sftp_data_t *data = (sftp_data_t *)protocol->protocol_data;

    attrs.flags = LIBSSH2_SFTP_ATTR_SIZE | LIBSSH2_SFTP_ATTR_UIDGID
                        | LIBSSH2_SFTP_ATTR_PERMISSIONS | LIBSSH2_SFTP_ATTR_ACMODTIME;

    attrs.filesize = fileinfo->filesize;
    attrs.permissions = fileinfo->permissions;
    attrs.uid = fileinfo->uid;
    attrs.gid = fileinfo->gid;
    attrs.mtime = fileinfo->mtime;
    attrs.atime = fileinfo->atime;

    return libssh2_sftp_setstat(data->sftp_session, filename, &attrs);
}

/* sftp��ȡ�ļ����� */
static int sftp_get_stat(protocol_data_t *protocol, char *filename, file_t *fileinfo)
{
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    
    if (protocol == NULL || protocol->protocol_data == NULL 
            || filename == NULL || fileinfo == NULL) {
        return -1;
    }

    sftp_data_t *data = (sftp_data_t *)protocol->protocol_data;

    libssh2_sftp_lstat(data->sftp_session, filename, &attrs);

    fileinfo->filesize = attrs.filesize;
    fileinfo->permissions = attrs.permissions;
    fileinfo->uid = attrs.uid;
    fileinfo->gid = attrs.gid;
    fileinfo->mtime = attrs.mtime;
    fileinfo->atime = attrs.atime;

    return 0;
}

const protocol_type_t ptotocol_type_sftp =
{
    sftp_connect,
    sftp_disconnect,

    sftp_open_dir,
    sftp_get_list,
    sftp_close_dir,

    sftp_open_file_for_read,
    sftp_read_file,
    sftp_close_file,

    sftp_rm_dir,
    sftp_rm_file,

    sftp_write_dir,
    sftp_write_file,

    sftp_open_file_for_write,

    sftp_get_stat,
    sftp_set_stat,

    sftp_init,
    sftp_uninit
};

/**
 * protocol_sftp_create -  Э�����ݴ���
 *
 * Э�����ݴ���
 *
 * @returns
 *     �ɹ�: ����Э������
 *     ʧ��: ����NULL
 */
protocol_data_t *protocol_sftp_create()
{
    protocol_data_t *type;
    sftp_data_t *data;

    type= (protocol_data_t *)malloc(sizeof(protocol_data_t));
    if (type == NULL) {
        return NULL;
    }

    data = (sftp_data_t *)malloc(sizeof(sftp_data_t));
    if (data == NULL) {
        free(type);
        return NULL;
    }
    memset(data, 0, sizeof(sftp_data_t));

    type->protocol_name = "SFTP";
    type->protocol_type = &ptotocol_type_sftp;
    type->protocol_data = data;

    return type;
}

/**
 * protocol_sftp_destroy -  Э����������
 * @param type: Э������
 *
 * Э����������
 */
void protocol_sftp_destroy(protocol_data_t *type)
{
    if (type == NULL) {
        return;
    }

    if (type->protocol_data) {
        free(type->protocol_data);
    }

    free(type);
}

