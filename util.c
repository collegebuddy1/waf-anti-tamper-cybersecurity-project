/*
 * $Id: util.c 2786 2013-07-09 16:42:55 FreeWAF Development Team $
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
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <apr_strings.h>
#include "print.h"
#include "list.h"
#include "core.h"
#include "tree.h"
#include "hash.h"
#include "tamperlog.h"
#include "log.h"

#define SQL_LEN   128
#define RECONNECT_NUM 3

/**
 * add_lastfilename -  ����ļ���
 * @param path: �ļ�·��
 * @param file: �ļ���
 *
 * ����ļ���
 */
void add_lastfilename(char *path, char *file)
{
    strcat(path, file);
}

/**
 * del_lastfilename -  ɾ���ļ���
 * @param path: �ļ�·��
 * @param file: �ļ���
 *
 * ɾ���ļ���
 */
void del_lastfilename(char *path)
{
    char *p;
    p = strrchr(path, '/');
    if (p) {
        *(p + 1) = '\0';
    }
}

/**
 * add_lastdirname -  ���Ŀ¼��
 * @param path: �ļ�·��
 * @param file: Ŀ¼��
 *
 * ���Ŀ¼��
 */
void add_lastdirname(char *path, char *dir)
{
    strcat(path, dir);
    strcat(path, "/");
}

/**
 * del_lastdirname -  ɾ��Ŀ¼��
 * @param path: �ļ�·��
 * @param file: Ŀ¼��
 *
 * ɾ��Ŀ¼��
 */
void del_lastdirname(char *path) 
{
    char *p;
    p = strrchr(path, '/');
    *p = '\0';
    p = strrchr(path, '/');
    if (p) {
        *(p + 1) = '\0';
    }
}

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
void *find_webserver(struct list_head *webserver_listhead, char *wsname)
{
    struct list_head *pos;
    webserver_t *wb;
    
    list_for_each(pos, webserver_listhead) {
        wb = list_entry(pos, webserver_t, list);
        if (wb && !strcmp(wb->cfg.wsname, wsname)) {
            return wb;
        }
    }

    return NULL;
}

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
int should_webserver_run(void *wbtmp)
{
    webserver_t *wb;

    if (wbtmp == NULL) {
        return 0;
    }

    wb = (webserver_t *)wbtmp;
    if (wb->cfg.prtcl_name[0] == '\0'
            || wb->cfg.host[0] == '\0'
            || wb->cfg.username[0] == '\0'
            || wb->cfg.password[0] == '\0'
            || wb->cfg.server_root[0] == '\0'
            /*|| wb->cfg.enable == 0*/) {
        return 0;
    }

    return 1;
}

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
int prepare_webserver(void *wbtmp)
{
    webserver_t *wb;
    int tmp_len;

    wb = (webserver_t *)wbtmp;

    if (!strcmp(wb->cfg.prtcl_name, "sftp")) {
        wb->protocol = protocol_sftp_create();
        if (wb->protocol == NULL) {
            return -1;
        }
    }
  
    /* ��ʼ��Ŀ¼�� */
    wb->dirtree = (directory_t *)malloc(sizeof(directory_t));
    if (wb->dirtree == NULL) {
        free(wb->change_root);
        free(wb->local_root);
        return -1;
    }
    memset(wb->dirtree, 0, sizeof(directory_t));
    wb->dirtree->name = wb->local_root;

    /* ��ʼ��Ŀ¼��ϣ�� */
    wb->dirhash = hash_create(hash_key_fun, dir_cmp_fun1);
    if (wb->dirhash == NULL) {
        free(wb->change_root);
        free(wb->local_root);
        free(wb->dirtree);
        return -1;   
    }

    /* ��ʼ��svn */
    wb->svn = svn_init();
    if (wb->svn == NULL) {
        free(wb->change_root);
        free(wb->local_root);
        free(wb->dirtree);
        hash_destroy(wb->dirhash, NULL);
        return -1;
    }

    /* ��ʼ��svn */
    wb->man_restore = svn_init(); 
    if (wb->man_restore == NULL) {
        free(wb->change_root);
        free(wb->local_root);
        free(wb->dirtree);
        svn_uninit(wb->svn);
        hash_destroy(wb->dirhash, NULL);
        return -1;
    }

    wb->man_reserve = svn_init(); 
    if (wb->man_reserve == NULL) {
        free(wb->change_root);
        free(wb->local_root);
        free(wb->dirtree);
        svn_uninit(wb->svn);
        svn_uninit(wb->man_restore);
        hash_destroy(wb->dirhash, NULL);
        return -1;
    }

    tmp_len = strlen(BACKUP_ROOT) + strlen(wb->cfg.wsname) + strlen("mark") + 2;
    wb->markfile = (char *)malloc(tmp_len);
    if (wb->markfile == NULL) {
        free(wb->change_root);
        free(wb->local_root);
        free(wb->dirtree);
        svn_uninit(wb->svn);
        svn_uninit(wb->man_restore);
        svn_uninit(wb->man_reserve);
        hash_destroy(wb->dirhash, NULL);
        return -1;
    }
    sprintf(wb->markfile, "%s%s/%s", BACKUP_ROOT, wb->cfg.wsname, "mark");

    wb->isbackuped = 0;

    pthread_mutex_init(&wb->wbmutex, NULL);
    sem_init(&wb->monitor_wait, 0, 0);
    sem_init(&wb->destroy_wait, 0, 0);

    return 0;
}

