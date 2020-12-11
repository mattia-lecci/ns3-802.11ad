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

#include "ns3/log.h"
#include "game-streaming-application.h"
#include "timestamp-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GameStreamingApplication");

NS_OBJECT_ENSURE_REGISTERED (GameStreamingApplication);

TypeId
GameStreamingApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GameStreamingApplication")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddAttribute ("BitRate",
                   "Application's data rate (in Mbps). If 0.0, The default application bitrate is used, instead.",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&GameStreamingApplication::SetScalingFactor),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets.",
                   AddressValue (),
                   MakeAddressAccessor (&GameStreamingApplication::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort",
                   "The destination port of the outbound packets.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&GameStreamingApplication::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&GameStreamingApplication::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("Rx", "A new packet has received",
                     MakeTraceSourceAccessor (&GameStreamingApplication::m_rxTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

GameStreamingApplication::GameStreamingApplication ()
  : m_referenceBitRate (1),
    m_seq (0),
    m_totalSentPackets (0),
    m_totalReceivedPackets (0),
    m_totalFailedPackets (0),
    m_totalSentBytes (0),
    m_totalReceivedBytes (0),
    m_socket (0)
{
  NS_LOG_FUNCTION (this);
}


GameStreamingApplication::~GameStreamingApplication ()
{
  NS_LOG_FUNCTION (this);
}

void
GameStreamingApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_trafficStreams.clear ();
  m_socket = nullptr;
  Application::DoDispose ();
}

void
GameStreamingApplication::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
GameStreamingApplication::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}

uint32_t
GameStreamingApplication::GetTotalSentPackets (void)
{
  NS_LOG_FUNCTION (this);
  return m_totalSentPackets;
}

uint32_t
GameStreamingApplication::GetTotalReceivedPackets (void)
{
  NS_LOG_FUNCTION (this);
  return m_totalReceivedPackets;
}

uint32_t
GameStreamingApplication::GetTotalFailedPackets (void)
{
  NS_LOG_FUNCTION (this);
  return m_totalFailedPackets;
}

uint32_t
GameStreamingApplication::GetTotalSentBytes (void)
{
  NS_LOG_FUNCTION (this);
  return m_totalSentBytes;
}

uint32_t
GameStreamingApplication::GetTotalReceivedBytes (void)
{
  NS_LOG_FUNCTION (this);
  return m_totalReceivedBytes;
}

void
GameStreamingApplication::EraseStatistics (void)
{
  NS_LOG_FUNCTION (this);
  m_totalSentPackets = 0;
  m_totalFailedPackets = 0;
  m_totalSentBytes = 0;
  m_totalReceivedBytes = 0;
  m_totalFailedPackets = 0;
}

void
GameStreamingApplication::AddNewTrafficStream (Ptr<RandomVariableStream> packetSize,
                                            Ptr<RandomVariableStream> interArrivalTime)
{
  NS_LOG_FUNCTION (this << packetSize << interArrivalTime);
  Ptr<TrafficStream> newTraffic = Create<TrafficStream> ();
  newTraffic->packetSizeVariable = packetSize;
  newTraffic->interArrivalTimesVariable = interArrivalTime;
  newTraffic->sendEvent = EventId ();

  m_trafficStreams.push_back (newTraffic);
}

void
GameStreamingApplication::Send (Ptr<TrafficStream> traffic)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (traffic->sendEvent.IsExpired ());

  uint32_t pktSize = traffic->packetSizeVariable->GetInteger ();

  Ptr<Packet> packet = Create<Packet> (pktSize);
  m_txTrace (packet);
  TimestampTag timestamp;
  timestamp.SetTimestamp (Simulator::Now ());
  packet->AddByteTag (timestamp);
  NS_ABORT_IF (packet->GetSize () != pktSize);

  std::stringstream addrString;
  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      addrString << Ipv4Address::ConvertFrom (m_peerAddress);
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      addrString << Ipv6Address::ConvertFrom (m_peerAddress);
    }

  if ((m_socket->Send (packet)) >= 0)
    {
      m_totalSentPackets++;
      m_totalSentBytes += pktSize;
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                              << "s gaming server sent "
                              << packet->GetSize () << " bytes to "
                              << addrString.str ()
                              << " port " << m_peerPort
                              << " total sent packets " << m_totalSentPackets
                              << " total Tx " << m_totalSentBytes << " bytes");
    }
  else
    {
      m_totalFailedPackets++;
      NS_LOG_INFO ("Error while sending " << packet->GetSize () << " bytes to "
                                          << addrString.str ());
    }
  ScheduleNextTx (traffic);
}


