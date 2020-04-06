/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020, University of Padova, Department of Information
 * Engineering, SIGNET Lab.
 *
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
 * Authors: Salman Mohebi <s.mohebi22@gmail.com>
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("EvaluateGamingTraffic");

Time delayAccumulator;
Ptr<GamingStreamingServer> gamingServer;  /* Pointer to the gaming server application */
Ptr<PacketSink> packetSink;               /* Pointer to the packet sink application */
uint64_t lastTotalRx = 0;                 /* The value of the last total received bytes */
Time lastPacketTime = Seconds (-1.0);     /* Time of last generated packet*/

/** Simulation Arguments **/
bool csv = false;                           /* Enable CSV output. */
Time computeThroughputPeriodicity = MilliSeconds (100);  /* Period in which throughput calculated */

void
GeneratedPacketsStats (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> packet)
{
  Time currentPacketTime = Simulator::Now ();
  if (lastPacketTime.IsStrictlyNegative ())
    {
      lastPacketTime = currentPacketTime;
      return;
    }

  Time packetsInterArrivalTime = currentPacketTime - lastPacketTime;
  lastPacketTime = currentPacketTime;
  *stream->GetStream () << packet->GetSize ()  << ","
                        << packetsInterArrivalTime.GetSeconds () << std::endl;
}

void
AccumulateDelay (Ptr<const Packet> packet, const Address & addr)
{
  TimestampTag timestamp;
  if (packet->FindFirstMatchingByteTag (timestamp))
    {
      Time packetTime = timestamp.GetTimestamp ();
      delayAccumulator += Simulator::Now () - packetTime;
    }
}

void
CalculateThroughput (Ptr<OutputStreamWrapper> stream)
{
  Time now = Simulator::Now ();    /* Return the simulator's virtual time. */
  double cur = (packetSink->GetTotalRx () - lastTotalRx) * 8.0 / computeThroughputPeriodicity.GetSeconds () / 1e3;     /* Convert Application RX Packets to kb. */
  *stream->GetStream () << now.GetSeconds ()  << "," << cur << std::endl;
  lastTotalRx = packetSink->GetTotalRx ();
  Simulator::Schedule (computeThroughputPeriodicity, &CalculateThroughput, stream);
}

int
main (int argc, char *argv[])
{
  bool summary = true;                                /* Print application layer traffic summary */
  double simulationTime = 10.0;                       /* Simulation time in seconds */
  std::string game = "ns3::CrazyTaxiStreamingServer"; /* TypeId of the game */

  CommandLine cmd;
  cmd.AddValue ("summary", "Print summary of application layer traffic", summary);
  cmd.AddValue ("time", "Simulation time (in Seconds)", simulationTime );
  cmd.AddValue ("throughput", "Period in which throughput calculated", computeThroughputPeriodicity );
  cmd.AddValue ("game", "The game TypeId", game );
  cmd.AddValue ("csv", "Enable saving result in .csv file", csv );
  cmd.Parse (argc, argv);

  LogComponentEnable ("GamingStreamingServer", LOG_LEVEL_INFO);
  LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("50Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  GamingStreamingServerHelper gamingStreamingHelper (game, interfaces.GetAddress (0), 9);

  ApplicationContainer serverApps = gamingStreamingHelper.Install (nodes.Get (1));
  gamingServer = StaticCast<GamingStreamingServer> (serverApps.Get (0));
  serverApps.Start (Seconds (0.01));
  serverApps.Stop (Seconds (simulationTime));
  /** End of Streaming Server*/

  PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer sinkApps = sink.Install (nodes.Get (0));
  packetSink = StaticCast<PacketSink> (sinkApps.Get (0));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (simulationTime));

  if (csv)
    {
      AsciiTraceHelper ascii;
      Ptr<OutputStreamWrapper> cdfResults = ascii.CreateFileStream ("cdfResults.csv");
      *cdfResults->GetStream () << "PKT_SIZE,IAT" << std::endl;
      gamingServer->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&GeneratedPacketsStats, cdfResults));

      Ptr<OutputStreamWrapper> throughputResults = ascii.CreateFileStream ("throughputResults.csv");
      *throughputResults->GetStream () << "TIME,THROUGHPUT" << std::endl;
      Simulator::Schedule (computeThroughputPeriodicity, &CalculateThroughput, throughputResults);
    }
  packetSink->TraceConnectWithoutContext ("Rx", MakeCallback (&AccumulateDelay));

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  if (summary)
    {
      NS_LOG_UNCOND ("\nApplication layer traffic summary: ");
      NS_LOG_UNCOND ("Total sent bytes: " << gamingServer->GetTotSentBytes ()
                     << " (" << gamingServer->GetTotSentPackets () << " packets)");
      NS_LOG_UNCOND ("Total received bytes: " << packetSink->GetTotalRx ()
                << " ( " << packetSink->GetTotalReceivedPackets () << " packets)");
      NS_LOG_UNCOND ("Number of failed packets: " << gamingServer->GetTotFailedPackets ());
      NS_LOG_UNCOND ("Average Delay: "
                << double(delayAccumulator.GetMilliSeconds ()) / packetSink->GetTotalReceivedPackets ()
                << " ms");
    }
  return 0;
}
