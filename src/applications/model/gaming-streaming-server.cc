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
#include "gaming-streaming-server.h"
#include "timestamp-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GamingStreamingServer");

NS_OBJECT_ENSURE_REGISTERED (GamingStreamingServer);

TypeId
GamingStreamingServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GamingStreamingServer")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets.",
                   AddressValue (),
                   MakeAddressAccessor (&GamingStreamingServer::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort",
                   "The destination port of the outbound packets.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&GamingStreamingServer::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&GamingStreamingServer::m_txTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

GamingStreamingServer::GamingStreamingServer ()
  : m_seq (0),
    m_totSentPackets (0),
    m_totFailedPackets (0),
    m_totSentBytes (0),
    m_socket (0),
    m_peerAddress (Address ()),
    m_peerPort (0)
{
  NS_LOG_FUNCTION (this);
}

GamingStreamingServer::GamingStreamingServer (Address ip, uint16_t port)
  : GamingStreamingServer ()
{
  NS_LOG_FUNCTION (this << ip << port);
  SetRemote (ip, port);
}

GamingStreamingServer::~GamingStreamingServer ()
{
  NS_LOG_FUNCTION (this);
}

void
GamingStreamingServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_trafficStreams.clear ();
  m_socket = nullptr;
  Application::DoDispose ();
}

void
GamingStreamingServer::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
GamingStreamingServer::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}

uint32_t
GamingStreamingServer::GetTotSentPackets (void)
{
  NS_LOG_FUNCTION (this);
  return m_totSentPackets;
}

uint32_t
GamingStreamingServer::GetTotFailedPackets (void)
{
  NS_LOG_FUNCTION (this);
  return m_totFailedPackets;
}

uint32_t
GamingStreamingServer::GetTotSentBytes (void)
{
  NS_LOG_FUNCTION (this);
  return m_totSentBytes;
}

void
GamingStreamingServer::EraseStatistics (void)
{
  NS_LOG_FUNCTION (this);
  m_totSentPackets = 0;
  m_totFailedPackets = 0;
  m_totSentBytes = 0;
}

void
GamingStreamingServer::AddNewTrafficStream (Ptr<RandomVariableStream> packetSize,
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
GamingStreamingServer::Send (Ptr<TrafficStream> traffic)
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
      m_totSentPackets++;
      m_totSentBytes += pktSize;
      NS_LOG_INFO("At time " << Simulator::Now ().GetSeconds ()
                             << "s gaming server sent "
                             << packet->GetSize () << " bytes to "
                             << addrString.str ()
                             << " port " << m_peerPort
                             << " total sent packets " << m_totSentPackets
                             << " total Tx " << m_totSentBytes << " bytes");
    }
  else
    {
      m_totFailedPackets++;
      NS_LOG_INFO ("Error while sending " << packet->GetSize () << " bytes to "
                                          << addrString.str ());
    }
  ScheduleNextTx (traffic);
}

void
GamingStreamingServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      if (Ipv4Address::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (m_peerAddress),
                                                m_peerPort));
        }
      else if (Ipv6Address::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
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
  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  m_socket->SetAllowBroadcast (true);

  for (auto &traffic : m_trafficStreams)
    {
      ScheduleNextTx (traffic);
    }
}

void
GamingStreamingServer::StopApplication ()
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
      NS_LOG_WARN ("GamingStreamingServer found null socket to close in StopApplication");
    }
}

void
GamingStreamingServer::ScheduleNextTx (Ptr<TrafficStream> traffic)
{
  NS_LOG_FUNCTION (this);

  Time nextTx = Seconds (-1);
  do // Iterate until the nextTx time is positive
    {
      // Using Seconds instead of MilliSeconds to avoid truncating to integer
      nextTx = Seconds(traffic->interArrivalTimesVariable->GetValue ()/1e3);
    } while (nextTx.IsNegative ());

  traffic->sendEvent = Simulator::Schedule (nextTx, &GamingStreamingServer::Send, this, traffic);

}

GamingStreamingServer::TrafficStream::~TrafficStream ()
{
  Simulator::Cancel (sendEvent);
  packetSizeVariable = nullptr;
  interArrivalTimesVariable = nullptr;
}

} // Namespace ns3
