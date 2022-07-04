#include <iostream>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <time.h>
#include <bits/stdc++.h> 

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
	threadInfo *tInfo=new threadInfo();	
public:
	int id,maxAmount,maxNodes,amount,tempCheckpoint,checkpoint,start,end;
	vector<int> adjacencyList;
	int ackReceived=-1,stop=0,handler=0,initiateRecovery=0,initiateCheckpoint=0,reject=0,rejectAck=0,count=0,accountUpdated=1,addCompleted=1;
	int dest,receiveResponse,transactionLength=0;
	int prevIndex=-1;
	int message,initiatorParent=0,processFailed=0;
	int clearToSend=1;
	std::vector<int> transactions,transactionsAmount;
	int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int parent=0;
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
		transactions.push_back(0);
		transactionsAmount.push_back(amount);
		transactionLength++;
		//count=n-1;
	}
	void accountHandler()
	{
		//cout<<id<<" inside accountHandler "<<endl;
		//cout<<endl;
		int t=0;
		while(!this->stop)
		{
			//sleep(getExpRandomValue(1));
			if(id==1)
			{
				std::unique_lock<std::mutex> lk(tInfo->finalBank_m); 
				tInfo->finalBank_cond.wait(
					lk,[&]{return amount==maxAmount;}); 
				cout<<id<<" closure started "<<endl;
				for(int i=0;i<=maxNodes;i++)
				{
					if(i!=id)
						sendMsg(i,"close,");
				}
				sleep(3);
				sendMsg(1,"close,");
				this->stop=1;
				break;
			}
			if(id==1)
				continue;
			sleep(1);
			//sleep(1+rand()%maxNodes);
	        //pick one parent
			parent=adjacencyList[rand()%adjacencyList.size()];

			while(parent==id)
				parent=adjacencyList[rand()%adjacencyList.size()];
	        //send some money if not in checkpoint or recovery phase
			if(amount==0)
			{
				if(t==0)
				cout<<"balance is zero at node "<<id<<endl;
				t++;
				continue;
			}
			t=0;
			while(!clearToSend || !addCompleted);
			int deductAmount=0;
			if(amount<=4)
				deductAmount=amount;
			else
			{	
			while(deductAmount==0)
				deductAmount=rand()%(amount/2);
			}
			while(!clearToSend || !addCompleted)
				continue;

			ackReceived=0;
			sendMsg(parent,to_string(id)+",add,"+to_string(deductAmount)+",");
			cout<<id<<" sent "<<deductAmount<<" to node "<<parent<<endl;			
	        //wait for ack
			//while(!ackReceived);
			std::unique_lock<std::mutex> lk(tInfo->ackCheck_m); 
			tInfo->ackCheck_cond.wait(
				lk,[&]{return this->ackReceived;}); 

			accountUpdated=0;
			transactions.push_back(-1*parent);
			
			this->amount-=deductAmount;
			transactionsAmount.push_back(this->amount);
			this->transactionLength+=1;
			this->checkpoint=amount;
			
			cout<<"The amount at "<<id<<" after sent to "<<parent<<" = "<<amount<<endl;
			accountUpdated=1;	
			lk.unlock();	
	
		}
	}
	void recoveryInitiator()
	{
		while(!this->stop)
		{
			//if(!initiateRecovery)
				//continue;
			std::unique_lock<std::mutex> lk(tInfo->initiateCheckpointRecoveryProc_m); 
            tInfo->initiateProc_cond.wait(
            lk,[&]{return this->stop || this->initiateRecovery;}); 
            if(this->stop)
            	break;

			while(this->ackReceived==0 || accountUpdated==0 || addCompleted==0);
			this->clearToSend=0;
			cout<<this->id<<" recovery initiated with failure as "<<processFailed<<endl;
			if(processFailed)
			{
				checkpoint=transactionsAmount[transactionLength-2];
				amount=checkpoint;
				transactionsAmount.pop_back();
				transactions.pop_back();
				transactionLength--;
			}
			map<int,int> sentCount;
			sentCount.clear();
			int c=0;
			for(int i=1;i<=maxNodes;i++)
			{
				c=0;
				if(i!=id)
				{
					for(int j=0;j<transactionLength;j++)
					{
						if( transactions[j]==-i || transactions[j]==i)
							c++;
					}
					sentCount[i]=c;
				}
			}
			for(int i=1;i<=maxNodes;i++)
				if(i!=id)
			sendMsg(i,to_string(id)+",control,"+to_string(sentCount[i])+",");

			std::unique_lock<std::mutex> lk1(tInfo->count_m); 
            tInfo->count_cond.wait(
            lk1,[&]{return count==maxNodes-1;});  //wait for rollback acks
				/*{
					sleep(5);
					cout<<id<<" got"<<count<<endl;
				}
				*/
			this->count=0;
			sendMsg(0,to_string(id)+",recovery success,");
			this->initiateRecovery=0;
			this->processFailed=0;
		}
		//cout<<id<<" recoceryInitiator terminated "<<endl;
	}
	
	void checkpointRecoveryHandler()
	{
		
		while(this->ackReceived==0 || accountUpdated==0 || addCompleted==0);
		this->clearToSend=0;
		int temp=0,receiveCount=0;
		if(prevIndex<transactionLength-1) //no need to reconcile if no new message sent/recieved after prev checkpoint
		{
			for(int i=0;i<transactionLength;i++)
			{
				if(transactions[i]==dest || transactions[i]==-dest)	
				{
					receiveCount++;
				//if(receiveCount==this->message)
				//temp=i;
				}
			//cout<<"at 1"<<receiveCount<<" & "<<this->message<<endl;
				if(receiveCount>this->message)
				{
					temp=i-1;
					cout<<"mismatch of "<<receiveCount-this->message<<" at "<<id<<" with "<<dest<<endl;
					cout<<id<<" before update has "<<this->amount<<endl;
					this->checkpoint=this->transactionsAmount[temp];
					this->amount=this->checkpoint;
					this->prevIndex =temp;
					for(int j=temp+1;j<transactionLength;j++) //pop untill count is matched
					{
						transactions.pop_back();
						transactionsAmount.pop_back();
					}
					transactionLength=temp+1;

					cout<<id<<" updated to "<<this->amount<<endl;
					sendMsg(dest,to_string(id)+",rollback ack,");
					return;
				}
			}
		}
		sendMsg(dest,to_string(id)+",rollback ack,");
		cout<<"no mismatch at "<<id<<" with "<<dest<<endl;
		return;
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
		char  const *msg = r; 
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
		send(sock , msg , strlen(msg) , 0 );
		close(sock); //closing socket
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
				this->stop=1;   //ends the autoBroadcast thread
				tInfo->initiateProc_cond.notify_one();  
				break;          //ends receive thread
			}
			char *msg = strtok(NULL, ",");
			//cout<<id<<" rec "<<string(msg)<<" from "<<string(sender)<<endl;
			if(strcmp(msg,"control")==0)
			{
				char *status = strtok(NULL, ",");
				cout<<id<<" rec "<<string(msg)<<" as "<<string(status)<<" from "<<string(sender)<<endl;
				//cout<<id<<" checkpointRecoveryHandler"<<endl;
				
				int sentMessages;
				sscanf(status,"%d",&sentMessages);
				this->message=sentMessages;
				int x;
				sscanf(sender,"%d",&x);
				this->dest=x;
				//cout<<"At "<<id<<" CR is "<<checkpointRecoveryHandler()<<endl;4
				checkpointRecoveryHandler();
			}
			else if(strcmp(msg,"add")==0)
			{
				char *a = strtok(NULL, ",");
				int addAmount,x;			
				sscanf(a,"%d",&addAmount);
				sscanf(sender,"%d",&x);

				std::unique_lock<std::mutex> lk(tInfo->ackCheck_m); 
				addCompleted=0;

				this->amount+=addAmount;
				transactions.push_back(x);
				transactionsAmount.push_back(this->amount);
				cout<<"Amount after add to node "<<id<<" = "<<amount<<endl;
				this->checkpoint=this->amount;
				this->transactionLength+=1;

				sendMsg(x,to_string(id)+",ack,");
				if(id ==1)
					tInfo->finalBank_cond.notify_one();
				addCompleted=1;
				lk.unlock();
			}
			else if(strcmp(msg,"ack")==0)
			{

				this->ackReceived=1;
				tInfo->ackCheck_cond.notify_one(); 
			}
			else if(strcmp(msg,"rollback ack")==0)
			{
				this->count+=1;
				if(this->count==maxNodes-1)
		    		tInfo->count_cond.notify_one(); 
			}
			else if(strcmp(msg,"initiate recovery")==0)
			{
				char *a = strtok(NULL, ",");
				if(strcmp(a,"failure")==0)
					processFailed=1;
				this->initiateRecovery=1;
				tInfo->initiateProc_cond.notify_one();  
			}
			else if(strcmp(msg,"rounds completed")==0)
			{
				cout<<id<<" has "<<amount<<endl;
				sleep(2);
				clearToSend=1;
			}
		}
		getDetails();  //to print details
		close(server_fd); //closing socket
	}
	void getDetails()
	{
		cout<<"My branch id is "<<id<<" and account balance is "<<amount<<endl;
	}	
};