/**
 * clean_webserver -  ������۸ķ���������
 * @param wbtmp: ���۸ķ�����
 *
 * ������۸ķ���������
 */
void clean_webserver(void *wbtmp)
{            
    if (wbtmp == NULL) {
        return ;
    }
  
    webserver_t *wb = (webserver_t *)wbtmp;
    
    delete_diskfiles(wb->local_root);
    delete_diskfiles(wb->change_root);
    dir_destroy_tree(wb->dirtree);
    hash_clean(wb->dirhash, NULL);
    svn_del_all_record(wb->svn);
    svn_del_all_record(wb->man_restore);
    svn_del_all_record(wb->man_reserve);
    wb->backupnum = 0;
    wb->backupsize = 0;
    wb->changednum = 0;
            
    wb->isbackuped = 0;
}

/**
 * destroy_webserver -  ���ٷ��۸ķ�����
 * @param wbtmp: ���۸ķ�����
 *
 * ���ٷ��۸ķ�����
 */
void destroy_webserver(void *wbtmp)
{       
    char rootpath[MAX_FILENAME_LEN];

    if (wbtmp == NULL) {
        return ;
    }
  
    webserver_t *wb = (webserver_t *)wbtmp;

    pthread_mutex_lock(&list_mutex);
    list_del(&wb->list);
    pthread_mutex_unlock(&list_mutex);
    
    if (!strcmp(wb->protocol->protocol_name, "sftp")) {
        protocol_sftp_destroy(wb->protocol);
    }

    snprintf(rootpath, MAX_FILENAME_LEN, "%s%s/", BACKUP_ROOT, wb->cfg.wsname);
    delete_diskdir(rootpath);
    free(wb->local_root);
    free(wb->change_root);
    free(wb->markfile);
      
    /* ���Ŀ¼�� */
    dir_destroy_tree(wb->dirtree);   

    /* �����ϣ�� */
    hash_destroy(wb->dirhash, NULL);
    
    /* ����۸�׷�ټ�¼ */
    svn_del_all_record(wb->svn);
    svn_del_all_record(wb->man_restore);
    svn_del_all_record(wb->man_reserve);

    pthread_mutex_destroy(&wb->wbmutex);
    sem_destroy(&wb->monitor_wait);
    sem_destroy(&wb->destroy_wait);
    
    free(wb); 
    wb = NULL;
}

/**
 * cfg_copy -  ��������
 * @param dst: Ŀ������
 * @param dst: Դ����
 *
 * @returns
 *     �����޸�: ����CHANGED
 *     ����δ�޸� ����UNCHANGED
 */
int cfg_copy(char *dst, char *src)
{
    if (dst[0] == '\0' || strcmp(dst, src)) {
        strcpy(dst, src);
        return CHANGED;
    }

    return UNCHANGED;
}

