/*
 * $Id: 3des.h 2786 2013-07-09 16:42:55 FreeWAF Development Team $
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
 
#ifndef _3DES_H_
#define _3DES_H_

#include "apr_pools.h"
#include "openssl/des.h"

#define LEN_OF_KEY              24
#define LEN_OF_STEP             8

/**
 * get_3des_key:��ȡ3des���ܵ���Կ������24���ֽ�
 * @msr:�����Ľṹ��
 *
 * ����ֵ��ʧ�ܷ���NULL���ɹ�����key�ַ���
 */
extern unsigned char *get_3des_key(apr_pool_t *pool, void *word);

/**
 * get_3des_vector:��ȡ3des���ܵ�������������������Ϊ8���ֽ�
 * @msr:�����Ľṹ��
 *
 * ����ֵ��ʧ�ܷ���NULL���ɹ�����vector�ַ���
 */
extern unsigned char *get_3des_vector(apr_pool_t *pool, void *word);

/**
 * msc_tripleDes:3des��/����
 * @mptmp:���ڷ����ڴ���ڴ��
 * @data:����
 * @kkey:��Կ
 * @iv:�������� 
 * @decode_or_encode_flag:����/���ܿ���
 *
 * ����ֵ��ʧ�ܷ���NULL���ɹ�����vector�ַ���
 */
extern unsigned char* tripleDes(apr_pool_t *mptmp, const unsigned char *data,
                        const unsigned char *kkey, const unsigned char *iv, int decode_or_encode_flag);

#endif

