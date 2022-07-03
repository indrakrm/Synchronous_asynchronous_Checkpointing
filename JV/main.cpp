#include <iostream>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <atomic>
#include <bits/stdc++.h>

#include "ProcessPicker.h" 
#include "Process.h"

using namespace std;
	string get_time()     //returns input time as HH:mm:ss:nanoseconds format
	{
		struct timespec now;
		clock_gettime(CLOCK_REALTIME, &now);
		tm *ltm=localtime(&now.tv_sec);
		string h=to_string(ltm->tm_hour);
		string m=to_string(ltm->tm_min);
		h.append(":");
		h.append(m);
		h.append(":");
		h.append(to_string(ltm->tm_sec));
		h.append(":");
		h.append(to_string(now.tv_nsec));  //nano seconds part
		return h;
	}
int main()
{
	int i=1,j=1,pid=1,s,ld,n,max_amount;
	cin>>n>>max_amount;
	vector<vector<int>> adjacencyList;//bank graph
	int amount_adjacencyArray[n]; //respective amounts
	
	int count=0;
	int l=0;
	for(int i=0;i<n;i++)
	{
		cin>>l;
		vector<int> temp;
		for(int j=0;j<l;j++)
		{
			int v;
			cin>>v;
			temp.push_back(v);
		}
		adjacencyList.push_back(temp);
	}


	l=0;
	while(count<n)
	{
		cin>>l;
		amount_adjacencyArray[count++]=l;
	}

	for (i=2;i<=n;i++)
	{
		if(pid>0)     //ensures that only main process is creating the processes
		{
			//j=0;
			pid=fork();
			j=i;
		}  
	}
	std::vector<std::thread> some_threads,some_threads1;
	if(pid>0)   //initiator process
	{
		j=0;
		Process main_lc=Process(1,max_amount,n,amount_adjacencyArray[0],adjacencyList[0]);
		main_lc.createSocket();
		main_lc.getDetails();
		some_threads1.push_back(std::thread(&Process::receive,&main_lc));
		sleep(5);
		some_threads1.push_back(std::thread(&Process::accountHandler,&main_lc));
		//some_threads1.push_back(std::thread(&Process::addAmountHandler,&main_lc));
		//some_threads1.push_back(std::thread(&Process::checkpointRecoveryHandler,&main_lc));
	//	some_threads1.push_back(std::thread(&Process::checkpointInitiator,&main_lc));
		some_threads1.push_back(std::thread(&Process::recoveryInitiator,&main_lc));
		ProcessPicker pp=ProcessPicker(n);
		pp.createSocket();
		sleep(5);
		some_threads1.push_back(std::thread(&ProcessPicker::receive,&pp));
		some_threads1.push_back(std::thread(&ProcessPicker::picker,&pp));
		for (auto& t: some_threads1) 
			t.join();
	}
	else  //other than initiator process.
	{
		Process lc=Process(j,max_amount,n,amount_adjacencyArray[j-1],adjacencyList[j-1]);
		lc.createSocket();
		lc.getDetails();
		some_threads.push_back(std::thread(&Process::receive, &lc));
		sleep(5);
		some_threads.push_back(std::thread(&Process::accountHandler,&lc));
		//some_threads.push_back(std::thread(&Process::addAmountHandler,&lc));
		//some_threads.push_back(std::thread(&Process::checkpointRecoveryHandler,&lc));
	//	some_threads.push_back(std::thread(&Process::checkpointInitiator,&lc));
		some_threads.push_back(std::thread(&Process::recoveryInitiator,&lc));
		for (auto& t: some_threads) 
			t.join();
	}
	cout<<j<<" exit "<<get_time()<<endl;
	//cout<<endl<<get_time()<<endl;
}
