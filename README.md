# LRU-based-MOESIF-cache-coherency-protocol, ECE M116C - CS M151B Computer Architecture Systems - UCLA

The goal of this project is to learn about cache coherency and cache management in a modern multi-core system. You should design an LRU-based MOESIF cache coherency protocol for a four-core machine and report various statistics including the number of cache misses, writeback, etc. 

Input File:

The input file has a sequence of reads and writes with the core ID and the tag/address. It looks like this:

P2: read <160>
P2: read <100>
P1: read <120>
P1: write <120>

In the example above, “P2” is the core ID and the number within <> is the tag.
Stats
The code should maintain the following statistics:
Number of cache hits
Number of cache misses
Number of writebacks
Number of broadcasts
Number of cache-to-cache transfers

For example, for the sequence shown here: 
P2: read <160>
P2: read <100>
P1: read <120>
P1: read <100>
P2: read <160>
P1: write <120>

Your code should print the following numbers:
2
4
0
4
1

The only requirement is that it should be compiled with g++ *.cpp.
Then run ./coherentsim <inputfile.txt>

