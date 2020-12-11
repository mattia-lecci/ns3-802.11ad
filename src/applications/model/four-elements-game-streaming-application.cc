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
#include "four-elements-game-streaming-application.h"
#include "three-lognormal-random-variable.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FourElementsGameStreamingApplication");

/*******************************/
/* FourElementsStreamingClient */
/*******************************/

NS_OBJECT_ENSURE_REGISTERED (FourElementsStreamingClient);

TypeId
FourElementsStreamingClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FourElementsStreamingClient")
    .SetParent<GameStreamingApplication> ()
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


/*******************************/
/* FourElementsStreamingServer */
/*******************************/

NS_OBJECT_ENSURE_REGISTERED (FourElementsStreamingServer);

TypeId
FourElementsStreamingServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FourElementsStreamingServer")
    .SetParent<GameStreamingApplication> ()
    .SetGroupName ("Applications")
    .AddConstructor<FourElementsStreamingServer> ()
  ;
  return tid;
}

FourElementsStreamingServer::FourElementsStreamingServer ()
{
  NS_LOG_FUNCTION (this);
  m_referenceBitRate = 2.544;
}

FourElementsStreamingServer::~FourElementsStreamingServer ()
{
  NS_LOG_FUNCTION (this);
}

void
FourElementsStreamingServer::InitializeStreams ()
{
  NS_LOG_FUNCTION (this);
  /** Add CBR audio stream */
  // Packet size
  Ptr<ConstantRandomVariable> pktCbrAudio = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (216));
  // Packet inter-arrival time
  Ptr<ConstantRandomVariable> iatCbrAudio = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (10));
  AddNewTrafficStream (pktCbrAudio, iatCbrAudio);

  /** Add Cursor stream */
  // Packet Size
  Ptr<ConstantRandomVariable> pktCursor = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (4));
  // Packet inter-arrival time
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
                                                                                            "Max", DoubleValue (m_scalingFactor * 1355));
  Ptr<ConstantRandomVariable> pktVideo2 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (m_scalingFactor * 1356));

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
