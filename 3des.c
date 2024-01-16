/*
 * $Id: 3des.c 2786 2013-07-09 16:42:55 FreeWAF Development Team $
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
 
#include "3des.h"
#include "apr_strings.h"

#define VALID_HEX(X) \
    (((X >= '0')&&(X <= '9')) || ((X >= 'a')&&(X <= 'f')) || ((X >= 'A')&&(X <= 'F')))

static unsigned char x2c(unsigned char *what) 
{
    register unsigned char digit;

    digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A') + 10 : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10 : (what[1] - '0'));

    return digit;
}

static char *bytes2hex(apr_pool_t *pool, unsigned char *data, int len) 
{
    static const unsigned char b2hex[] = "0123456789abcdef";
    char *hex = NULL;
    int i, j;

    hex = apr_palloc(pool, (len * 2) + 1);
    if (hex == NULL) return NULL;

    j = 0;
    for(i = 0; i < len; i++) {
        hex[j++] = b2hex[data[i] >> 4];
        hex[j++] = b2hex[data[i] & 0x0f];
    }

    hex[j] = 0;

    return hex;
}


static int hex2bytes_inplace_3des(unsigned char *data, int len)
{
	unsigned char *d = data;
    int i, count = 0;

    if ((data == NULL)||(len == 0)) return 0;

    for(i = 0; i <= len - 1; i++) {
        if(VALID_HEX(data[i]) && VALID_HEX(data[i+1]))  {
            *d++ = x2c(&data[i]);
            i++;            
            count++;
		} else {
			break;
		}
    }

    return count;
}

/**
 * get_3des_key:��ȡ3des���ܵ���Կ������24���ֽ�
 * @msr:�����Ľṹ��
 *
 * ����ֵ��ʧ�ܷ���NULL���ɹ�����key�ַ���
 */
unsigned char *get_3des_key(apr_pool_t *pool, void *word)
{
    unsigned char *key;
    
    if (pool == NULL) {
        return NULL;
    }
    
    /*��ȡkey��������ǰ25�ֽ�*/
    key = (unsigned char *)apr_pmemdup(pool, (void *)word, 25);
    if (key == NULL) {
        return NULL;
    }
    /*��25λ�����ַ���������������24���ֽڵ�key*/
    key[24] = 0;
    
    return key;
}

/**
 * get_3des_vector:��ȡ3des���ܵ�������������������Ϊ8���ֽ�
 * @msr:�����Ľṹ��
 *
 * ����ֵ��ʧ�ܷ���NULL���ɹ�����vector�ַ���
 */
