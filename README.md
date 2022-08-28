# Synchronous_Asynchronous_Checkpointing

This is a C++ based simulation of transaction reconcilliation with recovery capability from failure .
<br>
Implemented both coordinated and uncoordinated checkpointing approaches which are used in recovery of sytem from failure.
<br>
For coordinated checkpointing, Koo-and-Toueg-algorithm is used.
For uncoordinated checkpointing, juang venkatesan algorithm is used
<br>
Simulation Flow
<br>
1)Threads will be created for each node and all threads start sending/receiving random amount.
<br>
2)ProcessPicker periodically picks up a node to initiate chekpointing in case of coordinated checkpointing
<br>
3)ProcessPicker preiodically picksup a node and fails it to simulate recovery.
<br>
4) After recovery sum of amount at every node should be same as initial amount.No node sould have orphan messages.
<br>
5)Simulation ends when root node gets all the money from other nodes.
<Br>
6)At end, root node amount should be equal to amount of all nodes at the start of simulation.
