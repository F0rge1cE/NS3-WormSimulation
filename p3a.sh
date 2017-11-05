#!/bin/bash
# list="TcpNewReno TcpHybla TcpHighSpeed TcpHtcp TcpVegas TcpScalable TcpVeno TcpBic TcpYeah TcpIllinois TcpWestwood TcpWestwoodPlus TcpLedbat"
msg = "nullmsg"
list = [1, 2, 4]
for i in $list
do cd "scratch" 
mpirun --np2 ../waf --run "worm --threads=$i"
mpirun --np2 ../waf --run "worm --threads=$i --nullmsg=$msg"
# ./waf --run "p1 --nSpokes=$i --protocol=$one_thing" > "localtion_where_you_want_your_output_files_to_be" /$one_thing$i.txt
done