unsigned char *get_3des_vector(apr_pool_t *pool, void *word)
{

    unsigned char* vector;
    
    if (pool == NULL) {
        return NULL;
    }
    
    /*��ȡkey�������ĵ�25λ����33λ*/
    vector = (unsigned char *)apr_pmemdup(pool, (void *)word + 24, 9);
    if (vector == NULL) {
        return NULL;
    }
    
    /*��9λ�����ַ���������������8���ֽڵ�����*/
    vector[8] = 0;
    
    return vector;
}

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
unsigned char *tripleDes(apr_pool_t *mptmp, const unsigned char *data,
                const unsigned char *kkey, const unsigned char *iv, int decode_or_encode_flag)
{   
    int data_rest;
    int data_len;
    int count, i;
    unsigned char ch;

    unsigned char *tmp;
    unsigned char *data_real;
    unsigned char *src; /* ���������� */
    unsigned char *dst; /* ���ܺ������ */
    int len; 
    
    unsigned char in[LEN_OF_STEP];
    unsigned char out[LEN_OF_STEP];

    int key_len;
    unsigned char key[LEN_OF_KEY]; /* ��������Կ */
    unsigned char block_key[LEN_OF_STEP + 1];
    DES_key_schedule ks,ks2,ks3;
    
    /* ��μ�� */
    if (mptmp == NULL || data == NULL || kkey == NULL || iv == NULL) {
        return NULL;
    }
    if (decode_or_encode_flag != DES_ENCRYPT && decode_or_encode_flag != DES_DECRYPT) {
        return NULL;
    }

    /* ���첹������Կ */
    key_len = strlen((char *)kkey);
    memcpy(key, (char *)kkey, (key_len < LEN_OF_KEY)?key_len:LEN_OF_KEY);
    if (key_len < LEN_OF_KEY) {
        memset(key + key_len, 0x00, LEN_OF_KEY - key_len);
    }

    /* ����һ��ԭʼ���� */
    data_len = strlen((char *)data);
    data_real = apr_pmemdup(mptmp, data, data_len + 1);    
    
    /* ����/����ǰ���� */
    ch = '\0';
    len = 0;
    data_rest = 0;
    if (decode_or_encode_flag == DES_ENCRYPT) {
        data_real = (unsigned char *)apr_pstrcat(mptmp, (char *)data_real, "|", (char *)iv, NULL);
        
        /* ����������������ռ估����������� */
        data_len = strlen((char *)data_real);
        data_rest = data_len % LEN_OF_STEP;
        if (data_rest != 0) {
            len = data_len + (LEN_OF_STEP - data_rest);
            ch = LEN_OF_STEP - data_rest;
        } else {
            len = data_len;
        }
        
    } else {
        /* ����ǰҪ��16�����ַ���ת�����ֽ��� */
        data_len = hex2bytes_inplace_3des(data_real, data_len);
        len = data_len;
    }

    src = (unsigned char *)apr_palloc(mptmp, len + 1);
    dst = (unsigned char *)apr_palloc(mptmp, len + 1);
    if (src != NULL && dst != NULL) {

        /* ���첹���ļ������� */
        memset(src, 0, len + 1);
        memcpy(src, data_real, data_len);
        
        if (data_rest != 0) {
            memset(src + data_len, ch, LEN_OF_STEP - data_rest);
        }        
        
        /* ��Կ�û� */
        memset(block_key, 0, sizeof(block_key));
        memcpy(block_key, key + 0, LEN_OF_STEP);
        DES_set_key_unchecked((const_DES_cblock *)block_key, &ks);
        memcpy(block_key, key + LEN_OF_STEP, LEN_OF_STEP);
        DES_set_key_unchecked((const_DES_cblock *)block_key, &ks2);
        memcpy(block_key, key + LEN_OF_STEP * 2, LEN_OF_STEP);
        DES_set_key_unchecked((const_DES_cblock *)block_key, &ks3);

        /* ѭ������/���ܣ�ÿ8�ֽ�һ�� */
        count = len / LEN_OF_STEP;
        for (i = 0; i < count; i++) {
            memset(in, 0, LEN_OF_STEP);
            memset(out, 0, LEN_OF_STEP);
            memcpy(in, src + LEN_OF_STEP * i, LEN_OF_STEP);

            DES_ecb3_encrypt((const_DES_cblock *)in, (DES_cblock *)out, &ks, &ks2, &ks3, decode_or_encode_flag);
            /* �����ܵ����ݿ��������ܺ������ */
            memcpy(dst + LEN_OF_STEP * i, out, LEN_OF_STEP);
        }
        *(dst + len) = 0; 
        
        if (decode_or_encode_flag == DES_DECRYPT) {
            /* ������Ҫ�������ĺ�������������ȥ������ */
            tmp = (unsigned char *)strrchr((const char *)dst, '|');
            if (tmp != NULL) {
                *tmp++ = 0;
                if (strncasecmp((const char *)tmp, (const char *)iv, strlen((char *)iv)) == 0) {
                    return dst;
                }
            }
        } else {
            /* ������Ҫ�����ı����16�����ַ��� */
            return (unsigned char *)bytes2hex(mptmp, dst, len);
        }
        
    }
    
    return NULL;
}

