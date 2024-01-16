/*
 * $Id: core.h 2786 2013-07-09 16:42:55 FreeWAF Development Team $
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
#ifndef _CORE_H_
#define _CORE_H_

#include <pe_log_client.h>
#include <apr_pools.h>
#include <apr_time.h>
#include <apr_dbd.h>
#include <pthread.h>
#include <semaphore.h>
#include "apr_global_mutex.h"
#include "protocol_sftp.h"
#include "list.h"
#include "svn.h"
#include "md5.h"
#include "tree.h"

/* Ĭ��ǰ̨���� */
#define TEMPER_BACKGROUND_RUN    1

#define ERROR_MALLOC_FAILED      -1
#define CHANGED                  1
#define UNCHANGED                2

/* ����Ĭ�ϳ��� */
#define MAX_WSNAME_LEN           16
#define MAX_PRTCLNAME_LEN        16
#define MAX_DESCRIPTION_LEN      255
#define MAX_EMAIL_LEN            32
#define MAX_HOST_LEN             32
#define MAX_SERVERROOT_LEN       255
#define MAX_USERNAME_LEN         16
#define MAX_PASSWORD_LEN         64
#define MAX_NOTSUFFIX_LEN        255
#define MAX_CLI_PROMPT_LEN       32
#define MAX_SUFFIX_LEN           32

#define DEBUG_ALL                (1)
#define DEBUG_CORE               (1 << 1)
#define DEBUG_COMM               (1 << 2)

#define LOG_CACHE_TIME              60000000
#define LOG_CACHE_NUM               1000
#define LOG_SERVER_PATH             "/tmp/pe_log_server.uds"

extern struct list_head webserver_listhead;
extern pthread_mutex_t list_mutex;
extern unsigned long long total_disksize;
extern unsigned long long left_disksize;
extern pe_log_client_t *g_log_client;
extern apr_pool_t *g_global_pool;
extern const apr_dbd_driver_t *ap_logdb_driver;
extern apr_dbd_t *ap_logdb_handle;
extern char *g_open_db_parm;

typedef struct admin_log_s {
    apr_time_t time;
    char *ip;
    char *admin_name;
    char *tty;
    char *action;
} admin_log_t;

typedef struct suffix_s {
    struct list_head list;
    char suffix[MAX_SUFFIX_LEN];
} suffix_t;

typedef struct webserver_cfg_s {
    /* �������� */
    char wsname[MAX_WSNAME_LEN];             /* ��ҳ���۸����� */
    char description[MAX_DESCRIPTION_LEN];   /* ��ҳ���۸����� */
    char email[MAX_EMAIL_LEN];               /* �������ʼ���ַ */
    char prtcl_name[MAX_PRTCLNAME_LEN];      /* Э���� */
    int  prtcl_port;                         /* Э��˿� */
    char host[MAX_HOST_LEN];                 /* ��̨��������ַIP �� ����*/
    char server_root[MAX_SERVERROOT_LEN];    /* ��վ��Ŀ¼ */
    char username[MAX_USERNAME_LEN];         /* �û��� */
    char password[MAX_PASSWORD_LEN];         /* ���� */
    int isautorestore;                       /* �Ƿ��Զ��ָ� */
    int enable;                              /* ��ҳ���۸Ĺ����Ƿ��� */
    int depth;                               /* �ļ�������� */
    int maxfilesize;                         /* �����ļ������ߴ� */  
    struct list_head suffix_head;            /* ���������صĺ�׺ */
    int root_interval;                       /* ��Ŀ¼��ѯʱ��  */
    int other_interval;                      /* ����Ŀ¼��ѯʱ�� */
    int sysloglevel;                         /* ϵͳ��־�ȼ� */
    unsigned long long disksize;             /* ���ø�web�������Ĵ��̴�С,��λΪMB */
    int debug;                               /* debug���Կ��� */
    int destroywb;                           /* ���� */
    int iscomit;
} webserver_cfg_t;

typedef struct webserver_s {
    struct list_head list;

    /* ���ò���  */
    webserver_cfg_t cfg;

    /* ������� */
    protocol_data_t *protocol;               /* ʹ��Э��� */
    int running;                             /* ��ҳ���۸��߳��Ƿ񴴽� */
    int isbackuped;                          /* �Ƿ��Ѿ����� */
    int conect_status;                       /* ����״̬ 1�������� 0����δ���� */
    char *local_root;                        /* WAF�ϴ洢��λ�� */
    char *change_root;                       /* �ֶ��ָ�ģʽ���ļ��ݴ����λ�� */
    char *markfile;                          /* ��ȫ���ݱ���ļ� */

    /* ��ʾ���� */
    unsigned int totalnum;                   /* �ļ��ļ����� */
    unsigned int backupnum;                  /* �����ļ����� */
    unsigned long long backupsize;           /* �����ļ���С */
    unsigned int changednum;                 /* ���۸ĵ��ļ��� */
    int suffix_changed;

    /* �ļ����� */
    directory_t *dirtree;                    /* Ŀ¼�� */
    struct hash *dirhash;                    /* ��ϣ�� */
    svn_t *svn;                              /* �۸�׷�� */
    svn_t *man_restore;                      /* �ֶ��ָ����� */
    svn_t *man_reserve;                      /* �ֶ��������� */
    
    pthread_mutex_t wbmutex;                 /* ��̨���۸Ļ��� */
    sem_t monitor_wait;                      /* �۸ļ���ź��� */
    sem_t destroy_wait;                      /* ���۸�ɾ���ź��� */
} webserver_t;

extern void *worker_thread(void *arg);
extern int log_send(char *buf, int is_buffer);

#endif