void 
GameStreamingApplication::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      m_rxTrace (packet, from);
      if (packet->GetSize () == 0)
        {
          break;
        }

      std::stringstream addrString;
      std::stringstream portString;
      if (InetSocketAddress::IsMatchingType  (from))
        {
          addrString << InetSocketAddress::ConvertFrom (from).GetIpv4 ();
          portString << InetSocketAddress::ConvertFrom (from).GetPort ();
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          addrString << Inet6SocketAddress::ConvertFrom (from).GetIpv6 ();
          portString << Inet6SocketAddress::ConvertFrom (from).GetPort ();
        }

      m_totalReceivedBytes += packet->GetSize ();
      m_totalReceivedPackets++;

      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                              << "s gaming server received "
                              << packet->GetSize () << " bytes from "
                              << addrString.str ()
                              << " port " << portString.str ()
                              << " total received packets " << m_totalReceivedPackets
                              << " total Tx " << m_totalReceivedBytes << " bytes");
    }
}

void
GameStreamingApplication::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  InitializeStreams ();
  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);

      if (Ipv4Address::IsMatchingType (m_peerAddress) == true)
        {
          InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_peerPort);
          if (m_socket->Bind (local) == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (m_peerAddress),
                                                m_peerPort));
        }
      else if (Ipv6Address::IsMatchingType (m_peerAddress) == true)
        {
          Inet6SocketAddress local6 = Inet6SocketAddress (Ipv6Address::GetAny (), m_peerPort);
          if (m_socket->Bind (local6) == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom (m_peerAddress),
                                                 m_peerPort));
        }
      else if (InetSocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_peerAddress);
        }
      else if (Inet6SocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_peerAddress);
        }
      else
        {
          NS_ASSERT_MSG (false, "Incompatible address type: " << m_peerAddress);
        }
    }
  m_socket->SetRecvCallback (MakeCallback (&GameStreamingApplication::HandleRead, this));
  m_socket->SetAllowBroadcast (true);

  for (auto &traffic : m_trafficStreams)
    {
      ScheduleNextTx (traffic);
    }
}

void
GameStreamingApplication::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  for (const auto &traffic : m_trafficStreams)
    {
      if (traffic->sendEvent.IsRunning ())
        {
          Simulator::Cancel (traffic->sendEvent);
        }
    }
  if (m_socket != 0)
    {
      m_socket->Close ();
    }
  else
    {
      NS_LOG_WARN ("GameStreamingApplication found null socket to close in StopApplication");
    }
}

void
GameStreamingApplication::ScheduleNextTx (Ptr<TrafficStream> traffic)
{
  NS_LOG_FUNCTION (this);

  Time nextTx = Seconds (-1);
  do // Iterate until the nextTx time is positive
    {
      // Using Seconds instead of MilliSeconds to avoid truncating to integer
      nextTx = Seconds(traffic->interArrivalTimesVariable->GetValue () / 1e3);
    } while (nextTx.IsStrictlyNegative ());

  traffic->sendEvent = Simulator::Schedule (nextTx, &GameStreamingApplication::Send, this, traffic);

}

void
GameStreamingApplication::SetScalingFactor (double targetBitRate)
{
  NS_LOG_FUNCTION (this << targetBitRate);
  if (targetBitRate == 0)  // Generate traffics based on Reference BitRate if targetBitRate not defined
    {
      m_scalingFactor = 1;
    }
  else  // Scale up/down the traffic rate based on targetBitRate
    {
      m_scalingFactor = targetBitRate / m_referenceBitRate;
    }
}


GameStreamingApplication::TrafficStream::~TrafficStream ()
{
  Simulator::Cancel (sendEvent);
  packetSizeVariable = nullptr;
  interArrivalTimesVariable = nullptr;
}

} // Namespace ns3
