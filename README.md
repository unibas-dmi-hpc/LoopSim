## Loop-simulator
Simulates the execution of loop iterations, where each loop iteration is considered an independent sequential computational task, on multicore HPC cluster.
The simulator is based on SimGrid simulation toolkit (https://github.com/simgrid/simgrid) and uses its SimDag interface.
The simulator employs self-scheduling approach to load balance the loop execution.
It is used to examine the performance of 13 different loop scheduling techniques. 

Currently supported loop scheduling techniques are:
  1.  Straightforward static scheduluing (STATIC)
  2.  Self Scheduling (SS)
  3.  Fixed size chunk (FSC)
  4.  Modified fixed size chhunk (mFSC)
  5.  Guided self-scheduling (GSS) 
  6.  Trapezoid self-scheduling (TSS)
  7.  Factoring (FAC)
  8.  Weighted factoring (WF)
  9.  Adaptive weighted factoring (AWF)
  10. Adaptive weighted factoring - Batch (AWF-B)
  11. Adaptive weighted factoring - Chunk (AWF-C)
  12. Adaptive weighted factoring - Batch with scheduling overhead (AWF-D)
  13. Adaptive weighted factoring - Chunk with scheduling overhead (AWF-E)
  14. Adaptive factoring (AF)


Loop iterations and computing system characteristics need to be passed to the simulator as input files.
[1] FLOP file, where each line contains the the number of floating-point operations in this loop iteration 
[2] Platform file to describe the system


# Authors:

[1] Ali Mohammed

[2] Ahmed Eleliemy

[3] Florina M. Ciorba

[4] Franziska Kasielke

[5] Ioana Banicescu


# Publications:

[1] Ali Mohammed and Florina M. Ciorba. Research Report - University of Basel, Switzerland. https://drive.switch.ch/index.php/s/imo0kyLe3PmETWL. [Online; accessed 20 May 2019]. October 2018.

[2] Ali Mohammed and Florina M. Ciorba, “SiL: An Approach for Adjusting Applications to Heterogeneous Systems Under Perturbations”, in Proceedings of the International Workshop on Algorithms, Models and Tools for Parallel Computing on Heterogeneous Platforms (HeteroPar 2018) of the 24th International European Conference on Parallel and Distributed Computing (Euro-Par 2018), Turin, Italy, August 27-31, 2018.

[3] Ali Mohammed, Ahmed Eleliemy, and Florina M. Ciorba, “Performance Reproduction and Prediction of Selected Dynamic Loop Scheduling Experiments”, in Proceedings of the International Conference on High Performance Computing & Simulation (HPCS 2018), Orléans, France, July 16-20, 2018.

[4] Ali Mohammed, Ahemd Eleliemy, Florina M. Ciorba, Franziska Kasielke, and Ioana Banicescu, “Experimental Verification and Analysis of Dynamic Loop Scheduling in Scientific Applications”, in Proceedings of the 17th International Symposium on Parallel and Distributed Computing (ISPDC 2018), Geneva, June 25-28, 2018.
