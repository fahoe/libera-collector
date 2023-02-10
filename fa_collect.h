/* This tool is an extension  to capture the udp stream 
 * supplied by libera photon/spark , rearranges the data 
 * packages  , in order to supply the fa_archiver , provided by
 * Michael Abbott, Diamond Light Source Ltd.
 * 
 * Copyright (c) 2010-2015 Helmholtz-Zentrum Berlin f. Materialien
 *                       und Energie GmbH, Germany (HZB)
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact:
 *      Frank Hoeft,
 *      Helmholtz-Zentrum Berlin für Materialien und Energie,
 *      frank.hoeft@helmholtz-berlin.de
 */
#ifdef DEBUG
#define DEBUG_LEVEL 2 /*1=0x01 main, 2=0x02 libera threads -->, 3=0x04 ,4=0x08 <--,5=0x10 arch thread -->,6=0x20,7=0x40,8=0x80 <--,>8=0xff*/
#else
#define DEBUG_LEVEL 0
#endif
/*threads*/
#define MAX_LIB_THREADS 4
#define BUF 255
/*Libera UDP*/
//#define BUF_SIZE 16384
#define RCV_BUF_SIZE 64  //MAX Size of the Libera data struct,bc different size of Photon&Spark
#define BUF_SIZE 32 //Size of the Libera Photon data struct,the interesting spark size has also the same pattern.
#define PAYLOAD_BUF_SIZE 16 //bytes for the arch struct 
#define MAX_ERR_LENGTH  1024
#define SAMPLES 2048
#define STRLEN 30

/*
struct libera_payload {
    int32_t sum;
    int32_t x;
    int32_t y;
#if LIBERA_GROUPING == 0
    uint16_t counter;
    struct libera_status status;
#elif LIBERA_GROUPING == 1
    struct libera_status status;
    uint16_t counter;
#else
    #error "Invalid value for LIBERA_GROUPING, must be 0 or 1"
#endif

}
*/

struct libData {
   int nb;
   int *ex_flag;
   unsigned short debug_switch; //debug switch 
   unsigned short dlevel; //Debug level
   long stat; 
   char msg[BUF];
   char ipaddr[STRLEN];
   unsigned short port;
   ssize_t received;
   int     fd[2];
};

/*
the destination udp ips , the sequence corrensponds to the dport[] below
*/
char sip[MAX_LIB_THREADS][STRLEN]={"192.168.1.5","192.168.1.6","192.168.1.7","192.168.1.8"}; //source ips i.e. LiberaDev
/*
the destination udp ports , i use the same  on libera ports, the sequence corrensponds to the sip[] above
*/
const unsigned short dport[MAX_LIB_THREADS]={2048,2047,2049,2046};//Dest. Ports
/*
the dbswitch is only active if the Makefile flag is set
*/
const unsigned short dbswitch[MAX_LIB_THREADS]={0,1,0,0};// switch for selected lib thread debugging

struct archData {
   long stat; 
   int *ex_flag;
   unsigned short debug_switch; //debug switch
   unsigned short dlevel; //Debug level
   char msg[BUF];
   char ipaddr[STRLEN];
   unsigned short aport;
   //unsigned short cport;
   ssize_t received;
   int     fd[MAX_LIB_THREADS][2];
};
char archIP[STRLEN]="127.0.0.1"; 
const unsigned short aport=32768;//archPort
//const unsigned short cport=2046; //Client Port
const unsigned short dbaswitch=0;// switch for selected arch thread debugging
