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
#include "game-streaming-application-helper.h"
#include "ns3/udp-server.h"
#include "ns3/udp-client.h"
#include "ns3/udp-trace-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/game-streaming-application.h"

namespace ns3 {
GameStreamingApplicationHelper::GameStreamingApplicationHelper (std::string applicationType)
{
  m_factory.SetTypeId (applicationType);
}

GameStreamingApplicationHelper::GameStreamingApplicationHelper (std::string applicationType, Address address)
  : GameStreamingApplicationHelper (applicationType)
{
  SetAttribute ("RemoteAddress", AddressValue (address));
}

void
GameStreamingApplicationHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
GameStreamingApplicationHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<GameStreamingApplication> client = m_factory.Create<GameStreamingApplication> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}
} // namespace ns3
