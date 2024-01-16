/*
 * $Id: tree.c 2786 2013-07-09 16:42:55 FreeWAF Development Team $
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
#include <string.h>
#include "util.h"
#include "tree.h"
#include "print.h"

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
directory_t *dir_get_self_node(directory_t *root, char *path, int node_type)
{
    char *p, *q;
    directory_t *head;
    directory_t *tmp;
    struct list_head *pos;

    if (path == NULL || root == NULL) {
        return NULL;
    }
 
    if (!strcmp(path, root->name)) {
        return root;
    } else {
        head = root->firstchild;
        if (head == NULL) {
            return NULL;
        }
    }

    p = path + strlen(root->name);

    tmp = NULL;
    q = strchr(p, '/'); 
    if (q == NULL) {
        list_for_each(pos, &head->brother) {
            tmp = list_entry(pos, directory_t, brother);
            if (!strcmp(tmp->name, p)) {
                return tmp;
            }
        }
        return NULL;
    }   

    tmp = NULL;
    while (q) {
        list_for_each(pos, &head->brother) {
            tmp = list_entry(pos, directory_t, brother);
            if (!strncmp(tmp->name, p, q - p + 1)) {
                head = tmp->firstchild;
                if (head == NULL) {
                    return tmp;
                } else {
                    break;
                }
            } else {
                tmp = NULL;
            }
        }
        
        p = q + 1;
        q = strchr(p , '/');
    }

    if (tmp && node_type == NODE_FILE) {
        head = tmp->firstchild;
        list_for_each(pos, &head->brother) {
            tmp = list_entry(pos, directory_t, brother);
            if (strcmp(tmp->name, p)) {
               tmp = NULL;
            } else {
                break;
            }
        }
    }

    return tmp;
}

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
directory_t *dir_get_parent_node(directory_t *root, char *path, int node_type)
{
    char parentpath[MAX_PATH_LEN] = {0};
    char *p, *q;

    if (node_type == NODE_FILE) {
        p = strrchr(path, '/');
        strncpy(parentpath, path, p - path + 1);
    } else {
        p = strrchr(path, '/');
        q = p;
        *p = 0;
        p = strrchr(path, '/');
        *q = '/';
        strncpy(parentpath, path, p - path + 1);
    }   

    return dir_get_self_node(root, parentpath, NODE_DIR);
}

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
int dir_add_node(directory_t *parent, directory_t *node)
{
    if (!parent || !node) {
        return -1;
    }
    
    if (parent->firstchild) {
        node->parent = parent;
        list_add_tail(&node->brother, &parent->firstchild->brother);
    } else {
        directory_t *headnode = (directory_t *)malloc(sizeof(directory_t));
        if (headnode == NULL) {
            fprintf(stderr, "malloc failed\n");
            return -1;
        }
        headnode->parent = parent;
        parent->firstchild = headnode;
        node->parent = parent;
        INIT_LIST_HEAD(&headnode->brother);
        list_add_tail(&node->brother, &headnode->brother);
    }
   
    return 0;
}

/* ɾ���ڵ��µ��ӽڵ� */
static int delete_node(directory_t *dir)
{
    directory_t *head;
    directory_t *tmp;
    struct list_head *pos;
    struct list_head *postmp;

    if (dir->firstchild == NULL) {
        return -1;
    }

    head = dir->firstchild;
    list_for_each_safe(pos, postmp, &head->brother) {
        tmp = list_entry(pos, directory_t, brother);
        if (!tmp->isfile) {
            dir_destroy_tree(tmp);
        }
        free(tmp->name);
        tmp->name = NULL;
        list_del_init(&tmp->brother);
        free(tmp);
    }
    /* fixup: �ظ����ݳ����޸� */
    dir->firstchild = NULL;
    free(head);

    return 0;
}

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
int dir_del_node(directory_t *node)
{
    if (node == NULL) {
        return -1;
    }

    delete_node(node);

    free(node->name);
    node->name = NULL;
    list_del_init(&node->brother);
    free(node);
        
    return 0;
}

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
int dir_update_node(directory_t *node, file_t *fileinfo)
{
    if (node == NULL) {
        return -1;
    }

    node->atime = fileinfo->atime;
    node->filesize = fileinfo->filesize;
    node->gid = fileinfo->gid;
    node->mtime = fileinfo->mtime;
    node->permissions = fileinfo->permissions;
    node->uid = fileinfo->uid;

    return 0;
}

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
int dir_get_local_path(directory_t *node, char *path, int pathlen)
{
    if (node == NULL || path ==NULL) {
        return -1;
    }

    if (node->parent) {
        dir_get_local_path(node->parent, path, pathlen);
    }

    if (strlen(path) + strlen(node->name) < pathlen) {
        strcat(path, node->name);
    }

    return 0;
}

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
int dir_cmp_fun1(void *data1, void *data2)
{
    char abspath1[MAX_PATH_LEN] = {0};
    directory_t *hashdir;
    char *abspath2;
    
    hashdir = (directory_t *)data1;
    abspath2 = (char *)data2;

    /* ֻ��ȽϾ���·��(·��+�ļ���)�Ƿ���� */
    dir_get_local_path(hashdir, abspath1, MAX_PATH_LEN);

    return (!strcmp(abspath1, abspath2));
}

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
int dir_cmp_fun2(void *data1, void *data2)
{
    int ret;
    char abspath1[MAX_PATH_LEN] = {0};
    char abspath2[MAX_PATH_LEN] = {0};
    directory_t *hashdir1;
    directory_t *hashdir2;
    
    hashdir1 = (directory_t *)data1;
    hashdir2 = (directory_t *)data2;

    /* ֻ��ȽϾ���·��(·��+�ļ���)�Ƿ���� */
    dir_get_local_path(hashdir1, abspath1, MAX_PATH_LEN);
    dir_get_local_path(hashdir2, abspath2, MAX_PATH_LEN);

    ret = !strncmp(abspath1, abspath2, strlen(abspath2));

    return ret;
}

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
int dir_destroy_tree(directory_t *dir)
{
    return delete_node(dir);
}

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
int dir_walk_tree(directory_t *dir)
{
    directory_t *head;
    directory_t *tmp;
    struct list_head *pos;

    if(dir == NULL || dir->firstchild == NULL) {
        return 0;
    }

    head = dir->firstchild;
    list_for_each(pos, &head->brother) {
        tmp = list_entry(pos, directory_t, brother);
        if (tmp->isfile) {
            msg_printf("    %s   %d\n", tmp->name, tmp->touched);
        } else {
            dir_walk_tree(tmp);
        }
    }

    return 0;
}

