/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * Author: Aderinola Gbade-Alabi <aagbade@gmail.com>
 *         Jared S. Ivey         <jivey@gatech.edu>
 *         Drew Petry            <drew.petry@gatech.edu>
 *         Peter Vieira          <pete.vieira@gmail.com>
 *
 */

#include <iostream>
#include <stdio.h>
#include <string>
#include <sys/time.h>
#include <vector>
#include <time.h>
#include <iomanip>
#include <assert.h>

//#include "ns3/random-variable-stream.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/stats-module.h"

#include "ns3-worm.h"

//what I added//////////////////////////////////////////////////
#include "p2pCampusHelper.h"
//////////////////////////////////////////////////////////////////

// ****** For MPI
#include "ns3/mpi-interface.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/ipv4-address-helper.h"
#ifdef NS3_MPI
#include <mpi.h>
#endif
// ******


// ------------ Define worm types    ---------------
#define TCPWORMTYPE  1
#define UDPWORMTYPE  2
#define WORMTYPE     UDPWORMTYPE

// ------------ Define the topology  ---------------
#define TREES        4
#define FANOUT1      8
#define FANOUT2      16
#define TREELEGPROB  0.85

#define LINKBW       "1Mbps"
#define HLINKBW      "10Mbps"
#define BLINKBW      "100Mbps"

// ------------ Worm parameters -----------------------
#define VULNERABILITY  1.0
#define SCANRATE       100
#define SCANRANGE      0
#define PAYLOAD        1000
#define NUMCONN        1

// ----------- Simulation settings -------------------
#define SIMTIME        8
#define SEEDVALUE      1

// ****** For MPI
// ----------- MPI settings -------------------
#define NIX true
#define NULLMSG false
#define TRACING false
// ******

//
#define PATTERNID 2

using namespace ns3;
using namespace std;

