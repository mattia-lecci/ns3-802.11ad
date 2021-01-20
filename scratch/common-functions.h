/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-remote-station-manager.h"
#include "ns3/dmg-information-elements.h"
#include "ns3/status-code.h"
#include "ns3/dmg-wifi-phy.h"
#include <iomanip>

#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

namespace ns3 {

class DmgWifiMac;
class DmgApWifiMac;
class DmgStaWifiMac;
class PacketSink;

/* Type definitions */
struct Parameters : public SimpleRefCount<Parameters>
{
  uint32_t srcNodeId;
  uint32_t dstNodeId;
  Ptr<DmgWifiMac> wifiMac;
};

struct CommunicationPair
{
  Ptr<Application> srcApp;
  Ptr<PacketSink> packetSink;
  uint64_t totalRx = 0;
  Time jitter = Seconds (0);
  Time lastDelayValue = Seconds (0);
  uint64_t appDataRate;
  Time startTime;
};
bool operator== (const CommunicationPair& a, const CommunicationPair& b);
bool operator!= (const CommunicationPair& a, const CommunicationPair& b);

struct AssocParams
{
  CommunicationPair communicationPair;
  std::string phyMode;
  uint32_t msduAggregationSize;
  uint32_t mpduAggregationSize; 
  Ptr<DmgApWifiMac> apWifiMac;
  Ptr<DmgStaWifiMac> staWifiMac;
  uint8_t allocationId;
  uint16_t allocationPeriod;
};
bool operator== (const AssocParams& a, const AssocParams& b);
bool operator!= (const AssocParams& a, const AssocParams& b);

typedef std::map<Ptr<Node>, CommunicationPair> CommunicationPairMap;
typedef std::map<Mac48Address, uint32_t> Mac2IdMap;
typedef std::map<Mac48Address, std::pair<uint32_t, bool>> Mac2AppMap;
typedef std::map<Mac48Address, uint64_t> PacketCountMap;

/* Functions */
bool
operator== (const CommunicationPair& a, const CommunicationPair& b)
{
  return a.srcApp == b.srcApp && a.packetSink == b.packetSink &&
    a.totalRx == b.totalRx && a.jitter == b.jitter &&
    a.lastDelayValue == b.lastDelayValue && a.appDataRate == b.appDataRate &&
    a.startTime == b.startTime;
}

bool
operator!= (const CommunicationPair& a, const CommunicationPair& b)
{
  return !(a == b);
}

bool
operator== (const AssocParams& a, const AssocParams& b)
{
  return a.communicationPair == b.communicationPair && a.phyMode == b.phyMode &&
    a.msduAggregationSize == b.msduAggregationSize &&
    a.mpduAggregationSize == b.mpduAggregationSize &&
    a.apWifiMac == b.apWifiMac && a.staWifiMac == b.staWifiMac &&
    a.allocationId == b.allocationId && a.allocationPeriod == b.allocationPeriod;
}


bool
operator!= (const AssocParams& a, const AssocParams& b)
{
  return !(a == b);
}


void
PopulateArpCache (void)
{
  Ptr<ArpCache> arp = CreateObject<ArpCache> ();
  arp->SetAliveTimeout (Seconds (3600 * 24 * 365));

  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
      Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
      NS_ASSERT (ip != 0);
      ObjectVectorValue interfaces;
      ip->GetAttribute ("InterfaceList", interfaces);
      for (ObjectVectorValue::Iterator j = interfaces.Begin (); j != interfaces.End (); j++)
        {
          Ptr<Ipv4Interface> ipIface = (j->second)->GetObject<Ipv4Interface> ();
          NS_ASSERT (ipIface != 0);
          Ptr<NetDevice> device = ipIface->GetDevice ();
          NS_ASSERT (device != 0);
          Mac48Address addr = Mac48Address::ConvertFrom (device->GetAddress ());
          for (uint32_t k = 0; k < ipIface->GetNAddresses (); k++)
            {
              Ipv4Address ipAddr = ipIface->GetAddress (k).GetLocal ();
              if (ipAddr == Ipv4Address::GetLoopback ())
                {
                  continue;
                }
              ArpCache::Entry *entry = arp->Add (ipAddr);
              entry->MarkWaitReply (0);
              entry->MarkAlive (addr);
            }
        }
    }

  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
      Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
      NS_ASSERT (ip != 0);
      ObjectVectorValue interfaces;
      ip->GetAttribute ("InterfaceList", interfaces);
      for (ObjectVectorValue::Iterator j = interfaces.Begin (); j != interfaces.End (); j++)
        {
          Ptr<Ipv4Interface> ipIface = (j->second)->GetObject<Ipv4Interface> ();
          ipIface->SetAttribute ("ArpCache", PointerValue (arp));
        }
    }
}


