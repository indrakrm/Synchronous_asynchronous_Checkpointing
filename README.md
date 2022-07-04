# Synchronous_asynchronous_Checkpointing

This is a C++ based simulation of transaction reconcilliation with recovery from failure capability.
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
3)ProcessPicker preiodically picksup a node and fails it to simulate recovery
<br>
4)Simulation ends when node 1 gets all the money from other nodes.
<Br>
5)At end, node 1 amount should be equal to amount of all nodes at the start of simulation.
