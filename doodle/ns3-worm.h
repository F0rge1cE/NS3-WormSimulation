
#ifndef WORM_APPLICATION_H
#define WORM_APPLICATION_H

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

class Worm : public ns3::Application{
public:
    Worm();
    ~Worm();

    void SetUp(std::string protocol, uint32_t infectionPort);
    void SetInfected(bool alreadyInfected);
    void SetVulnerable(bool vulnerable);
    void SetName(std::string name);
    
    static ns3::TypeId GetTypeId(void);
    static std::vector<int> GetInfectionArray ();
    static void SetX (uint32_t xInt);
    static void SetY (uint32_t yInt);
    static void SetExistNodes (uint32_t existNodes);
    static void SetPacketSize (uint32_t pktSize);
    static uint32_t GetTotalNodes ();
    static uint32_t GetExistNodes ();
    static uint32_t GetInfectedNodes ();
    static void SetNumInfected();

protected:
    void DoDispose (void);

    static uint32_t m_totalInfected;
    static uint32_t m_totalNodes;
    static uint32_t m_existNodes;
    static uint32_t m_pktSize;
    static std::vector<int> m_curInfected;
    static ns3::Ptr<ns3::UniformRandomVariable> tx;
    static ns3::Ptr<ns3::UniformRandomVariable> ty;
    
    bool m_infected;
    bool m_vulnerable;
    bool m_connected;

    uint16_t m_infectionPort;
    uint32_t m_residualBits; //!< Number of generated, but not sent, bits
    uint32_t m_totalBytes;
    
    std::string m_protocol;
    std::string m_name;

    ns3::Time m_lastStartTime;
    ns3::EventId m_startStopEvent;
    ns3::EventId m_sendEvent;
    ns3::TypeId m_typeId;
    ns3::Ptr<ns3::RandomVariableStream> m_onTime;       //!< rng for On Time
    ns3::Ptr<ns3::RandomVariableStream> m_offTime;      //!< rng for Off Time
    ns3::Ptr<ns3::Socket> m_sinkSocket;
    std::vector< ns3::Ptr<ns3::Socket> > m_onoffSocket;
    ns3::DataRate m_cbrRate;    //!< Rate that data is generated
    ns3::DataRate m_cbrRateFailSafe;
    ns3::Ipv4Address guessIP(void);
    ns3::TracedCallback<ns3::Ptr<const ns3::Packet> > m_txTrace;

    void Write32 (uint8_t *buffer, const uint32_t data);
    void Read32 (const uint8_t *buffer, uint32_t &data);

    
private:
//     inherited from Application base class.
    virtual void StartApplication (void);    // Called at time specified by Start
    virtual void StopApplication (void);     // Called at time specified by Stop
    void StartInfectingNodes();
    void Listen(ns3::Ptr<ns3::Socket> socket);
    void CloseAndPrint();
    void ConnectionSucceeded(ns3::Ptr<ns3::Socket> socket);
    void ConnectionFailed(ns3::Ptr<ns3::Socket> socket);
    void ScheduleStartEvent(uint32_t index);
    void ScheduleStopEvent();
    void SendPacket(uint32_t index);
    void StartSending(uint32_t index);
    void StopSending();
    void ScheduleNextTx(uint32_t index);
    void CancelEvents();
};

#endif // WORM_APPLICATION_H
