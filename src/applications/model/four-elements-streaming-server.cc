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
#include "four-elements-streaming-server.h"
#include "three-lognormal-random-variable.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FourElementsStreamingServer");

NS_OBJECT_ENSURE_REGISTERED (FourElementsStreamingServer);

TypeId
FourElementsStreamingServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FourElementsStreamingServer")
    .SetParent<GamingStreamingServer> ()
    .SetGroupName ("Applications")
    .AddConstructor<FourElementsStreamingServer> ()
  ;
  return tid;
}

FourElementsStreamingServer::FourElementsStreamingServer ()
{
  NS_LOG_FUNCTION (this);
  InitializeStreams ();
}

FourElementsStreamingServer::FourElementsStreamingServer (Address ip, uint16_t port)
  : GamingStreamingServer (ip, port)
{
  NS_LOG_FUNCTION (this << ip << port);
  InitializeStreams ();
}

FourElementsStreamingServer::~FourElementsStreamingServer ()
{
  NS_LOG_FUNCTION (this);
}

void
FourElementsStreamingServer::InitializeStreams ()
{
  /** Add CBR audio stream */
  Ptr<ConstantRandomVariable> pktCbrAudio = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (216));
  Ptr<ConstantRandomVariable> iatCbrAudio = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (10));
  AddNewTrafficStream (pktCbrAudio, iatCbrAudio);

  /** Add Cursor stream */
  Ptr<ConstantRandomVariable> pktCursor = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (4));
  Ptr<ConstantRandomVariable> iatCursor = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (50));
  AddNewTrafficStream (pktCursor, iatCursor);

  /** Add VBR audio stream */
  // Packet size
  Ptr<ConstantRandomVariable> pktVbrAudio1 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (244));
  Ptr<UniformRandomVariable> pktVbrAudio2 = CreateObjectWithAttributes<UniformRandomVariable> ("Min", DoubleValue (245),
                                                                                               "Max", DoubleValue (1383));
  Ptr<ConstantRandomVariable> pktVbrAudio3 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (1384));

  Ptr<MixtureRandomVariable> pktVbrAudio = CreateObject<MixtureRandomVariable> ();
  pktVbrAudio->SetRandomVariables ({pktVbrAudio1, pktVbrAudio2, pktVbrAudio3}, {0.0532, 0.3028, 0.644});

  // Packet inter-arrival time
  Ptr<ConstantRandomVariable> iatVbrAudio = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (50));
  AddNewTrafficStream (pktVbrAudio, iatVbrAudio);

  /** Add video stream */
  // Packet size
  Ptr<UniformRandomVariable> pktVideo1 = CreateObjectWithAttributes<UniformRandomVariable> ("Min", DoubleValue (1),
                                                                                            "Max", DoubleValue (1355));
  Ptr<ConstantRandomVariable> pktVideo2 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (1356));

  Ptr<MixtureRandomVariable> pktVideo = CreateObject<MixtureRandomVariable> ();
  pktVideo->SetRandomVariables ({pktVideo1, pktVideo2}, {0.7393, 0.2607});

  // Packet inter-arrival time
  Ptr<ConstantRandomVariable> iatVideo1 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (0));
  Ptr<ThreeLogNormalRandomVariable> iatVideo2 = CreateObjectWithAttributes<ThreeLogNormalRandomVariable> ("Mu", DoubleValue (2.055),
                                                                                                          "Sigma", DoubleValue (0.2038),
                                                                                                          "Threshold", DoubleValue (-3.894));

  Ptr<MixtureRandomVariable> iatVideo = CreateObject<MixtureRandomVariable> ();
  iatVideo->SetRandomVariables ({iatVideo1, iatVideo2}, {0.2423, 0.7577});

  AddNewTrafficStream (pktVideo, iatVideo);
}

} // Namespace ns3