/**
 * cfg_copy -  ��������
 * @param dst: Ŀ������
 * @param dst: Դ����
 *
 * @returns
 *     �����޸�: ����CHANGED
 *     ����δ�޸� ����UNCHANGED
 */
int cfg_check(char *dst, char *src)
{
    if (dst[0] == '\0' || strcmp(dst, src)) {
        return CHANGED;
    }

    return UNCHANGED;
}

/**
 * delete_diskfiles -  ɾ��Ŀ¼�µ��ļ�
 * @param path: Ŀ¼·��
 *
 * ɾ��Ŀ¼�µ��ļ���������Ŀ¼
 */
void delete_diskfiles(char *path)
{
    int ret;    
    char cmd[MAX_PATH_LEN] = {0};

    if (path == NULL) {        
        return;    
    }    

    ret = snprintf(cmd, MAX_PATH_LEN, "rm -rf %s*", path);    
    if (ret > 0) {        
        system(cmd);    
    }
}

/**
 * delete_diskdir -  ɾ��Ŀ¼�����ļ�
 * @param path: Ŀ¼·��
 *
 * ɾ��Ŀ¼�µ��ļ���������Ŀ¼
 */
void delete_diskdir(char *path)
{
    if (path == NULL) {
        return;    
    }    

    delete_diskfiles(path);    
    (void)rmdir(path);
}

/**
 * webtamper_sleep -  ����
 * @param time: ����ʱ��
 *
 * ����
 */
void webtamper_sleep(long time) 
{
    struct timeval tv;
    tv.tv_usec = 0;
    tv.tv_sec = time;
    select(0, NULL, NULL, NULL, &tv);
}

/**
 * get_info -  ��ȡ�ļ�����
 * @param name: �ļ���
 * @param statbuf: ��������
 * @param dir: ���ʱ��
 *
 * @returns
 *     �ɹ�: ����0
 *     ʧ��: ����-1
 */
static int get_info(char *fulname, char *name, struct stat *statbuf, directory_t *dir)
{
    dir->name = (char *)malloc(strlen(name) + strlen("/") + 1);  /* �ڴ���Ҫ���� */
    if (dir->name == NULL) {
        return -1; 
    }
    
    strcpy(dir->name, name);
    if (S_ISDIR(statbuf->st_mode) && !strchr(dir->name, '/')) {
        strcat(dir->name, "/");
    }

    dir->isfile = !S_ISDIR(statbuf->st_mode);
    dir->mtime = statbuf->st_mtime;
    dir->atime = statbuf->st_atime;
    dir->gid = statbuf->st_gid;
    dir->uid = statbuf->st_uid;
    dir->permissions = statbuf->st_mode;

    if (dir->isfile) {
        dir->filesize = statbuf->st_size;
        dir->md5_value = md5_value_align(dir->md5_value_unaligned, MD5_ALIGN);
        md5_generate_value(fulname, dir->md5_value);
    }

    return 0;
}

/**
 * build_hashtree -  ������ϣ���Ŀ¼������ʼ�����۸ķ�����ʹ��
 * @param mutex: Ŀ¼λ��
 * @param root: Ŀ¼���ڵ�
 * @param hash: ��ϣ��
 *
 * ������ϣ���Ŀ¼������ʼ�����۸ķ�����ʹ��
 */
