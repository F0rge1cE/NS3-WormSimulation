#include "worm.h"
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "p2pCampusHelper.h"

NS_LOG_COMPONENT_DEFINE ("ns3-worm");

uint32_t Worm::m_totalInfected = 1;
uint32_t Worm::m_existNodes = 0;
uint32_t Worm::m_totalNodes = 0;
uint32_t Worm::m_numConn = 1;
uint32_t Worm::m_pktSize = 512;
double Worm::m_chooseLocal = 0.65;
uint32_t Worm::m_addressForPattern3 = 0;
std::vector<int> Worm::m_curInfected;

ns3::TypeId Worm::GetTypeId(void)
{
  static ns3::TypeId tid = ns3::TypeId ("ns3::Worm")
    .SetParent<ns3::Application> ()
    .AddConstructor<Worm> ()
    .AddAttribute ("DataRate", "The data rate in on state.",
                   ns3::DataRateValue (ns3::DataRate ("500kb/s")),
                   ns3::MakeDataRateAccessor (&Worm::m_cbrRate),
                   ns3::MakeDataRateChecker ())
//    .AddAttribute ("PacketSize", "The size of packets sent in on state",
//                   ns3::UintegerValue (512),
//                   ns3::MakeUintegerAccessor (&Worm::m_pktSize),
//                   ns3::MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("OnTime", "A RandomVariableStream used to pick the duration of the 'On' state.",
                   ns3::StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"),
                   ns3::MakePointerAccessor (&Worm::m_onTime),
                   ns3::MakePointerChecker <ns3::RandomVariableStream>())
    .AddAttribute ("OffTime", "A RandomVariableStream used to pick the duration of the 'Off' state.",
                   ns3::StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"),
                   ns3::MakePointerAccessor (&Worm::m_offTime),
                   ns3::MakePointerChecker <ns3::RandomVariableStream>())
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. Once these bytes are sent, "
                   "no packet is sent again, even in on state. The value zero means "
                   "that there is no limit.",
                   ns3::UintegerValue (50000),
                   ns3::MakeUintegerAccessor (&Worm::m_maxBytes),
                   ns3::MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   ns3::TypeIdValue (ns3::UdpSocketFactory::GetTypeId ()),
                   ns3::MakeTypeIdAccessor (&Worm::m_typeId),
                   ns3::MakeTypeIdChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     ns3::MakeTraceSourceAccessor (&Worm::m_txTrace));
  return tid;
}


Worm::Worm(){
	m_infected = false;
	m_connected = false;
    m_exist = false;
}

Worm::~Worm(){}

void Worm::SetInfected(bool isInfected){
  m_infected = isInfected;
}

void Worm::SetPacketSize(uint32_t pktSize){
  m_pktSize = pktSize;
}

void Warm::SetExist(bool nodeExist){
  m_exist = nodeExist;
}

void SetUp(std::string protocol, uint32_t infectionPort, uint16_t subnetId){
	m_protocol = protocol;
	m_infectionPort = infectionPort;
  m_subnetId = subnetId;
	m_typeId = ns3::TypeId::LookupByName(m_protocol);

	// Sink socket
	m_sinkSocket = ns3::Socket::CreateSocket(GetNode(), m_typeId);
	NS_ASSERT(m_sinkSocket != 0);
	ns3::InetSocketAddress local = ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(), m_infectionPort);
    m_sinkSocket->Bind(local);
    m_sinkSocket->Listen();
    m_sinkSocket->SetRecvCallback(MakeCallback(&Worm::Listen, this));
    
    // onoff socket vector
    m_onoffSocket = std::vector< ns3::Ptr<ns3::Socket> > (m_numConn);                                                   
}

