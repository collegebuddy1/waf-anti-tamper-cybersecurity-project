/*
 * $Id: log.c 2786 2013-07-09 16:42:55 FreeWAF Development Team $
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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include "log.h"

/* ��־���Ƴ��� */
#define LOG_FILENAME_LEN    512

/* ������־�������� */
#define LOG_LINE_LEN        1024

/* ȫ����־����ָ�� */
pe_log_t *the_log = 0;

/* ��־���� */
struct pe_log_t {
    /* ��־���� */
    char log_file[LOG_FILENAME_LEN];

    /* ��־���� */
    int level;

    /* ��־�� */
    sem_t mtx_op;

    /* ��־�ļ���С���� */
    int max_fsize;

    /* ��־�ļ���ǰ��С */
    int cur_fsize;

    /* �ļ���� */
    int file;
};

/**
 * pe_log_create - ������־���� 
 * ���������
 * 
 * �ɹ�����һ����־����ʧ�ܷ���NULL
 * 
 */
pe_log_t *pe_log_create(void)
{
    pe_log_t *log;
    int rv;
    
    /* �ӻ���ط�����־����ṹ */
    log = malloc(sizeof(pe_log_t));
    if (log == NULL) {
        return NULL;
    }

    /* ��ʼ��������Ա */
    (void)memset(log, 0, sizeof(pe_log_t));
    rv = sem_init(&log->mtx_op, 1, 1);
    if (rv == -1) {
        free(log);
        return NULL;
    }

    return log;
}

/**
 * pe_log_initialize - ��־�����ʼ��
 * @log: ��־����
 * @name: ��־·��
 * @level: ��־�ȼ�
 * @fsize: ��־����
 *
 * �ɹ�����һ��0��ʧ�ܷ���-1
 * 
 */
int pe_log_initialize(pe_log_t *log, const char *name, int level, int fsize)
{
    int ret;
    struct stat file_stat;

    if (!log || !name || level < PE_LOG_DEBUG || level > PE_LOG_STOP) {
        return -1;
    }
    
    if (level == PE_LOG_STOP) {
        /* ��ֹ��־ */
        return 0;
    }
    
    log->level = level;
    log->max_fsize = fsize;
    log->cur_fsize = 0;

    /* ��־�ļ����� */
    (void)snprintf(log->log_file, LOG_FILENAME_LEN, "%s", name);

    /* ��ȡ�ļ����� */
    ret = stat(log->log_file, &file_stat);
    if (ret < 0) {
        log->cur_fsize = 0;
    } else {
        log->cur_fsize = file_stat.st_size;
    }

    /* ����־�ļ� */
    log->file = open(log->log_file, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);
    if (log->file == -1) {
        (void)printf("log open fail!errno(%d),%s\n", errno, strerror(errno));
        return -1;
    }

    the_log = log;

    return 0;
}

/**
 * pe_log_initialize - ��־����д��
 * @log: ��־����
 * @level: ��־�ȼ�
 * @file: д����־���ļ�
 * @line: ����
 * @fmt: ����
 *
 * �޷���ֵ
 * 
 */
