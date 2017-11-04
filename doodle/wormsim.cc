#include <iostream>
#include <stdio.h>
#include <string>
#include <sys/time.h>
#include <vector>
#include <time.h>
#include <iomanip>
#include <assert.h>

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
#include "p2pCampusHelper.h"

// ------------ Define worm types    ---------------
#define TCPWORMTYPE  1
#define UDPWORMTYPE  2
#define WORMTYPE     UDPWORMTYPE

// ------------ Worm parameters -----------------------
#define VULNERABILITY  1
#define SCANRATE       100
#define SCANRANGE      0
#define PAYLOAD        1000

// ----------- Simulation settings -------------------
#define SIMTIME        2.0
#define SEEDVALUE      1

using namespace ns3;
using namespace std;

int main(int argc, char* argv[]){
  uint32_t wormtype = WORMTYPE;
  uint32_t scanrate = SCANRATE;
  uint32_t payload = PAYLOAD;
  uint32_t seedValue = SEEDVALUE;
  double vulnerability = VULNERABILITY;
  double simtime = SIMTIME;
  std::string dataFileName = "p4.data";

  CommandLine cmd;
  cmd.AddValue ("wormtype",      "Type of worm: UDP or TCP",     wormtype);
  cmd.AddValue ("scanrate",      "Scan rate",                    scanrate);
  cmd.AddValue ("payload",       "Payload",                      payload);
  cmd.AddValue ("seedvalue",     "Seed value for RNG",           seedValue);
  cmd.AddValue ("vulnerability", "Vulnerability to infection",   vulnerability);
  cmd.AddValue ("simtime",       "Simulator time in seconds",    simtime);
  cmd.AddValue ("filename",      "Name of output file",          dataFileName);

  cmd.Parse (argc,argv);

  // Set the random number generator
  SeedManager::SetSeed (seedValue);
  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));

  uint32_t nInner = 8;
  uint32_t nChild = 2;

  PointToPointHelper hubInner;
  hubInner.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
  hubInner.SetChannelAttribute("Delay", StringValue("5ms"));

  PointToPointHelper innerChild;
  innerChild.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
  innerChild.SetChannelAttribute("Delay", StringValue("8ms"));

  PointToPointCampusHelper bomb(nInner, hubInner, nChild, innerChild);
  InternetStackHelper stack;
  bomb.InstallStack(stack);
  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  bomb.AssignIpv4Addresses(address);

  ApplicationContainer wormApps;
  Worm::SetPacketSize(payload);
  uint32_t numVulnerableNodes = 0;

  for(uint32_t i=0; i < nChild * nInner; i++){
    Ptr<Worm> wormApp = CreateObject<Worm> ();
    //wormApp->SetMaxBytes(50000);

    if (uv->GetValue(0.0, 1.0) <= vulnerability) {
      wormApp->SetVulnerable (true);
      numVulnerableNodes++;
    }

    wormApp->SetName(std::to_string(i));
    // First node to send worm
    if(i == 0) wormApp -> SetInfected (true);

    wormApp->SetStartTime (Seconds (0.0));
    wormApp->SetStopTime (Seconds (simtime));

    bomb.GetChildNode(i)->AddApplication (wormApp);
    wormApp->SetUp ("ns3::UdpSocketFactory", 5000);
  }
    Worm::SetExistNodes(numVulnerableNodes);

  for (int i = 0; i < 5000; ++i) {
      ns3::Simulator::Schedule(ns3::Seconds((double)i*.1), &Worm::SetNumInfected);
  }

  // Populate routing tables.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  if (simtime != 0) Simulator::Stop(Seconds(simtime));
  Simulator::Run();

  // Print results
  double percInfected = 100.*(double)Worm::GetInfectedNodes() / (double)(nChild * nInner);
  double percVulnerable = 100.*(double)numVulnerableNodes/(double)(nChild * nInner);
  double percInfToVuln = percInfected / percVulnerable;
  cerr << "Time(s)\tInf(#)\tTot(#)\tConn(#)\tPerc(%)\tVuln(%)\tInf/Vul(%)" << std::endl;
  cerr << setprecision(3) << Simulator::Now().GetSeconds() << "\t"
       << Worm::GetInfectedNodes() << "\t"
       << nChild * nInner << "\t"
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
           << payload << "\t"
           << "\n";
  for (size_t i = 0; i < infectionArray.size(); ++i) {
      dataFile << (float)i*0.1 << "\t"
               << infectionArray.at(i) << "\t"
               << "\n";
  }
  dataFile  << "\n";
  dataFile.close();

  Simulator::Destroy();
}
