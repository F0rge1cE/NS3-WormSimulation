#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/packet-sink.h"

using namespace ns3;


int 
main (int argc, char *argv[])
{

	SeedManager::SetSeed(1);


  CommandLine cmd;
  //cmd.AddValue ("nSpokes", "Number of spokes to place in the star", nSpokes);
  // cmd.AddValue("Protocol", "TCP Variant: TCPNewReno, TCPHybla, TcpHighSpeed", protocol);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create(2)；

  InternetStackHelper internet;
  internet.Install (nodes);


  	PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("5ms"));

    NetDeviceContainer p2pDevice;
    p2pDevice = p2p.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    p2pDevice.Assign(p2pDevice);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    //creating apps, need implement

    Simulator::Stop (Seconds (5.0));
	Simulator::Run ();
}
