#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>


struct client{
	int index;//this field has nothing to with threadid.just a unique id[0,MAX_CLIENTS-1] for all clients.
	int socketID;
	char name[256];
};

//global data item can be accessed by all threads.so for this data items modification/reading synchronisation is required.
static int MAX_CLIENTS=0;
static int client_count=0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex2= PTHREAD_MUTEX_INITIALIZER;
  //mutex to synchronize betweeen threads
struct client *clientlist[256];
pthread_t thread[256];
//

//add client to clientlist[]
void addclient(struct client *client)
{
	pthread_mutex_lock(&mutex);//clientlist[] is common to all threads ,so synchronisation is required.
	for(int i=0;i<MAX_CLIENTS;i++)
	{
		if(!clientlist[i]){
			clientlist[i] = client;
			break;
		}
	}
	pthread_mutex_unlock(&mutex);
	//here we are adding in middle indexes also where the place is available,we are not adding at last.so that some indexes are not wasted and at any time MAX_CLIENTS can be there.
}

//removing client info with index k in clientlist[]
//here we are deleting the client solely based on its index,even though unique index for every client,some problem might occur bcz based on which thread accesses first there might be problem i guess not sure.(refer bug in context.txt).
void deleteclient(int k)
{
	pthread_mutex_lock(&mutex);//clientlist[] is common to all threads ,so synchronisation is required.

	for(int i=0;i<MAX_CLIENTS;i++)
	{   if(clientlist[i]){
			if(clientlist[i]->index==k)
			{clientlist[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&mutex);
}

//send buffer[] to all clients except client with index k(sender)
void sendtoall(char buffer[], int k)
{
	pthread_mutex_lock(&mutex);//clientlist[] is common to all threads ,so synchronisation is required.
	for(int i=0;i<MAX_CLIENTS;i++){
		if(clientlist[i]){
			if(clientlist[i]->index!=k)
			{
				send(clientlist[i]->socketID, buffer,256, 0);
			}}}
	pthread_mutex_unlock(&mutex);
}

//client speicific thread,this will be created for every client accepted by server(original process).
//this thread will be dedicated to single client.
void * clientthread(void * clientinfo)
{
//argument is pass by reference so copy info to other memory belonging to this thread only,typecast and use it.
        char name[256],buffer[256];
	struct client* client = (struct client *) clientinfo;
	 //used other structure bcz we must typecast received argument and avoid use of the memory inside main stack.-common data.
	int socketID = client->socketID,index = client->index; strcpy(name, client->name);

      if(client_count==1) { //only this client is present
	    sprintf(buffer, "5.No students present currently.\n\nDiscussion:\n");  send(socketID, buffer, sizeof(buffer), 0);
	     }
	else{
	//send current members info to new client added
	sprintf(buffer, "5.Current members of the discussion are:\n");
	send(socketID, buffer, sizeof(buffer), 0);
	pthread_mutex_lock(&mutex);
	for(int i=0;i<MAX_CLIENTS;i++){
		if(clientlist[i]){
		//active++;
		if(clientlist[i]->index!=index){
		bzero(buffer,256);
		sprintf(buffer, "%d.%s\n",i+1,clientlist[i]->name);
               send(socketID, buffer, sizeof(buffer), 0);
                                              }
                  }
                  }
                  bzero(buffer,256);
		sprintf(buffer, "\nDiscussion:\n");
               send(socketID, buffer, sizeof(buffer), 0);
                  pthread_mutex_unlock(&mutex);
                  }

      bzero(buffer,sizeof(buffer));
	sprintf(buffer,"--%s joined the discussion--\n", name);
	printf("%s", buffer);
	sendtoall(buffer, index);

	while(1)
	{
		bzero(buffer,256);
		char message[256];
		//receive messages from client
		int bytes = recv(socketID, buffer, sizeof(buffer), 0);
		buffer[bytes] = '\0';
		if(strcmp(buffer, "exit")==0)
		{
		deleteclient(index);
		        pthread_mutex_lock(&mutex2);
	            client_count--;
	            //printf("client_count decremented to %d bcz\n",client_count);
	            pthread_mutex_unlock(&mutex2);
			sprintf(message, "--%s left the discussion--\n", name);
			sendtoall(message, index);
			printf("%s",message);
		 	//remove client data in clientlist[]
	            close(socketID); free(client);
			break;
		}

		sprintf(message, ">%s:%s\n",name, buffer);
		sendtoall(message, index);
		printf("%s",message);
	}
  pthread_detach(pthread_self());
	return NULL;
}


int main(int argc,char **argv){
   if(argc!=2){
    printf("provide port number also\n");
    exit(0);
 }
	int PORT=atoi(argv[1]);
        char *ip="127.0.0.1";
	// create a server socket
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(server_socket < 0)
	{
		printf("Error in connection\n");
		exit(1);
	}
	printf("1.Server socket created successfully\n");

	//define server address
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = inet_addr(ip);

	//bind the socket to specified IP and port
	int bind_status = bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));
	if(bind_status < 0)
	{
		printf("Error in binding\n");
		exit(1);
	}
	printf("2.Binded to port %d successfully\n", PORT);
	printf("3.Enter students limit:");
	scanf("%d",&MAX_CLIENTS);

	//listen to connections
	int listen_status = listen(server_socket, 5);
	if(listen_status!=0){
		printf("Error in listening\n");
		exit(1);  }
	printf("4.server started listening on port %d\n",PORT);
	printf("5.Maximum number of students allowed at a time are %d\n\n",MAX_CLIENTS);
	//printf("-----------------------------------------------------------\n");
	printf("VVIT ONLINE DISCUSSION PLATFORM\n");

	//accept connections
	while(1)
	{
		int socketID = accept(server_socket, NULL, NULL);
		//we put NULL bcz we dont want clients ip address,port number et bcz we are receiving name of the client.

		pthread_mutex_lock(&mutex2);
		client_count++;
		//printf("client_count incremented to %d bcz\n",client_count);
		pthread_mutex_unlock(&mutex2);
		if(client_count == MAX_CLIENTS+1)
		{
		  //dealt with client some smoothly,without closing the socket apruptly.
		  char m[50];
		     sprintf(m, "limitreached");
		     send(socketID,m, sizeof(m), 0);
			close(socketID);
			         pthread_mutex_lock(&mutex2);
	            client_count--;
	          //  printf("client_count decremented to %d bcz\n",client_count);
	            pthread_mutex_unlock(&mutex2);

	            char errormsg[50];
	            sprintf(errormsg,"--one student attempted to join the discussion but server discarded the request since students limit is reached--\n");
	printf("%s", errormsg);
	sendtoall(errormsg,MAX_CLIENTS+1); //sending this as index will make all clients to recv this errormsg
			continue;
		}

		struct client *clientinfo = (struct client *)malloc(sizeof(struct client));
	char name[256];
	 int bytes = recv(socketID, name, sizeof(name), 0);
	 

	clientinfo->socketID = socketID; clientinfo->index = client_count; strcpy(clientinfo->name, name);

		//add client to clientlist[](array of structures).
		addclient(clientinfo);
		//thread to handle clients
		pthread_create(&thread[client_count], NULL, &clientthread, (void *) clientinfo);
	//thread[client_count] will get filled by thread id of created thread,clientinfo is argument to that function thread.
	//may be we have to only pass void data types,not sure.
	}

	return 0;
}

