/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Copyright (c) 2020, University of Padova, Department of Information
 * Engineering, SIGNET Lab.
 *
 * Author: Hany Assasa <hany.assasa@gmail.com>
 *         Tommy Azzino <tommy.azzino@gmail.com>
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/wifi-module.h"
#include "common-functions.h"
#include <iomanip>
#include <sstream>

/**
 * Simulation Objective:
 * This script is used to evaluate the performance and behaviour of a scheduling alogorithm and admission policy for IEEE 802.11ad.
 * This script is based on "evaluate_qd_dense_scenario_single_ap".
 *
 * Network Topology:
 * The network consists of a single AP in the center of a room surrounded by 10 DMG STAs.
 *
 *
 *                                 DMG STA (10)
 *
 *
 *
 *                  DMG STA (1)                     DMG STA (9)
 *
 *
 *
 *          DMG STA (2)                                     DMG STA (8)
 *
 *                                    DMG AP
 *
 *          DMG STA (3)                                     DMG STA (7)
 *
 *                                       
 *
 *                  DMG STA (4)                     DMG STA (6)
 *
 *                                     
 *
 *                                  DMG STA (5)
 *
 *
 * Requested Service Periods:
 * DMG STA (1) --> DMG AP
 * DMG STA (2) --> DMG AP
 * DMG STA (3) --> DMG AP
 * DMG STA (4) --> DMG AP
 * DMG STA (5) --> DMG AP
 * DMG STA (6) --> DMG AP
 * DMG STA (7) --> DMG AP
 * DMG STA (8) --> DMG AP
 * DMG STA (9) --> DMG AP
 * DMG STA (10)--> DMG AP
 *
 * Running the Simulation:
 * ./waf --run "evaluate_scheduler_qd_dense_scenario"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. APP layer metrics for each Traffic Stream.
 * 2. PCAP traces for each station (if enabled).
 *
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateScheduler");

using namespace ns3;

Ptr<QdPropagationLossModel> lossModelRaytracing;

/** Simulation Arguments **/
std::string schedulerType;                         /* The type of scheduler to be used */
uint16_t allocationPeriod = 0;                     /* The periodicity of the requested SP allocation, 0 if not periodic */
std::string applicationType = "onoff";             /* Type of the Tx application */
std::string socketType = "ns3::UdpSocketFactory";  /* Socket Type (TCP/UDP) */
uint16_t schedulerTypeIdx = 0;                     /* The scheduler type: 0= CbapOnly, 1 basic, >=2 periodic */
std::string phyMode = "DMG_MCS12";                 /* The MCS to be used at the Physical Layer. */
uint32_t packetSize = 1448;                        /* Application payload size [bytes]. */
std::string tcpVariant = "NewReno";                /* TCP Variant Type. */
uint32_t maxPackets = 0;                           /* Maximum Number of Packets */
uint32_t msduAggregationSize = 7935;               /* The maximum aggregation size for A-MSDU [bytes]. */
uint32_t mpduAggregationSize = 262143;             /* The maximum aggregation size for A-MPDU [bytes]. */
double simulationTime = 10;                        /* Simulation time [s]. */
uint8_t allocationId = 1;                          /* The allocation ID of the DMG Tspec element to create */
Time thrLogPeriodicity = MilliSeconds (100);       /* The log periodicity for the throughput of each STA [ms] */

Mac2IdMap mac2IdMap;
Mac2AppMap mac2AppMap;

/** Applications **/
CommunicationPairMap communicationPairMap;  /* List of communicating devices. */

/* MAC layer Statistics */
PacketCountMap macTxDataFailed;
PacketCountMap macTxDataOk;
PacketCountMap macRxDataOk;
Ptr<DmgApWifiMac> apWifiMac;

/* Received packets output stream */
Ptr<OutputStreamWrapper> receivedPktsTrace;
/* SPs output stream */
Ptr<OutputStreamWrapper> spTrace;
/* MAC queue size output stream */
Ptr<OutputStreamWrapper> queueTrace;

