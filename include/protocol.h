/*
 * $Id: protocol.h 2786 2013-07-09 16:42:55 FreeWAF Development Team $
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
#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include "util.h"

typedef struct protocol_type_s protocol_type_t;
typedef struct protocol_data_s {
    const char *protocol_name;
    const protocol_type_t *protocol_type;
    void *protocol_data;
} protocol_data_t;

 struct protocol_type_s {
    int (*connect)(protocol_data_t *protocol, char *host, int port, char *username, char *password);
    void (*disconnect)(protocol_data_t *protocol);

    int (*open_dir)(protocol_data_t *protocol, char *dirpath);
    int (*get_list)(protocol_data_t *protocol, char **filelist, int *filecounts);
    int (*close_dir)();

    int (*open_file_for_read)(protocol_data_t *protocol, char *filepath);
    int (*read_file)(protocol_data_t *protocol, char *buffer, int len);
    int (*close_file)(protocol_data_t *protocol);

    int (*rm_dir)(protocol_data_t *protocol, char *dirname);
    int (*rm_file)(protocol_data_t *protocol, char *filename);

    int (*write_dir)(protocol_data_t *protocol, char *dirname, long mode);
    int (*write_file)(protocol_data_t *protocol, char *buffer, int len);

    int (*open_file_for_write)(protocol_data_t *protocol, char *filepath);

    int (*get_stat)(protocol_data_t *protocol, char *filepath, file_t *fileinfo);
    int (*set_stat)(protocol_data_t *protocol, char *filepath, file_t *fileinfo);

    int (*init)(protocol_data_t *protocol);
    void (*uninit)(protocol_data_t *protocol);
} ;

/* Э���ʼ�� */
#define protocol_init(e) \
            (e)->protocol_type->init(e)

/* Э�鷴��ʼ�� */
#define protocol_uninit(e) \
            (e)->protocol_type->uninit(e)

/* ���ӽӿ� */
#define protocol_connect(e, host, port, username, password) \
            (e)->protocol_type->connect(e, host, port, username, password)

/* �Ͽ����ӽӿ� */
#define protocol_disconnect(e) \
            (e)->protocol_type->disconnect(e)

/* ���ļ��нӿ� */
#define protocol_open_dir(e, dirpath) \
            (e)->protocol_type->open_dir(e, dirpath)

/* ��ȡ�ļ��б�ӿ� */
#define protocol_get_list(e, dirpath, filecounts) \
            (e)->protocol_type->get_list(e, dirpath, filecounts)

/* �ر��ļ��нӿ� */
#define protocol_close_dir(e) \
            (e)->protocol_type->close_dir(e)

/* ���ļ��ӿ�(��) */
#define protocol_open_file_for_read(e, filepath) \
            (e)->protocol_type->open_file_for_read(e, filepath)

/* ��ȡ�ļ��ӿ� */
#define protocol_read_file(e, buffer, len) \
            (e)->protocol_type->read_file(e, buffer, len)

/* �ر��ļ��ӿ� */
#define protocol_close_file(e) \
            (e)->protocol_type->close_file(e)

/* ɾ���ļ��� */
#define protocol_rm_dir(e, dirname) \
            (e)->protocol_type->rm_dir(e, dirname)

/* ɾ���ļ� */
#define protocol_rm_file(e, filename) \
            (e)->protocol_type->rm_file(e, filename)

/* �����ļ��� */
#define protocol_write_dir(e, dirname, mode) \
            (e)->protocol_type->write_dir(e, dirname, mode)

/* д�ļ� */
#define protocol_write_file(e, buffer, len) \
            (e)->protocol_type->write_file(e, buffer, len)

/* ���ļ��ӿ�(д) */
#define protocol_open_file_for_write(e, filepath) \
            (e)->protocol_type->open_file_for_write(e, filepath)

/* �����ļ����� */
#define protocol_set_stat(e, filepath, fileinfo) \
            (e)->protocol_type->set_stat(e, filepath, fileinfo)

/* ��ȡ�ļ����� */
#define protocol_get_stat(e, filepath, fileinfo) \
            (e)->protocol_type->get_stat(e, filepath, fileinfo)

#endif