template <typename T>
std::string to_string_with_precision (const T a_value, const int n)
{
  std::ostringstream out;
  out.precision (n);
  out << std::fixed << a_value;
  return out.str ();
}


std::vector<std::string>
SplitString (const std::string &str, char delimiter)
{
  std::stringstream ss (str);
  std::string token;
  std::vector<std::string> container;

  while (getline (ss, token, delimiter))
    {
      container.push_back (token);
    }
  return container;
}


void
EnableMyLogs (std::vector<std::string> &logComponents, Time tLogStart, Time tLogEnd)
{
  for (size_t i = 0; i < logComponents.size (); ++i)
    {
      const char* component = logComponents.at (i).c_str ();
      if (strlen (component) > 0)
        {
          std::cout << "Logging component " << component << std::endl;
          Simulator::Schedule (tLogStart, &LogComponentEnable, component, LOG_LEVEL_ALL);
          Simulator::Schedule (tLogEnd, &LogComponentDisable, component, LOG_LEVEL_ALL);
        }
    }
}


std::string
GetInputPath (std::vector<std::string> &pathComponents)
{
  std::string inputPath = "/";
  std::string dir;
  for (size_t i = 0; i < pathComponents.size (); ++i)
    {
      dir = pathComponents.at (i);
      if (dir == "")
        continue;
      inputPath += dir + "/";
      if (dir == "ns3-802.11ad")
        break;
    }
  return inputPath;
}

void
StartApplication(CommunicationPair& communicationPair)
{
  communicationPair.startTime = Simulator::Now ();
  communicationPair.srcApp->StartApplication ();
}

void
SuspendApplication(CommunicationPair& communicationPair)
{
  Ptr<OnOffApplication> app = DynamicCast<OnOffApplication> (communicationPair.srcApp);
  if (app != 0)
  {
    app->SuspendApplication ();
  }
}

void
ReceivedPacket (Ptr<OutputStreamWrapper> receivedPktsTrace, CommunicationPairMap* communicationPairMap, Ptr<Node> srcNode, Ptr<const Packet> packet, const Address &address)
{
  TimestampTag timestamp;
  NS_ABORT_MSG_IF (!packet->FindFirstMatchingByteTag (timestamp),
                   "Packet timestamp not found");

  CommunicationPair &commPair = communicationPairMap->at (srcNode);
  Time delay = Simulator::Now () - timestamp.GetTimestamp ();
  Time jitter = Seconds (std::abs (delay.GetSeconds () - commPair.lastDelayValue.GetSeconds ()));
  commPair.jitter += jitter;
  commPair.lastDelayValue = delay;

  *receivedPktsTrace->GetStream () << srcNode->GetId () << "," << timestamp.GetTimestamp ().GetNanoSeconds () << ","
                                   << Simulator::Now ().GetNanoSeconds () << "," << packet->GetSize ()  << std::endl;
}


double
CalculateSingleStreamThroughput (Ptr<PacketSink> sink, uint64_t &lastTotalRx, double timeInterval)
{
  double rxBits = (sink->GetTotalRx () - lastTotalRx) * 8.0; /* Total Rx Bits in the last period of duration timeInterval */
  double rxBitsPerSec = rxBits / timeInterval; /* Total Rx bits per second */
  double thr = rxBitsPerSec / 1e6; /* Conversion from bps to Mbps */
  lastTotalRx = sink->GetTotalRx ();
  return thr;
}


