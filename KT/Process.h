#include <iostream>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <atomic>
#include <time.h>
#include <bits/stdc++.h> 
#include <mutex>
#include <thread>
#include <condition_variable>
using namespace std;
struct threadInfo
{
	std::condition_variable count_cond;
	std::mutex count_m;

	std::condition_variable initiateProc_cond;
	std::mutex initiateCheckpointRecoveryProc_m;

	std::condition_variable ackCheck_cond;
    std::mutex ackCheck_m;

    std::condition_variable finalBank_cond;
    std::mutex finalBank_m;

    std::mutex m;

};
class Process
{
private :
	threadInfo *t=new threadInfo();	
public:
	int id,maxAmount,maxNodes,amount,tempCheckpoint,checkpoint;
	vector<int> adjacencyList;
	int ackReceived=-1,stop=0,handler=0,initiateRecovery=0,initiateCheckpoint=0,reject=0,count=0,accountUpdated=1,addCompleted=1,startControl=0;
	int dest;
	string message;
	int clearToSend=1;
	int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	char buffer[1024] = {0}; 


	Process(int id,int maxAmount,int n,int amount,vector<int> adjacencyList)
	{
		srand(time(0)); 
		this->id=id;
		this->maxAmount=maxAmount;
		this->maxNodes=n;
		this->amount=amount;
		this->checkpoint=amount;
		this->adjacencyList=adjacencyList;
	}
	void accountHandler()
	{
	//cout<<id<<" inside accountHandler "<<endl;
		int restart=0;
		while(!this->stop)
		{
			if(id==1)
			{
				std::unique_lock<std::mutex> lk(t->finalBank_m); 
				t->finalBank_cond.wait(
					lk,[&]{return amount==maxAmount;}); 
				sleep(2);
				cout<<id<<" closure started "<<endl;
				for(int i=0;i<=maxNodes;i++)
				{
					if(i!=id)
						sendMsg(i,"close,");
				}
				sleep(3);
				lk.unlock();
				sendMsg(1,"close,");
				this->stop=1;
				break;
			}
			//if(id==1)
				//continue;
			sleep(1);
            //send some money if not in checkpoint or recovery phase
			if(amount==0)
			{
				cout<<"balance is zero at node "<<id<<endl;
				continue;
			}
			//pick one parent
			int parent=adjacencyList[rand()%(adjacencyList.size())];
			int deductAmount=0;
			if(amount<=4)
				deductAmount=amount;
			else			
				while(deductAmount==0)
					deductAmount=rand()%(amount/2);

			while(!clearToSend)
			{
				continue;
			}

			cout<<id<<" sent "<<deductAmount<<" to node "<<parent<<endl;
			sendMsg(parent,to_string(id)+",add,"+to_string(deductAmount)+",");
			ackReceived=0;
			accountUpdated=0;
            //wait for ack
			std::unique_lock<std::mutex> lk(t->ackCheck_m); 
            t->ackCheck_cond.wait(
            lk,[&]{return this->ackReceived;}); 
            //update account
            if(ackReceived>0)
            {
            	amount-=deductAmount;
            	cout<<"Balace at "<<id<<" after sent to "<<parent<<"= "<<amount<<endl;
            }
            accountUpdated=1;
            //ackReceived=0;
			lk.unlock();
			//t->ackCheck_cond.notify_one(); 
		}

	}
	void checkpointInitiator()
	{
		while(!this->stop)
		{
			//if(!this->initiateCheckpoint)
				//continue;
			std::unique_lock<std::mutex> lk(t->initiateCheckpointRecoveryProc_m); 
            t->initiateProc_cond.wait(
            lk,[&]{return this->stop || this->initiateCheckpoint;}); 
            if(this->stop)
            	break;


			while(this->ackReceived==0 || accountUpdated==0 || addCompleted==0);
			this->clearToSend=0;
			cout<<"Node "<<this->id<<" initiated checkpoint procedure"<<endl;
			for(int i=1;i<=maxNodes;i++)
			{
				sendMsg(i,to_string(id)+",control,checkpoint,");
			}
			//while(count<maxNodes);

		    std::unique_lock<std::mutex> lk1(t->count_m); 
            t->count_cond.wait(
            lk1,[&]{return count==maxNodes;}); 

			for(int i=1;i<=maxNodes;i++)
			{
				if(reject)
					sendMsg(i,to_string(id)+",control,checkpoint failure,");
				else
					sendMsg(i,to_string(id)+",control,checkpoint success,");						
			}
			this->count=0;
			this->reject=0;
			this->initiateCheckpoint=0;
			
			//sendMsg(0,to_string(id)+",checkpoint failu,");
			if(!reject)
			{
				sendMsg(0,to_string(id)+",checkpoint success,");
				cout<<this->id<<" checkpoint success"<<endl;
			}
			else
			{
				sendMsg(0,to_string(id)+",checkpoint failure,");
				cout<<this->id<<" checkpoint failure"<<endl;
			}

		}

	}
	void recoveryInitiator()
	{
		while(!this->stop)
		{
			//if(!initiateRecovery)
				//continue;
			std::unique_lock<std::mutex> lk(t->initiateCheckpointRecoveryProc_m); 
            t->initiateProc_cond.wait(
            lk,[&]{return this->stop || this->initiateRecovery;}); 
            if(this->stop)
            	break;
			while(this->ackReceived==0 || accountUpdated==0 || addCompleted==0)
				sleep(1);
			this->clearToSend=0;

			cout<<"Node "<<this->id<<" initiated recovery procedure"<<endl;
			for(int i=1;i<=maxNodes;i++)
			{
				sendMsg(i,to_string(id)+",control,recovery,");
			}
			
			std::unique_lock<std::mutex> lk1(t->count_m); 
            t->count_cond.wait(
            lk1,[&]{return count==maxNodes;}); 
			
			for(int i=1;i<=maxNodes;i++)
			{
				if(reject)
					sendMsg(i,to_string(id)+",control,recovery failure,");
				else
					sendMsg(i,to_string(id)+",control,recovery success,");						
			}	
			this->reject=0;
			this->count=0;
			this->initiateRecovery=0;
			
			if(!reject)
			{
				sendMsg(0,to_string(id)+",recovery success,");
				cout<<this->id<<" recovery success"<<endl;
			}
			else
			{
				sendMsg(0,to_string(id)+",recovery failure,");
				cout<<this->id<<" recovery failure"<<endl;
			}

		//this->clearToSend=1;
		}

	}
	void checkpointRecoveryHandler()
	{
        //cout<<id<<" CR started"<<endl;
		while(!this->stop)
		{	
			if(!startControl)
				continue;
			//cout<<id<<" "<<"into"<<endl;
			if(this->message=="checkpoint" || this->message=="recovery")
			{

				while(this->ackReceived==0 || accountUpdated==0 || addCompleted==0);
				//cout<<id<<" "<<"in"<<endl;
			    this->tempCheckpoint = this->amount;
				sendMsg(dest, to_string(id) + ",yes,");
			}
			if (this->message == "checkpoint success" || this->message == "recovery success")
			{

				if (this->message == "recovery success")
				{
					this->amount = this->checkpoint;
					cout << "After Recovery "
						 << "Node " << this->id << "'s account has " << this->amount << endl;
				}
				else
					this->checkpoint = this->tempCheckpoint;
				this->clearToSend=1;
			}
			if(this->message=="checkpoint failure" || this->message=="recovery failure")
			{
				this->clearToSend=1;
			}
			startControl=0;
		}
	}
    void sendMsg(int dest,string result)   //used tcp sockets to simulate message passing
    {
    	int sock = 0, valread; 
    	struct sockaddr_in serv_addr;
    	char numstr[2],numstr1[2]; 
    	sprintf(numstr, "%d", this->id);
    	sprintf(numstr1,"%d",dest);
    	char r[result.length()+1];
    	strcpy(r, result.c_str()); 
    	char  const *msg = r; 
    	char buffer[1024] = {0}; 
    	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    	{ 
    		printf("\n Socket creation error \n"); 
    	} 
    	serv_addr.sin_family = AF_INET; 
	    serv_addr.sin_port = htons(5344+dest);   //port of destination 
	    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	    { 
	    	printf("\nInvalid address/ Address not supported \n"); 
	    } 
	    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)   //connecting to destination socket 
	    { 
	    	printf("\nConnection Failed from %d to %d\n",this->id,dest); 
	    } 
	    send(sock , msg , strlen(msg) , 0 );
	    close(sock);
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
	    address.sin_port = htons( 5344+this->id );    //all ports will be in the range of [5344,5344-no of processes-1]
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
			    this->stop=1;   //ends the autoBroadcast thread
			    t->initiateProc_cond.notify_all();  
			    break;          //ends receive thread
		    }
		    char *msg = strtok(NULL, ",");

		    if(strcmp(msg,"control")==0)
		    {

		    	this->clearToSend=0;
		    	cout<<id<<" rec "<<string(msg)<<" from "<<string(sender)<<endl;

		    	char *status = strtok(NULL, ",");
		    	this->message=string(status);
		    	int x;
		    	sscanf(sender,"%d",&x);
		    	this->dest=x;

		    	this->startControl=1;
		    	if(this->message=="checkpoint success" || this->message=="recovery success"|| this->message=="checkpoint failure" || this->message=="recovery failure")
		    		while(startControl);

		    }  
		    else if(strcmp(msg,"add")==0)
		    { 
		    	char *a = strtok(NULL, ",");

		    	int addAmount,x;			
		    	sscanf(a,"%d",&addAmount);
		    	sscanf(sender,"%d",&x);
		    	if(this->message =="checkpoint" || this->message =="recovery")
		    	{
		    		sendMsg(x,to_string(id)+",nack,");;
		    		continue;
		    	}
		    	addCompleted=0;
		    	std::unique_lock<std::mutex> lk(t->ackCheck_m); 
		    	this->amount+=addAmount;
		    	if(id ==1)
		    		t->finalBank_cond.notify_one();
		    	cout<<"After addition, node "<<id<<"\'s amount = "<<amount<<endl;
		    	sendMsg(x,to_string(id)+",ack,");
		    	addCompleted=1;
		    	lk.unlock();
		    }
		    else if(strcmp(msg,"ack")==0)
		    {
		    	this->ackReceived=1;
		    	t->ackCheck_cond.notify_one(); 

		    	//while(!accountUpdated);
		    }
		    else if(strcmp(msg,"nack")==0)
		    {
		    	this->ackReceived=-1;
		    	t->ackCheck_cond.notify_one(); 

		    	//while(!accountUpdated);
		    }
		    else if(strcmp(msg,"initiate recovery")==0)
		    {
		    	if(this->initiateRecovery)
		    		continue;
		    	this->initiateRecovery=1;
		    	t->initiateProc_cond.notify_all();  
		    }
		    else if(strcmp(msg,"initiate checkpoint")==0)
		    {
		    	if(this->initiateCheckpoint)
		    		continue;
		    	this->initiateCheckpoint=1;
		    	t->initiateProc_cond.notify_all();  
			//cv.notify_one();
		    }
		    else if(strcmp(msg,"yes")==0 || strcmp(msg,"no")==0)
		    {
		    	cout<<id<<" rec "<<string(msg)<<" from "<<string(sender)<<endl;
		    	if(strcmp(msg,"no")==0)
		    		this->reject=1;
		    	this->count++;
		    	if(this->count==maxNodes)
		    		t->count_cond.notify_one();  
		    }

		}
	    getDetails();  //to print details
	    close(server_fd);
	}
	void getDetails()
	{
		cout<<"My branch id is "<<id<<" and account balance is "<<amount<<endl;
	}
};