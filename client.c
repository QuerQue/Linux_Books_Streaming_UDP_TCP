#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/md5.h>
#include <fcntl.h>


void print_usage(char *executable_name) {
  printf("Usage: %s -t num -s num...\n", executable_name);
}


void MD5hash(unsigned char *data, unsigned int dataLen, unsigned char *digest) {
    MD5_CTX c;
    MD5_Init(&c);
    MD5_Update(&c, data, dataLen);
    MD5_Final(digest, &c);
}


//struktura z datagramem
typedef struct{
	int nr_ksiegi;
	int sec;
	int ssec;
	int jednostka;
	int port_u;
	int port_t;
}data;

//struktura z sumą kontrolną
typedef struct{
	unsigned char hasz[16];
}suma;

//struktura odbierająca infromacje z serwera
typedef struct{
	char text[2048];
	int nr_kom;
	int nr_jedn;
}recv_data;


int main(int argc, char *argv[])
{

	suma suma_kontr;
	
	recv_data recv_text;
	char ip[20];
	//int port_u;
	char c;
//i - ip, n-numer_ksiegi, t-czas, s-czas po przecinku, j-jednostka, p=port serwera, o-port na ktorym oczekuje streamingu
data send_data = { -1, -1, -1, -1, -1, -1 };

	while((c = getopt(argc, argv, "i:n:t:s:j:p:o:")) != -1) {
    switch(c) {
      case 'i' : sprintf(ip,"%s",optarg);                  
        break;
      case 'n' : send_data.nr_ksiegi=atoi(optarg);                  
        break;
      case 't' : send_data.sec=atoi(optarg);
        break;
      case 's' : send_data.ssec=atoi(optarg);
        break;
      case 'j' : send_data.jednostka=atoi(optarg);
        break;
      case 'p' : send_data.port_u=atoi(optarg);
        break;
      case 'o' : send_data.port_t=atoi(optarg);
        break;
      default: print_usage(argv[0]); 
        exit(EXIT_FAILURE);
    }
  }


	 if (send_data.nr_ksiegi == -1 || send_data.sec == -1 || send_data.ssec == -1 || send_data.jednostka == -1 || send_data.port_u == -1 || send_data.port_t == -1) {
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  } 

	int sock_u;

	struct sockaddr_in server_addr_u, server_addr_t;
	struct hostent *host;

	int fptr;
	fptr = open("stream.txt", O_CREAT | O_WRONLY, 0600);
	if(fptr == -1) {
		printf("Error, you can't open the output file");
		exit(1);
	}
	

	host= (struct hostent *) gethostbyname((char *)ip);


	if ((sock_u = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	server_addr_u.sin_family = AF_INET;
	server_addr_u.sin_port = htons(send_data.port_u);
	server_addr_u.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(server_addr_u.sin_zero),8);


    	sendto(sock_u, &send_data, sizeof(send_data), 0, (struct sockaddr *)&server_addr_u, sizeof(struct sockaddr));


//######################################################################################


        int sock_t;  

        if ((sock_t = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Socket");
            exit(1);
        }


        server_addr_t.sin_family = AF_INET;     
        server_addr_t.sin_port = htons(send_data.port_t);   
        server_addr_t.sin_addr = *((struct in_addr *)host->h_addr);
        bzero(&(server_addr_t.sin_zero),8); 

        if (connect(sock_t, (struct sockaddr *)&server_addr_t, sizeof(struct sockaddr)) == -1) {
            perror("Connect");
            exit(1);
        }
	
	

        while(1) { 
		recv(sock_t,&recv_text,sizeof(recv_text),0);
		printf("%d  %d  %s\n" ,recv_text.nr_kom, recv_text.nr_jedn, recv_text.text);
		fflush(stdout);
		const char *s = recv_text.text;
		unsigned int l = (unsigned int)strlen(s);
		MD5hash((unsigned char *)s, l, suma_kontr.hasz);
		
		write(fptr, recv_text.text, strlen(recv_text.text));
		write(fptr, "\n", 1);

		send(sock_t, suma_kontr.hasz, 16, 0);
		//for(i = 0; i < 16; ++i)
			//printf("%02x", suma_kontr.hasz[i]);	
          
	  }
return 0;
}

