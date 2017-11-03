
#include "ns3-worm.h"
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

NS_LOG_COMPONENT_DEFINE ("ns3-worm");

ns3::Ptr<ns3::UniformRandomVariable> Worm::tx = ns3::CreateObject<ns3::UniformRandomVariable> ();
ns3::Ptr<ns3::UniformRandomVariable> Worm::ty = ns3::CreateObject<ns3::UniformRandomVariable> ();

uint32_t Worm::m_totalInfected = 1;
uint32_t Worm::m_existNodes = 1;

uint32_t Worm::m_pktSize = 512;
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
    .AddAttribute ("OnTime", "A RandomVariableStream used to pick the duration of the 'On' state.",
                   ns3::StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"),
                   ns3::MakePointerAccessor (&Worm::m_onTime),
                   ns3::MakePointerChecker <ns3::RandomVariableStream>())
    .AddAttribute ("OffTime", "A RandomVariableStream used to pick the duration of the 'Off' state.",
                   ns3::StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"),
                   ns3::MakePointerAccessor (&Worm::m_offTime),
                   ns3::MakePointerChecker <ns3::RandomVariableStream>())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     ns3::MakeTraceSourceAccessor (&Worm::m_txTrace));
  return tid;
}

// Constructors

Worm::Worm()
  : m_infected(false),
    m_vulnerable(true),
    m_residualBits(0),
    m_totalBytes(0),
    m_lastStartTime(ns3::Seconds(0)),
    m_sinkSocket(0)
{}

Worm::~Worm(){}

void Worm::SetInfected(bool alreadyInfected){
  m_infected = alreadyInfected;
}

void Worm::SetVulnerable(bool vulnerable){
  m_vulnerable = vulnerable;
}

void Worm::SetMaxBytes(uint32_t maxBytes){
  m_maxBytes = maxBytes;
}

void Worm::SetPacketSize(uint32_t pktSize){
  m_pktSize = pktSize;
}

void Worm::SetExistNodes (uint32_t existNodes){
  m_existNodes = existNodes;
}

uint32_t Worm::GetInfectedNodes (){
  return m_totalInfected;
}

void Worm::SetUp(std::string protocol, uint32_t infectionPort){
  m_protocol = protocol;
  m_infectionPort = infectionPort;
  m_typeId = ns3::TypeId::LookupByName(m_protocol);

  // Sink socket
  m_sinkSocket = ns3::Socket::CreateSocket (GetNode (), m_typeId);
  NS_ASSERT (m_sinkSocket != 0);
  ns3::InetSocketAddress local = ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(),
                                                        m_infectionPort);
  m_sinkSocket->Bind(local);
  m_sinkSocket->Listen();
  m_sinkSocket->SetRecvCallback (MakeCallback (&Worm::Listen, this));

  // Initialize number of simultaneous connections
  m_onoffSocket = std::vector< ns3::Ptr<ns3::Socket> > (1);
}

ns3::Ipv4Address Worm::guessIP(){
  uint32_t first, second;
  char buff[13];

  first = Worm::tx->GetInteger(1,32);
  second = Worm::ty->GetInteger(1,256);
  sprintf(buff, "192.%d.%d.2",first,second);

  ns3::Ipv4Address address = ns3::Ipv4Address(buff);
  return address;
}

void Worm::Write32 (uint8_t *buffer, const uint32_t data){
  NS_LOG_FUNCTION (this << buffer << data);
  buffer[0] = (data >> 0) & 0xff;
  buffer[1] = (data >> 8) & 0xff;
  buffer[2] = (data >> 16) & 0xff;
  buffer[3] = (data >> 24) & 0xff;
}

void Worm::Read32 (const uint8_t *buffer, uint32_t &data){
  NS_LOG_FUNCTION (this << buffer << data);
  data = (buffer[3] << 24) + (buffer[2] << 16) + (buffer[1] << 8) + buffer[0];
}

void Worm::SendPacket(uint32_t index){
  uint8_t* data = new uint8_t[m_pktSize];
  for (uint32_t i = 0; i < m_pktSize; ++i) data[i] = 0;

  uint32_t tmp = m_infectionPort;
  Write32 (&data[0 * sizeof(uint32_t)], tmp);

  ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet> ((uint8_t *) data, m_pktSize);

  m_onoffSocket[index]->SendTo(packet, 0,
                               ns3::InetSocketAddress(guessIP(), m_infectionPort+index));

  //std::cerr << "Test: " << testVal << std::endl;
  m_totalBytes += m_pktSize;
  m_lastStartTime = ns3::Simulator::Now();
  m_residualBits = 0;
  ScheduleNextTx (index);
}

void Worm::ScheduleStartEvent (uint32_t index){ 
 // Schedules the event to start sending data (switch to the "On" state)
  ns3::Time offInterval = ns3::Seconds (m_offTime->GetValue ());
  NS_LOG_LOGIC ("start at " << offInterval);
  ns3::Simulator::ScheduleNow(&Worm::StartSending, this, index);
}