ns3::Ipv4Address Worm::generateIP(uint32_t patternId){
	// TODO: use diff scan pattern to generate target ips;
  //patternId: different scanning pattern ID
  //0: Unifformly Random Scanning
  //1：Local Prefernce Scanning
  //2: Sequential Scanning
  if(patternId == 0){
    // double min = 1.0;
    // double max = 8.99;
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();

    double i = uv->GetValue(1.0, 5.0);
    double j = uv->GetValue(1.0, 9.0);
    double k = uv->GetValue(1.0, 3.0);

    char buff[13];
    sprintf(buff, "10.%d.%d.%d", (uint32_t)i, (uint32_t)j, ((uint32_t)(k) * 2 + 1));
    ns3::Ipv4Address address = ns3::Ipv4Address(buff);
    return address;
  }
  
  elseif(patternId == 1){
    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
    double i = (double) m_subnetId;
    if(uv->GetValue(0.0, 1.0) > m_chooseLocal){
      while((uint32_t) i == m_subnetId){
        i = uv->GetValue(1.0, 5.0);
      }
    }
    double j = uv->GetValue(1.0, 9.0);
    double k = uv->GetValue(1.0, 3.0);

    char buff[13];
    sprintf(buff, "10.%d.%d.%d", (uint32_t)i, (uint32_t)j, ((uint32_t)(k) * 2 + 1));
    ns3::Ipv4Address address = ns3::Ipv4Address(buff);
    return address;
  }

  elseif(patternId == 2){
  	uint32_t i = m_addressForPattern3 / 16 + 1;
  	uint32_t j = m_addressForPattern3 % 16 / 2 + 1;
  	uint32_t k = m_addressForPattern3 % 16 % 2 + 1;
  	m_addressForPattern3++；
  	char buff[13];
    sprintf(buff, "10.%d.%d.%d", (uint32_t)i, (uint32_t)j, ((uint32_t)(k) * 2 + 1));
    ns3::Ipv4Address address = ns3::Ipv4Address(buff);
    return address;
  }
	return ns3::Ipv4Address::GetAny();
}
void Worm::Write32 (uint8_t *buffer, const uint32_t data){
  buffer[0] = (data >> 0) & 0xff;
  buffer[1] = (data >> 8) & 0xff;
  buffer[2] = (data >> 16) & 0xff;
  buffer[3] = (data >> 24) & 0xff;
}

void Worm::Read32 (const uint8_t *buffer, uint32_t &data){
  data = (buffer[3] << 24) + (buffer[2] << 16) + (buffer[1] << 8) + buffer[0];
}

void Worm::SendPacket(uint32_t index){
	uint8_t* data = new uint8_t[m_pktSize];
	for(uint32_t i = 0; i<m_pktSize; i++) data[i] = 0;

	uint32_t temp = m_infectionPort;
	Write32(&data[0], temp);

	ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet> ((uint8_t*) data, m_pktSize);
	m_onoffSocket[index]->SendTo(packet, 0, ns3::InetSocketAddress(generateIP()), m_infectionPort + index);
	m_totalBytes += m_pktSize;
	m_lastStartTime = ns3::Simulator::Now();
	ScheduleNextTx(index);
}