void
CalculateThroughput (Time thrLogPeriodicity, CommunicationPairMap& communicationPairMap)
{
  double totalThr = 0;
  double thr;
  /* duration is the time period which corresponds to the logged throughput values */
  std::string duration = to_string_with_precision<double> (Simulator::Now ().GetSeconds () - thrLogPeriodicity.GetSeconds (), 2) +
                         " - " + to_string_with_precision<double> (Simulator::Now ().GetSeconds (), 2) + ", ";
  std::string thrString;

  /* calculate the throughput over the last window with length thrLogPeriodicity for each communication Pair */
  for (auto it = communicationPairMap.begin (); it != communicationPairMap.end (); ++it)
    {
      thr = CalculateSingleStreamThroughput (it->second.packetSink, it->second.totalRx, thrLogPeriodicity.GetSeconds ());
      totalThr += thr;
      thrString += to_string_with_precision<double> (thr, 3) + ", ";
    }
  std::cout << duration << thrString << totalThr << std::endl;

  Simulator::Schedule (thrLogPeriodicity, &CalculateThroughput, thrLogPeriodicity, communicationPairMap);
}


void 
DtiStarted (Ptr<OutputStreamWrapper> spTrace, Mac2IdMap* mac2IdMap, Mac48Address apAddr, Time duration)
{
  *spTrace->GetStream () << mac2IdMap->at (apAddr) << "," << Simulator::Now ().GetNanoSeconds () << "," << true << std::endl;
  *spTrace->GetStream () << mac2IdMap->at (apAddr) << "," << (Simulator::Now () + duration).GetNanoSeconds () << "," << false << std::endl;
}


void
ServicePeriodStartedSmart (Ptr<OutputStreamWrapper> spTrace, 
  Mac2AppMap* mac2AppMap, 
  CommunicationPair& communicationPair,
  Mac48Address srcAddr, 
  Mac48Address destAddr, 
  bool isSource)
{
  if (!mac2AppMap->at (srcAddr).second)
  {
    StartApplication (communicationPair);
    mac2AppMap->at (srcAddr).second = true;
  }
  *spTrace->GetStream () << mac2AppMap->at (srcAddr).first << "," << Simulator::Now ().GetNanoSeconds () << "," << true << std::endl;
}


void
ServicePeriodStarted (Ptr<OutputStreamWrapper> spTrace, 
  Mac2AppMap* mac2AppMap, 
  Mac48Address srcAddr, 
  Mac48Address destAddr, 
  bool isSource)
{
  *spTrace->GetStream () << mac2AppMap->at (srcAddr).first << "," << Simulator::Now ().GetNanoSeconds () << "," << true << std::endl;
}


void
ServicePeriodEnded (Ptr<OutputStreamWrapper> spTrace, Mac2IdMap* mac2IdMap, Mac48Address srcAddr, Mac48Address destAddr, bool isSource)
{
  *spTrace->GetStream () << mac2IdMap->at (srcAddr) << "," << Simulator::Now ().GetNanoSeconds () << "," << false << std::endl;
}


void
ContentionPeriodStarted (Ptr<OutputStreamWrapper> spTrace, Mac48Address address, TypeOfStation stationType)
{
  *spTrace->GetStream () << 255 << "," << Simulator::Now ().GetNanoSeconds () << "," << true << std::endl;
}


void
ContentionPeriodEnded (Ptr<OutputStreamWrapper> spTrace, Mac48Address address, TypeOfStation stationType)
{
  *spTrace->GetStream () << 255 << "," << Simulator::Now ().GetNanoSeconds () << "," << false << std::endl;
}

void 
OnOffTrace (Ptr<OutputStreamWrapper> appTrace, uint32_t staID, Ptr<Packet const> packet)
{
  *appTrace->GetStream () << staID << "," << Simulator::Now ().GetNanoSeconds () << "," << packet->GetSize() << std::endl;
}

uint32_t
ComputeServicePeriodDuration (uint64_t appDataRate, uint64_t phyModeDataRate, uint64_t biDurationUs)
{
  double dataRateRatio = double (appDataRate) / phyModeDataRate;
  uint32_t spDuration = ceil (dataRateRatio * biDurationUs);

  return spDuration * 1.2;
}