void Worm::ScheduleStopEvent (){  
// Schedules the event to stop sending data (switch to "Off" state)
  ns3::Time onInterval = ns3::Seconds (m_onTime->GetValue ());
  NS_LOG_LOGIC ("stop at " << onInterval);
  ns3::Simulator::Schedule (onInterval, &Worm::StopSending, this);
}

// Event handlers
void Worm::StartSending (uint32_t index){
  m_lastStartTime = ns3::Simulator::Now ();
  ScheduleNextTx (index);  // Schedule the send packet event
}

// Private helpers
void Worm::ScheduleNextTx (uint32_t index){
      uint32_t bits = m_pktSize * 8 - m_residualBits;
      NS_LOG_LOGIC ("bits = " << bits);
      ns3::Time nextTime (ns3::Seconds (bits /
                          static_cast<double>(m_cbrRate.GetBitRate ()))); // Time till next packet
      NS_LOG_LOGIC ("nextTime = " << nextTime);
      m_sendEvent = ns3::Simulator::Schedule (nextTime,
                                         &Worm::SendPacket, this, index);
}

void Worm::StopSending (){
  NS_LOG_FUNCTION (this);
  CancelEvents ();
}

void Worm::SetNumInfected(){
  m_curInfected.push_back(Worm::GetInfectedNodes());
}

std::vector<int> Worm::GetInfectionArray(){
    return m_curInfected;
}

// Application Methods
void Worm::StartApplication()    // Called at time specified by Start
{
  // If we're vulnerable, if already infected, start infected other nodes
  // else listen for packets on infection port
  if (m_vulnerable) {
      if (m_infected) {
          StartInfectingNodes();
      }
  }
}

void Worm::Listen(ns3::Ptr<ns3::Socket> socket){
  while (socket->GetRxAvailable () > 0)
    {
      ns3::Address from;
      ns3::Ptr<ns3::Packet> p = socket->RecvFrom (0xffffffff, 0, from);
      NS_ASSERT (ns3::InetSocketAddress::IsMatchingType (from));

      uint32_t recvSize = p->GetSize ();
      uint32_t *buffer = new uint32_t [recvSize];
      p->CopyData((uint8_t *)buffer, recvSize);
      uint32_t pktMsg;

      Read32 ((const uint8_t *) &buffer[0], pktMsg);
      //std::cerr << "[" << m_name << "] Received one packet!" << std::endl;
//      for (uint32_t i=0; i < 1; ++i)
//        {
          if ((((uint32_t)m_infectionPort) == pktMsg) && (!m_infected))
            {
              m_infected = true;
              m_totalInfected++;
              std::cerr << m_totalInfected << " This worked! sort of" << std::endl;

              if (m_totalInfected >= m_existNodes) ns3::Simulator::Stop();

              StartInfectingNodes();
            }
//        }
    }
}


void Worm::StartInfectingNodes(){
//  for (uint32_t i=0; i<1; ++i){
      // OnOff socket
      m_onoffSocket[0] = ns3::Socket::CreateSocket (GetNode (), m_typeId);
      NS_ASSERT (m_onoffSocket[0] != 0);
      m_onoffSocket[0]->Bind();

      m_onoffSocket[0]->SetAllowBroadcast(false);
      m_cbrRateFailSafe = m_cbrRate;
      ScheduleStartEvent(0);
//    }

    ns3::Simulator::Schedule(ns3::Seconds(2.0),&Worm::StartInfectingNodes, this);
}

void Worm::CancelEvents ()
{
  NS_LOG_FUNCTION (this);

  if (m_sendEvent.IsRunning () && m_cbrRateFailSafe == m_cbrRate )
    { // Cancel the pending send packet event
      // Calculate residual bits since last packet sent
      ns3::Time delta (ns3::Simulator::Now () - m_lastStartTime);
      ns3::int64x64_t bits = delta.To (ns3::Time::S) * m_cbrRate.GetBitRate ();
      m_residualBits += bits.GetHigh ();
    }
  m_cbrRateFailSafe = m_cbrRate;
  ns3::Simulator::Cancel (m_sendEvent);
  ns3::Simulator::Cancel (m_startStopEvent);
}

void Worm::StopApplication()     // Called at time specified by Stop
{
  //std::cerr << "[" << m_name << "] Stopping Application" << std::endl;
  if (m_sinkSocket != 0) {
      m_sinkSocket->Close();
  }
//  for (uint32_t i=0; i<1; ++i) {
      if (m_onoffSocket[0] != 0) {
          m_onoffSocket[0]->Close();
      }
//  }
  ns3::Simulator::Schedule(ns3::Seconds(2.0), &Worm::CloseAndPrint, this);
}

void Worm::CloseAndPrint()
{

}

void Worm::DoDispose()
{
  m_sinkSocket = 0;

//  for (uint32_t i=0; i<1; ++i)
    m_onoffSocket[0] = 0;
  Application::DoDispose();
}

void Worm::PrintState()
{
  std::cerr << "[" << m_name << "] State at time " << ns3::Simulator::Now() << ":\n"
            << "\tInfected: " << (m_infected ? "True\n" : "False\n")
            << "\tVulnerable: " << (m_vulnerable ? "True\n" : "False\n")
            << std::endl;
}