void Worm::ConnectionSucceeded(ns3::Ptr<ns3::Socket> socket){
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void Worm::ConnectionFailed(ns3::Ptr<ns3::Socket> socket){
  NS_LOG_FUNCTION (this << socket);
}

void Worm::ScheduleStartEvent (uint32_t index){  // Schedules the event to start sending data (switch to the "On" state)
  //ns3::Time offInterval = ns3::Seconds (m_offTime->GetValue ());
  //NS_LOG_LOGIC ("start at " << offInterval);
  ns3::Simulator::ScheduleNow(&Worm::StartSending, this, index);
}

void Worm::ScheduleStopEvent (){  // Schedules the event to stop sending data (switch to "Off" state)
  ns3::Time onInterval = ns3::Seconds (m_onTime->GetValue ());
  // NS_LOG_LOGIC ("stop at " << onInterval);
  ns3::Simulator::Schedule (onInterval, &Worm::StopSending, this);
}

// Event handlers
void Worm::StartSending (uint32_t index){
  NS_LOG_FUNCTION (this);
  m_lastStartTime = ns3::Simulator::Now ();
  ScheduleNextTx (index);  // Schedule the send packet event
  //ScheduleStopEvent ();
}

void Worm::ScheduleNextTx(uint32_t index){
	NS_LOG_FUNCTION(this);
	if(m_maxByte == 0 || m_totalBytes < m_maxByte){
		uint32_t bits = m_pktSize * 8;
		// How to compute next time?
		ns3::Time nextTime (ns3::Seconds (bits / static_cast<double>(m_cbrRate.GetBitRate ()))); // Time till next packet
		m_sendEvent = ns3::Simulator::Schedule(nextTime, &Worm::SendPacket, this, index);
	}
}

void Worm::StopSending ()
{
  CancelEvents ();
  //ScheduleStartEvent ();
}

// Application Methods
void Worm::StartApplication(){ 
	// Called at time specified by Start
  if(m_exist){
    if (m_infected) {
      StartInfectingNodes();
    }
  }
	
}

void Worm::Listen(ns3::Ptr<ns3::Socket> socket)
{
  while (socket->GetRxAvailable () > 0){
      ns3::Address from;
      ns3::Ptr<ns3::Packet> p = socket->RecvFrom (0xffffffff, 0, from);
      NS_ASSERT (ns3::InetSocketAddress::IsMatchingType (from));

      uint32_t recvSize = p->GetSize ();
      uint32_t *buffer = new uint32_t [recvSize];
      p->CopyData((uint8_t *)buffer, recvSize);
      uint32_t pktMsg;

      Read32 ((const uint8_t *) &buffer[0], pktMsg);

      for (uint32_t i=0; i < m_numConn; ++i)
        {
          if ((((uint32_t)m_infectionPort + i) == pktMsg) && (!m_infected))
            {
              m_infected = true;
              m_totalInfected++;
              std::cerr << m_totalInfected << " This worked! sort of" << std::endl;

              if (m_totalInfected >= m_existNodes) ns3::Simulator::Stop();

              StartInfectingNodes();
            }
        }
    }
}

void Worm::StartInfectingNodes(){
  for (uint32_t i = 0; i < m_numConn; ++i){
      // OnOff socket
      m_onoffSocket[i] = ns3::Socket::CreateSocket (GetNode (), m_typeId);
      NS_ASSERT (m_onoffSocket[i] != 0);
      m_onoffSocket[i]->Bind();

      m_onoffSocket[i]->SetAllowBroadcast(false);
      m_onoffSocket[i]->SetConnectCallback(ns3::MakeCallback(&Worm::ConnectionSucceeded, this),
                                        ns3::MakeCallback(&Worm::ConnectionFailed, this));

      m_cbrRateFailSafe = m_cbrRate;

      //CancelEvents ();

      ScheduleStartEvent(i);
    }

    ns3::Simulator::Schedule(ns3::Seconds(2.0),&Worm::StartInfectingNodes, this);
}

void Worm::CancelEvents (){
  ns3::Simulator::Cancel (m_sendEvent);
  ns3::Simulator::Cancel (m_startStopEvent);
}

void Worm::StopApplication(){
  // Called at time specified by Stop
  if (m_sinkSocket != 0) {
      m_sinkSocket->Close();
  }
  for (uint32_t i=0; i<m_numConn; ++i) {
      if (m_onoffSocket[i] != 0) {
          m_onoffSocket[i]->Close();
      }
  }
  ns3::Simulator::Schedule(ns3::Seconds(2.0), &Worm::CloseAndPrint, this);
}

void Worm::CloseAndPrint(){
	// TODO: print all data here.
	std::cerr << " State at time " << ns3::Simulator::Now() << ":\n"
            << "\tInfected: " << (m_infected ? "True\n" : "False\n")
            << "\tVulnerable: " << (m_vulnerable ? "True\n" : "False\n")
            << std::endl;
}
