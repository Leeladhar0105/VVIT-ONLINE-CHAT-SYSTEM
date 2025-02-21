#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>


//global data item these two are accessible by all threads(2 threads)-common data items
int aliveflag=1,sockfd;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//
//thread to send messages to server
void* sender()
{
        char msg[256];  // this msg buffer is not common to two threads so mutexes are not required.
	while(aliveflag==1)
	{       bzero(msg,256);
		scanf(" %[^\n]%*c", msg);
		int v=send(sockfd, msg, sizeof(msg), 0);

		if(strcmp(msg, "exit")==0){
		  pthread_mutex_lock(&mutex);  aliveflag=0;    pthread_mutex_unlock(&mutex);

	       printf("--You left the discussion--\n");
		 close(sockfd);
  //dont put this close after the loop bcz already closed due to receiver if loop is terminated due to aliveflag=0.
			break;
			}
		if(v<=0)
		printf("(This %s msg is not sent to server,may be you are disconnected)\n",msg);
		//else
		//printf("(sent %s msg to all current group members by you)\n",msg);
	}


}

//thread to receive messages from server
void* receiver()
{
char msg[256];  // this msg buffer is not common to two threads so mutexes are not required.
	while(aliveflag==1)
	{
		bzero(msg,256);
		if(recv(sockfd, msg, sizeof(msg), 0) > 0)
		{
		 if(strcmp(msg, "limitreached")==0){
		   printf("server sent a msg saying that maxlimit of students are reached just before we connected to server\n");
		   printf("it closed our socket on it's side,so closing created socket and terminating the client process\n");
		   printf("please try again after some time\n");
		 close(sockfd);
		  pthread_mutex_lock(&mutex);  aliveflag=0;    pthread_mutex_unlock(&mutex);
		  //even though we made aliveflag=0 saying to sender thread saying that terminate yourself,it already entered the iteration so after that extra iteration execution only it will stopp the execution.
		 break;//this break is not required actually since aliveflag became 0
		                }

			printf("%s", msg);
		}
	}
	//no close(sockfd) required bcz already closed by sender thread.
	//there is no break in this while loop you can observe.this loop stops once aliveflag becomes 0.
}


int main(int argc,char **argv){
   if(argc!=2){
    printf("provide port number also beside ./a.out\n");
    exit(0);
 }
	//creating a socket
	sockfd= socket(AF_INET, SOCK_STREAM, 0);
	int PORT=atoi(argv[1]);
        char *ip="127.0.0.1";
	if(sockfd< 0)
	{
		printf("Error in creating socket\n");
		exit(1);
	}
	printf("-----WELCOME TO VVIT ONLINE DISCUSSION PLATFORM FOR SHARING NEW IDEAS----\n");
	printf("a.Socket created successfully\n");
	//specify/initialise an address for socket
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = inet_addr(ip);

	//Establish a connection using connect()
	int connect_status = connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address));
	if(connect_status < 0)
	{
		printf("Connection failed\n");
		close(sockfd);
		exit(1);
	}
	

	printf("b.Connected to server successfully\n");//connected to group server

	char name[256];
	printf("Your name: ");
	scanf("%[^\n]%*c",name);
	send(sockfd, name, sizeof(name), 0);
	printf("\n>Rules of the online discussion platform:\n");
	printf("1.You joined the discussion with %s as your name.\n",name);
     printf("2.If entered exit you will leave the discussion and all students connected at that time will get informed.\n");
       printf("3.If entered any other msg it will be sent to all students connected at that time by you.\n");
     printf("4.This platform is developed by cse student of VVIT.Feel free to suggest any updates to this platform.\n");
	//sender thread
	pthread_t sender_thread;
	if(pthread_create(&sender_thread, NULL, &sender, NULL)!=0){
		printf("Error in creating sender thread\n");
		close(sockfd);
		 exit(1);
	}
	//receiver thread
	pthread_t receiver_thread;
	if(pthread_create(&receiver_thread, NULL, &receiver, NULL)!=0){
		printf("Error in creating receiver thread\n");
		close(sockfd);
		 exit(1);
		}

	//join threads,it will make process to wait/not terminate itself until two threads terminates.
	pthread_join(sender_thread, 0);
	pthread_join(receiver_thread, 0);
    //so one process for managing and two threads overall.
    //even though 2 threads are present,no need of mutexes bcz they are not modifying common data items(sockfd,aliveflag)
	return 0;
}

