#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/md5.h>
#include <time.h>

void MD5hash(unsigned char *data, unsigned int dataLen, unsigned char *digest) {
    MD5_CTX c;
    MD5_Init(&c);
    MD5_Update(&c, data, dataLen);
    MD5_Final(digest, &c);
}


//struktura odbierająca datagram przez UDP
typedef struct{
	int nr_ksiegi;
	int sec;
	int ssec;
	int jednostka;
	int port_u;
	int port_t;
}data;

//struktura odbierająca sumę kontrolną przez TCP
typedef struct{
	unsigned char hasz[16];
}suma;

//struktura wysyłająca informacje do klienta
typedef struct{
	char text[2048];
	int nr_kom;
	int nr_jedn;
}send_data;


int main(int argc, char** argv)
{
	
	suma suma_kontr;
	data recv_data;
	send_data send_text;

        int sock_u;
        socklen_t addr_len;
      
        struct sockaddr_in server_addr_u, server_addr_t, client_addr_u, client_addr_t;	
	struct timespec ts;  // nanosleep

	if(argc!=3){
           printf("Nieprawdilowa ilosc parametrow! \n");
	   exit(1);
	}

	int port_u=atoi(argv[1]); //port na ktorym jest serwer
	
	//gniazdo UDP
	if ((sock_u = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {  //zwraca deskryptor pliku
            perror("Socket");
            exit(1);
        }

        server_addr_u.sin_family = AF_INET;
        server_addr_u.sin_port = htons(port_u);
        server_addr_u.sin_addr.s_addr = INADDR_ANY;
        bzero(&(server_addr_u.sin_zero),8);


        if (bind(sock_u,(struct sockaddr *)&server_addr_u,
            sizeof(struct sockaddr)) == -1)
        {
            perror("Bind");
            exit(1);
        }

        addr_len = sizeof(struct sockaddr);
	
	
	printf("\nUDPServer Waiting for client");
        fflush(stdout);
          
	recvfrom(sock_u,&recv_data,sizeof(recv_data),0 ,(struct sockaddr *)&client_addr_u, &addr_len);

        printf("\nIP SERWERA: %s ",inet_ntoa(client_addr_u.sin_addr));
       
	fflush(stdout);
	

//################################################################################################
	
	
        int sock_t, connected;
        int reuse =1;
        socklen_t sin_size;
        
        if ((sock_t = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Socket");
            exit(1);
        }

        if (setsockopt(sock_t,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int)) == -1) {
            perror("Setsockopt");
            exit(1);
        }
      
        server_addr_t.sin_family = AF_INET;         
        server_addr_t.sin_port = htons(recv_data.port_t);     
        server_addr_t.sin_addr.s_addr = INADDR_ANY; 
        bzero(&(server_addr_t.sin_zero),8); 

        if (bind(sock_t, (struct sockaddr *)&server_addr_t, sizeof(struct sockaddr))
                                                                       == -1) {
            perror("Unable to bind");
            exit(1);
        }

        if (listen(sock_t, 5) == -1) {
            perror("Listen");
            exit(1);
        }
		
        fflush(stdout);
	
	
	ts.tv_sec=recv_data.sec;
	ts.tv_nsec=recv_data.ssec;
	
	char strf[20];
	sprintf(strf,"%sksiega%d.txt",argv[2], recv_data.nr_ksiegi);
	
	FILE *fp;
	fp= fopen(strf, "r");
	if(fp==NULL)
	{
		printf("Error in opening");
		exit(0);
	}


	sin_size = sizeof(struct sockaddr_in);

        connected = accept(sock_t, (struct sockaddr *)&client_addr_t,&sin_size);

        printf("\nconnection from (%s , %d)", inet_ntoa(client_addr_t.sin_addr),ntohs(client_addr_t.sin_port));
		
	int cmp;		
	unsigned char digest[16];
	char c;
	int flag;
	send_text.nr_kom=0;
	send_text.nr_jedn=0;
	if(recv_data.jednostka==1){
		while((c=fscanf(fp, "%s", send_text.text)) != EOF) {
			send_text.nr_kom+=1;
			send_text.nr_jedn+=1;
			flag = 0;
			while(!flag) {
				
				const char *s = send_text.text;
				unsigned int l = (unsigned int)strlen(s);
				MD5hash((unsigned char *)s, l, digest);
				send(connected, &send_text,sizeof(send_text), 0);
				recv(connected, suma_kontr.hasz, 16, 0);
				cmp = strncmp((const char *)digest,(const char *)suma_kontr.hasz, 16);
				if(cmp==0)
					flag=1;
				else
					send_text.nr_kom+=1;	
			}
			fflush(stdout);
	 		nanosleep(&ts, NULL);
		}
	}

	else if(recv_data.jednostka==2) {
		while((c=fgetc(fp)) != EOF) {
			send_text.nr_kom+=1;
			send_text.nr_jedn+=1;
			flag = 0;
			while(!flag) {
				send_text.text[0]=c;
				const char *s = send_text.text;
				unsigned int l = (unsigned int)strlen(s);
				MD5hash((unsigned char *)s, l, digest);
				send(connected, &send_text,sizeof(send_text), 0);
				recv(connected, suma_kontr.hasz, 16, 0);
				cmp = strncmp((const char *)digest,(const char *)suma_kontr.hasz, 16);
				if(cmp==0)
					flag=1;
				else
					send_text.nr_kom+=1;
			}
			fflush(stdout);
	 		nanosleep(&ts, NULL);
		} 
	}
	else {
      		while ( fgets( send_text.text, sizeof(send_text.text), fp ) != NULL ) {
			send_text.nr_kom+=1;
			send_text.nr_jedn+=1;
			flag = 0;
			send_text.text[strlen(send_text.text)-1]='\0'; //doklejam koniec lini przed przejsciem do nowej lini, w przeciwnym razie miałbym dwa "\n" przy sreamingu
			while(!flag) {
				const char *s = send_text.text;
				unsigned int l = (unsigned int)strlen(s);
				MD5hash((unsigned char *)s, l, digest);
				send(connected, &send_text,sizeof(send_text), 0);
				recv(connected, suma_kontr.hasz, 16, 0);
				cmp = strncmp((const char *)digest,(const char *)suma_kontr.hasz, 16);
				if(cmp==0)
					flag=1;
				else
					send_text.nr_kom+=1;
				
			}
			

			fflush(stdout);
	 		nanosleep(&ts, NULL);
			
		}
	}
		
		           
	fclose(fp);
               

      close(sock_t);
      return 0;
} 
