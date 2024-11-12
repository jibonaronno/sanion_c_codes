#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "tcpipprotocol.h"
#include <errno.h>
#include <error.h>
#include "sys/types.h"
#include "sys/sysinfo.h"

#define MAX 1024
#define PORT 9090
#define SA struct sockaddr
// Function designed for chat between client and server.
void func(int sockfd)
{
    int funccode;
	char *strsecond;
    int n;
	char itsdata[MAX];
	char senddata[MAX];
	int result;
	int b;
	struct dirent *dir;
    // infinite loop for chat
    while (1) {
        // read the message from client and copy it in buffer
        b = read(sockfd, itsdata, sizeof(itsdata));
		if (b>0)
		{
			if (itsdata[0] == 0)
			{
				memset(senddata,0,sizeof(senddata));
				printf("Polling request received\n");
				FILE *fp;
				size_t nread;
				ssize_t read;
				strsecond = (char *)malloc(1000);
				memset(strsecond,0,sizeof(strsecond));
				fp = fopen("current.txt", "rb");
				if(fp == NULL)
			    {
			        /* File not created hence exit */
			        printf("Unable to create file.\n");
			    }
				else
				{
					//printf("before strsecond: %s\n",strsecond);
					while ((read = getline(&strsecond, &nread, fp)) != -1) 
					{
				        //printf("Retrieved line of length %zu:\n", nread);
				        //printf("%s", strsecond);
				    }
					//strsecond[count] = '\0';
					//printf("after strsecond: %s\n",strsecond);
					senddata[0] = 0;
					senddata[1] = strlen(strsecond);
					for (int i=0;i<senddata[1];i++)
					{
						senddata[i+2] = strsecond[i];
						//printf("%c\n",senddata[i+2]);
					}
					//
					//U32 *tempval = convert(strsecond);
					//memcpy(&senddata[2],tempval,senddata[1]*4);
					/*
					printf("length: %d\n",senddata[1]);
					printf("senddatalength: %d\n",sizeof(senddata));
					for (int i = 0;i<senddata[1];i++){
						printf("data: %d\n",senddata[2+i]);
						}
						*/
					write(sockfd, senddata, sizeof(senddata));
					printf("Write finished!!!!\n");
					fclose(fp);
    			}
				free(strsecond);
			}
			else if (itsdata[0]  == 1){
				int tot=0;
				int retcode;
				printf("File request received\n");
				const double megabyte = 1024 * 1024;
			    /* Obtain system statistics. */
			    struct sysinfo si;
				/*
			    sysinfo (&si);
				printf ("free RAM   : %5.1f MB\n", si.freeram / megabyte);*/
				
				printf("zip file starts!\n");
				int a = system("mkdir /data/test;");
				if (a<0){
					retcode = errno;
					perror("Create File");
					sysinfo (&si);
					printf ("free RAM   : %5.1f MB\n", si.freeram / megabyte);
					}
				printf("Copy files...\n");
				a = system("cp appstart.sh leafname.csv startup.cfg sanion_sample.cid /data/test;");
				if (a<0){
					retcode = errno;
					perror("Copy File");
					sysinfo (&si);
					printf ("free RAM   : %5.1f MB\n", si.freeram / megabyte);
					}
				printf("zip Files...\n");
				a = system("tar -cvf /data/send.tar /data/test;");
				if (a<0){
					retcode = errno;
					perror("Zip File");
					sysinfo (&si);
					printf ("free RAM   : %5.1f MB\n", si.freeram / megabyte);
					}
				//usleep(100);
				printf("zip file finished!\n");
				int b;
				FILE *fp = fopen("/data/send.tar", "rb");
				if(fp == NULL){
			        perror("File error");
					return;
			    }
				fseek(fp, 0L, SEEK_END);
				int sz = ftell(fp);
				//sz = sz / 1000;
				printf("sz: %d\n",sz);
				rewind(fp);
				senddata[0] = 1;
				//senddata[1] = sz;
				memcpy(&senddata[1],&sz,sizeof(sz));
				/*
				for(int i =1;i<3;i++){
					printf("itsdata: %x\n",senddata[i]);
					}*/
				b = fread(&senddata[4], 1, MAX-4, fp);
				tot+=b;
				//printf("b: %d\n",b);
				send(sockfd, senddata, MAX, 0);			    

			    while( (b = fread(senddata, 1, sizeof(senddata), fp))>0 ){
			        send(sockfd, senddata, b, 0);
					tot+=b;
					printf("%d\n",tot);
			    }

			    fclose(fp);
				printf("file sent\n");
			}
			else if (itsdata[0]  == 2){
				printf("File Upload Request received\n");
				int a, retcode;
				int confd=0,tot=0;
				char lenbuff[3] = {};
				char temp[1] = {};
				char temp2[6] = {};
				char sending[MAX] = {};
				int round;
				lenbuff[0] = itsdata[3];
				lenbuff[1] = itsdata[2];
				lenbuff[2] = itsdata[1];
				//temp = (char *)malloc(strlen(lenbuff));
				//strcpy(temp,lenbuff);
				
				for (int i = 0;i<3;i++){
					//printf("len: %x\n",lenbuff[i]);
					sprintf(temp, "%02x", lenbuff[i]);
					strcat(temp2,temp);
					//printf("temp2: %s\n",temp2);
					}
				
				//printf("temp:%s\n",temp2);
				round = (int)strtol(temp2, NULL, 16);
				if(lenbuff == temp){
					printf("strtol failed!!\n");
					}
				printf("round: %d\n",round);
				FILE* fp = fopen( "/data/recv.tar", "wb");
				fwrite(&itsdata[4], 1, b-4, fp);
				tot+=b;
				int flag = 0;
		        if(fp != NULL){
					//printf("this is b: %d\n",b);
					//printf("this is round: %d\n",round);
		            while(flag == 0){
						//printf("Read Starts\n");
						b = read(sockfd, itsdata, sizeof(itsdata));
						fwrite(itsdata, 1, b, fp);
		                tot+=b;
						//printf("tot: %d\n",tot);
						if (tot >= round+3){
							//printf("last round\n");
							//b = read(sockfd, itsdata, (tot-round));
							//printf("read done!\n");
							//fwrite(itsdata, 1, b, fp);
							flag = 1;
							//break;
							}
		            }

		            printf("Received byte: %d\n",tot);
		            if (b<0){
		               perror("Receiving error");
		            	}
		            fclose(fp);
		        }
		        else {
		            perror("File error");
		        }
				printf("zip file starts!\n");
				system("tar -xvf /data/recv.tar;");
				if (a<0){
					retcode = errno;
					perror("Zip File");
					}
				//usleep(100);
				printf("zip file finished!\n");
				system("cp -r data/test/. .;");
				if (a<0){
					retcode = errno;
					perror("Zip File");
					}
				printf("copy finished!\n");
				system("rm -r data/test;");
				if (a<0){
					retcode = errno;
					perror("Zip File");
					}
				sending[0] = 2;
				send(sockfd, sending, MAX, 0);	
		        //close(confd);
				}
			else if (itsdata[0]  == 3){
				printf("Reboot received\n");
				int a = system("reboot;");
				int retcode;
				if (a<0){
					printf("%d\n",a);
					retcode = errno;
					perror("Reboot");
					}
				printf("reboot finished\n");
				break;
				}
			else if (itsdata[0]  == 4){
				printf("break code received\n");
				break;
				}
			else {
				printf("Function Code Unknown\n");
				}
		}
       usleep(100);
    }
}


