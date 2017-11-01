#include "ns3/core-module.h"
#include "ns3/application.h"
#include "ns3/internet-module.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"

#include <vector>

//NS_LOG_COMPONENT_DEFINE ("ns3-worm");

/**
 * \class Worm Worm.h
 * \brief Worm application
 */

 class Worm : public ns3::Application{
 public:
 	static ns3::TypeId GetTypeId(void);
 	Worm();
 	~Worm();

 	void SetUp(std::string protocol, uint32_t infectionPort, uint16_t subnetId);

 	void SetInfected(bool isInfected);
 	
 	ns3::Ipv4Address generateIP(void);



 protected:
 	static uint32_t m_totalInfected;
 	static uint32_t m_totalNodes;
 	static uint32_t m_exsitNodes;
 	static uint32_t m_numConn;
 	static uint32_t m_pktSize;
 	static double m_chooseLocal;

 	static std::std::vector<int> m_curInfected;

 	bool m_infected;
 	bool m_connected;
 	bool m_exist;

 	uint16_t m_infectionPort;
 	uint32_t m_totalBytes;
 	uint32_t m_maxBytes;
 	uint16_t m_subnetId;

 	std::string m_protocol;
 	std::vector< ns3::Ptr<ns3::Socket> > m_onoffSocket;

 	ns3::TypeId m_typeId;
 	ns3::Time m_lastStartTime;
 	ns3::Ptr<ns3::RandomVariableStream> m_onTime;       //!< rng for On Time
    ns3::Ptr<ns3::RandomVariableStream> m_offTime;      //!< rng for Off Time
 	ns3::Ptr<ns3::Socket> m_sinkSocket;
 	ns3::EventId m_startStopEvent;
    ns3::EventId m_sendEvent;
    ns3::DataRate m_cbrRate;    //!< Rate that data is generated

 	void Write32 (uint8_t *buffer, const uint32_t data);
    void Read32 (const uint8_t *buffer, uint32_t &data);
 
 private:
 	virtual void StartApplication(void);
 	virtual void StopApplication(void);

 	void StartInfectingNodes();
 	void Listen(ns3::Ptr<ns3::Socket> socket);
 	void CloseAndPrint();
 	
 	void ConnectionSucceeded(ns3::Ptr<ns3::Socket> socket);
 	void ConnectionFailed(ns3::Ptr<ns3::Socket> socket);
 	
 	void ScheduleStartEvent(uint32_t index);
 	void ScheduleStopEvent(uint32_t index);
 	
 	void SendPacket(uint32_t index);
 	
 	void StartSending(uint32_t index);
 	void StopSending();
 	
 	void ScheduleNextTx(uint32_t index);
 	void CancelEvent();
 }