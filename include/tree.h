/*
 * $Id: tree.h 2786 2013-07-09 16:42:55 FreeWAF Development Team $
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
#ifndef _TREE_H_
#define _TREE_H_

#include <stdlib.h>
#include "list.h"
#include "util.h"

#define TYPE_UNTOUCHED 0
#define TYPE_TOUCHED   1

typedef struct directory_s {
    struct directory_s *parent;     
    struct directory_s *firstchild;

    struct list_head brother;
    char *name;
    int  isfile;

    /* �ļ����� */
    unsigned long long filesize;                                /* �ļ����� */
    unsigned long uid, gid;                                     /* �û�ID�� ��ID */
    unsigned long permissions;                                  /* �ļ�Ȩ�ޡ��ļ����� */
    unsigned long atime, mtime;                                 /* �ļ�����ʱ�䡢 �ļ��޸�ʱ�� */
    unsigned char md5_value_unaligned[MD5_BYTES + MD5_ALIGN];   /* �ļ�md5ֵ */
    unsigned char *md5_value;

    int  touched;
} directory_t;

/**
 * dir_add_node -  ��ӽڵ�
 * @param parent: ���ڵ�
 * @param node: �ӽڵ�
 *
 * �ڸ��ڵ�������ӽڵ�
 *
 * @returns
 *     �ɹ�: ����0
 *     ʧ��: ����-1
 */
extern int dir_add_node(directory_t *parent, directory_t *node);

/**
 * dir_walk_tree -  ����Ŀ¼��
 * @param dir: Ŀ¼���ڵ�
 *
 * ��������Ŀ¼��
 *
 * @returns
 *     �ɹ�: ����0
 *     �ɹ�: ����-1
 */
extern int dir_walk_tree(directory_t *dir);

/**
 * dir_get_local_path -  ��ȡ�ļ�����Ӧ�Ĵ���·��
 * @param node: �ڵ�
 * @param path: ·��������
 * @param pathlen: ·������������
 *
 * ��ȡ�ļ�����Ӧ�Ĵ���·��
 *
 * @returns
 *     �ɹ�: ����0
 *     �ɹ�: ����-1
 */
extern int dir_get_local_path(directory_t *node, char *path, int pathlen);

/**
 * dir_cmp_fun1 -  ��ϣ��ͻ�Ƚ��㷨
 * @param data1: �Ƚ�����1
 * @param data2: �Ƚ�����2
 *
 * ��ϣ��ͻ�Ƚ��㷨
 *
 * @returns
 *    ���رȽϽ������ͬ����1����ͬ����0
 */
extern int dir_cmp_fun1(void *data1, void *data2);

/**
 * dir_cmp_fun2 -  ��ϣ��ͻ�Ƚ��㷨
 * @param data1: �Ƚ�����1
 * @param data2: �Ƚ�����2
 *
 * ��ϣ��ͻ�Ƚ��㷨
 *
 * @returns
 *    ���رȽϽ������ͬ����1����ͬ����0
 */
extern int dir_cmp_fun2(void *data1, void *data2);

/**
 * dir_destroy_tree -  ��������Ŀ¼��
 * @param dir: Ŀ¼�����ڵ�
 *
 * ��������Ŀ¼��
 *
 * @returns
 *     �ɹ�: ����0
 *     �ɹ�: ����-1
 */
extern int dir_destroy_tree(directory_t *dir);

/**
 * dir_get_parent_node -  ��ȡ�ļ����ڵ��ַ
 * @param root: ���ڵ�
 * @param path: �ļ�·��
 * @param node_type: �ڵ�����
 *
 * ��ȡ�ļ����ڵ��ַ
 *
 * @returns
 *     �ɹ�: �����ļ����ڵ��ַ
 *     ʧ��: NULL
 */
extern directory_t *dir_get_parent_node(directory_t *root, char *path, int node_type);

/**
 * dir_get_self_node -  ��ȡ�ļ��ڵ��ַ
 * @param root: ���ڵ�
 * @param path: �ļ�·��
 * @param node_type: �ڵ�����
 *
 * ��ȡ�ļ��ڵ��ַ
 *
 * @returns
 *     �ɹ�: �����ļ��ڵ��ַ
 *     ʧ��: NULL
 */
extern directory_t *dir_get_self_node(directory_t *root, char *name, int node_type);

/**
 * dir_update_node -  ���½ڵ�����
 * @param node: �ڵ�
 * @param fileinfo: �ڵ��������
 *
 * ���½ڵ�����
 *
 * @returns
 *     �ɹ�: ����0
 *     �ɹ�: ����-1
 */
extern int dir_update_node(directory_t *node, file_t *fileinfo);

/**
 * dir_del_node -  ɾ���������ӽڵ�
 * @param node: �ڵ�
 *
 * ɾ���������ӽڵ�
 *
 * @returns
 *     �ɹ�: ����0
 *     �ɹ�: ����-1
 */
extern int dir_del_node(directory_t *node);

#endif

