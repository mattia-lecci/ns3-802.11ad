/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


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
#include "periodic-application.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"
#include "timestamp-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PeriodicApplication");

NS_OBJECT_ENSURE_REGISTERED (PeriodicApplication);

TypeId
PeriodicApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PeriodicApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<PeriodicApplication> ()
    .AddAttribute ("PacketSize", "The size of packets sent in on state",
                   UintegerValue (512),
                   MakeUintegerAccessor (&PeriodicApplication::m_pktSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&PeriodicApplication::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("PeriodRv", "A RandomVariableStream used to pick the duration of the period [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                   MakePointerAccessor (&PeriodicApplication::m_periodRv),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("BurstSizeRv", "A RandomVariableStream used to pick the burst size in [B].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"),
                   MakePointerAccessor (&PeriodicApplication::m_burstSizeRv),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("Protocol", "The type of protocol to use. This should be "
                   "a subclass of ns3::SocketFactory",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&PeriodicApplication::m_socketTid),
                   // This should check for SocketFactory as a parent
                   MakeTypeIdChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&PeriodicApplication::m_txTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}


PeriodicApplication::PeriodicApplication ()
  : m_socket (0),
    m_connected (false),
    m_totBytes (0),
    m_txPackets (0)
{
  NS_LOG_FUNCTION (this);
}

PeriodicApplication::~PeriodicApplication()
{
  NS_LOG_FUNCTION (this);
}

Ptr<Socket>
PeriodicApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

uint64_t
PeriodicApplication::GetTotalTxPackets (void) const
{
  return m_txPackets;
}

uint64_t
PeriodicApplication::GetTotalTxBytes (void) const
{
  return m_totBytes;
}

int64_t 
PeriodicApplication::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_periodRv->SetStream (stream);
  m_burstSizeRv->SetStream (stream + 1);
  return 2;
}

void
PeriodicApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

// Application Methods
void
PeriodicApplication::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_socketTid);
      if (Inet6SocketAddress::IsMatchingType (m_peer))
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
        }
      else if (InetSocketAddress::IsMatchingType (m_peer) ||
               PacketSocketAddress::IsMatchingType (m_peer))
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
        }
      m_socket->Connect (m_peer);
      m_socket->SetAllowBroadcast (true);
      m_socket->ShutdownRecv ();

      m_socket->SetConnectCallback (
        MakeCallback (&PeriodicApplication::ConnectionSucceeded, this),
        MakeCallback (&PeriodicApplication::ConnectionFailed, this));
    }

  // Ensure no pending event
  CancelEvents (); 
  StartSending ();
}

void
PeriodicApplication::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();
  if(m_socket != 0)
    {
      m_socket->Close ();
    }
  else
    {
      NS_LOG_WARN ("PeriodicApplication found null socket to close in StopApplication");
    }
}

void
PeriodicApplication::SuspendApplication ()
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();
}

void
PeriodicApplication::CancelEvents ()
{
  NS_LOG_FUNCTION (this);

  Simulator::Cancel (m_nextBurstEvent);
}


void
PeriodicApplication::StartSending ()
{
  NS_LOG_FUNCTION (this);

  // send packets for current burst
  uint32_t burstSize = m_burstSizeRv->GetInteger (); // NOTE: limited to 4 GB per burst by GetInteger
  uint32_t numPkts = burstSize / m_pktSize; // integer division
  uint32_t lastPktSize = burstSize - numPkts * m_pktSize;
  NS_LOG_DEBUG ("Current burst size: " << burstSize << " B. " <<
                "Sending " << numPkts << " packets of " << m_pktSize << " B, and one of " << lastPktSize << " B");

  for (uint32_t i = 0; i < numPkts; i++)
  {
    SendPacket (m_pktSize);
  }

  if (lastPktSize > 0)
  {
    SendPacket (lastPktSize);
  }
  
  // schedule next burst
  Time period = Seconds (m_periodRv->GetValue ());
  NS_LOG_DEBUG ("Next burst scheduled in " << period);
  NS_ABORT_MSG_IF (!period.IsPositive (), "Period must be non-negative, instead found period=" << period);
  m_nextBurstEvent = Simulator::Schedule (period, &PeriodicApplication::StartSending, this);
}


void
PeriodicApplication::SendPacket (uint32_t pktSize)
{
  NS_LOG_FUNCTION (this << pktSize);

  TimestampTag timestamp;
  timestamp.SetTimestamp (Simulator::Now ());
  Ptr<Packet> packet = Create<Packet> (pktSize);
  packet->AddByteTag (timestamp);
  // TODO add info on burst size and current packet counter

  m_txTrace (packet);
  m_socket->Send (packet);

  m_totBytes += pktSize;
  m_txPackets++;
  if (InetSocketAddress::IsMatchingType (m_peer))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                   << "s PeriodicApplication sent "
                   <<  packet->GetSize () << " bytes to "
                   << InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()
                   << " port " << InetSocketAddress::ConvertFrom (m_peer).GetPort ()
                   << " total Tx " << m_totBytes << " bytes");
    }
  else if (Inet6SocketAddress::IsMatchingType (m_peer))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                   << "s PeriodicApplication sent "
                   <<  packet->GetSize () << " bytes to "
                   << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6 ()
                   << " port " << Inet6SocketAddress::ConvertFrom (m_peer).GetPort ()
                   << " total Tx " << m_totBytes << " bytes");
    }

}


void
PeriodicApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void
PeriodicApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}


} // Namespace ns3