CommunicationPair
InstallApplication (Ptr<Node> srcNode, Ptr<Node> dstNode, Ipv4Address address, std::string appDataRate, uint16_t appNumber)
{
  NS_LOG_FUNCTION (srcNode->GetId () << dstNode->GetId () << address << appDataRate << +appNumber);
  CommunicationPair commPair;
  /* Install TCP/UDP Transmitter on the source node */
  Address dest (InetSocketAddress (address, 9000 + appNumber));
  ApplicationContainer srcApp;
  if (applicationType == "onoff")
    {
      OnOffHelper src (socketType, dest);
      src.SetAttribute ("MaxBytes", UintegerValue (maxPackets));
      src.SetAttribute ("PacketSize", UintegerValue (packetSize));
      src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
      src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      src.SetAttribute ("DataRate", DataRateValue (DataRate (appDataRate)));
      srcApp = src.Install (srcNode);
    }
  else if (applicationType == "bulk")
    {
      BulkSendHelper src (socketType, dest);
      srcApp = src.Install (srcNode);
    }
  /* The APP is manually started when the corresponding ADDTS request succeeded (or failed only for CbapOnlyDmgWifiScheduler) */
  /* Here the start time is to a value greater than the simulation time otherwise the APP will start at 0 by default */
  srcApp.Start (Seconds (simulationTime + 1));
  srcApp.Stop (Seconds (simulationTime));
  commPair.srcApp = srcApp.Get (0);
  commPair.appDataRate = DataRate (appDataRate).GetBitRate ();

  /* Install Simple TCP/UDP Server on the destination node */
  PacketSinkHelper sinkHelper (socketType, InetSocketAddress (Ipv4Address::GetAny (), 9000 + appNumber));
  ApplicationContainer sinkApp = sinkHelper.Install (dstNode);
  commPair.packetSink = StaticCast<PacketSink> (sinkApp.Get (0));
  commPair.packetSink->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&ReceivedPacket, receivedPktsTrace, &communicationPairMap, srcNode));
  sinkApp.Start (Seconds (0));

  return commPair;
}