int main(int argc, char* argv[])
{
// ****** For MPI
#ifdef NS3_MPI
// ******

  uint32_t wormtype = WORMTYPE;
  uint32_t nt = TREES;
  uint32_t nf1 = FANOUT1;
  uint32_t nf2 = FANOUT2;
  string linkbw  = LINKBW;
  string hlinkbw = HLINKBW;
  string blinkbw = BLINKBW;
  uint32_t scanrate = SCANRATE;
  uint32_t payload = PAYLOAD;
  uint32_t seedValue = SEEDVALUE;
  uint32_t numConn = NUMCONN;
  double vulnerability = VULNERABILITY;
  double treelegprob = TREELEGPROB;
  double simtime = SIMTIME;
  bool logTop = 0;
  std::string dataFileName = "p4.data";

  // ****** For MPI
  // Default configuration
  bool nix = NIX;
  bool nullmsg = NULLMSG;
  bool tracing = TRACING;
  // ******

  CommandLine cmd;
  cmd.AddValue ("wormtype",      "Type of worm: UDP or TCP",     wormtype);
  cmd.AddValue ("trees",         "Number of trees",              nt);
  cmd.AddValue ("fanout1",       "First fanout of trees",        nf1);
  cmd.AddValue ("fanout2",       "Second fanout of trees",       nf2);
  cmd.AddValue ("linkbw",        "Link bandwidth",               linkbw);
  cmd.AddValue ("hlinkbw",       "HLink bandwidth",              hlinkbw);
  cmd.AddValue ("blinkbw",       "BLink bandwidth",              blinkbw);
  cmd.AddValue ("scanrate",      "Scan rate",                    scanrate);
  cmd.AddValue ("payload",       "Payload",                      payload);
  cmd.AddValue ("seedvalue",     "Seed value for RNG",           seedValue);
  cmd.AddValue ("vulnerability", "Vulnerability to infection",   vulnerability);
  cmd.AddValue ("numConn",       "Number of TCP connections",    numConn);
  cmd.AddValue ("treelegprob",   "Probability of tree legs",     treelegprob);
  cmd.AddValue ("simtime",       "Simulator time in seconds",    simtime);
  cmd.AddValue ("logTop",        "Display the topology stats",   logTop);
  cmd.AddValue ("filename",      "Name of output file",          dataFileName);

  // ****** For MPI
  cmd.AddValue ("nix", "Enable the use of nix-vector or global routing", nix);
  cmd.AddValue ("nullmsg", "Enable the use of null-message synchronization", nullmsg);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  // ******

  cmd.Parse (argc,argv);

  // Set the random number generator
  SeedManager::SetSeed (seedValue);
  //SeedManager::SetSeed ((int) clock());
  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));

  // ****** For MPI
  // Distributed simulation setup; by default use granted time window algorithm.
  if(nullmsg)
    {
      GlobalValue::Bind ("SimulatorImplementationType",
                         StringValue ("ns3::NullMessageSimulatorImpl"));
    }
  else
    {
      GlobalValue::Bind ("SimulatorImplementationType",
                         StringValue ("ns3::DistributedSimulatorImpl"));
    }

  // Enable parallel simulator with the command line arguments
  MpiInterface::Enable (&argc, &argv);

  // LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);

  // Get rank and total number of CPUs.
  uint32_t systemId = MpiInterface::GetSystemId ();
  uint32_t systemCount = MpiInterface::GetSize ();

  // We only do simulation with 4 processors.
  if (systemCount != 1 && systemCount != 2 && systemCount != 4)
    {
      std::cout << "Only 1, 2 or 4 processors are accepted. Now have " << systemCount << std::endl;
      return 1;
    }
  // ******



  //what I added////////////////////////////////////////////////
  uint32_t nInner = 8;
  uint32_t nChild = 2;

  PointToPointHelper hubInner;
  hubInner.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
  hubInner.SetChannelAttribute("Delay", StringValue("5ms"));

  PointToPointHelper innerChild;
  innerChild.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
  innerChild.SetChannelAttribute("Delay", StringValue("8ms"));

  // ****** For MPI
  // P2P between hubs
  PointToPointHelper hub2hub_10ms;
  hub2hub_10ms.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
  hub2hub_10ms.SetChannelAttribute("Delay", StringValue("10ms"));

  PointToPointHelper hub2hub_100ms;
  hub2hub_100ms.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
  hub2hub_100ms.SetChannelAttribute("Delay", StringValue("100ms"));

  PointToPointHelper hub2hub_500ms;
  hub2hub_500ms.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
  hub2hub_500ms.SetChannelAttribute("Delay", StringValue("500ms"));

  PointToPointHelper hub2hub_200ms;
  hub2hub_500ms.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
  hub2hub_500ms.SetChannelAttribute("Delay", StringValue("200ms"));

  // Create nodes
  PointToPointCampusHelper bomb0(nInner, hubInner, nChild, innerChild, 0%systemCount);
  PointToPointCampusHelper bomb1(nInner, hubInner, nChild, innerChild, 1%systemCount);
  PointToPointCampusHelper bomb2(nInner, hubInner, nChild, innerChild, 2%systemCount);
  PointToPointCampusHelper bomb3(nInner, hubInner, nChild, innerChild, 3%systemCount);

  NetDeviceContainer hubDevice;
  NetDeviceContainer hub2hub_dev1 = hub2hub_10ms.Install (bomb0.GetHub(), bomb1.GetHub());
  NetDeviceContainer hub2hub_dev2 = hub2hub_100ms.Install (bomb1.GetHub(), bomb2.GetHub());
  NetDeviceContainer hub2hub_dev3 = hub2hub_500ms.Install (bomb2.GetHub(), bomb3.GetHub());
  NetDeviceContainer hub2hub_dev4 = hub2hub_200ms.Install (bomb3.GetHub(), bomb0.GetHub());

  InternetStackHelper stack;

  // Apply Nix Vector
  if (nix)
    {
      std::cout << "Nix Vector Enabled " << std::endl;
      Ipv4NixVectorHelper nixRouting;
      stack.SetRoutingHelper (nixRouting); // has effect on the next Install ()
    }

  // stack.InstallAll ();

  bomb0.InstallStack(stack);
  bomb1.InstallStack(stack);
  bomb2.InstallStack(stack);
  bomb3.InstallStack(stack);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  bomb0.AssignIpv4Addresses(address);

  // bomb1.InstallStack(stack);
  address.SetBase("10.2.1.0", "255.255.255.0");
  bomb1.AssignIpv4Addresses(address);

  // bomb2.InstallStack(stack);
  address.SetBase("10.3.1.0", "255.255.255.0");
  bomb2.AssignIpv4Addresses(address);

  // bomb3.InstallStack(stack);
  address.SetBase("10.4.1.0", "255.255.255.0");
  bomb3.AssignIpv4Addresses(address);

  address.SetBase("11.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer hub2hub_inter1 = address.Assign(hub2hub_dev1);

  address.SetBase("12.4.1.0", "255.255.255.0");
  Ipv4InterfaceContainer hub2hub_inter2 = address.Assign(hub2hub_dev2);

  address.SetBase("13.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer hub2hub_inter3 = address.Assign(hub2hub_dev3);

  address.SetBase("14.4.1.0", "255.255.255.0");
  Ipv4InterfaceContainer hub2hub_inter4 = address.Assign(hub2hub_dev4);

  // ApplicationContainer wormApps; // ???
  // Worm::SetX (1 + nInner);
  // Worm::SetY (nInner * nChild);
  // Worm::SetTotalNodes (nInner * nChild);
  //Worm::SetNumConn(numConn);
  Worm::SetPacketSize(payload);
  uint32_t numVulnerableNodes = 0;

  // Add the worm application to each node.
  if(systemId == 0%systemCount){
    for(uint32_t i=0; i < nChild * nInner; i++)
    {
      Ptr<Worm> wormApp = CreateObject<Worm> ();
      //wormApp->SetMaxBytes(50000);

      if (uv->GetValue(0.0, 1.0) <= vulnerability) {
        wormApp->SetVulnerable (true);
        numVulnerableNodes++;
      }

      wormApp->SetName(std::to_string(i));

      // Set the initial infected node.
      if(i==0){
        wormApp -> SetInfected (true);
      }

      wormApp->SetStartTime (Seconds (0.0));
      wormApp->SetStopTime (Seconds (simtime));
      wormApp->SetPatternId (PATTERNID);

      bomb0.GetChildNode(i)->AddApplication (wormApp);
      wormApp->SetUp ("ns3::UdpSocketFactory", 5000, systemId);
    }
  }

  if(systemId == 1%systemCount){
    for(uint32_t i=0; i < nChild * nInner; i++)
    {
      Ptr<Worm> wormApp = CreateObject<Worm> ();
      //wormApp->SetMaxBytes(50000);

      if (uv->GetValue(0.0, 1.0) <= vulnerability) {
        wormApp->SetVulnerable (true);
        numVulnerableNodes++;
      }

      wormApp->SetName(std::to_string(i));


      wormApp->SetStartTime (Seconds (0.0));
      wormApp->SetStopTime (Seconds (simtime));

      bomb1.GetChildNode(i)->AddApplication (wormApp);
      wormApp->SetUp ("ns3::UdpSocketFactory", 5000, systemId);
    }
  }

  if(systemId == 2%systemCount){
    for(uint32_t i=0; i < nChild * nInner; i++){
      Ptr<Worm> wormApp = CreateObject<Worm> ();
      //wormApp->SetMaxBytes(50000);

      if (uv->GetValue(0.0, 1.0) <= vulnerability) {
        wormApp->SetVulnerable (true);
        numVulnerableNodes++;
      }

      wormApp->SetName(std::to_string(i));


      wormApp->SetStartTime (Seconds (0.0));
      wormApp->SetStopTime (Seconds (simtime));

      bomb2.GetChildNode(i)->AddApplication (wormApp);
      wormApp->SetUp ("ns3::UdpSocketFactory", 5000, systemId);
    }
  }

  if(systemId == 3%systemCount){
    for(uint32_t i=0; i < nChild * nInner; i++){
      Ptr<Worm> wormApp = CreateObject<Worm> ();
      //wormApp->SetMaxBytes(50000);

      if (uv->GetValue(0.0, 1.0) <= vulnerability) {
        wormApp->SetVulnerable (true);
        numVulnerableNodes++;
      }

      wormApp->SetName(std::to_string(i));


      wormApp->SetStartTime (Seconds (0.0));
      wormApp->SetStopTime (Seconds (simtime));

      bomb3.GetChildNode(i)->AddApplication (wormApp);
      wormApp->SetUp ("ns3::UdpSocketFactory", 5000, systemId);
    }
  }
    Worm::SetExistNodes(numVulnerableNodes);

/////////////////////////////////////////////////////////////////////

  for (int i = 0; i < 5000; ++i) {
      ns3::Simulator::Schedule(ns3::Seconds((double)i*.1), &Worm::SetNumInfected);
  }

  // Populate routing tables.
  if (!nix)
  {
    std::cout << "Using IPv4 Routing!" << std::endl;
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  }
  // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  if (simtime != 0)
    Simulator::Stop(Seconds(simtime));
  Simulator::Run();

  double percInfected = 100.*(double)Worm::GetInfectedNodes() / (double)(nChild * nInner);
  double percVulnerable = 100.*(double)numVulnerableNodes/(double)(nChild * nInner);
  double percInfToVuln = percInfected / percVulnerable;
  cerr << "Time(s)\tInf(#)\tTot(#)\tConn(#)\tPerc(%)\tVuln(%)\tInf/Vul(%)" << std::endl;
  cerr << setprecision(3) << Simulator::Now().GetSeconds() << "\t"
       << Worm::GetInfectedNodes() << "\t"
       << nChild * nInner << "\t"
       << Worm::GetNumConn() << "\t"
       << setprecision(4) << percInfected << "\t"
       << setprecision(4) << percVulnerable << "\t"
       << setprecision(4) << percInfToVuln*100. << "\t"
       << std::endl;


  std::vector<int> infectionArray = Worm::GetInfectionArray();
  for (int i = 0; i < 5; ++i) {
    infectionArray.push_back(Worm::GetInfectedNodes());
  }

  //-------------------------
  //    WRITE DATA TO FILE
  //-------------------------
  //"#Nodes %Vul %Inf #inf #Conn Time"
  // Write results to data file
  std::ofstream dataFile;
  dataFile.open(dataFileName.c_str(), std::fstream::out | std::fstream::app);
  assert(dataFile.is_open());
  dataFile << "0.0" << "\t"
           << Worm::GetInfectedNodes() << "\t"
           << nChild * nInner << "\t"
           << percVulnerable << "\t"
           << percInfected << "\t"
           << Worm::GetNumConn() << "\t"
           << payload << "\t"
           << "\n";
  for (size_t i = 0; i < infectionArray.size(); ++i) {
      dataFile << (float)i*0.1 << "\t"
               << infectionArray.at(i) << "\t"
               << "\n";
  }
  dataFile  << "\n";
  dataFile.close();
  //flowmon->SerializeToXmlFile ("p4.flowmon", false, false);
  Simulator::Destroy();

  // ****** For MPI
  // Exit the MPI execution environment
  MpiInterface::Disable ();
  return 0;

#else
  NS_FATAL_ERROR ("Can't use distributed simulator without MPI compiled in");
#endif
// ******

}
