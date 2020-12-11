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
#include "crazy-taxi-game-streaming-application.h"
#include "three-lognormal-random-variable.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CrazyTaxiGameStreamingApplication");

/****************************/
/* CrazyTaxiStreamingClient */
/****************************/

NS_OBJECT_ENSURE_REGISTERED (CrazyTaxiStreamingClient);

TypeId
CrazyTaxiStreamingClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CrazyTaxiStreamingClient")
    .SetParent<GameStreamingApplication> ()
    .SetGroupName ("Applications")
    .AddConstructor<CrazyTaxiStreamingClient> ()
  ;
  return tid;
}

CrazyTaxiStreamingClient::CrazyTaxiStreamingClient ()
{
  NS_LOG_FUNCTION (this);
  m_referenceBitRate = 0.033;
}

CrazyTaxiStreamingClient::~CrazyTaxiStreamingClient ()
{
  NS_LOG_FUNCTION (this);
}

void
CrazyTaxiStreamingClient::InitializeStreams ()
{
  NS_LOG_FUNCTION (this);
    
  /** Add key stream */
  // Packet size
  Ptr<UniformRandomVariable> pktKey = CreateObjectWithAttributes<UniformRandomVariable> ("Min", DoubleValue (25),
                                                                                         "Max", DoubleValue (210));

  // Packet inter-arrival time
  Ptr<ConstantRandomVariable> iatKey1 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (50));
  Ptr<WeibullRandomVariable> iatKey2 = CreateObjectWithAttributes<WeibullRandomVariable> ("Scale", DoubleValue (22.7),
                                                                                          "Shape", DoubleValue (1.33),
                                                                                          "Bound", DoubleValue (50));

  Ptr<MixtureRandomVariable> iatKey = CreateObject<MixtureRandomVariable> ();
  iatKey->SetRandomVariables ({iatKey1, iatKey2}, {0.3231, 0.6769});

  AddNewTrafficStream (pktKey, iatKey);
}


/****************************/
/* CrazyTaxiStreamingServer */
/****************************/

NS_OBJECT_ENSURE_REGISTERED (CrazyTaxiStreamingServer);

TypeId
CrazyTaxiStreamingServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CrazyTaxiStreamingServer")
    .SetParent<GameStreamingApplication> ()
    .SetGroupName ("Applications")
    .AddConstructor<CrazyTaxiStreamingServer> ()
  ;
  return tid;
}

CrazyTaxiStreamingServer::CrazyTaxiStreamingServer ()
{
  NS_LOG_FUNCTION (this);
  m_referenceBitRate = 5.948;
}

CrazyTaxiStreamingServer::~CrazyTaxiStreamingServer ()
{
  NS_LOG_FUNCTION (this);
}

void
CrazyTaxiStreamingServer::InitializeStreams ()
{
  NS_LOG_FUNCTION (this);

  /** Add CBR audio stream */
  // Packet size
  Ptr<ConstantRandomVariable> pktCbrAudio = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (216));
  // Packet inter-arrival time
  Ptr<ConstantRandomVariable> iatCbrAudio = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (10));
  AddNewTrafficStream (pktCbrAudio, iatCbrAudio);

  /** Add Cursor stream */
  // Packet size
  Ptr<ConstantRandomVariable> pktCursor = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (28));
  // Packet inter-arrival time
  Ptr<ConstantRandomVariable> iatCursor = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (50));
  AddNewTrafficStream (pktCursor, iatCursor);

  /** Add VBR audio stream */
  // Packet size
  Ptr<ConstantRandomVariable> pktVbrAudio1 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (244));
  Ptr<ConstantRandomVariable> pktVbrAudio2 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (1384));
  Ptr<MixtureRandomVariable> pktVbrAudio = CreateObject<MixtureRandomVariable> ();
  pktVbrAudio->SetRandomVariables ({pktVbrAudio1, pktVbrAudio2}, {0.0776, 0.9224});
  // Packet inter-arrival time
  Ptr<ConstantRandomVariable> iatVbrAudio = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (50));
  AddNewTrafficStream (pktVbrAudio, iatVbrAudio);

  /** Add video stream */
  // Packet size
  Ptr<UniformRandomVariable> pktVideo1 = CreateObjectWithAttributes<UniformRandomVariable> ("Min", DoubleValue (1),
                                                                                            "Max", DoubleValue (m_scalingFactor * 1355));
  Ptr<ConstantRandomVariable> pktVideo2 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (m_scalingFactor * 1356));
  Ptr<MixtureRandomVariable> pktVideo = CreateObject<MixtureRandomVariable> ();
  pktVideo->SetRandomVariables ({pktVideo1, pktVideo2}, {0.3606, 0.6394});

  // Packet inter-arrival time
  Ptr<ConstantRandomVariable> iatVideo1 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (0));
  Ptr<ThreeLogNormalRandomVariable> iatVideo2 = CreateObjectWithAttributes<ThreeLogNormalRandomVariable> ("Mu", DoubleValue (1.729),
                                                                                                          "Sigma", DoubleValue (0.343),
                                                                                                          "Threshold", DoubleValue (-2.25));
  Ptr<MixtureRandomVariable> iatVideo = CreateObject<MixtureRandomVariable> ();
  iatVideo->SetRandomVariables ({iatVideo1, iatVideo2}, {0.5725, 0.4275});
  AddNewTrafficStream (pktVideo, iatVideo);
}

} // Namespace ns3
