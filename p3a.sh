#!/bin/bash
list="TcpNewReno TcpHybla TcpHighSpeed TcpHtcp TcpVegas TcpScalable TcpVeno TcpBic TcpYeah TcpIllinois TcpWestwood TcpWestwoodPlus TcpLedbat"
for one_thing in $list
do for ((i=1;i<64;i=i*2))
do cd "localtion_of_your_ns_3_folder_where_waf_is_located" 
./waf --run "p1 --nSpokes=$i --protocol=$one_thing" > "localtion_where_you_want_your_output_files_to_be" /$one_thing$i.txt
done
done