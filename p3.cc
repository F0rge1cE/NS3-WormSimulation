#include <iostream>

#include "p2pCampusHelper.h"

int main(int argc, char** argv) {
	
	SeedManager::SetSeed(1);
	uint32_t nInner = 8;
	uint32_t nChild = 2;
    double endTime = 60.0;

    CommandLine cmd;
  	cmd.AddValue("nInner", "Number of inner nodes in each bomb", nInner);
  	cmd.AddValue("nChild", "Number of child nodes in each bomb", nChild);
  	cmd.Parse(argc, argv);

  	PointToPointHelper hubInner;
    hubInner.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    hubInner.SetChannelAttribute("Delay", StringValue("5ms"));
	
	PointToPointHelper innerChild;
	innerChild.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
	innerChild.SetChannelAttribute("Delay", StringValue("8ms"));

    p2pCampusHelper bomb(nInner, hubInner, nChild, innerChild);
    InternetStackHelper stack;
    bomb.InstallStack(stack);

    bool IsValidInnerNode [nInner];
    for(uint32_t i = 0; i < nInner; ++i) IsValidInnerNode[i] = true;
    bool IsValidChildNode [nInner * nChild];
	for(uint32_t i = 0; i < nInner * nChild; ++i) IsValidChildNode = true;

	Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    bomb.AssignIpv4Addresses(address);
	std::cout << "Hello" << std::endl; 

}
