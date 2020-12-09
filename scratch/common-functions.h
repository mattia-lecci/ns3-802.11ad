/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"

#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

using namespace ns3;

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
std::string to_string_with_precision (const T a_value, const int n = 6)
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
          NS_LOG_UNCOND ("Logging component " << component);
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
ReceivedPacket (Ptr<OutputStreamWrapper> receivedPktsTrace, Ptr<Node> srcNode, Ptr<const Packet> packet, const Address &address)
{
  NS_ABORT_MSG_IF (!packet->FindFirstMatchingByteTag (timestamp),
                   "Packet timestamp not found");

  CommunicationPair &commPair = communicationPairList.at (srcNode);
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
CalculateThroughput (Time thrLogPeriodicity)
{
  double totalThr = 0;
  double thr;
  /* duration is the time period which corresponds to the logged throughput values */
  std::string duration = to_string_with_precision<double> (Simulator::Now ().GetSeconds () - thrLogPeriodicity.GetSeconds (), 2) +
                         " - " + to_string_with_precision<double> (Simulator::Now ().GetSeconds (), 2) + ", ";
  std::string thrString;

  /* calculate the throughput over the last window with length thrLogPeriodicity for each communication Pair */
  for (auto it = communicationPairList.begin (); it != communicationPairList.end (); ++it)
    {
      thr = CalculateSingleStreamThroughput (it->second.packetSink, it->second.totalRx, thrLogPeriodicity.GetSeconds ());
      totalThr += thr;
      thrString += to_string_with_precision<double> (thr, 3) + ", ";
    }
  NS_LOG_UNCOND (duration << thrString << totalThr);

  Simulator::Schedule (thrLogPeriodicity, &CalculateThroughput, thrLogPeriodicity);
}


void 
DtiStarted (Ptr<OutputStreamWrapper> spTrace, Mac48Address apAddr, Time duration)
{
  NS_LOG_DEBUG ("DTI started at " << apAddr);
  *spTrace->GetStream () << mac2IdMap.at (apAddr) << "," << Simulator::Now ().GetNanoSeconds () << "," << true << std::endl;
  *spTrace->GetStream () << mac2IdMap.at (apAddr) << "," << (Simulator::Now () + duration).GetNanoSeconds () << "," << false << std::endl;
}


void
ServicePeriodStarted (Ptr<OutputStreamWrapper> spTrace, Mac48Address srcAddr, Mac48Address destAddr, bool isSource)
{
  NS_LOG_DEBUG ("Starting SP with source=" << srcAddr << ", dest=" << destAddr << ", isSource=" << isSource);
  *spTrace->GetStream () << mac2IdMap.at (srcAddr) << "," << Simulator::Now ().GetNanoSeconds () << "," << true << std::endl;
}


void
ServicePeriodEnded (Ptr<OutputStreamWrapper> spTrace, Mac48Address srcAddr, Mac48Address destAddr, bool isSource)
{
  NS_LOG_DEBUG ("Ending SP with source=" << srcAddr << ", dest=" << destAddr << ", isSource=" << isSource);
  *spTrace->GetStream () << mac2IdMap.at (srcAddr) << "," << Simulator::Now ().GetNanoSeconds () << "," << false << std::endl;
}


void
ContentionPeriodStarted (Ptr<OutputStreamWrapper> spTrace, Mac48Address address, TypeOfStation stationType)
{
  NS_LOG_DEBUG ("Starting CBAP at station=" << address << ", type of station=" << stationType);
  *spTrace->GetStream () << 255 << "," << Simulator::Now ().GetNanoSeconds () << "," << true << std::endl;
}


void
ContentionPeriodEnded (Ptr<OutputStreamWrapper> spTrace, Mac48Address address, TypeOfStation stationType)
{
  NS_LOG_DEBUG ("Ending CBAP at station=" << address << ", type of station=" << stationType);
  *spTrace->GetStream () << 255 << "," << Simulator::Now ().GetNanoSeconds () << "," << false << std::endl;
}


// TODO: improve
uint32_t
ComputeServicePeriodDuration (uint64_t appDataRate, uint64_t phyModeDataRate, uint64_t biDurationUs)
{
  NS_LOG_FUNCTION (appDataRate << phyModeDataRate);

  double dataRateRatio = double (appDataRate) / phyModeDataRate;
  // uint64_t biDurationUs = apWifiMac->GetBeaconInterval ().GetMicroSeconds ();
  uint32_t spDuration = ceil (dataRateRatio * biDurationUs);

  return spDuration * 1.3;
}


// TODO: improve
DmgTspecElement
GetDmgTspecElement (uint8_t allocId, bool isPseudoStatic, uint32_t minAllocation, uint32_t maxAllocation, uint16_t period)
{
  NS_LOG_FUNCTION (+allocId << isPseudoStatic << minAllocation << maxAllocation);
  /* Simple assert for the moment */
  NS_ABORT_MSG_IF (minAllocation > maxAllocation, "Minimum Allocation cannot be greater than Maximum Allocation");
  NS_ABORT_MSG_IF (maxAllocation > MAX_SP_BLOCK_DURATION, "Maximum Allocation exceeds Max SP block duration");
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
      minAllocation /= period;
      maxAllocation /= period;
      element.SetAllocationPeriod (period, false); // false: The allocation period must not be a multiple of the BI
    }
  element.SetMinimumAllocation (minAllocation);
  element.SetMaximumAllocation (maxAllocation);
  element.SetMinimumDuration (minAllocation);

  return element;
}

#endif /* COMMON_FUNCTIONS_H */
