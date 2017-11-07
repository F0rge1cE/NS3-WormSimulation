
# 1)Make a file called "anyname".sh (e.g. homework1.sh)
# 2) paste the code underneath:

#!/bin/bash
list="Uniform Local Sequential"
rate="5 10 20"
for one_thing in $list
do for scan in $rate
do cd ~/Desktop/NS3/ns-allinone-3.26/ns-3.26
mpirun -np 1 ./waf --run "betaRelease --ScanRate=$scan --ScanPattern=$one_thing BackbonDelay=10ms --SyncType=Yawns" >> ~/Desktop/p4cResults/$scan$one_thing$i.txt
# ./waf --run "p1 --nSpokes=$i --Protocol=$one_thing" > /home/parallels/Desktop/p1Results/$one_thing$i.txt
done
done

# 3) Make sure you give your script permission "chmod 777 "anyname".sh" (e.g. chmod 777 script.sh)
# 4) Run the script as ./"anyname".sh (e.g. ./script.sh)

# For Reference: My exact script.sh

#!/bin/bash
# list="TcpNewReno TcpHybla TcpHighSpeed TcpHtcp TcpVegas TcpScalable TcpVeno TcpBic TcpYeah TcpIllinois TcpWestwood TcpWestwoodPlus TcpLedbat"
# for one_thing in $list
# do for ((i=1;i<64;i=i*2))
# do cd /home/programmer/Desktop/NS3/ns-3-allinone/ns-3-dev
# ./waf --run "p1 --nSpokes=$i --tcpProtocol=$one_thing" > /home/programmer/Desktop/Results/P1-ECE6110/$one_thing$i.txt
# done
# done

# Troubleshooting Notes: Make sure your argument names in cmd.parse are the same (nSpokes,protocol). I named mine nSpokes and tcpProtocol, but yours may be different.

# -Chris
