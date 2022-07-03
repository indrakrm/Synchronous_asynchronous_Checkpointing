#include <iostream>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <atomic>
#include <time.h>
#include <bits/stdc++.h> 
#include <chrono> 
using namespace std::chrono; 
using namespace std;

class ProcessPicker
{
public:
	int totalNodes,stop=0,wait=0,id=0,count=0;
	int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	char buffer[1024] = {0}; 
	ProcessPicker(int totalNodes)
	{
		srand(0);
		this->totalNodes=totalNodes;
	}
	void picker()
	{
		int i=0;
		while(!this->stop)
		{
			auto start = high_resolution_clock::now(); 
			if(i==2)
				continue;
			int node_id=2+rand()%(totalNodes-1);
			//sleep(getExpRandomValue(0.01));
			sleep(6);
			sendMsg(node_id,"0,initiate recovery,failure,");
			for(int j=0;j<totalNodes;j++)
			{
				this->count=0;
				sleep(1);
				int k;
				for(k=1;k<=totalNodes;k++)
					if(k!=node_id || (k==node_id && j!=0))
						sendMsg(k,"0,initiate recovery,no failure,");
				wait=1;
				while(wait);
				cout<<"round "<<j+1<<" completed"<<endl; 
			//cout<<"intiate recovery to "<<node_id<<endl;
				
				}
				auto stop = high_resolution_clock::now(); 
				auto duration = duration_cast<microseconds>(stop - start);
				cout<<"Rollback"<<i+1<<" completed and it took "<<duration.count()<<" ms."<<endl;
				for(int k=1;k<=totalNodes;k++)
					sendMsg(k,"0,rounds completed,");
				i++;
//sleep(getExpRandomValue(0.01));
			}
		//cout<<id<<" picker terminated "<<endl;
		}
	void sendMsg(int dest,string result)   //used tcp sockets to simulate message passing
	{	
		//sleep(getExpRandomValue(lsend));  
		int sock = 0, valread; 
		struct sockaddr_in serv_addr;
		char numstr[2],numstr1[2]; 
		sprintf(numstr, "%d", this->id);
		sprintf(numstr1,"%d",dest);
		char r[result.length()+1];
		strcpy(r, result.c_str()); 
		char  const *hello = r; 
		char buffer[1024] = {0}; 
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		{ 
			printf("\n Socket creation error \n"); 
		} 
		serv_addr.sin_family = AF_INET; 
		serv_addr.sin_port = htons(4444+dest);   //port of destination 
		if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
		{ 
			printf("\nInvalid address/ Address not supported \n"); 
		} 
		if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)   //connecting to destination socket 
		{
			printf("\nConnection Failed from %d to %d\n",this->id,dest); 
		} 
		send(sock , hello , strlen(hello) , 0 );
	}
	void createSocket()  //creats socket for the cell to receive and send messages
	{
		if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
		{ 
			perror("socket failed"); 
			exit(EXIT_FAILURE); 
		}  
		if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
			&opt, sizeof(opt))) 
		{ 
			perror("setsockopt"); 
			exit(EXIT_FAILURE); 
		} 
		address.sin_family = AF_INET; 
		address.sin_addr.s_addr = INADDR_ANY;  
		address.sin_port = htons( 4444+this->id );    //all ports will be in the range of [4944,4944-no of processes-1]
		if (bind(server_fd, (struct sockaddr *)&address, 
			sizeof(address))<0) 
		{ 
			cout<<"bind "<<id<<endl;
			exit(EXIT_FAILURE); 
		}

	}

	void receive()  //socket implementation to receive messages
	{
		while(!this->stop)
		{
			if (listen(server_fd, 3) < 0) 
			{ 
				perror("listen"); 
				exit(EXIT_FAILURE); 
			}      
			if ((new_socket = accept(server_fd, (struct sockaddr *)&address,     //catches messages sent to the correspomding process
				(socklen_t*)&addrlen))<0) 
			{ 
				perror("accept"); 
				exit(EXIT_FAILURE); 
			} 
			valread = read( new_socket , buffer, 1024);  //storing message into buffer.
			char *sender = strtok(buffer, ",");
			if(strcmp(sender,"close")==0)
			{
				//cout<<id<<" rec "<<string(sender)<<endl;
				this->stop=1;   //ends the autoBroadcast thread
				break;          //ends receive thread
			}
			char *message = strtok(NULL, ",");
			//cout<<id<<" received "<<string(sender)<<endl;
			if(strcmp(message,"recovery success")==0)
			{
				count++;
				
			}
			if(count==totalNodes)
				wait=0;
		}
		//cout<<id<<" receive terminated "<<endl;
		//getDetails();  //to print details
		close(server_fd);
	}
		double getExpRandomValue(double lambda)   //generates random values corresponding to exponential distribution
	{
		int seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::default_random_engine generator (seed);
		std::exponential_distribution<double> distribution(1/lambda);
		double number = distribution(generator);
		return number;
	}
};