// TODO: improve
DmgTspecElement
GetDmgTspecElement (uint8_t allocId, bool isPseudoStatic, uint32_t minAllocation, uint32_t maxAllocation, uint16_t period)
{
  /* Simple assert for the moment */
  NS_ABORT_MSG_IF (minAllocation > maxAllocation, minAllocation << " > " << maxAllocation);
  NS_ABORT_MSG_IF (maxAllocation > MAX_SP_BLOCK_DURATION, maxAllocation << " > " << MAX_SP_BLOCK_DURATION);
  DmgTspecElement element;
  DmgAllocationInfo info;
  info.SetAllocationID (allocId);
  info.SetAllocationType (SERVICE_PERIOD_ALLOCATION);
  info.SetAllocationFormat (ISOCHRONOUS);
  info.SetAsPseudoStatic (isPseudoStatic);
  info.SetDestinationAid (AID_AP);
  element.SetDmgAllocationInfo (info);
  if (period > 0)
    {
      element.SetAllocationPeriod (period, false); // false: The allocation period must not be a multiple of the BI
    }
  element.SetMinimumAllocation (minAllocation);
  element.SetMaximumAllocation (maxAllocation);
  element.SetMinimumDuration (minAllocation);

  return element;
}


uint64_t
GetWifiRate (std::string phyMode, uint32_t msduAggregationSize_B, uint32_t mpduAggregationSize_B, std::string rateType)
{
  if (rateType == "phy")
  {
    uint64_t rate = WifiMode (phyMode).GetPhyRate ();
    return rate;
  }
  
  if (rateType == "mac")
  {
    int mcs = std::stoi (phyMode.substr (7));
    
    if (msduAggregationSize_B == 7935 && mpduAggregationSize_B == 0)
    {
      switch (mcs)
      {
        case 0:
          return 34908414;
        case 1:
          return 254512319;
        case 2:
          return 379912948;
        case 3:
          return 420954907;
        case 4:
          return 455202086;
        case 5:
          return 467890645;
        case 6:
          return 504683434;
        case 7:
          return 539629057;
        case 8:
          return 566234187;
        case 9:
          return 576709613;
        case 10:
          return 603839501;
        case 11:
          return 628175602;
        case 12:
          return 644883635;
        default:
          NS_FATAL_ERROR ("mcs=" << mcs << " not recognized (phyMode=" << phyMode << ")");
      }

    }
    else if (msduAggregationSize_B == 7935 && mpduAggregationSize_B == 262143)
    {
      switch (mcs)
      {
        case 0:
          return 36610012;
        case 1:
          return 379110719;
        case 2:
          return 746778458;
        case 3:
          return 926434274;
        case 4:
          return 1103569911;
        case 5:
          return 1191091513;
        case 6:
          return 1449796626;
        case 7:
          return 1785991762;
        case 8:
          return 2113204353;
        case 9:
          return 2273125221;
        case 10:
          return 2739606669;
        case 11:
          return 3332262090;
        case 12:
          return 3893826210;
        default:
          NS_FATAL_ERROR ("mcs=" << mcs << " not recognized (phyMode=" << phyMode << ")");
      }
    }
  }

  NS_FATAL_ERROR ("Invalid configuration: phyMode=" << phyMode << ", msduAggregationSize_B=" << msduAggregationSize_B <<
                  ", mpduAggregationSize_B=" << mpduAggregationSize_B << ", rateType=" << rateType);
  return 0;
}