// Driver function
void* tcpserverrun(void* params)
{

    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;
    struct ifreq ifr;
    struct sockaddr_in* addr;
    int fd;
	char* ps8IPAddr = (char *)params;
	int helperA[] = {1};

    printf(" ==========================================tcpserverrun PID: %lu\n", pthread_self());

    // Create a socket to perform the ioctl operations.
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket() failed");
        return (void *)EXIT_FAILURE;
    }

    // Zero out the ifreq structure.
    memset(&ifr, 0, sizeof(ifr));

    // Set the name of the interface we wish to modify ("eth0").
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);

    // Get the current IP address of the interface.
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        perror("SIOCGIFADDR");
    }

    // Set the new IP address.
    addr = (struct sockaddr_in*)&ifr.ifr_addr;
    addr->sin_family = AF_INET;
    if (inet_pton(AF_INET, ps8IPAddr, &addr->sin_addr) <= 0) {
        perror("inet_pton() failed");
        close(fd);
        return (void *)EXIT_FAILURE;
    }

    // Apply the change using the SIOCSIFADDR ioctl command.
    if (ioctl(fd, SIOCSIFADDR, &ifr) < 0) {
        perror("SIOCSIFADDR");
        close(fd);
        return (void *)EXIT_FAILURE;
    }

    printf("IP Address set successfully.\n");

    // Close the socket.
    close(fd);
	
	printf("TCP Connection Start!!!\n");
    while(1){
	    // socket create and verification
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		    if (sockfd == -1) {
		        printf("socket creation failed...\n");
		        exit(0);
		    }
		    else
		        printf("Socket successfully created..\n");
		    	   
		//if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)) < 0)
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, helperA, sizeof(int)) < 0)
			{printf("setsockopt(SO_REUSEADDR) failed");} // it was error() function
		else
				{printf("Socket successfully created..\n");}

	    bzero(&servaddr, sizeof(servaddr));
	   
	    // assign IP, PORT
	    servaddr.sin_family = AF_INET;
	    //servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		//printf("IP Address': %s\n", ps8IPAddr);		    
	    servaddr.sin_addr.s_addr = inet_addr(ps8IPAddr);	    
	    //servaddr.sin_addr.s_addr = inet_addr("192.168.247.106");
		//servaddr.sin_addr.s_addr = inet_addr("192.168.10.106");
	    servaddr.sin_port = htons(PORT);
		printf("MPU IP : %s\n", ps8IPAddr);
	   
	    // Binding newly created socket to given IP and verification
	    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
	        printf("socket bind failed... %s \n", ps8IPAddr);
	        exit(0);
	    }
	    else
	        printf("Socket successfully binded..\n");
	   
	    // Now server is ready to listen and verification
	    if ((listen(sockfd, 5)) != 0) {
	        printf("Listen failed...\n");
	        exit(0);
	    }
	    else
	        printf("Server listening..\n");
	    len = sizeof(cli);
	   
	    // Accept the data packet from client and verification
	    connfd = accept(sockfd, (SA*)&cli, (unsigned int *)&len);
	    if (connfd < 0) {
	        printf("server accept failed...\n");
	        exit(0);
	    }
	    else
	        printf("server accept the client...\n");
	   
	    // Function for chatting between client and server
	    func(connfd);
	   
	    // After chatting close the socket
	    close(connfd);
	    close(sockfd);
		usleep(100);
   	}
}

