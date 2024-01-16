/*
 * $Id: tamperlog.h 2786 2013-07-09 16:42:55 FreeWAF Development Team $
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
#ifndef _TAMPERLOG_H_
#define _TAMPERLOG_H_

enum RESTORE_TYPE {
    RESTORE_DECTET,
    RESTORE_AUTO,
    RESTORE_MANUAL,
    RESTORE_UNKNOWN
};

/**
 * log_write -  �۸���־�ӿ�
 * @param wsname: ���۸�����
 * @param host: ������
 * @param action: ������
 *
 * �۸���־�ӿ�
 *
 * @returns
 *     �ɹ�: ����0
 *     ʧ��: ����-1
 */
extern void log_write(char *wsname, char *host, const char *action, unsigned long tampertime, 
                unsigned long detecttime, unsigned long restoretime, int version, int beh, char *filename);

#endif