void
StationAssociated (AssocParams params, Mac48Address apAddress, uint16_t aid)
{
  // std::cout << "DMG STA=" << params.staWifiMac->GetAddress () << " associated with DMG PCP/AP=" << apAddress << ", AID=" << aid << std::endl;

  uint32_t spDurationOverBi = ComputeServicePeriodDuration (params.communicationPair.appDataRate,
                                                      GetWifiRate (params.phyMode, params.msduAggregationSize, params.mpduAggregationSize, "mac"),
                                                      params.apWifiMac->GetBeaconInterval ().GetMicroSeconds ());
  
  uint32_t spBlockDuration = spDurationOverBi;
  if (params.allocationPeriod > 0)
    {
      spBlockDuration /= params.allocationPeriod;
    }

  // spBlockDuration might be larger than MAX_SP_BLOCK_DURATION: split it in sub blocks of equal duration
  uint32_t nSubBlocks = std::ceil (double (spBlockDuration) / MAX_SP_BLOCK_DURATION);
  uint32_t subBlockDuration = spBlockDuration / nSubBlocks;

  std::cout << "nSubBlocks=" << nSubBlocks << " of duration subBlockDuration=" << subBlockDuration << std::endl;
  for (uint32_t i = 0; i < nSubBlocks; i++)
  {
    params.staWifiMac->CreateAllocation (GetDmgTspecElement (params.allocationId, true, subBlockDuration, subBlockDuration, params.allocationPeriod));
  }
}


void
StationDeAssociated (CommunicationPair& communicationPair, Ptr<DmgWifiMac> staWifiMac, Mac48Address apAddress)
{
  // std::cout << "DMG STA=" << staWifiMac->GetAddress () << " deassociated from DMG PCP/AP=" << apAddress << std::endl;

  communicationPair.srcApp->StopApplication ();
}

void
ADDTSResponseReceived (std::string schedulerType, 
  CommunicationPair& communicationPair, 
  Mac48Address address, 
  StatusCode status, 
  DmgTspecElement element)
{
  // TODO: Add this code to DmgStaWifiMac class.
  // std::cout << "DMG STA=" << address << " received ADDTS response with status=" << status.IsSuccess () << std::endl;
  if (status.IsSuccess () || schedulerType == "ns3::CbapOnlyDmgWifiScheduler")
  {
    StartApplication (communicationPair);
  }
}

void
ADDTSResponseReceivedSmart (std::string schedulerType, 
    CommunicationPair& communicationPair, 
    uint64_t biDurationUs,
    Mac48Address address, 
    StatusCode status, 
    DmgTspecElement element)
{
  // TODO: Add this code to DmgStaWifiMac class.
  // std::cout << "DMG STA=" << address << " received ADDTS response with status=" << status.IsSuccess () << std::endl;
  
  if (schedulerType == "ns3::CbapOnlyDmgWifiScheduler")
  {
    // With smartStart at true, the applications at the STAs begin at distributed
    // time-instants, on an interval of BI duration.
    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
    x->SetAttribute ("Min", DoubleValue (0));
    x->SetAttribute ("Max", DoubleValue (biDurationUs));
    Time startTime = MicroSeconds(x->GetValue ());
    Simulator::Schedule (startTime, &StartApplication, communicationPair);
  }
  else
  {
    // APP is started right away if using SPs
    // Looks like there are problems to start directly from a SP (probably the setup of block ACKs)
    // The app is thus started during the CBAP and suspended immediately
    // NOTE: stopping the application results in closing the socket (cannot restart in a later moment).
    // The application is thus suspended (custom method)
    StartApplication (communicationPair);
    Simulator::Schedule (MilliSeconds (1), &SuspendApplication, communicationPair);
  }
  
}

void
SLSCompleted (Ptr<Parameters> parameters,
              Mac48Address address,
              ChannelAccessPeriod accessPeriod,
              BeamformingDirection beamformingDirection,
              bool isInitiatorTxss,
              bool isResponderTxss,
              SECTOR_ID sectorId, ANTENNA_ID antennaId)
{
  std::string stationType;
  if (parameters->wifiMac->GetTypeOfStation () == DMG_AP)
    stationType = "DMG  AP=";    
  else
    stationType = "DMG STA=";

  // std::cout << stationType << parameters->wifiMac->GetAddress () << " completed SLS phase with " << address 
  //               << ", antennaID=" << +antennaId << ", sectorID=" << +sectorId << ", accessPeriod=" << accessPeriod
  //               << ", IsInitiator=" << (beamformingDirection == 0) << std::endl;
    
}


