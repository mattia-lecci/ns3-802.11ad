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
#include "timestamp-tag.h"
#include "four-elements-streaming-client.h"
#include "three-lognormal-random-variable.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FourElementsStreamingClient");

NS_OBJECT_ENSURE_REGISTERED (FourElementsStreamingClient);

TypeId
FourElementsStreamingClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FourElementsStreamingClient")
    .SetParent<GamingStreamingServer> ()
    .SetGroupName ("Applications")
    .AddConstructor<FourElementsStreamingClient> ()
  ;
  return tid;
}

FourElementsStreamingClient::FourElementsStreamingClient ()
{
  NS_LOG_FUNCTION (this);
  m_referenceBitRate = 0.056;
}

FourElementsStreamingClient::~FourElementsStreamingClient ()
{
  NS_LOG_FUNCTION (this);
}

void
FourElementsStreamingClient::InitializeStreams ()
{
  NS_LOG_FUNCTION (this);

  /** Add key stream */
  // Packet size
  Ptr<UniformRandomVariable> pktKey = CreateObjectWithAttributes<UniformRandomVariable> ("Min", DoubleValue (25),
                                                                                         "Max", DoubleValue (170));

  // Packet inter-arrival time
  Ptr<ConstantRandomVariable> iatKey1 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (9));
  Ptr<ConstantRandomVariable> iatKey2 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (50));
  Ptr<WeibullRandomVariable> iatKey3 = CreateObjectWithAttributes<WeibullRandomVariable> ("Scale", DoubleValue (12.40),
                                                                                          "Shape", DoubleValue (0.89),
                                                                                          "Bound", DoubleValue (50));

  Ptr<MixtureRandomVariable> iatKey = CreateObject<MixtureRandomVariable> ();
  iatKey->SetRandomVariables ({iatKey1, iatKey2, iatKey3}, {0.4391, 0.0936, 0.4673});

  AddNewTrafficStream (pktKey, iatKey);
}

} // Namespace ns3