int
main (int argc, char *argv[])
{
  uint32_t bufferSize = 131072;                   /* TCP Send/Receive Buffer Size [bytes]. */
  uint32_t queueSize = 1000;                      /* Wifi MAC Queue Size [packets]. */
  std::string appDataRate = "300Mbps";            /* Application data rate. */
  bool frameCapture = false;                      /* Use a frame capture model. */
  double frameCaptureMargin = 10;                 /* Frame capture margin [dB]. */
  bool verbose = false;                           /* Print Logging Information. */
  bool pcapTracing = false;                       /* Enable PCAP Tracing. */
  uint16_t numStas = 8;                           /* The number of DMG STAs. */
  std::map<std::string, std::string> tcpVariants; /* List of the TCP Variants */
  std::string qdChannelFolder = "DenseScenario";  /* The name of the folder containing the QD-Channel files. */
  std::string logComponentsStr = "";              /* Components to be logged from tLogStart to tLogEnd separated by ':' */
  uint32_t biDurationUs = 102400;                    /* Duration of a BI [us]. Must be a multiple of 1024 us */
  double tLogStart = 0.0;                         /* Log start [s] */
  double tLogEnd = simulationTime;                /* Log end [s] */
  std::string appDataRateStr = "";                /* List of App Data Rates for each SP allocation separated by ':' */
  uint32_t interAllocDistance = 10;               /* Duration of a broadcast CBAP between two ADDTS allocations [us] */
  bool accessCbapIfAllocated = true;              /* Enable the access to a broadcast CBAP for a STA with scheduled SP/CBAP */
  bool smartStart = false;                        /* Enable the applications to start at optimized instants */

  /** TCP Variants **/
  tcpVariants.insert (std::make_pair ("NewReno",       "ns3::TcpNewReno"));
  tcpVariants.insert (std::make_pair ("Hybla",         "ns3::TcpHybla"));
  tcpVariants.insert (std::make_pair ("HighSpeed",     "ns3::TcpHighSpeed"));
  tcpVariants.insert (std::make_pair ("Vegas",         "ns3::TcpVegas"));
  tcpVariants.insert (std::make_pair ("Scalable",      "ns3::TcpScalable"));
  tcpVariants.insert (std::make_pair ("Veno",          "ns3::TcpVeno"));
  tcpVariants.insert (std::make_pair ("Bic",           "ns3::TcpBic"));
  tcpVariants.insert (std::make_pair ("Westwood",      "ns3::TcpWestwood"));
  tcpVariants.insert (std::make_pair ("WestwoodPlus",  "ns3::TcpWestwoodPlus"));

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("applicationType", "Type of the Tx Application: onoff or bulk", applicationType);
  cmd.AddValue ("packetSize", "Application packet size [bytes]", packetSize);
  cmd.AddValue ("dataRate", "Application data rate", appDataRate);
  cmd.AddValue ("tcpVariant", "Transport protocol to use: TcpHighSpeed, TcpVegas, TcpNewReno, TcpWestwood, TcpWestwoodPlus", tcpVariant);
  cmd.AddValue ("socketType", "Socket type (default: ns3::UdpSocketFactory)", socketType);
  cmd.AddValue ("bufferSize", "TCP Buffer Size (Send/Receive) [bytes]", bufferSize);
  cmd.AddValue ("msduAggregation", "The maximum aggregation size for A-MSDU [bytes]", msduAggregationSize);
  cmd.AddValue ("mpduAggregation", "The maximum aggregation size for A-MPDU [bytes]", mpduAggregationSize);
  cmd.AddValue ("queueSize", "The maximum size of the Wifi MAC Queue [packets]", queueSize);
  cmd.AddValue ("frameCapture", "Use a frame capture model", frameCapture);
  cmd.AddValue ("frameCaptureMargin", "Frame capture model margin [dB]", frameCaptureMargin);
  cmd.AddValue ("phyMode", "802.11ad PHY Mode", phyMode);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time [s]", simulationTime);
  cmd.AddValue ("qdChannelFolder", "The name of the folder containing the QD-Channel files", qdChannelFolder);
  cmd.AddValue ("numSTAs", "The number of DMG STA", numStas);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.AddValue ("interAllocation", "Duration of a broadcast CBAP between two ADDTS allocations [us]", interAllocDistance);
  cmd.AddValue ("logComponentsStr", "Components to be logged from tLogStart to tLogEnd separated by ':'", logComponentsStr);
  cmd.AddValue ("tLogStart", "Log start [s]", tLogStart);
  cmd.AddValue ("tLogEnd", "Log end [s]", tLogEnd);
  cmd.AddValue ("schedulerTypeIdx", "Scheduler type: 0 CbapOnly, 1 Basic, >=2 Periodic", schedulerTypeIdx);
  cmd.AddValue ("allowAccessCbapIfAllocated", "Enable the access to a broadcast CBAP for a STA with scheduled SP/CBAP", accessCbapIfAllocated);
  cmd.AddValue ("smartStart", "Enable applications smart start", smartStart);
  cmd.Parse (argc, argv);

  if (schedulerTypeIdx == 0)
  {
    schedulerType = "ns3::CbapOnlyDmgWifiScheduler";
  }
  else if (schedulerTypeIdx == 1)
  {
    schedulerType = "ns3::BasicDmgWifiScheduler";
  }
  else
  {
    schedulerType = "ns3::PeriodicDmgWifiScheduler";
    allocationPeriod = schedulerTypeIdx;
  }

  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> e2eResults = ascii.CreateFileStream ("results.csv");
  *e2eResults->GetStream () << "TxPkts_pkts,TxBytes_B,RxPkts_pkts,RxBytes_B,AvgThroughput_Mbps,AvgDelay_s,AvgJitter_s" << std::endl;
  receivedPktsTrace = ascii.CreateFileStream ("packetsTrace.csv");
  *receivedPktsTrace->GetStream () << "SrcNodeId,TxTimestamp_ns,RxTimestamp_ns,PktSize_B" << std::endl;
  spTrace = ascii.CreateFileStream ("spTrace.csv");
  *spTrace->GetStream () << "SrcNodeId,Timestamp_ns,isStart" << std::endl;
  queueTrace = ascii.CreateFileStream ("queueTrace.csv");
  *queueTrace->GetStream () << "SrcNodeId,Timestamp_ns,queueSize_pkts" << std::endl;

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::QueueBase::MaxPackets", UintegerValue (queueSize));
  Config::SetDefault ("ns3::BasicDmgWifiScheduler::InterAllocationDistance", UintegerValue (interAllocDistance));
  Config::SetDefault ("ns3::DmgWifiMac::AccessCbapIfAllocated", BooleanValue (accessCbapIfAllocated));
  /* Enable Log of specific components from tLogStart to tLogEnd */  
  std::vector<std::string> logComponents = SplitString (logComponentsStr, ':');
  EnableMyLogs (logComponents, Seconds (tLogStart), Seconds (tLogEnd));

  /* Compute system path in order to import correctly DmgFiles */
  std::string systemPath = SystemPath::FindSelfDirectory ();
  std::vector<std::string> pathComponents = SplitString (systemPath, '/');
  std::string inputPath = GetInputPath (pathComponents);
  NS_LOG_UNCOND (inputPath);

  /*** Configure TCP Options ***/
  std::map<std::string, std::string>::const_iterator iter = tcpVariants.find (tcpVariant);
  NS_ASSERT_MSG (iter != tcpVariants.end (), "Cannot find Tcp Variant");
  TypeId tid = TypeId::LookupByName (iter->second);
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (tid));
  if (tcpVariant.compare ("Westwood") == 0)
    {
      Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOOD));
      Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (TcpWestwood::TUSTIN));
    }
  else if (tcpVariant.compare ("WestwoodPlus") == 0)
    {
      Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
      Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (TcpWestwood::TUSTIN));
    }

  /* Configure TCP Segment Size */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (packetSize));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (bufferSize));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (bufferSize));

  /**** Set up Channel ****/
  Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel> ();
  Ptr<QdPropagationDelay> propagationDelayRayTracing = CreateObject<QdPropagationDelay> ();
  lossModelRaytracing = CreateObject<QdPropagationLossModel> ();
  lossModelRaytracing->SetAttribute ("QDModelFolder", StringValue (inputPath + "DmgFiles/QdChannel/" + qdChannelFolder + "/"));
  propagationDelayRayTracing->SetAttribute ("QDModelFolder", StringValue (inputPath + "DmgFiles/QdChannel/" + qdChannelFolder + "/"));
  spectrumChannel->AddSpectrumPropagationLossModel (lossModelRaytracing);
  spectrumChannel->SetPropagationDelayModel (propagationDelayRayTracing);

  /**** Setup physical layer ****/
  SpectrumDmgWifiPhyHelper spectrumWifiPhyHelper = SpectrumDmgWifiPhyHelper::Default ();
  spectrumWifiPhyHelper.SetChannel (spectrumChannel);
  /* All nodes transmit at 10 dBm == 10 mW, no adaptation */
  spectrumWifiPhyHelper.Set ("TxPowerStart", DoubleValue (10.0));
  spectrumWifiPhyHelper.Set ("TxPowerEnd", DoubleValue (10.0));
  spectrumWifiPhyHelper.Set ("TxPowerLevels", UintegerValue (1));

  if (frameCapture)
    {
      /* Frame Capture Model */
      spectrumWifiPhyHelper.Set ("FrameCaptureModel", StringValue ("ns3::SimpleFrameCaptureModel"));
      Config::SetDefault ("ns3::SimpleFrameCaptureModel::Margin", DoubleValue (frameCaptureMargin));
    }
  /* Set operating channel */
  spectrumWifiPhyHelper.Set ("ChannelNumber", UintegerValue (2));
  /* Set error model */
  spectrumWifiPhyHelper.SetErrorRateModel ("ns3::DmgErrorModel",
                                           "FileName", StringValue (inputPath + "DmgFiles/ErrorModel/LookupTable_1458.txt"));
  /* Sensitivity model includes implementation loss and noise figure */
  spectrumWifiPhyHelper.Set ("CcaMode1Threshold", DoubleValue (-79));
  spectrumWifiPhyHelper.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));

  /* Create 1 DMG PCP/AP */
  NodeContainer apWifiNode;
  apWifiNode.Create (1);
  /* Create numStas DMG STAs */
  NodeContainer staWifiNodes;
  staWifiNodes.Create (numStas);

  /**** WifiHelper is a meta-helper: it helps to create helpers ****/
  DmgWifiHelper wifiHelper;

  /* Set default algorithm for all nodes to be constant rate */
  wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "ControlMode", StringValue (phyMode),
                                      "DataMode", StringValue (phyMode));

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMacHelper = DmgWifiMacHelper::Default ();

  Ssid ssid = Ssid ("SchedulerScenario");
  wifiMacHelper.SetType ("ns3::DmgApWifiMac",
                         "Ssid", SsidValue (ssid),
                         "BE_MaxAmpduSize", UintegerValue (mpduAggregationSize),
                         "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                         "BK_MaxAmpduSize", UintegerValue (mpduAggregationSize),
                         "BK_MaxAmsduSize", UintegerValue (msduAggregationSize),
                         "VI_MaxAmpduSize", UintegerValue (mpduAggregationSize),
                         "VI_MaxAmsduSize", UintegerValue (msduAggregationSize),
                         "VO_MaxAmpduSize", UintegerValue (mpduAggregationSize),
                         "VO_MaxAmsduSize", UintegerValue (msduAggregationSize));

  wifiMacHelper.SetAttribute ("SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (13),
                              "BeaconInterval", TimeValue (MicroSeconds (102400)),
                              "ATIPresent", BooleanValue (false));
  
  /* Set Parametric Codebook for the DMG AP */
  wifiHelper.SetCodebook ("ns3::CodebookParametric",
                          "FileName", StringValue (inputPath + "DmgFiles/Codebook/CODEBOOK_URA_AP_8x4_notNorm.txt"));

  /* Set the Scheduler for the DMG AP */
  wifiHelper.SetDmgScheduler (schedulerType);

  /* Create Wifi Network Devices (WifiNetDevice) */
  NetDeviceContainer apDevice;
  apDevice = wifiHelper.Install (spectrumWifiPhyHelper, wifiMacHelper, apWifiNode);

  wifiMacHelper.SetType ("ns3::DmgStaWifiMac",
                         "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                         "BE_MaxAmpduSize", UintegerValue (mpduAggregationSize),
                         "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                         "BK_MaxAmpduSize", UintegerValue (mpduAggregationSize),
                         "BK_MaxAmsduSize", UintegerValue (msduAggregationSize),
                         "VO_MaxAmpduSize", UintegerValue (mpduAggregationSize),
                         "VO_MaxAmsduSize", UintegerValue (msduAggregationSize),
                         "VI_MaxAmpduSize", UintegerValue (mpduAggregationSize),
                         "VI_MaxAmsduSize", UintegerValue (msduAggregationSize));

  /* Set Parametric Codebook for the DMG STA */
  wifiHelper.SetCodebook ("ns3::CodebookParametric",
                          "FileName", StringValue (inputPath + "DmgFiles/Codebook/CODEBOOK_ULA_STA_1x4_notNorm.txt"));

  NetDeviceContainer staDevices;
  staDevices = wifiHelper.Install (spectrumWifiPhyHelper, wifiMacHelper, staWifiNodes);

  /* MAP MAC Addresses to NodeIDs */
  NetDeviceContainer devices;
  Ptr<WifiNetDevice> netDevice;
  devices.Add (apDevice);
  devices.Add (staDevices);
  for (uint32_t i = 0; i < devices.GetN (); i++)
    {
      netDevice = StaticCast<WifiNetDevice> (devices.Get (i));
      mac2IdMap[netDevice->GetMac ()->GetAddress ()] = netDevice->GetNode ()->GetId ();
      mac2AppMap[netDevice->GetMac ()->GetAddress ()] = std::make_pair(netDevice->GetNode ()->GetId (), false);
    }

  /* Setting mobility model for AP */
  MobilityHelper mobilityAp;
  mobilityAp.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityAp.Install (apWifiNode);

  /* Setting mobility model for STA */
  MobilityHelper mobilitySta;
  mobilitySta.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilitySta.Install (staWifiNodes);

  /* Internet stack*/
  InternetStackHelper stack;
  stack.Install (apWifiNode);
  stack.Install (staWifiNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface;
  apInterface = address.Assign (apDevice);
  Ipv4InterfaceContainer staInterfaces;
  staInterfaces = address.Assign (staDevices);

  /* We do not want any ARP packets */
  PopulateArpCache ();

  /** Install Applications **/
  std::vector<std::string> appDataRates = SplitString (appDataRateStr, ':');
  NS_ABORT_IF (appDataRates.size () != 0 &&
               appDataRates.size () != staWifiNodes.GetN ());
  for (uint32_t i = 0; i < staWifiNodes.GetN (); i++)
    {
      std::string dataRate = appDataRate;
      if (appDataRates.size () != 0)
      {
        dataRate = appDataRates.at (i);
      }
      communicationPairMap[staWifiNodes.Get (i)] = InstallApplication (staWifiNodes.Get (i), apWifiNode.Get (0),
                                                                         apInterface.GetAddress (0), dataRate, i);
    }

  /* Print Traces */
  if (pcapTracing)
    {
      spectrumWifiPhyHelper.SetPcapDataLinkType (SpectrumWifiPhyHelper::DLT_IEEE802_11_RADIO);
      spectrumWifiPhyHelper.EnablePcap ("Traces/AccessPoint", apDevice, false);
      spectrumWifiPhyHelper.EnablePcap ("Traces/STA", staDevices, false);
    }

  /* Turn on logging */
  if (verbose)
    {
      LogComponentEnable ("EvaluateScheduler", LOG_LEVEL_ALL);
      wifiHelper.EnableDmgMacLogComponents ();
      wifiHelper.EnableDmgPhyLogComponents ();
    }

  Ptr<WifiNetDevice> wifiNetDevice;
  Ptr<DmgStaWifiMac> staWifiMac;
  Ptr<WifiRemoteStationManager> remoteStationManager;
  /* By default the generated traffic is associated to AC_BE */
  /* Therefore we keep track of changes in the BE Queue */
  Ptr<WifiMacQueue> beQueue;

  /* Connect DMG PCP/AP traces */
  wifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  apWifiMac = StaticCast<DmgApWifiMac> (wifiNetDevice->GetMac ());
  macTxDataFailed.insert (std::make_pair (apWifiMac->GetAddress (), 0));
  macTxDataOk.insert (std::make_pair (apWifiMac->GetAddress (), 0));
  macRxDataOk.insert (std::make_pair (apWifiMac->GetAddress (), 0));
  remoteStationManager = wifiNetDevice->GetRemoteStationManager ();
  Ptr<Parameters> parameters = Create<Parameters> ();
  parameters->srcNodeId = wifiNetDevice->GetNode ()->GetId ();
  parameters->wifiMac = apWifiMac;
  apWifiMac->TraceConnectWithoutContext ("DTIStarted", MakeBoundCallback (&DtiStarted, spTrace, &mac2IdMap));
  apWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, parameters));
  apWifiMac->TraceConnectWithoutContext ("ContentionPeriodStarted", MakeBoundCallback (&ContentionPeriodStarted, spTrace));
  apWifiMac->TraceConnectWithoutContext ("ContentionPeriodEnded", MakeBoundCallback (&ContentionPeriodEnded, spTrace));
  remoteStationManager->TraceConnectWithoutContext ("MacRxOK", MakeBoundCallback (&MacRxOk, macRxDataOk, apWifiMac));

  /* Connect DMG STA traces */
  AssocParams ap;
  ap.phyMode = phyMode;
  ap.msduAggregationSize = msduAggregationSize;
  ap.mpduAggregationSize = mpduAggregationSize; 
  ap.apWifiMac = apWifiMac;
  ap.allocationId = allocationId;
  ap.allocationPeriod = allocationPeriod;

  for (uint32_t i = 0; i < staDevices.GetN (); i++)
    {
      wifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (i));
      staWifiMac = StaticCast<DmgStaWifiMac> (wifiNetDevice->GetMac ());
      beQueue = staWifiMac->GetBEQueue ()->GetQueue ();

      auto it = communicationPairMap.find (staWifiNodes.Get (i));
      NS_ABORT_MSG_IF (it == communicationPairMap.end (), "Could not find application for this node.");
      CommunicationPair& communicationPair = it->second;

      ap.communicationPair = communicationPair; // &?
      ap.staWifiMac = staWifiMac;

      macTxDataFailed.insert (std::make_pair (staWifiMac->GetAddress (), 0));
      macTxDataOk.insert (std::make_pair (staWifiMac->GetAddress (), 0));
      macRxDataOk.insert (std::make_pair (staWifiMac->GetAddress (), 0));
      remoteStationManager = wifiNetDevice->GetRemoteStationManager ();
      remoteStationManager->TraceConnectWithoutContext ("MacRxOK", MakeBoundCallback (&MacRxOk, macRxDataOk, staWifiMac));
      remoteStationManager->TraceConnectWithoutContext ("MacTxOK", MakeBoundCallback (&MacTxOk, macTxDataOk, staWifiMac));
      remoteStationManager->TraceConnectWithoutContext ("MacTxDataFailed", MakeBoundCallback (&MacTxDataFailed, macTxDataFailed, staWifiMac));
      staWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssociated, ap));
      staWifiMac->TraceConnectWithoutContext ("DeAssoc", MakeBoundCallback (&StationDeAssociated, communicationPair, staWifiMac));
      if (smartStart)
      {
        staWifiMac->TraceConnectWithoutContext ("ADDTSResponse", MakeBoundCallback (&ADDTSResponseReceivedSmart, schedulerType, communicationPair, biDurationUs));
        staWifiMac->TraceConnectWithoutContext ("ServicePeriodStarted", MakeBoundCallback (&ServicePeriodStartedSmart, spTrace, &mac2AppMap, communicationPair));
      }
      else
      {
        staWifiMac->TraceConnectWithoutContext ("ADDTSResponse", MakeBoundCallback (&ADDTSResponseReceived, schedulerType, communicationPair));
        staWifiMac->TraceConnectWithoutContext ("ServicePeriodStarted", MakeBoundCallback (&ServicePeriodStarted, spTrace, &mac2AppMap));
      }
      staWifiMac->TraceConnectWithoutContext ("ServicePeriodEnded", MakeBoundCallback (&ServicePeriodEnded, spTrace, &mac2IdMap));
      beQueue->TraceConnectWithoutContext ("OccupancyChanged", MakeBoundCallback (&MacQueueChanged, queueTrace, staWifiNodes.Get (i)));

      Ptr<Parameters> parameters = Create<Parameters> ();
      parameters->srcNodeId = wifiNetDevice->GetNode ()->GetId ();
      parameters->wifiMac = staWifiMac;
      staWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, parameters));
    }

  /* Install FlowMonitor on all nodes */
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  /* Print Output */
  NS_LOG_UNCOND ("Application Layer Throughput per Communicating Pair [Mbps]");
  std::string rowOutput = "Time [s],";
  std::string columnName;
  for (auto it = communicationPairMap.cbegin (); it != communicationPairMap.cend (); ++it)
    {
      columnName = " SrcNodeId=" + std::to_string (it->second.srcApp->GetNode ()->GetId ()) + ",";
      rowOutput += columnName;
    }
  NS_LOG_UNCOND (rowOutput + " Aggregate");

  /* Schedule Throughput Calulcations */
  Simulator::Schedule (thrLogPeriodicity, &CalculateThroughput, thrLogPeriodicity, communicationPairMap);

  Simulator::Stop (Seconds (simulationTime + 0.101));
  Simulator::Run ();
  Simulator::Destroy ();

  /* Print per flow statistics */
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")" << std::endl;
      std::cout << "  Tx Packets: " << i->second.txPackets << std::endl;
      std::cout << "  Tx Bytes:   " << i->second.txBytes << std::endl;
      std::cout << "  Rx Packets: " << i->second.rxPackets << std::endl;
      std::cout << "  Rx Bytes:   " << i->second.rxBytes << std::endl;
    }

  /* Print Application Layer Results Summary */
  std::cout << "\nApplication Layer Statistics:" << std::endl;
  Ptr<OnOffApplication> onoff;
  Ptr<BulkSendApplication> bulk;
  Ptr<PacketSink> packetSink;
  Time avgJitter;
  uint16_t communicationLinks = 1;
  double aggregateThr = 0;
  double thr;
  for (auto it = communicationPairMap.cbegin (); it != communicationPairMap.cend (); ++it)
    {
      std::cout << "Communication Link (" << communicationLinks << ") Statistics:" << std::endl;
      if (applicationType == "onoff")
        {
          onoff = StaticCast<OnOffApplication> (it->second.srcApp);
          std::cout << "  Tx Packets: " << onoff->GetTotalTxPackets () << std::endl;
          std::cout << "  Tx Bytes:   " << onoff->GetTotalTxBytes () << std::endl;
          *e2eResults->GetStream () << onoff->GetTotalTxPackets () << ","
                                    << onoff->GetTotalTxBytes () << ",";
        }
      else
        {
          bulk = StaticCast<BulkSendApplication> (it->second.srcApp);
          std::cout << "  Tx Packets: " << bulk->GetTotalTxPackets () << std::endl;
          std::cout << "  Tx Bytes:   " << bulk->GetTotalTxBytes () << std::endl;
          *e2eResults->GetStream () << bulk->GetTotalTxPackets () << ","
                                    << bulk->GetTotalTxBytes () << ",";
        }
      
      packetSink = it->second.packetSink;
      thr = packetSink->GetTotalRx () * 8.0 / ((simulationTime - it->second.startTime.GetSeconds ()) * 1e6);
      avgJitter = packetSink->GetTotalReceivedPackets () == 0 ? Seconds (0) : it->second.jitter / packetSink->GetTotalReceivedPackets ();
      aggregateThr += thr;
      std::cout << "  Rx Packets: " << packetSink->GetTotalReceivedPackets () << std::endl;
      std::cout << "  Rx Bytes:   " << packetSink->GetTotalRx () << std::endl;
      std::cout << "  Throughput: " << thr << " Mbps" << std::endl;
      std::cout << "  Avg Delay:  " << packetSink->GetAverageDelay ().GetSeconds () << " s" << std::endl;
      std::cout << "  Avg Delay:  " << packetSink->GetAverageDelay ().GetMicroSeconds () << " us" << std::endl;
      std::cout << "  Avg Jitter: " << avgJitter.GetSeconds () << " s" << std::endl;
      std::cout << "  Avg Jitter: " << avgJitter.GetMicroSeconds () << " us" << std::endl;

      *e2eResults->GetStream () << packetSink->GetTotalReceivedPackets () << "," << packetSink->GetTotalRx () << ","
                                << thr << "," << packetSink->GetAverageDelay ().GetSeconds () << ","
                                << avgJitter.GetSeconds () << std::endl;

      communicationLinks++;
    }
  std::cout << "\nAggregate Throughput: " << aggregateThr << std::endl;  

  return 0;
}