void
MacQueueChanged (Ptr<OutputStreamWrapper> queueTrace, Ptr<Node> srcNode, uint32_t oldQueueSize, uint32_t newQueueSize)
{
  *queueTrace->GetStream () << srcNode->GetId () << "," << Simulator::Now ().GetNanoSeconds () << "," << newQueueSize << std::endl;
}


void
PhyTxBegin (Ptr<OutputStreamWrapper> phyTxBeginTrace, Ptr<Node> srcNode, Ptr<Packet const> p)
{
  *phyTxBeginTrace->GetStream () << srcNode->GetId () << "," << Simulator::Now ().GetNanoSeconds () << std::endl;
}


void
MacRxOk (PacketCountMap& macRxDataOk, Ptr<DmgWifiMac> wifiMac, WifiMacType type, 
         Mac48Address address, double snrValue)
{
  macRxDataOk.at (wifiMac->GetAddress ()) += 1;
}


void
MacTxDataFailed (PacketCountMap& macTxDataFailed, Ptr<DmgWifiMac> wifiMac, Mac48Address address)
{
  macTxDataFailed.at (wifiMac->GetAddress ()) += 1;
}


void 
MacTxOk (PacketCountMap& macTxDataOk, Ptr<DmgWifiMac> wifiMac, Mac48Address address)
{
  macTxDataOk.at (wifiMac->GetAddress ()) += 1;
}


uint64_t
GetDmgPhyRate (std::string phyMode)
{
    NS_ABORT_MSG_IF (phyMode.substr (0, 7) != "DMG_MCS", "Invalid phyMode=" << phyMode);
    int mcs = std::stoi (phyMode.substr (7));

    WifiMode mode;
    switch (mcs)
    {
      case 0:
        mode = DmgWifiPhy::GetDMG_MCS0 ();
        break;
      case 1:
        mode = DmgWifiPhy::GetDMG_MCS1 ();
        break;
      case 2:
        mode = DmgWifiPhy::GetDMG_MCS2 ();
        break;
      case 3:
        mode = DmgWifiPhy::GetDMG_MCS3 ();
        break;
      case 4:
        mode = DmgWifiPhy::GetDMG_MCS4 ();
        break;
      case 5:
        mode = DmgWifiPhy::GetDMG_MCS5 ();
        break;
      case 6:
        mode = DmgWifiPhy::GetDMG_MCS6 ();
        break;
      case 7:
        mode = DmgWifiPhy::GetDMG_MCS7 ();
        break;
      case 8:
        mode = DmgWifiPhy::GetDMG_MCS8 ();
        break;
      case 9:
        mode = DmgWifiPhy::GetDMG_MCS9 ();
        break;
      case 10:
        mode = DmgWifiPhy::GetDMG_MCS10 ();
        break;
      case 11:
        mode = DmgWifiPhy::GetDMG_MCS11 ();
        break;
      case 12:
        mode = DmgWifiPhy::GetDMG_MCS12 ();
        break;
      default:
        NS_FATAL_ERROR ("Invalid mcs=" << mcs);
    }

    // For DMG WiFi the inputs are ignored (see GetDataRate)
    return mode.GetPhyRate (0, 0, 0);
}


std::string 
ComputeUserDataRateFromNormOfferedTraffic (std::string phyMode, uint16_t numStas, double normOfferedTraffic)
{
    NS_ABORT_MSG_IF (normOfferedTraffic < 0.0 || normOfferedTraffic > 1.0, "Invalid normOfferedTraffic=" << normOfferedTraffic);
    
    double phyRate = GetDmgPhyRate (phyMode); // avoid integer approximations
    double maxRatePerSta = phyRate / numStas;
    double ratePerSta = normOfferedTraffic * maxRatePerSta;

    std::cout << "phyRate=" << phyRate/1e6 << " Mbps, maxRatePerSta=" << maxRatePerSta/1e6 << " Mbps, ratePerSta=" << ratePerSta/1e6 << " Mbps" << std::endl;

    std::stringstream ss;
    ss << std::fixed << std::setprecision(0) << ratePerSta << "bps";
    return ss.str();
}


} // namespace ns3

#endif /* COMMON_FUNCTIONS_H */