void build_hashtree(char *path, void *root, struct hash *hash)
{
    DIR *dp;
    struct dirent *entry;  
    struct stat statbuf;  
    directory_t *node;
    int ret;
    char dirdp[MAX_PATH_LEN] = {0};
    directory_t *dir;

    dir = (directory_t *)root;
    
    strcpy(dirdp, path);
    
    if((dp = opendir(path)) == NULL) {  
        TAMPER_LOG_WRITE(PE_LOG_INFO, "can not open directory: %s\n", path);
        return;
    }  
    
    chdir(path);
    while((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name, &statbuf);
        if(S_ISDIR(statbuf.st_mode)) {
            if(strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0) {
                continue;  
            }
            
            /* ����Ŀ¼���ڵ� */
            node = (directory_t *)malloc(sizeof(directory_t));
            if (node == NULL) {
                return;
            }
            memset(node, 0, sizeof(directory_t));

            /* ��ȡ�ļ������� */
            add_lastdirname(dirdp, entry->d_name);
            ret = get_info(dirdp, entry->d_name, &statbuf, node);
            if (ret == -1) {
                free(node);
                return;
            }

            /* ����ڵ� */
            ret = dir_add_node(dir, node);
            if (ret == -1) {
                free(node->name);
                free(node);
                return;
            }
            
            hash_set(hash, dirdp, node);
            build_hashtree(dirdp, node, hash);  
            del_lastdirname(dirdp);
        } else {
            /* ����Ŀ¼���ڵ� */
            node = (directory_t *)malloc(sizeof(directory_t));
            if (node == NULL) {
                return;
            }
            memset(node, 0, sizeof(directory_t));
            
            /* ��ȡ�ļ����� */
            add_lastfilename(dirdp, entry->d_name);
            ret = get_info(dirdp, entry->d_name, &statbuf, node);
            if (ret == -1) {
                free(node);
                return;
            }

            /* ����ڵ� */
            ret = dir_add_node(dir, node);
            if (ret == -1) {
                free(node->name);
                free(node);
                return;
            }
        
            /* �����ϣ�� */
            hash_set(hash, dirdp, node);
            del_lastfilename(dirdp);
        }
    }  
    chdir("..");  
    
    closedir(dp); 
}

/**
 * interval_check -  ��ѯ���
 * @param mutex: ������
 * @param cond: ��������
 * @param interval_time: ���ʱ��
 *
 * ���سɹ�����־
 */
int interval_check(pthread_mutex_t *mutex, pthread_cond_t *cond, int interval_time)
{
    int  condret;
    struct timespec timeout;

    timeout.tv_sec = time(0) + interval_time;
    timeout.tv_nsec = 0;

    pthread_mutex_lock(mutex);
    condret = pthread_cond_timedwait(cond, mutex, &timeout);
    pthread_mutex_unlock(mutex);

    return condret;
}

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
int download_mark(const char *filename, const char *mark)
{
    FILE *fp;
    
    fp = fopen(filename, "w");
    if (fp == NULL) {
        return -1;
    }

    fputs(mark, fp);

    fclose(fp);

    return 0;
}

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
int is_download_success(const char *filename)
{
    FILE *fp;
    char mark[2] = {0};    /* �ļ���ֻ����ȡ1���ַ� */
    
    fp = fopen(filename, "r");
    if (fp == NULL) {
        return 0;
    }
    fgets(mark, 2, fp);    /* �ļ���ֻ����ȡ1���ַ� */
    fclose(fp);

    return !(strcmp(mark, "1"));
}

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
int make_previous_dir(char *path)
{
    char cmd[MAX_PATH_LEN] = {0};
    char *p;    

    p = strrchr(path, '/');
    if (p != NULL) {
        snprintf(cmd, p - path + 12, "mkdir -p '%s", path);
        strcat(cmd, "'");
        system(cmd);
        return 0;
    }

    return -1;
}

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
void webtamper_log(char *wsname, char *host, char *action, svn_t *svn, char *local_root, 
        char *server_root)
{
    struct list_head *pos;
    struct list_head *tmppos;
    svn_node_t *tmp;
    char filename[MAX_PATH_LEN] = {0};

    list_for_each_safe(pos, tmppos, &svn->add_files.brother) {
        tmp = list_entry(pos, svn_node_t, brother);
        sprintf(filename, "%s%s", server_root ,tmp->filename + strlen(local_root));
        if (!tmp->isstored && !strcmp(action, "check")) {
            log_write(wsname, host, action, tmp->detecttime, tmp->tampertime, tmp->restoretime, svn->last_version, tmp->action, 
                filename);

            syslog_print_info("WAF", "WEBTAMPER","File [%s] has been [%s] on website[%s] (host is [%s])\n",
                    filename, "Added", wsname, host);

            tmp->isstored = !tmp->isstored;
        }

        if (!strcmp(action, "arestore") || !strcmp(action, "mrestore")) {
            log_write(wsname, host, action, tmp->detecttime, tmp->tampertime, tmp->restoretime, svn->last_version, tmp->action, 
                filename);

            syslog_print_info("WAF", "WEBTAMPER","File [%s] has been %s successfully on website[%s]"
                " (host is [%s])\n", filename, action, wsname, host);
        }
    }

    list_for_each_safe(pos, tmppos, &svn->update_files.brother) {
        tmp = list_entry(pos, svn_node_t, brother);
        sprintf(filename, "%s%s", server_root ,tmp->filename + strlen(local_root));
        if (!tmp->isstored && !strcmp(action, "check")) {
            log_write(wsname, host, action, tmp->detecttime, tmp->tampertime, tmp->restoretime, svn->last_version, tmp->action, 
                filename);

            syslog_print_info("WAF", "WEBTAMPER","File [%s] has been [%s] on website[%s] (host is [%s])\n",
                    filename, "Modified", wsname, host);

            tmp->isstored = !tmp->isstored;
        }

        if (!strcmp(action, "arestore") || !strcmp(action, "mrestore")) {
            log_write(wsname, host, action, tmp->detecttime, tmp->tampertime, tmp->restoretime, svn->last_version, tmp->action, 
                filename);

            syslog_print_info("WAF", "WEBTAMPER","File [%s] has been %s successfully on website[%s]"
                " (host is [%s])\n", filename, action, wsname, host);
        }
    }

    list_for_each_safe(pos, tmppos, &svn->delete_files.brother) {
        tmp = list_entry(pos, svn_node_t, brother);
        sprintf(filename, "%s%s", server_root ,tmp->filename + strlen(local_root));
        if (!tmp->isstored && !strcmp(action, "check")) {
            log_write(wsname, host, action, tmp->detecttime, tmp->tampertime, tmp->restoretime, svn->last_version, tmp->action, filename);

            syslog_print_info("WAF", "WEBTAMPER","File [%s] has been [%s] on website[%s] (host is [%s])\n",
                filename, "Deleted", wsname, host);

            tmp->isstored = !tmp->isstored;
        }

        if (!strcmp(action, "arestore") || !strcmp(action, "mrestore")) {
            log_write(wsname, host, action, tmp->detecttime, tmp->tampertime, tmp->restoretime, svn->last_version, tmp->action, filename);

            syslog_print_info("WAF", "WEBTAMPER","File [%s] has been %s successfully on website[%s]"
                " (host is [%s])\n", filename, action, wsname, host);
        }
    }
}

