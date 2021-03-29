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
#ifndef GAME_STREAMING_APPLICATION_HELPER_H
#define GAME_STREAMING_APPLICATION_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/udp-server.h"
#include "ns3/udp-client.h"
namespace ns3 {

/**
 * \ingroup applications
 * \brief Create a gaming server application that streams UDP packets
 */
class GameStreamingApplicationHelper
{
public:
  /**
   * \brief Create an GameStreamingApplicationHelper to make it easier to work with GameStreamingApplication
   *
   * \param applicationType identifier of the gaming server
   */
  GameStreamingApplicationHelper (std::string applicationType);
  /**
   * Create an GameStreamingApplicationHelper to make it easier to work with GameStreamingApplication
   *
   * \param applicationType identifier of the gaming server
   * \param address the address of the remote node to send traffic to
   */
  GameStreamingApplicationHelper (std::string applicationType, Address address);

  /**
    * Record an attribute to be set in each Application after it is is created.
    *
    * \param name the name of the attribute to set
    * \param value the value of the attribute to set
    */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
    * Create one Gaming Streaming Server application on each of the input nodes
    *
    * \param c the nodes
    * \returns the applications created, one application per input node.
    */
  ApplicationContainer Install (NodeContainer c);

private:
  ObjectFactory m_factory; //!< Object factory.
};

} // namespace ns3

#endif /* GAME_STREAMING_APPLICATION_HELPER_H */
