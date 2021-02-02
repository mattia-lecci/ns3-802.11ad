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
    .AddAttribute ("DataRate",
                   "Application's data rate. If 0 bps, the default application bitrate is used.",
                   DataRateValue (DataRate ("0bps")),
                   MakeDataRateAccessor (&GameStreamingApplication::GetTargetDataRate,
                                         &GameStreamingApplication::SetTargetDataRate),
                   MakeDataRateChecker ())
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets.",
                   AddressValue (),
                   MakeAddressAccessor (&GameStreamingApplication::m_peerAddress),
                   MakeAddressChecker ())
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
  : m_referenceDataRate (DataRate ("0bps")),
    m_seq (0),
    m_totalSentPackets (0),
    m_totalReceivedPackets (0),
    m_totalFailedPackets (0),
    m_totalSentBytes (0),
    m_totalReceivedBytes (0),
    m_socket (0),
    m_isOn (false)
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
  if (!m_isOn)
  {
    NS_LOG_LOGIC ("App is not on: packet not sent");
    return;
  }

  NS_ASSERT (traffic->sendEvent.IsExpired ());

  std::stringstream addrString;
  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      addrString << Ipv4Address::ConvertFrom (m_peerAddress);
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      addrString << Ipv6Address::ConvertFrom (m_peerAddress);
    }
  else if (InetSocketAddress::IsMatchingType (m_peerAddress))
  {
    addrString << InetSocketAddress::ConvertFrom (m_peerAddress).GetIpv4 () << ":"
               << InetSocketAddress::ConvertFrom (m_peerAddress).GetPort ();
  }
  else
  {
    addrString << "UNKNOWN";
  }

  uint32_t totPktSize = traffic->packetSizeVariable->GetInteger ();
  uint32_t pktSizeLeft = totPktSize;
  uint32_t pktSizeLimit = m_socket->GetTxAvailable ();

  while (pktSizeLeft > 0)
    {
      uint32_t pktSize = pktSizeLeft > pktSizeLimit ? pktSizeLimit : pktSizeLeft;
      pktSizeLeft -= pktSize;
        
      Ptr<Packet> packet = Create<Packet> (pktSize);
      m_txTrace (packet);
      TimestampTag timestamp;
      timestamp.SetTimestamp (Simulator::Now ());
      packet->AddByteTag (timestamp);
      NS_ABORT_IF (packet->GetSize () != pktSize);

      if ((m_socket->Send (packet)) >= 0)
        {
          m_totalSentPackets++;
          m_totalSentBytes += pktSize;
          NS_LOG_INFO ("Sending packet of size " << pktSize << " B " <<
                       "(out of " << totPktSize << " B) " <<
                       "to " << addrString.str ());
        }
      else
        {
          m_totalFailedPackets++;
          NS_LOG_INFO ("Error while sending packet of size " << pktSize << " B " <<
                       "(out of " << totPktSize << " B) " <<
                       "to " << addrString.str ());
        }
    }
  
  ScheduleNextTx (traffic);
}


void GameStreamingApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}


void GameStreamingApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
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
  if (!m_socket)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);

      if (InetSocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), InetSocketAddress::ConvertFrom (m_peerAddress).GetPort ());
          NS_ABORT_MSG_IF (m_socket->Bind (local) == -1, "Failed to bind socket");
        }
      else if (Inet6SocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          Inet6SocketAddress local6 = Inet6SocketAddress (Ipv6Address::GetAny (), Inet6SocketAddress::ConvertFrom (m_peerAddress).GetPort ());
          NS_ABORT_MSG_IF (m_socket->Bind (local6) == -1, "Failed to bind socket");
        }
      else
        {
          NS_ASSERT_MSG (false, "Incompatible address type: " << m_peerAddress);
        }
    }
  m_socket->Connect (m_peerAddress);
  m_socket->SetRecvCallback (MakeCallback (&GameStreamingApplication::HandleRead, this));
  m_socket->SetAllowBroadcast (true);
  m_isOn = true;

  for (auto &traffic : m_trafficStreams)
    {
      ScheduleNextTx (traffic);
    }
}

void
GameStreamingApplication::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  
  CancelEvents ();
  m_isOn = false;

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
GameStreamingApplication::SuspendApplication ()
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();
  m_isOn = false;
}

void
GameStreamingApplication::CancelEvents ()
{
  NS_LOG_FUNCTION (this);

  for (const auto &traffic : m_trafficStreams)
    {
      if (traffic->sendEvent.IsRunning ())
        {
          Simulator::Cancel (traffic->sendEvent);
        }
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
GameStreamingApplication::SetTargetDataRate (DataRate targetDataRate)
{
  NS_LOG_FUNCTION (this << targetDataRate);
  if (targetDataRate.GetBitRate () == 0)  // Generate traffics based on Reference BitRate if targetDataRate not defined
    {
      m_scalingFactor = 1;
      m_tagetDataRate = m_referenceDataRate;
    }
  else  // Scale up/down the traffic rate based on targetDataRate
    {
      m_scalingFactor = ((double) targetDataRate.GetBitRate ()) / m_referenceDataRate.GetBitRate ();
      m_tagetDataRate = targetDataRate;
    }

  NS_LOG_DEBUG ("targetDataRate=" << targetDataRate << ", m_referenceDataRate=" << m_referenceDataRate << ", m_scalingFactor=" << m_scalingFactor);
}

DataRate
GameStreamingApplication::GetTargetDataRate () const
{
  NS_LOG_FUNCTION (this);
  return m_tagetDataRate;
}

DataRate
GameStreamingApplication::GetReferenceDataRate () const
{
  NS_LOG_FUNCTION (this);
  return m_referenceDataRate;
}


GameStreamingApplication::TrafficStream::~TrafficStream ()
{
  Simulator::Cancel (sendEvent);
  packetSizeVariable = nullptr;
  interArrivalTimesVariable = nullptr;
}

} // Namespace ns3
