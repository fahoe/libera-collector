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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

#include "fa_collect.h"


static inline void error(const char *msg, const int err_no)
    {
        char err_string[1024];
        if (err_no != 0)
            sprintf(err_string, "%s: %s\n", msg, strerror(err_no));
        else
            sprintf(err_string, "%s\n", msg);

        fprintf(stderr, "%s", err_string);
    }
static  inline void CleanupWSA(void) {}   
static inline void ipCloseSocket(int sd) {close(sd);}

/* Send to Arch - Thread-Function */
static void *archthread (void *arg) {
   struct archData *f= (struct archData *)arg;
   snprintf (f->msg, BUF, "I'm Thread Nr. %ld", pthread_self());
   printf(" Initialize ArchThread: %s\n",f->msg);
   /*----------------------*/
   int i=0,j=0,k=0;
   int rc=0;
   int pn[MAX_LIB_THREADS]; //pipe ret n bytes
   int client_sd;
   struct sockaddr_in archServAddr;
   struct sockaddr_in cliAddr;
   //char *hostname = NULL;
   struct hostent *hp;
   char err_string[MAX_ERR_LENGTH];
   char abuf[MAX_LIB_THREADS][PAYLOAD_BUF_SIZE];
   char *pabuf;
   int tmp_val[MAX_LIB_THREADS][PAYLOAD_BUF_SIZE];
   /*
   for (i = 0; i < MAX_LIB_THREADS; i++){
          close(f->fd[i][1]); close the write site of the pipe 
   }*/
    
   pabuf=&abuf[0][0];
   memset((void*)&cliAddr, 0, sizeof(cliAddr));   

   if ( (hp = gethostbyname(f->ipaddr)) == NULL ) {
            sprintf(err_string,": Unknown Arch Client IP: %s", f->ipaddr);
            error(err_string, 0);
            CleanupWSA();
            f->stat=-1; //return(-1);
            pthread_exit(f) ; //return(-1);
   }else{
	 archServAddr.sin_family = hp->h_addrtype;
         archServAddr.sin_port = htons (f->aport); /* 32768d->8000h->little-endian format->00h 80 h-> 80h->128d */
         memcpy((char*)&archServAddr.sin_addr, hp->h_addr, hp->h_length);
         if(f->dlevel&0x10) printf("arch: sende Daten an '%s' (IP : %s:%d) port: %d\n",  hp->h_name, inet_ntoa (*(struct in_addr *) hp->h_addr_list[0]), archServAddr.sin_port,f->aport  );
   }

   // Creating socket file descriptor
    if ( (client_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        sprintf(err_string,"socket creation failed: %s", f->ipaddr);
        error(err_string, 0);
        //exit(EXIT_FAILURE);
        CleanupWSA();
        f->stat=-1; //return(-1);
        pthread_exit(f) ; 
    }
   
    /* Port bind(en) */
    /* Lokalen Server Port bind(en) */
    cliAddr.sin_family = AF_INET;
    //cliAddr.sin_addr.s_addr = htonl (f->ipaddr);
    cliAddr.sin_addr.s_addr = htonl (INADDR_ANY);
    //cliAddr.sin_port = htons (f->aport);
    cliAddr.sin_port = htons (0);
    rc = bind ( client_sd, (struct sockaddr *) &cliAddr,sizeof (cliAddr) );
    if (rc < 0) {
          sprintf(err_string,"Client: Unknown ArchServer: %s", f->ipaddr);
          error(err_string, errno);
          CleanupWSA();
          f->stat=-1; //return(-1);
          pthread_exit(f) ; //return(-1);

    }


    while(*(f->ex_flag)==0){
       	memset(abuf, 0, PAYLOAD_BUF_SIZE*MAX_LIB_THREADS);
        pabuf=&abuf[0][0];

	if(f->debug_switch && f->dlevel&0x10){
			printf("ArchThread ready to receive pipe values -> \n");

	}
        /* read the pipe (PIPE_BUF Bytes) */
        for(i=0;i<MAX_LIB_THREADS;i++){
       	    pn[i] = read (f->fd[i][0], (pabuf+i* PAYLOAD_BUF_SIZE), PAYLOAD_BUF_SIZE); // !BUF_SIZE not sizeof(buf),bc only one libera part 
            if (pn[i]<0) {
                   sprintf(err_string,"read pipe%d \t %d data read failed %d", 0,pn[i],errno);
                   error(err_string, 0);
                   CleanupWSA();
                   f->stat=-1; //return(-1);
                   ipCloseSocket(client_sd);

	           for (i = 0; i < MAX_LIB_THREADS; i++){
     		           close(f->fd[i][0]); /*close the read site of the pipe */
     	           }    
                   pthread_exit(f) ;
            }
      	    //if(f->debug_switch) printf(" dev %d received %d Bytes\n",i,pn[i]); 
        }
        /*-----------------------------*/
        if(f->debug_switch){	
		//printf("pabuf %x:%x:%x",abuf,&abuf,pabuf);
        	pabuf=&abuf[0][0];
		if(f->dlevel&0x10){
			for (i = 0 ; i < (PAYLOAD_BUF_SIZE*MAX_LIB_THREADS) ; i++) {
				printf("%d::%hhx\t",i,*pabuf);
				//Error -> printf("%d::%hhx\t",i,abuf[i]);
				pabuf++;
			}
		}
		//sleep(1);
                if(f->dlevel&0x20){
                    printf("\nArchThread Received Size:%d \n",sizeof(abuf));
                    for(k=0;k<MAX_LIB_THREADS;k++){
                        printf("\nDevNb %d->\t ",k);
                        for(i=0;i<PAYLOAD_BUF_SIZE;i++)
                            printf("%d:%d:%hhx\t",k,i,abuf[k][i]);
                           // printf("%hhx\t",abuf[k][i]);
                   printf("\n----------------\n");    
                    }
                    
                }
               if(f->dlevel&0x40){
                for (k = 0 ; k < MAX_LIB_THREADS ; k++) {
                    for (i = 0 ; i < (PAYLOAD_BUF_SIZE/4) ; i++) {
                        tmp_val[k][i] =0;
               	 		for (j = 0 ; j < 4 ; j++){ 
                                    tmp_val[k][i] |= ((unsigned char)abuf[k][i*4+j]) << (j*8) ;  
                                    //printf("%d:%d rcv temp_val%d\t %016x\n",i,j,k,tmp_val[k][i]);
                        }
                    printf("arch thread LiberaNb:\t%d \t received data byte\t%d:\t \t%20d:%#010x I32\n",k,i,tmp_val[k][i],tmp_val[k][i]);
                  }
       				
        	}//rof
        		printf("\n--------------------\n");
		}//fi
       } //fi debug_switch



        /**/  
    
   	pabuf=&abuf[0][0];
        f->received= sendto (client_sd, pabuf,sizeof(abuf) ,  0,
                 	(struct sockaddr *) &archServAddr,
                	 sizeof (archServAddr));
    	if (f->received < 0) {
	 		sprintf(err_string,"data send failed: %s", f->ipaddr);
         		error(err_string, errno);
         		CleanupWSA();
         		f->stat=-1; //return(-1);
         		ipCloseSocket(client_sd);
         		pthread_exit(f) ;
       	}
        
        if((f->dlevel&0x80) && (f->debug_switch) ){
   	    pabuf=&abuf[0][0];
            for(i=0;i<f->received;i++){
                printf("arch sent data :%ld \t idx:%d \t data:%#10x   I32\n",f->received,i,*pabuf);
                pabuf++;
            }
        }//fi debug
        
    }//while
    ipCloseSocket(client_sd);
    /*----------------------*/
    /* Thread finish - with returned data */
    //return arg;
    /* Thread finish - with pthread_exit( f ); */
    if(f->dlevel&0x40) printf("Exit Thread Nr. %ld\n", pthread_self());
    printf("Arch Exit Thread Nr. %ld\n", pthread_self() );
    pthread_exit(f);
  
}   
   
   
/* Receiver Thread-Function */
static void *libthread (void *arg) {
   struct libData *f= (struct libData *)arg;
   snprintf (f->msg, BUF, "I'm Thread Nr. %d: %ld", f->nb,pthread_self());
   printf(" Initialize LibThread: %s\n",f->msg);
   /*----------------------*/
   int pn=0; //pipe ret n bytes
   int server_sd;
   struct sockaddr_in server_addr;
   struct sockaddr_in client_addr;
   char *hostname = NULL;
   unsigned short port = f->port; /*SERVER_PORT;*/
   struct hostent *hp;
   char err_string[MAX_ERR_LENGTH];
   char rbuf[RCV_BUF_SIZE];
   char wbuf[PAYLOAD_BUF_SIZE];
   char *prbuf;
   char *pwbuf;
   int tmp_val;
   //close (f->fd[0]);   /*close the read site of the pipe */
   int count=0;
   
   memset((void*)&server_addr, 0, sizeof(server_addr));
   memset((void*)&client_addr, 0, sizeof(client_addr));
   



   // Source address filter
   if ( (hp = gethostbyname(f->ipaddr)) == NULL ) {
            sprintf(err_string,"Server: Unknown source: %s", f->ipaddr);
            error(err_string, 0);
            CleanupWSA();
            f->stat=-1; //return(-1);
            pthread_exit(f) ; //return(-1);
   }else{
         memcpy((char*)&client_addr.sin_addr, hp->h_addr, hp->h_length);
         
   }
   // Create a socket
   if ( (server_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            ipCloseSocket(server_sd);
            error("Server: socket()", errno);
            CleanupWSA();
            f->stat=-1; //return(-1);
            pthread_exit(f) ; 
        }
   
   // Reuse address
   const int ON = 1;
   if ( setsockopt(server_sd, SOL_SOCKET, SO_REUSEADDR, (const char *)&ON,
            sizeof(ON)) < 0 ){
            ipCloseSocket(server_sd);
            error("setsockopt(): SO_REUSEADDR", errno);
            f->stat=-1; //return(-1);
            pthread_exit(f) ; 
        }

   // Set Receiver buffer size
   //
   // NOTE: Linux kernel might impose its own max SO_RCVBUF values.
   //       Check /proc/sys/net/core/rmem_max & set it acoordingly.
   //
   int setrcv_buf_size = 128;
   if ( setsockopt(server_sd, SOL_SOCKET, SO_RCVBUF, (const char *)&setrcv_buf_size,
                sizeof(int)) < 0 ){
            ipCloseSocket(server_sd);
            error("setsockopt(): SO_RCVBUF", errno);
            f->stat=-1; //return(-1);
            pthread_exit(f) ; 
   }

        
   // Check & report Receiver buffer size
   int rcv_buf_size;
   int rcv_buf_size_len = sizeof(int);
   if ( getsockopt(server_sd, SOL_SOCKET, SO_RCVBUF, (char *)&rcv_buf_size,
            (socklen_t *)&rcv_buf_size_len) < 0 ){
            ipCloseSocket(server_sd);
            error("getsockopt(): SO_RCVBUF", errno);
            f->stat=-1; //return(-1);
            pthread_exit(f) ;            
        }
    if(f->dlevel&0x02) fprintf(stderr, "Socket RCVBUF = %d\n", rcv_buf_size);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);     
        
    // Get host info FQDN + IP in standard dot notation
    if (hostname) {
            if(f->dlevel&0x02){
			printf("Server: looking for %s hostname info \n", hostname);
	    }
            if ( (hp = gethostbyname(hostname)) == NULL ) {
                sprintf(err_string,"Server: Unknown host: %s", hostname);
                error(err_string, 0);
                ipCloseSocket(server_sd);
                CleanupWSA();
                f->stat=-1; //return(-1);
                pthread_exit(f) ; 
            }
            memcpy((char*)&server_addr.sin_addr, hp->h_addr, hp->h_length);
    }
    else { // Use the host's own IP for server
            server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }   
    
    // Bind a name to a socket
    if ( bind(server_sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 ) {
            error("Server: bind()", errno);
            ipCloseSocket(server_sd);
            CleanupWSA();
            f->stat=-1; //return(-1);
            pthread_exit(f) ; 
    }
    
    if(f->dlevel&0x02){
          fprintf(stderr,"Server: listening on %s:%u/udp\n", inet_ntoa(server_addr.sin_addr), port);
    }
    
    
    
    int i=0,j=0;
    int dloop_cnt=0;
    
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(struct sockaddr);

    while(*(f->ex_flag)==0)
    {
    	//long rcount = 0;
       	memset(rbuf, 0, RCV_BUF_SIZE);
       	memset(wbuf, 0, PAYLOAD_BUF_SIZE);

       	f->received = recvfrom(server_sd, rbuf, RCV_BUF_SIZE, 0,(struct sockaddr *)&from, &fromlen);

	
        if(f->debug_switch)
	{
       		if(f->dlevel&0x02) fprintf(stdout, "thread %d \t received libera data %lu \n", f->nb,f->received);
		if((f->dlevel&0x04) && (f->received>0)){
       			for(i=0;i<(f->received);i++) printf("\n-> \t %x\n",rbuf[i]);
         		printf("-----------\n");
       		}//fi dlevel
         
	  
        	if(f->dlevel&0x08){
			for (i = 0 ; i < (BUF_SIZE/4) ; i++) {
                 		tmp_val = 0 ;
                  		for (j = 0 ; j < 4 ; j++) {
                      			 tmp_val |= ((unsigned char)rbuf[i*4+j]) << (j*8) ;
                  		}
                	 	printf("lib thread %d\t  received data: %09d  \t %09d:%#08x   \n",f->nb,i,tmp_val,tmp_val);
         		}
         		printf("--------------------\n");
		}

	}//fi debug_switch
	

         /*
	  -we dont need photons va,vb,vc,d for the archiver, cut the first 16 bytes 
	  - 16 ... 19 sum
	  - 20 ... 23 res we dont need it
	  - 24 ... 27 x
	  - 28 ... 31 y 
	  */   
       prbuf=rbuf;
       pwbuf=wbuf;
        
       for(j=0;j<BUF_SIZE;j++){

            switch(j){
                       case 16:
                       case 17:
                       case 18:
                       case 19: *pwbuf=*prbuf;
                		pwbuf++;
                		prbuf++;  
                                break;
                       case 24:
                       case 25:
                       case 26:
                       case 27:
                       case 28:
                       case 29:
                       case 30:
                       case 31: *pwbuf=*prbuf;
                		pwbuf++;
                		prbuf++;  
                                break;

		       default: prbuf++;
            }//switch
        
            if(f->dlevel&0x10) printf("lib:%d \t ix:%d \t rbuf: %x:%x \t wbuf: %x:%x\n",i,j, prbuf ,*prbuf, pwbuf, *pwbuf);
        }//rof BUF_SIZE
            
	    *pwbuf|=(f->nb)<<2;
            if(f->dlevel&0x10) printf("lib:%d \t ix:%d \t rbuf:%x:%x \t wbuf: %x:%d\n",i,j, prbuf, *prbuf, pwbuf, *pwbuf);
            j++;
            
            pwbuf++;
            //*pwbuf|=0x80;  
 	    if((f->received==32)|(f->received==64))  *pwbuf|=0x08; else *pwbuf&=0xf7;
            if(f->dlevel&0x10) printf("lib:%d \t ix:%d \t ruf:%x:%x \t wbuf: %x:%d\n",i,j, prbuf, *prbuf, pwbuf, *pwbuf);
            j++;
            
            pwbuf++;
	     *pwbuf=count&0x00ff;
            if(f->dlevel&0x10) printf("lib:%d \t ix:%d \t rbuf:%x:%x \t wbuf: %x:%d\n",i,j, prbuf, *prbuf, pwbuf, *pwbuf);
            j++;
            
            pwbuf++;
	    //printf("count: %x:%x",count,count>>8);
            *pwbuf=(count>>8)&0x00ff;  
            if(f->dlevel&0x10) printf("lib:%d \t ix:%d \t rbuf:%x:%x \t wbuf: %x:%x\n",i,j, prbuf, *prbuf, pwbuf, *pwbuf);
            pwbuf++;
            //printf("received %d",f->received); 
        /*-------------------------------------------------------------------------------------------*/
         
         if(f->debug_switch){
 	               
            if(f->dlevel&0x04){
                
                for (i = 0 ; i < (BUF_SIZE/4) ; i++) {
                        tmp_val = 0 ;
              		for (j = 0 ; j < 4 ; j++) {
                     		tmp_val |= ((unsigned char)rbuf[i*4+j]) << (j*8) ;
                 	}
                        printf("lib thread received data:%d\t %20d:%#08x\n",i,tmp_val,tmp_val);            	
                }//rof BUF_SIZE
            
                printf("--------------------\n");
            	for (i = 0 ; i < (PAYLOAD_BUF_SIZE/4) ; i++) {
                	tmp_val = 0 ;
                	for (j = 0 ; j < 4 ; j++) {
                     	        tmp_val |= ((unsigned char)wbuf[i*4+j]) << (j*8) ;
                	}
                        printf("lib thread send data:%d\t %20d:%x\n",i,tmp_val,tmp_val);
                }//rof PAYLOAD_BUF_SIZE
                printf("--------------------\n");
            }//fi dlevel
         
      	    /*---------------------------*/
            
            if(f->dlevel&0x20){
  
               for (i = 0 ; i < (PAYLOAD_BUF_SIZE/4) ; i++) {
                   	 tmp_val = 0 ;
                   	 for (j = 0 ; j < 4 ; j++) {
                     		tmp_val |= ((unsigned char)wbuf[i*4+j]) << (j*8) ;
                       	        //printf("send temp_val %08x\n",tmp_val);
                   	 }
                	printf("lib thread %d send data:%d \t %20d:%x\n",f->nb,i,tmp_val,tmp_val);
               		//sleep(0.1);
               }//rof PAYLOAD_BUF_SIZE
               printf("--------------------\n");
               
           }//fi dlevel
           
           if(f->dlevel&0x40){
                printf("\nLib thread %d send data , Size:%d \n",f->nb,sizeof(wbuf));
                for(i=0;i<PAYLOAD_BUF_SIZE;i++) printf(" %hhx\t",wbuf[i]);
               printf("\n----------------\n");    
           }
      }//fi debug_switch
         
        

	pn=write (f->fd[1], wbuf, sizeof(wbuf));
	if (pn<0) {
        	sprintf(err_string,"write pipe of thread %d failed %d\t wrote data nb:%d of %ld", f->nb,errno,pn,f->received);
                error(err_string, 0);
                ipCloseSocket(server_sd);
                CleanupWSA();
                f->stat=-1; //return(-1);
                pthread_exit(f) ;
      	}
        

 	//if(f->debug_switch) sleep(1);           
      	dloop_cnt++;
      	if((f->dlevel&0x02) && (f->debug_switch) ){
		 printf("New data loop cnt: %d\n",dloop_cnt);
       		 printf("exit-flag:\t %x  \n", *(f->ex_flag)); 
		//sleep(2);
        }//fi 
	count++;
	if(count&0x10000) count=0;
    }//while

    if((f->dlevel&0x01) && (f->debug_switch)) printf("exit was set\n");
    ipCloseSocket(server_sd);
      /*----------------------*/
   /* Thread finish - with pthread_exit( f ); */
   if((f->dlevel&0x10) && (f->debug_switch)) printf("Exit Thread Nr. %ld\n", pthread_self() );
   printf("Lib Exit Thread Nr. %ld\n", pthread_self() );
   pthread_exit(f);
}       


static void *exthread (void *arg) {
    int *flag=arg;
    char c;
    printf ("I'm the exit Thread Nr. %ld\n", pthread_self());
    while(*flag<1){
        c = getchar();
        if( c == 'q' ){
            *flag=1;   
        }
        printf("exit-flag:\t %x \t char: %c\n", *flag,c); 
        sleep(1);
    }
    printf("exit-flag set ->exit:\t %x \t char: %c\n", *flag,c);   
    pthread_exit(flag);
}    




int main(void)
    {
    unsigned int dlevel=0;
    #ifdef DEBUG    
       printf("\n--------\n");  
       printf("\nDEBUG Mode\n");    
       printf("\n--------\n");   
    #else       
       printf("\n--------\n");  
       printf("\nNo DEBUG Mode\n");    
       printf("\n--------\n");   
    #endif         
    printf("DEBUG_LEVEL %d", DEBUG_LEVEL);

    #if DEBUG_LEVEL>0
       //printf("\nLEVEL %d\n",1);
       dlevel=0x01;
    #endif	 
    #if DEBUG_LEVEL==2
       dlevel= (dlevel<<1);//0x02   

    #elif DEBUG_LEVEL==3
       dlevel= (dlevel<<2);//0x04

    #elif DEBUG_LEVEL==4
       dlevel= (dlevel<<3);//0x08

    #elif DEBUG_LEVEL==5
       dlevel= (dlevel<<4);//0x10

    #elif DEBUG_LEVEL==6
       dlevel= (dlevel<<5);//0x20 

    #elif DEBUG_LEVEL==7
       dlevel= (dlevel<<6);//0x40

    #elif DEBUG_LEVEL==8
       dlevel= (dlevel<<7);//0x80

    #elif DEBUG_LEVEL>8
       dlevel=0xff;//0xff
    #endif   
    if(dlevel>0) {
	printf("\n-> Debug level bit:\t %d\n",dlevel);
	sleep(2);
    }


    //if(dlevel>0) {
	printf("\n-> Debug level bit:\t %04X\n",dlevel);
	sleep(2);
    //}


    //char buf[BUF_SIZE];
    //int received = 0;

    pthread_t th[MAX_LIB_THREADS];
    pthread_t exth;
    pthread_t archth;
    int i,p;
    int exit_flag=0;
    
    struct libData *ret[MAX_LIB_THREADS];
    struct archData *aDat;
    /* Main-Thread started */
    
    if(dlevel&0x01) printf("\n-> Main-Thread startet (ID:%ld)\n", pthread_self());
    
    /* fill mem */
     //memset(buf, 0, BUF_SIZE);
   

    /* allocate mem */
    for (i = 0; i < MAX_LIB_THREADS; i++){
      ret[i] = (struct libData *)malloc(sizeof(struct libData));
      if(ret[i] == NULL) {
         printf("memory allocation failed ...!\n");
         exit(EXIT_FAILURE);
      }else{
        if(dlevel&0x01) printf("\n-> prepare LibThread %d Data\n", i);
        ret[i]->nb=i;
        strcpy(ret[i]->ipaddr,sip[i]);
        ret[i]->port=dport[i];
        ret[i]->dlevel=dlevel;
        ret[i]->debug_switch=dbswitch[i];
        ret[i]->ex_flag=&exit_flag;
        ret[i]->received=0;
        p=pipe(ret[i]->fd);
        if(p<0){
            printf("libera thread pipe allocation failed ...!:%d\n",i);
                exit(EXIT_FAILURE);
        }

        if(dlevel&0x01) 
             printf("\n LibThread %d \t sip:%s \t port:%d\n\n",i, ret[i]->ipaddr, ret[i]->port);
      }
    }  

	
           

    /* allocate mem */
    
    aDat = (struct archData *)malloc(sizeof(struct archData));
      if(aDat == NULL) {
         printf("memory allocation failed ...!\n");
         exit(EXIT_FAILURE);
      }else{
          strcpy(aDat->ipaddr,archIP);
          aDat->aport=aport;
          //aDat->cport=cport;
          aDat->dlevel=dlevel;
          aDat->debug_switch=dbaswitch;
          aDat->ex_flag=&exit_flag;
	  aDat->received=0;
          if(dlevel&0x01) 
              printf("\n ArchThread %d \t archip:%s \t aport:%d \n\n",i, aDat->ipaddr, aDat->aport);

	  for (i = 0; i < MAX_LIB_THREADS; i++){
		 aDat->fd[i][0]=ret[i]->fd[0];
		 aDat->fd[i][1]=ret[i]->fd[1];
	  }
    }
    

    /*create threads */
    if(pthread_create(&exth,NULL,&exthread,&exit_flag) !=0) {
         fprintf (stderr, "exthread creation failed\n");
         exit (EXIT_FAILURE);
    }
    sleep(1);
    for (i = 0; i < MAX_LIB_THREADS; i++) {
      if(pthread_create(&th[i],NULL,&libthread,ret[i]) !=0) {
         fprintf (stderr, "libthread creation failed\n");
         exit (EXIT_FAILURE);
      }

      if(dlevel&0x01){ 
    	 sleep(5);
         printf("\n create LibThreads %d \n",i);
      }
    }

    if(dlevel&0x01){ 
    	sleep(10);
        printf("\n create ArchThread \n");
    }


    if(pthread_create(&archth,NULL,&archthread,aDat) !=0) {
         fprintf (stderr, "exthread creation failed\n");
         exit (EXIT_FAILURE);
    }

   
    if(dlevel&0x01) printf("\n-> Main -  waiting for exth\n");                           
    pthread_join(exth, (void **)&exit_flag);  
    if(dlevel&0x01) printf("\n-> Main- exth stopped\n");
    
    /* watching for threads */
    for( i=0; i < MAX_LIB_THREADS; i++)
      pthread_join(th[i], (void **)&ret[i]);
    
    printf("\n-> Main- all libth stopped\n");

    pthread_join(archth, (void **)&aDat);
    printf("\n-> Main- archth stopped\n");

    /* Daten ausgeben */
    for( i=0; i < MAX_LIB_THREADS; i++) {
      if(dlevel&0x01) printf("Main-Thread: received data: \n");
      if(dlevel&0x01) printf("\t\tmsg  = \"%s\"\n", ret[i]->msg);
    }
   
    /* Main Thread finished */
    printf("<- Main-Thread finished (ID:%ld)\n", pthread_self());
    return EXIT_SUCCESS;     
          
}