void pe_log_write(pe_log_t *log, int level, const char *file, int line, const char *fmt, ...)
{
    /* ���ڼ�¼��־�г��� */
    int log_len;
    /* ������־���� */
    char *str_level;
    /* ���ڻ�ȡ��ǰ�Ŀɶ�ʱ�� */
    time_t time_current;
    struct tm systime;
    struct timeval nowtimeval;
    /* ������־����װ���ļ��ض� */
    char buf_log[LOG_LINE_LEN] = { 0 };
    char buf_fmt[LOG_LINE_LEN] = { 0 };
    /* ���ڶ�ȡ���� */
    va_list args;

    if (log == 0 || log->file == 0 || file == 0) {
        return;
    }
    if (level < log->level) {
        /* ��־���𲻹� */
        return;
    }

    /* ��ȡ���� */
    va_start(args, fmt);
    (void)vsnprintf(buf_fmt, sizeof(buf_fmt) - 1, fmt, args);
    va_end(args);

    /* ��־���� */
    switch (level) {
    case PE_LOG_DEBUG:
        str_level = "DEBUG";
        break;
    case PE_LOG_INFO:
        str_level = "INFO";
        break;
    case PE_LOG_WARN:
        str_level = "WARN";
        break;
    case PE_LOG_FATAL:
        str_level = "FATAL";
        break;
    default:
        return;
    }

    /* ��ȡʱ�� */
    (void)gettimeofday(&nowtimeval, 0);
    time_current = nowtimeval.tv_sec;
    (void)gmtime_r(&time_current, &systime);

    /* ��װ log ���� */
    log_len = snprintf(buf_log, sizeof(buf_log), 
        "[%02d.%02d %02d:%02d:%02d.%06lu][%40s:%4d][%5s][%5lu][%5lu] %s\n", 
        systime.tm_mon + 1, 
        systime.tm_mday, 
        systime.tm_hour, 
        systime.tm_min, 
        systime.tm_sec, 
        nowtimeval.tv_usec, 
        file, line, 
        str_level, 
        (unsigned long)getpid(), 
        (unsigned long)syscall(SYS_gettid), 
        buf_fmt);

    /* д����־ */
    if (log->max_fsize == 0) {  
        return;
    }
    (void)sem_wait(&log->mtx_op);
    if (log->cur_fsize + log_len >= log->max_fsize) {
        int read_size;
        int file_tmp;
        char name_tmp[LOG_FILENAME_LEN] = { 0 };

        /* ����һ����ʱ�ļ������ڽضϿ�����־�ļ����� */
        (void)snprintf(name_tmp, LOG_FILENAME_LEN - 4, "%s", log->log_file);
        (void)strcat(name_tmp, ".tmp");
        file_tmp = open(name_tmp, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);
        if (file_tmp == -1) {
            (void)sem_post(&log->mtx_op);
            return;
        }
        
        /* ����־�ļ��н�ȡ����һ�룬���������ʱ�ļ��� */
        (void)lseek(log->file, log->cur_fsize / 2, SEEK_SET);
        read_size = 0;
        do {
            read_size = read(log->file, buf_fmt, sizeof(buf_fmt));
            if (read_size > 0) {
                if (write(file_tmp, buf_fmt, read_size) == -1) {
                    return;
                }
                
                (void)fsync(file_tmp);
            }
        } while (sizeof(buf_fmt) == read_size);
        
        /* �����־�ļ� */
        if (ftruncate(log->file, 0) == -1) {
            return;
        }
        
        (void)lseek(log->file, 0, SEEK_SET);
        /* ������־�ļ���С */
        log->cur_fsize /= 2;
        
        /* ����ʱ�ļ�����������д�ص���־�ļ� */
        read_size = 0;
        do {
            read_size = read(file_tmp, buf_fmt, sizeof(buf_fmt));
            if (read_size > 0) {
                if (write(log->file, buf_fmt, read_size) == -1) {
                    return;    
                }
                
                (void)fsync(log->file);
            }
        } while (sizeof(buf_fmt) == read_size);
        
        /* �ضϿ�����ϣ�ɾ����ʱ�ļ� */
        (void)close(file_tmp);
        (void)remove(name_tmp);
    }
    
    if (log->file != 0) {
        if (write(log->file, buf_log, log_len) == -1) {
            return;
        }
        
        (void)fsync(log->file);
        log->cur_fsize += log_len;
    }
    (void)sem_post(&log->mtx_op);
}

/**
 * pe_log_initialize - ������־���� 
 * @log: ��־����
 *
 * �޷���ֵ
 * 
 */
void pe_log_destroy(pe_log_t *log)
{
    if (log == 0) {
        return;
    }
    
    (void)sem_destroy(&log->mtx_op);
    if (log->file) {
        (void)close(log->file);
        log->file = 0;
    }  
    free(log);
}

