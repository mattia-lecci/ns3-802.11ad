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
 * Authors: Tommy Azzino <tommy.azzino@gmail.com>
 *
 */

#include <ns3/assert.h>
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/pointer.h>
#include <ns3/boolean.h>

#include "dmg-wifi-scheduler.h"
#include "dmg-ap-wifi-mac.h"

namespace ns3	{

NS_LOG_COMPONENT_DEFINE ("DmgWifiScheduler");

NS_OBJECT_ENSURE_REGISTERED (DmgWifiScheduler);

TypeId
DmgWifiScheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgWifiScheduler")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
  ;
  return tid;
}

DmgWifiScheduler::DmgWifiScheduler ()
{
	NS_LOG_FUNCTION (this);
}

DmgWifiScheduler::~DmgWifiScheduler ()
{
  NS_LOG_FUNCTION (this);
}

void
DmgWifiScheduler::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_mac = 0;
}

void 
DmgWifiScheduler::SetMac (Ptr<DmgApWifiMac> mac)
{
	NS_LOG_FUNCTION (this << mac);
	m_mac = mac;
}

void
DmgWifiScheduler::Initialize (void)
{
	NS_LOG_FUNCTION (this);
	DoInitialize ();
}

void
DmgWifiScheduler::DoInitialize (void)
{
	NS_LOG_FUNCTION (this);
	m_mac->TraceConnectWithoutContext ("ADDTSReceived", MakeCallback (&DmgWifiScheduler::ReceiveAddtsRequest, this));
}

void
DmgWifiScheduler::ReceiveAddtsRequest (Mac48Address address, DmgTspecElement element)
{
	NS_LOG_DEBUG ("Receive ADDTS request from " << address);
}

} // namespace ns3