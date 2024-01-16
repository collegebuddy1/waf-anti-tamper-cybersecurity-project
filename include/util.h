/*
 * $Id: util.h 2786 2013-07-09 16:42:55 FreeWAF Development Team $
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
#ifndef _UTIL_H_
#define _UTIL_H_

#include "list.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/vfs.h>
#include "md5.h"
#include "svn.h"
#include "apr_pools.h"

#define MAX_BUF_LEN              256
#define MAX_FILENAME_LEN         256
#define MAX_PATH_LEN             4097
#define MD5_BYTES                DIGEST_BIN_BYTES
#define MD5_ALIGN                DIGEST_ALIGN
#define TAMPER_VERSION           "1.0.0"
#define TAMPER_BUILD             "437"
#define TAMPER_RELEASE           "7098"

#define BACKUP_ROOT              "/var/tamper/"
#define WEBTAMPER_FLOCK_PATH     "/tmp/webtamper.flock"

#define NODE_DIR  0
#define NODE_FILE 1

typedef struct file_s {
    char filename[MAX_FILENAME_LEN];
    unsigned long long filesize;
    unsigned long uid, gid;
    unsigned long permissions;
    unsigned long atime, mtime;
} file_t;

/**
 * add_lastfilename -  ����ļ���
 * @param path: �ļ�·��
 * @param file: �ļ���
 *
 * ����ļ���
 */
extern void add_lastfilename(char *path, char *file);

/**
 * del_lastfilename -  ɾ���ļ���
 * @param path: �ļ�·��
 * @param file: �ļ���
 *
 * ɾ���ļ���
 */
extern void del_lastfilename(char *path);

/**
 * add_lastdirname -  ���Ŀ¼��
 * @param path: �ļ�·��
 * @param file: Ŀ¼��
 *
 * ���Ŀ¼��
 */
extern void add_lastdirname(char *path, char *dir);

/**
 * del_lastdirname -  ɾ��Ŀ¼��
 * @param path: �ļ�·��
 * @param file: Ŀ¼��
 *
 * ɾ��Ŀ¼��
 */
extern void del_lastdirname(char *path);

/**
 * find_webserver -  ���ҷ��۸ķ�����
 * @param webserver_listhead: ���۸ķ���������ͷ���
 * @param wsname: ���۸ķ�������
 *
 * ���ҷ��۸ķ�����
 *
 * @returns
 *     �ɹ�: ���ط��۸ķ������ַ
 *     ʧ��: NULL
 */
extern void *find_webserver(struct list_head *webserver_listhead, char *wsname);

/**
 * destroy_webserver -  ���ٷ��۸ķ�����
 * @param wbtmp: ���۸ķ�����
 *
 * ���ٷ��۸ķ�����
 */
extern void destroy_webserver(void *wb);

/**
 * add_lastfilename -  �жϷ��۸ķ������Ƿ�߱���������
 * @param wbtmp: ���۸ķ�����
 *
 * �жϷ��۸ķ������Ƿ�߱���������
 *
 * @returns
 *     �ɹ�: ����1
 *     ʧ��: ����0
 */
extern int should_webserver_run(void *wbtmp);

/**
 * prepare_webserver -  ��ʼ�����۸ķ�������������
 * @param wbtmp: ���۸ķ�����
 *
 * ��ʼ�����۸ķ�������������
 *
 * @returns
 *     �ɹ�: ����0
 *     ʧ��: ����-1
 */
extern int prepare_webserver(void *wbtmp);

/**
 * cfg_copy -  ��������
 * @param dst: Ŀ������
 * @param dst: Դ����
 *
 * @returns
 *     �����޸�: ����CHANGED
 *     ����δ�޸� ����UNCHANGED
 */
extern int cfg_copy(char *dst, char *src);

/**
 * cfg_copy -  ��������
 * @param dst: Ŀ������
 * @param dst: Դ����
 *
 * @returns
 *     �����޸�: ����CHANGED
 *     ����δ�޸� ����UNCHANGED
 */
extern int cfg_check(char *dst, char *src);

/**
 * delete_diskfiles -  ɾ��Ŀ¼�µ��ļ�
 * @param path: Ŀ¼·��
 *
 * ɾ��Ŀ¼�µ��ļ���������Ŀ¼
 */
extern void delete_diskfiles(char *path);

/**
 * delete_diskdir -  ɾ��Ŀ¼�����ļ�
 * @param path: Ŀ¼·��
 *
 * ɾ��Ŀ¼�µ��ļ���������Ŀ¼
 */
extern void delete_diskdir(char *path);

/**
 * webtamper_sleep -  ����
 * @param time: ����ʱ��
 *
 * ����
 */
extern void webtamper_sleep(long time);

/**
 * build_hashtree -  ������ϣ���Ŀ¼������ʼ�����۸ķ�����ʹ��
 * @param mutex: Ŀ¼λ��
 * @param root: Ŀ¼���ڵ�
 * @param hash: ��ϣ��
 *
 * ������ϣ���Ŀ¼������ʼ�����۸ķ�����ʹ��
 */
extern void build_hashtree(char *path, void *root, struct hash *hash);

/**
 * clean_webserver -  ������۸ķ���������
 * @param wbtmp: ���۸ķ�����
 *
 * ������۸ķ���������
 */
extern void clean_webserver(void *wbtmp);

/**
 * interval_check -  ��ѯ���
 * @param mutex: ������
 * @param cond: ��������
 * @param interval_time: ���ʱ��
 *
 * ���سɹ�����־
 */
extern int interval_check(pthread_mutex_t *mutex, pthread_cond_t *cond, int interval_time);

/**
 * download_mark -  ���سɹ�����־
 * @param filename: ��־�洢�ļ���
 * @param mark: ��־���ɹ�Ϊ"1",ʧ��Ϊ"0"
 *
 * ���سɹ�����־
 *
 * @returns
 *     �ɹ�: ����0
 *     ʧ��: ����-1
 */
extern int download_mark(const char *filename, const char *mark);

/**
 * is_download_success -  �ж��ļ��Ƿ����سɹ�
 * @param filename: ��־�洢�ļ���
 *
 * �ж��ļ��Ƿ����سɹ�
 *
 * @returns
 *     �ɹ�: ����1
 *     ʧ��: ����0
 */
extern int is_download_success(const char *filename);

/**
 * webtamper_log -  �۸���־����ӿ�
 * @param wsname: ���۸ķ���������
 * @param host: ���۸ķ�����������IP
 * @param action: �ж�
 * @param svn: �۸�׷�ټ�¼
 * @param local_root: ���ظ�·��
 * @param server_root: ��������·��
 *
 * �۸���־����ӿ�
 */
extern void webtamper_log(char *wsname, char *host, char *action, svn_t *svn, char *local_root, 
        char *server_root);

/**
 * make_previous_dir -  �����߳�Ŀ¼
 * @param wbtmp: �ļ������ļ��о���·��
 *
 *  �����߳�Ŀ¼
 *
 * @returns
 *     �ɹ�: ����-1
 *     ʧ��: ����0
 */
extern int make_previous_dir(char *path);

extern void send_detect_log(char *ip);

extern unsigned long long get_tamper_total_sizes(char *backup_path);

#endif