int log_send(char *buf, int is_buffer)
{
    int rv, i;
    
    if (!buf) {
        return -1;
    }

    i = 0;
    rv = pe_log_client_write(g_log_client, is_buffer, buf, strlen(buf));
    if (rv < 0) {
        for (i = 0; i < RECONNECT_NUM; ++i) {
            if (!pe_log_client_check_conn(g_log_client)) {
                    rv = pe_log_client_connect(g_log_client);
                    if (rv != 0) {
                        continue;
                    }
                }

            rv = pe_log_client_write(g_log_client, is_buffer, buf, strlen(buf));
            break ;
        }
    }

    if (i == RECONNECT_NUM) {
        TAMPER_LOG_WRITE(PE_LOG_FATAL, "reconnect failure %d", rv);
    }
    
    return rv;
}

void send_detect_log(char *ip)
{
#define SQL_STATEMENT_LEN 10000
    int rv;
    char sql_statement[SQL_STATEMENT_LEN] = {0};
 
    snprintf(sql_statement, SQL_STATEMENT_LEN, "insert into tamper_log_temp_table values(UNIX_TIMESTAMP(), '%s');", ip);

    rv = log_send(sql_statement, 0);
    if (rv < 0) {
        TAMPER_LOG_WRITE(PE_LOG_INFO, "send tamper temp record to log server failed.");
    }
}

unsigned long long get_tamper_total_sizes(char *backup_path)
{
    struct statfs diskInfo;
    int ret;
    
    ret = statfs(backup_path, &diskInfo);
    if (ret == 0) {
        return (unsigned long long)diskInfo.f_bsize * (unsigned long long)diskInfo.f_blocks;
    } else {
        return 0;
    }
}

