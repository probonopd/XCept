/*
 * Copyright (c) 1992, 1993 Arno Augustin, Frank Hoering, University of
 * Erlangen-Nuremberg, Germany.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	Erlangen-Nuremberg, Germany.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This software has not been validated by the ``Bundesamt fuer Zulassungen in
 * der Telekommunikation'' of the ``Deutsche Bundepost Telekom'' and thus
 * must not be used for accessing the BTX-Network of the Telekom in Germany.
 *
 * Diese Software hat keine Zulassung durch das Bundesamt fuer Zulassungen in
 * der Telekommunikation der Deutschen Bundespost Telekom und darf daher nicht
 * am Netz der Deutschen Bundespost Telekom in Deutschland betrieben werden.
 */


/********************  This is CONTROL SET C0 of CEPT ***********************/

#define NUL 0x00         /* Null                     (data link) */
#define SOH 0x01         /* start of heading         (data link) */
#define STX 0x02         /* start text               (data link) */
#define ETX 0x03         /* end text                 (data link) */
#define EOT 0x04         /* end of transmission      (data link) */
#define ENQ 0x05         /* enquiry                  (data link) */
#define ACK 0x06         /* acknowledge              (data link) */
#define ITB 0x07         /* end intermediate block   (data link) */
#define APB 0x08         /* active position back     (control sequence) */
#define APF 0x09         /* active position forward  (control sequence) */
#define APD 0x0a         /* active position down     (control sequence) */
#define APU 0x0b         /* active position up       (control sequence) */
#define CS  0x0c         /* clear screen             (control sequence) */
#define APR 0x0d         /* active position return   (control sequence) */
#define LS1 0x0e         /* locking shift 1          (control sequence) */
#define LS0 0x0f         /* locking shift 0          (control sequence) */
#define DLE 0x10         /* data link escape         (data link) */
#define CON 0x11         /* cursor on                (control sequence) */
#define RPT 0x12         /* repeat last character    (control sequence) */
#define INI 0x13         /* initiator '*'            (btx special) */
#define COF 0x14         /* cursor off               (control sequence) */
#define NAK 0x15         /* negative acknowledge     (data link) */
#define SYN 0x16         /*                          (data link) */
#define ETB 0x17         /* end textblock            (data link) */
#define CAN 0x18         /* cancel,clear eol         (control sequence) */
#define SS2 0x19         /* single shift for G2 SET  (control sequence) */
#define DCT 0x1a         /*                          (btx special) */
#define ESC 0x1b         /* escape                   (control sequence) */
#define TER 0x1c         /* terminator '#'           (btx special) */
#define SS3 0x1d         /* single shift for G3 SET  (control sequence) */
#define APH 0x1e         /* active position home     (control sequence) */
#define US  0x1f         /* active position (x,y)    (control sequence) */


/****************************************************************************/
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int UINT;
typedef unsigned int FLAG;


#ifndef NULL
#define NULL 0
#endif



#ifndef HAS_STRERROR
char *strerror();
#endif

#define MAX(x,y) ((x)>=(y) ? (x) : (y))

extern int errno;






