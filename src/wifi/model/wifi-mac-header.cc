/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 * Copyright (c) 2015-2019 IMDEA Networks Institute
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 *          Hany Assasa <hany.assasa@gmail.com>
 */

#include "ns3/address-utils.h"
#include "wifi-mac-header.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (WifiMacHeader);

/// type enumeration
enum
{
  TYPE_MGT = 0,
  TYPE_CTL  = 1,
  TYPE_DATA = 2,
  TYPE_Extension = 3  // Extension frame type for DMG.
};

enum
{
  SUBTYPE_CTL_EXTENSION = 6, // Extension subtype for DMG Control Frame Extension.
  SUBTYPE_CTL_CTLWRAPPER = 7,
  SUBTYPE_CTL_BACKREQ = 8,
  SUBTYPE_CTL_BACKRESP = 9,
  SUBTYPE_CTL_RTS = 11,
  SUBTYPE_CTL_CTS = 12,
  SUBTYPE_CTL_ACK = 13
};

/*
 * Enumeration for DMG Control Frame Extension.
 */
enum
{
  SUBTYPE_CTL_EXTENSION_POLL = 2,
  SUBTYPE_CTL_EXTENSION_SPR = 3,
  SUBTYPE_CTL_EXTENSION_GRANT = 4,
  SUBTYPE_CTL_EXTENSION_DMG_CTS = 5,
  SUBTYPE_CTL_EXTENSION_DMG_DTS = 6,
  SUBTYPE_CTL_EXTENSION_GRANT_ACK = 7,
  SUBTYPE_CTL_EXTENSION_SSW = 8,
  SUBTYPE_CTL_EXTENSION_SSW_FBCK = 9,
  SUBTYPE_CTL_EXTENSION_SSW_ACK = 10
};

WifiMacHeader::WifiMacHeader ()
  : m_ctrlMoreData (0),
    m_ctrlWep (0),
    m_ctrlOrder (1),
    m_amsduPresent (0),
    m_dmgPpdu (false),
    m_beamRefinementRequired (false),
    m_beamTrackingRequired (false),
    m_trainingFieldLength (0)
{
}

WifiMacHeader::~WifiMacHeader ()
{
}

void
WifiMacHeader::SetDsFrom (void)
{
  m_ctrlFromDs = 1;
}

void
WifiMacHeader::SetDsNotFrom (void)
{
  m_ctrlFromDs = 0;
}

void
WifiMacHeader::SetDsTo (void)
{
  m_ctrlToDs = 1;
}

void
WifiMacHeader::SetDsNotTo (void)
{
  m_ctrlToDs = 0;
}

void
WifiMacHeader::SetAddr1 (Mac48Address address)
{
  m_addr1 = address;
}

void
WifiMacHeader::SetAddr2 (Mac48Address address)
{
  m_addr2 = address;
}

void
WifiMacHeader::SetAddr3 (Mac48Address address)
{
  m_addr3 = address;
}

void
WifiMacHeader::SetAddr4 (Mac48Address address)
{
  m_addr4 = address;
}

void
WifiMacHeader::SetType (WifiMacType type)
{
  switch (type)
    {
    case WIFI_MAC_CTL_CTLWRAPPER:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_CTLWRAPPER;
      break;
    case WIFI_MAC_CTL_BACKREQ:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_BACKREQ;
      break;
    case WIFI_MAC_CTL_BACKRESP:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_BACKRESP;
      break;
    case WIFI_MAC_CTL_RTS:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_RTS;
      break;
    case WIFI_MAC_CTL_CTS:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_CTS;
      break;
    case WIFI_MAC_CTL_ACK:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_ACK;
      break;
    case WIFI_MAC_CTL_DMG_POLL:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_POLL;
      break;
    case WIFI_MAC_CTL_DMG_SPR:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_SPR;
      break;
    case WIFI_MAC_CTL_DMG_GRANT:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_GRANT;
      break;
    case WIFI_MAC_CTL_DMG_CTS:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_DMG_CTS;
      break;
    case WIFI_MAC_CTL_DMG_DTS:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_DMG_DTS;
      break;
    case WIFI_MAC_CTL_DMG_SSW:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_SSW;
      break;
    case WIFI_MAC_CTL_DMG_SSW_FBCK:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_SSW_FBCK;
      break;
    case WIFI_MAC_CTL_DMG_SSW_ACK:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_SSW_ACK;
      break;
    case WIFI_MAC_CTL_DMG_GRANT_ACK:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_GRANT_ACK;
      break;
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 0;
      break;
    case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 1;
      break;
    case WIFI_MAC_MGT_REASSOCIATION_REQUEST:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 2;
      break;
    case WIFI_MAC_MGT_REASSOCIATION_RESPONSE:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 3;
      break;
    case WIFI_MAC_MGT_PROBE_REQUEST:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 4;
      break;
    case WIFI_MAC_MGT_PROBE_RESPONSE:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 5;
      break;
    case WIFI_MAC_MGT_BEACON:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 8;
      break;
    case WIFI_MAC_MGT_DISASSOCIATION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 10;
      break;
    case WIFI_MAC_MGT_AUTHENTICATION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 11;
      break;
    case WIFI_MAC_MGT_DEAUTHENTICATION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 12;
      break;
    case WIFI_MAC_MGT_ACTION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 13;
      break;
    case WIFI_MAC_MGT_ACTION_NO_ACK:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 14;
      break;
    case WIFI_MAC_MGT_MULTIHOP_ACTION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 15;
      break;
    case WIFI_MAC_DATA:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 0;
      break;
    case WIFI_MAC_DATA_CFACK:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 1;
      break;
    case WIFI_MAC_DATA_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 2;
      break;
    case WIFI_MAC_DATA_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 3;
      break;
    case WIFI_MAC_DATA_NULL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 4;
      break;
    case WIFI_MAC_DATA_NULL_CFACK:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 5;
      break;
    case WIFI_MAC_DATA_NULL_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 6;
      break;
    case WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 7;
      break;
    case WIFI_MAC_QOSDATA:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 8;
      break;
    case WIFI_MAC_QOSDATA_CFACK:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 9;
      break;
    case WIFI_MAC_QOSDATA_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 10;
      break;
    case WIFI_MAC_QOSDATA_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 11;
      break;
    case WIFI_MAC_QOSDATA_NULL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 12;
      break;
    case WIFI_MAC_QOSDATA_NULL_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 14;
      break;
    case WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 15;
      break;
    case WIFI_MAC_EXTENSION_DMG_BEACON:
      m_ctrlType = TYPE_Extension;
      m_ctrlSubtype = 0;
      break;
    }
  m_ctrlToDs = 0;
  m_ctrlFromDs = 0;
}

void
WifiMacHeader::SetRawDuration (uint16_t duration)
{
  m_duration = duration;
}

void
WifiMacHeader::SetDuration (Time duration)
{
  int64_t duration_us = ceil (static_cast<double> (duration.GetNanoSeconds ()) / 1000);
  NS_ASSERT (duration_us >= 0 && duration_us <= 0x7fff);
  m_duration = static_cast<uint16_t> (duration_us);
}

void
WifiMacHeader::SetId (uint16_t id)
{
  m_duration = id;
}

void
WifiMacHeader::SetSequenceNumber (uint16_t seq)
{
  m_seqSeq = seq;
}

void
WifiMacHeader::SetFragmentNumber (uint8_t frag)
{
  m_seqFrag = frag;
}

void
WifiMacHeader::SetNoMoreFragments (void)
{
  m_ctrlMoreFrag = 0;
}

void
WifiMacHeader::SetMoreFragments (void)
{
  m_ctrlMoreFrag = 1;
}

void
WifiMacHeader::SetOrder (void)
{
  m_ctrlOrder = 1;
}

void
WifiMacHeader::SetNoOrder (void)
{
  m_ctrlOrder = 0;
}

void
WifiMacHeader::SetRetry (void)
{
  m_ctrlRetry = 1;
}

void
WifiMacHeader::SetNoRetry (void)
{
  m_ctrlRetry = 0;
}

void
WifiMacHeader::SetMoreData (void)
{
  m_ctrlMoreData = 1;
}

void
WifiMacHeader::SetNoMoreData (void)
{
  m_ctrlMoreData = 0;
}

void
WifiMacHeader::SetQosTid (uint8_t tid)
{
  m_qosTid = tid;
}

void
WifiMacHeader::SetQosEosp ()
{
  m_qosEosp = 1;
}

void
WifiMacHeader::SetQosNoEosp ()
{
  m_qosEosp = 0;
}

void WifiMacHeader::SetQosAckPolicy (QosAckPolicy policy)
{
  switch (policy)
    {
    case NORMAL_ACK:
      m_qosAckPolicy = 0;
      break;
    case NO_ACK:
      m_qosAckPolicy = 1;
      break;
    case NO_EXPLICIT_ACK:
      m_qosAckPolicy = 2;
      break;
    case BLOCK_ACK:
      m_qosAckPolicy = 3;
      break;
    }
}

void
WifiMacHeader::SetQosNormalAck ()
{
  m_qosAckPolicy = 0;
}

void
WifiMacHeader::SetQosBlockAck ()
{
  m_qosAckPolicy = 3;
}

void
WifiMacHeader::SetQosNoAck ()
{
  m_qosAckPolicy = 1;
}

void WifiMacHeader::SetQosAmsdu (void)
{
  m_amsduPresent = 1;
}

void WifiMacHeader::SetQosNoAmsdu (void)
{
  m_amsduPresent = 0;
}

void WifiMacHeader::SetQosTxopLimit (uint8_t txop)
{
  m_qosStuff = txop;
}

void WifiMacHeader::SetQosMeshControlPresent (void)
{
  //Mark bit 0 of this variable instead of bit 8, since m_qosStuff is
  //shifted by one byte when serialized
  m_qosStuff = m_qosStuff | 0x01; //bit 8 of QoS Control Field
}

void WifiMacHeader::SetQosNoMeshControlPresent ()
{
  //Clear bit 0 of this variable instead of bit 8, since m_qosStuff is
  //shifted by one byte when serialized
  m_qosStuff = m_qosStuff & 0xfe; //bit 8 of QoS Control Field
}


Mac48Address
WifiMacHeader::GetAddr1 (void) const
{
  return m_addr1;
}

Mac48Address
WifiMacHeader::GetAddr2 (void) const
{
  return m_addr2;
}

Mac48Address
WifiMacHeader::GetAddr3 (void) const
{
  return m_addr3;
}

Mac48Address
WifiMacHeader::GetAddr4 (void) const
{
  return m_addr4;
}

WifiMacType
WifiMacHeader::GetType (void) const
{
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      switch (m_ctrlSubtype)
        {
        case 0:
          return WIFI_MAC_MGT_ASSOCIATION_REQUEST;
        case 1:
          return WIFI_MAC_MGT_ASSOCIATION_RESPONSE;
        case 2:
          return WIFI_MAC_MGT_REASSOCIATION_REQUEST;
        case 3:
          return WIFI_MAC_MGT_REASSOCIATION_RESPONSE;
        case 4:
          return WIFI_MAC_MGT_PROBE_REQUEST;
        case 5:
          return WIFI_MAC_MGT_PROBE_RESPONSE;
        case 8:
          return WIFI_MAC_MGT_BEACON;
        case 10:
          return WIFI_MAC_MGT_DISASSOCIATION;
        case 11:
          return WIFI_MAC_MGT_AUTHENTICATION;
        case 12:
          return WIFI_MAC_MGT_DEAUTHENTICATION;
        case 13:
          return WIFI_MAC_MGT_ACTION;
        case 14:
          return WIFI_MAC_MGT_ACTION_NO_ACK;
        case 15:
          return WIFI_MAC_MGT_MULTIHOP_ACTION;
        }
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_BACKREQ:
          return WIFI_MAC_CTL_BACKREQ;
        case SUBTYPE_CTL_BACKRESP:
          return WIFI_MAC_CTL_BACKRESP;
        case SUBTYPE_CTL_RTS:
          return WIFI_MAC_CTL_RTS;
        case SUBTYPE_CTL_CTS:
          return WIFI_MAC_CTL_CTS;
        case SUBTYPE_CTL_ACK:
          return WIFI_MAC_CTL_ACK;
        case SUBTYPE_CTL_EXTENSION:
          switch (m_ctrlFrameExtension)
            {
            case SUBTYPE_CTL_EXTENSION_POLL:
              return WIFI_MAC_CTL_DMG_POLL;
            case SUBTYPE_CTL_EXTENSION_SPR:
              return WIFI_MAC_CTL_DMG_SPR;
            case SUBTYPE_CTL_EXTENSION_GRANT:
              return WIFI_MAC_CTL_DMG_GRANT;
            case SUBTYPE_CTL_EXTENSION_DMG_CTS:
              return WIFI_MAC_CTL_DMG_CTS;
            case SUBTYPE_CTL_EXTENSION_DMG_DTS:
              return WIFI_MAC_CTL_DMG_DTS;
            case SUBTYPE_CTL_EXTENSION_GRANT_ACK:
              return WIFI_MAC_CTL_DMG_GRANT_ACK;
            case SUBTYPE_CTL_EXTENSION_SSW:
              return WIFI_MAC_CTL_DMG_SSW;
            case SUBTYPE_CTL_EXTENSION_SSW_FBCK:
              return WIFI_MAC_CTL_DMG_SSW_FBCK;
            case SUBTYPE_CTL_EXTENSION_SSW_ACK:
              return WIFI_MAC_CTL_DMG_SSW_ACK;
            }
        }
      break;
    case TYPE_DATA:
      switch (m_ctrlSubtype)
        {
        case 0:
          return WIFI_MAC_DATA;
        case 1:
          return WIFI_MAC_DATA_CFACK;
        case 2:
          return WIFI_MAC_DATA_CFPOLL;
        case 3:
          return WIFI_MAC_DATA_CFACK_CFPOLL;
        case 4:
          return WIFI_MAC_DATA_NULL;
        case 5:
          return WIFI_MAC_DATA_NULL_CFACK;
        case 6:
          return WIFI_MAC_DATA_NULL_CFPOLL;
        case 7:
          return WIFI_MAC_DATA_NULL_CFACK_CFPOLL;
        case 8:
          return WIFI_MAC_QOSDATA;
        case 9:
          return WIFI_MAC_QOSDATA_CFACK;
        case 10:
          return WIFI_MAC_QOSDATA_CFPOLL;
        case 11:
          return WIFI_MAC_QOSDATA_CFACK_CFPOLL;
        case 12:
          return WIFI_MAC_QOSDATA_NULL;
        case 14:
          return WIFI_MAC_QOSDATA_NULL_CFPOLL;
        case 15:
          return WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL;
        }
	break;

	case TYPE_Extension:
	  switch (m_ctrlSubtype)
	    {
	    case 0:
	      return WIFI_MAC_EXTENSION_DMG_BEACON;
	    }
	    break;
    }
  // NOTREACHED
  NS_ASSERT (false);
  return (WifiMacType) - 1;
}

bool
WifiMacHeader::IsFromDs (void) const
{
  return m_ctrlFromDs == 1;
}

bool
WifiMacHeader::IsToDs (void) const
{
  return m_ctrlToDs == 1;
}

bool
WifiMacHeader::IsData (void) const
{
  return (m_ctrlType == TYPE_DATA);
}

bool
WifiMacHeader::IsQosData (void) const
{
  return (m_ctrlType == TYPE_DATA && (m_ctrlSubtype & 0x08));
}

bool
WifiMacHeader::IsCtl (void) const
{
  return (m_ctrlType == TYPE_CTL);
}

bool
WifiMacHeader::IsMgt (void) const
{
  return (m_ctrlType == TYPE_MGT);
}

bool
WifiMacHeader::IsExtension (void) const
{
  return (m_ctrlType == TYPE_Extension);
}

bool
WifiMacHeader::IsCfpoll (void) const
{
  switch (GetType ())
    {
    case WIFI_MAC_DATA_CFPOLL:
    case WIFI_MAC_DATA_CFACK_CFPOLL:
    case WIFI_MAC_DATA_NULL_CFPOLL:
    case WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA_CFPOLL:
    case WIFI_MAC_QOSDATA_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL:
      return true;
    default:
      return false;
    }
}

bool
WifiMacHeader::IsRts (void) const
{
  return (GetType () == WIFI_MAC_CTL_RTS);
}

bool
WifiMacHeader::IsCts (void) const
{
  return (GetType () == WIFI_MAC_CTL_CTS);
}

bool
WifiMacHeader::IsDmgCts (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_CTS);
}

bool
WifiMacHeader::IsDmgDts (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_DTS);
}

bool
WifiMacHeader::IsAck (void) const
{
  return (GetType () == WIFI_MAC_CTL_ACK);
}

bool
WifiMacHeader::IsAssocReq (void) const
{
  return (GetType () == WIFI_MAC_MGT_ASSOCIATION_REQUEST);
}

bool
WifiMacHeader::IsAssocResp (void) const
{
  return (GetType () == WIFI_MAC_MGT_ASSOCIATION_RESPONSE);
}

bool
WifiMacHeader::IsReassocReq (void) const
{
  return (GetType () == WIFI_MAC_MGT_REASSOCIATION_REQUEST);
}

bool
WifiMacHeader::IsReassocResp (void) const
{
  return (GetType () == WIFI_MAC_MGT_REASSOCIATION_RESPONSE);
}

bool
WifiMacHeader::IsProbeReq (void) const
{
  return (GetType () == WIFI_MAC_MGT_PROBE_REQUEST);
}

bool
WifiMacHeader::IsProbeResp (void) const
{
  return (GetType () == WIFI_MAC_MGT_PROBE_RESPONSE);
}

bool
WifiMacHeader::IsBeacon (void) const
{
  return (GetType () == WIFI_MAC_MGT_BEACON);
}

bool
WifiMacHeader::IsDisassociation (void) const
{
  return (GetType () == WIFI_MAC_MGT_DISASSOCIATION);
}

bool
WifiMacHeader::IsAuthentication (void) const
{
  return (GetType () == WIFI_MAC_MGT_AUTHENTICATION);
}

bool
WifiMacHeader::IsDeauthentication (void) const
{
  return (GetType () == WIFI_MAC_MGT_DEAUTHENTICATION);
}

bool
WifiMacHeader::IsAction (void) const
{
  return (GetType () == WIFI_MAC_MGT_ACTION);
}

bool
WifiMacHeader::IsActionNoAck () const
{
  return (GetType () == WIFI_MAC_MGT_ACTION_NO_ACK);
}

bool
WifiMacHeader::IsMultihopAction (void) const
{
  return (GetType () == WIFI_MAC_MGT_MULTIHOP_ACTION);
}

bool
WifiMacHeader::IsBlockAckReq (void) const
{
  return (GetType () == WIFI_MAC_CTL_BACKREQ) ? true : false;
}

bool
WifiMacHeader::IsBlockAck (void) const
{
  return (GetType () == WIFI_MAC_CTL_BACKRESP) ? true : false;
}

bool
WifiMacHeader::IsDMGBeacon (void) const
{
  return (GetType () == WIFI_MAC_EXTENSION_DMG_BEACON);
}

bool
WifiMacHeader::IsSSW (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_SSW);
}

bool
WifiMacHeader::IsSSW_FBCK (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_SSW_FBCK);
}

bool
WifiMacHeader::IsSSW_ACK (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_SSW_ACK);
}

bool
WifiMacHeader::IsPollFrame (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_POLL);
}

bool
WifiMacHeader::IsSprFrame (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_SPR);
}

bool
WifiMacHeader::IsGrantFrame (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_GRANT);
}

uint16_t
WifiMacHeader::GetRawDuration (void) const
{
  return m_duration;
}

Time
WifiMacHeader::GetDuration (void) const
{
  return MicroSeconds (m_duration);
}

uint16_t
WifiMacHeader::GetSequenceControl (void) const
{
  return (m_seqSeq << 4) | m_seqFrag;
}

uint16_t
WifiMacHeader::GetSequenceNumber (void) const
{
  return m_seqSeq;
}

uint8_t
WifiMacHeader::GetFragmentNumber (void) const
{
  return m_seqFrag;
}

bool
WifiMacHeader::IsRetry (void) const
{
  return (m_ctrlRetry == 1);
}

bool
WifiMacHeader::IsMoreData (void) const
{
  return (m_ctrlMoreData == 1);
}

bool
WifiMacHeader::IsMoreFragments (void) const
{
  return (m_ctrlMoreFrag == 1);
}

bool
WifiMacHeader::IsQosBlockAck (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosAckPolicy == 3);
}

bool
WifiMacHeader::IsQosNoAck (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosAckPolicy == 1);
}

bool
WifiMacHeader::IsQosAck (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosAckPolicy == 0);
}

bool
WifiMacHeader::IsQosEosp (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosEosp == 1);
}

bool
WifiMacHeader::IsQosAmsdu (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_amsduPresent == 1);
}

uint8_t
WifiMacHeader::GetQosTid (void) const
{
  NS_ASSERT (IsQosData ());
  return m_qosTid;
}

WifiMacHeader::QosAckPolicy
WifiMacHeader::GetQosAckPolicy (void) const
{
  switch (m_qosAckPolicy)
    {
    case 0:
      return NORMAL_ACK;
    case 1:
      return NO_ACK;
    case 2:
      return NO_EXPLICIT_ACK;
    case 3:
      return BLOCK_ACK;
    }
  // NOTREACHED
  NS_ASSERT (false);
  return (QosAckPolicy) - 1;
}

uint8_t
WifiMacHeader::GetQosTxopLimit (void) const
{
  NS_ASSERT (IsQosData ());
  return m_qosStuff;
}

uint16_t
WifiMacHeader::GetFrameControl (void) const
{
  uint16_t val = 0;
  val |= (m_ctrlType << 2) & (0x3 << 2);
  val |= (m_ctrlSubtype << 4) & (0xf << 4);
  val |= (m_ctrlMoreData << 13) & (0x1 << 13);
  val |= (m_ctrlWep << 14) & (0x1 << 14);
  val |= (m_ctrlOrder << 15) & (0x1 << 15);

  if ((m_ctrlType == 1) && (m_ctrlSubtype == 6))
    {
      /* Frame Control for DMG */
      val |= (m_ctrlFrameExtension << 8) & (0xf << 8);
    }
  else
    {
      /* Frame Control for Non-DMG */
      val |= (m_ctrlToDs << 8) & (0x1 << 8);
      val |= (m_ctrlFromDs << 9) & (0x1 << 9);
      val |= (m_ctrlMoreFrag << 10) & (0x1 << 10);
      val |= (m_ctrlRetry << 11) & (0x1 << 11);
    }

  return val;
}

uint16_t
WifiMacHeader::GetQosControl (void) const
{
  uint16_t val = 0;
  val |= m_qosTid;
  val |= m_qosEosp << 4;
  val |= m_qosAckPolicy << 5;
  val |= m_amsduPresent << 7;
  if (m_dmgPpdu)
    {
      val |= m_qosAmsduType << 8;
      val |= m_qosRdg << 9;
      val |= m_qosAcConstraint << 15;
    }
  else
    {
      val |= m_qosStuff << 8;
    }
  return val;
}

void
WifiMacHeader::SetFrameControl (uint16_t ctrl)
{
  m_ctrlType = (ctrl >> 2) & 0x03;
  m_ctrlSubtype = (ctrl >> 4) & 0x0f;

  if ((m_ctrlType == 1) && (m_ctrlSubtype == 6))
    {
      /* Frame Control for DMG */
      m_ctrlFrameExtension = (ctrl >> 8) & 0x0F;
      m_ctrlMoreData = (ctrl >> 13) & 0x01;
      m_ctrlWep = (ctrl >> 14) & 0x01;
      m_ctrlOrder = (ctrl >> 15) & 0x01;
    }
  else
    {
      /* Frame Control for Non-DMG */
      m_ctrlToDs = (ctrl >> 8) & 0x01;
      m_ctrlFromDs = (ctrl >> 9) & 0x01;
      m_ctrlMoreFrag = (ctrl >> 10) & 0x01;
      m_ctrlRetry = (ctrl >> 11) & 0x01;
      m_ctrlMoreData = (ctrl >> 13) & 0x01;
      m_ctrlWep = (ctrl >> 14) & 0x01;
      m_ctrlOrder = (ctrl >> 15) & 0x01;
    }
}

void
WifiMacHeader::SetSequenceControl (uint16_t seq)
{
  m_seqFrag = seq & 0x0f;
  m_seqSeq = (seq >> 4) & 0x0fff;
}

void
WifiMacHeader::SetQosControl (uint16_t qos)
{
  m_qosTid = qos & 0x000f;
  m_qosEosp = (qos >> 4) & 0x0001;
  m_qosAckPolicy = (qos >> 5) & 0x0003;
  m_amsduPresent = (qos >> 7) & 0x0001;
  if (m_dmgPpdu)
    {
      m_qosAmsduType = (qos >> 8) & 0x1;
      m_qosRdg = (qos >> 9) & 0x1;
      m_qosAcConstraint = (qos >> 15) & 0x1;
    }
  else
    {
      m_qosStuff = (qos >> 8) & 0x00ff;
    }
}

uint32_t
WifiMacHeader::GetSize (void) const
{
  uint32_t size = 0;
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      size = 2 + 2 + 6 + 6 + 6 + 2;
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_RTS:
          size = 2 + 2 + 6 + 6;
          break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
          size = 2 + 2 + 6;
          break;
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
          size = 2 + 2 + 6 + 6;
          break;
        case SUBTYPE_CTL_CTLWRAPPER:
          size = 2 + 2 + 6 + 2 + 4;
          break;
        case SUBTYPE_CTL_EXTENSION:
          switch (m_ctrlFrameExtension)
            {
            case SUBTYPE_CTL_EXTENSION_POLL:
            case SUBTYPE_CTL_EXTENSION_SPR:
            case SUBTYPE_CTL_EXTENSION_GRANT:
            case SUBTYPE_CTL_EXTENSION_DMG_CTS:
            case SUBTYPE_CTL_EXTENSION_SSW:
            case SUBTYPE_CTL_EXTENSION_SSW_FBCK:
            case SUBTYPE_CTL_EXTENSION_SSW_ACK:
            case SUBTYPE_CTL_EXTENSION_GRANT_ACK:
              size = 2 + 2 + 6 + 6;
              break;
            case SUBTYPE_CTL_EXTENSION_DMG_DTS:
              size = 2 + 2 + 6;
              break;
            }
        }
      break;
    case TYPE_DATA:
      size = 2 + 2 + 6 + 6 + 6 + 2;
      if (m_ctrlToDs && m_ctrlFromDs)
        {
          size += 6;
        }
      if (m_ctrlSubtype & 0x08)
        {
          size += 2;
        }
      break;
    case TYPE_Extension:
      size = 2 + 2 + 6;
      break;
    }
  return size;
}

void
WifiMacHeader::SetAsDmgPpdu (void)
{
  m_dmgPpdu = true;
  m_ctrlOrder = 0;
}

bool
WifiMacHeader::IsDmgPpdu (void) const
{
  return m_dmgPpdu;
}

void
WifiMacHeader::SetQosAmsduType (AMSDU_Type type)
{
  m_qosAmsduType = static_cast<uint8_t> (type);
}

void WifiMacHeader::SetQosRdGrant (bool value)
{
  m_qosRdg = value;
}

void
WifiMacHeader::SetQosAcConstraint (bool value)
{
  m_qosAcConstraint = value;
}

AMSDU_Type
WifiMacHeader::GetQosAmsduType (void) const
{
  NS_ASSERT (m_dmgPpdu && IsQosData ());
  return static_cast<AMSDU_Type> (m_qosAmsduType);
}

bool
WifiMacHeader::IsQosRdGrant (void) const
{
  NS_ASSERT (m_dmgPpdu && IsQosData ());
  return (m_qosRdg == 1);
}

bool
WifiMacHeader::GetQosAcConstraint (void) const
{
  NS_ASSERT (m_dmgPpdu && IsQosData ());
  return m_qosAcConstraint;
}

const char *
WifiMacHeader::GetTypeString (void) const
{
#define FOO(x) \
case WIFI_MAC_ ## x: \
  return # x; \
  break;

  switch (GetType ())
    {
      FOO (CTL_RTS);
      FOO (CTL_CTS);
      FOO (CTL_ACK);
      FOO (CTL_BACKREQ);
      FOO (CTL_BACKRESP);
      FOO (CTL_DMG_POLL);
      FOO (CTL_DMG_SPR);
      FOO (CTL_DMG_GRANT);
      FOO (CTL_DMG_CTS);
      FOO (CTL_DMG_DTS);
      FOO (CTL_DMG_GRANT_ACK);
      FOO (CTL_DMG_SSW);
      FOO (CTL_DMG_SSW_FBCK);
      FOO (CTL_DMG_SSW_ACK);

      FOO (EXTENSION_DMG_BEACON);

      FOO (MGT_BEACON);
      FOO (MGT_ASSOCIATION_REQUEST);
      FOO (MGT_ASSOCIATION_RESPONSE);
      FOO (MGT_DISASSOCIATION);
      FOO (MGT_REASSOCIATION_REQUEST);
      FOO (MGT_REASSOCIATION_RESPONSE);
      FOO (MGT_PROBE_REQUEST);
      FOO (MGT_PROBE_RESPONSE);
      FOO (MGT_AUTHENTICATION);
      FOO (MGT_DEAUTHENTICATION);
      FOO (MGT_ACTION);
      FOO (MGT_ACTION_NO_ACK);
      FOO (MGT_MULTIHOP_ACTION);

      FOO (DATA);
      FOO (DATA_CFACK);
      FOO (DATA_CFPOLL);
      FOO (DATA_CFACK_CFPOLL);
      FOO (DATA_NULL);
      FOO (DATA_NULL_CFACK);
      FOO (DATA_NULL_CFPOLL);
      FOO (DATA_NULL_CFACK_CFPOLL);
      FOO (QOSDATA);
      FOO (QOSDATA_CFACK);
      FOO (QOSDATA_CFPOLL);
      FOO (QOSDATA_CFACK_CFPOLL);
      FOO (QOSDATA_NULL);
      FOO (QOSDATA_NULL_CFPOLL);
      FOO (QOSDATA_NULL_CFACK_CFPOLL);
    default:
      return "ERROR";
    }
#undef FOO
  // needed to make gcc 4.0.1 ppc darwin happy.
  return "BIG_ERROR";
}

TypeId
WifiMacHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiMacHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<WifiMacHeader> ()
  ;
  return tid;
}

TypeId
WifiMacHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
WifiMacHeader::PrintFrameControl (std::ostream &os) const
{
  os << "ToDS=" << std::hex << (int) m_ctrlToDs << ", FromDS=" << std::hex << (int) m_ctrlFromDs
     << ", MoreFrag=" <<  std::hex << (int) m_ctrlMoreFrag << ", Retry=" << std::hex << (int) m_ctrlRetry
     << ", MoreData=" <<  std::hex << (int) m_ctrlMoreData << std::dec
  ;
}

void
WifiMacHeader::Print (std::ostream &os) const
{
  os << GetTypeString () << " ";
  switch (GetType ())
    {
    case WIFI_MAC_CTL_RTS:
      os << "Duration/ID=" << m_duration << "us"
         << ", RA=" << m_addr1 << ", TA=" << m_addr2;
      break;
    case WIFI_MAC_CTL_CTS:
    case WIFI_MAC_CTL_ACK:
      os << "Duration/ID=" << m_duration << "us"
         << ", RA=" << m_addr1;
      break;
    case WIFI_MAC_CTL_BACKREQ:
    case WIFI_MAC_CTL_BACKRESP:
    case WIFI_MAC_CTL_CTLWRAPPER:
      break;
    case WIFI_MAC_MGT_BEACON:
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
    case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
    case WIFI_MAC_MGT_DISASSOCIATION:
    case WIFI_MAC_MGT_REASSOCIATION_REQUEST:
    case WIFI_MAC_MGT_REASSOCIATION_RESPONSE:
    case WIFI_MAC_MGT_PROBE_REQUEST:
    case WIFI_MAC_MGT_PROBE_RESPONSE:
    case WIFI_MAC_MGT_AUTHENTICATION:
    case WIFI_MAC_MGT_DEAUTHENTICATION:
      PrintFrameControl (os);
      os << " Duration/ID=" << m_duration << "us"
         << ", DA=" << m_addr1 << ", SA=" << m_addr2
         << ", BSSID=" << m_addr3 << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec
         << ", SeqNumber=" << m_seqSeq;
      break;
    case WIFI_MAC_MGT_ACTION:
    case WIFI_MAC_MGT_ACTION_NO_ACK:
      PrintFrameControl (os);
      os << " Duration/ID=" << m_duration << "us"
         << ", DA=" << m_addr1 << ", SA=" << m_addr2 << ", BSSID=" << m_addr3
         << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec << ", SeqNumber=" << m_seqSeq;
      break;
    case WIFI_MAC_MGT_MULTIHOP_ACTION:
      os << " Duration/ID=" << m_duration << "us"
         << ", RA=" << m_addr1 << ", TA=" << m_addr2 << ", DA=" << m_addr3
         << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec << ", SeqNumber=" << m_seqSeq;
      break;
    case WIFI_MAC_DATA:
      PrintFrameControl (os);
      os << " Duration/ID=" << m_duration << "us";
      if (!m_ctrlToDs && !m_ctrlFromDs)
        {
          os << ", DA=" << m_addr1 << ", SA=" << m_addr2 << ", BSSID=" << m_addr3;
        }
      else if (!m_ctrlToDs && m_ctrlFromDs)
        {
          os << ", DA=" << m_addr1 << ", SA=" << m_addr3 << ", BSSID=" << m_addr2;
        }
      else if (m_ctrlToDs && !m_ctrlFromDs)
        {
          os << ", DA=" << m_addr3 << ", SA=" << m_addr2 << ", BSSID=" << m_addr1;
        }
      else if (m_ctrlToDs && m_ctrlFromDs)
        {
          os << ", DA=" << m_addr3 << ", SA=" << m_addr4 << ", RA=" << m_addr1 << ", TA=" << m_addr2;
        }
      else
        {
          NS_FATAL_ERROR ("Impossible ToDs and FromDs flags combination");
        }
      os << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec
         << ", SeqNumber=" << m_seqSeq;
      break;
    case WIFI_MAC_CTL_DMG_POLL:
    case WIFI_MAC_CTL_DMG_SPR:
    case WIFI_MAC_CTL_DMG_GRANT:
    case WIFI_MAC_CTL_DMG_CTS:
    case WIFI_MAC_CTL_DMG_DTS:
    case WIFI_MAC_CTL_DMG_SSW:
    case WIFI_MAC_CTL_DMG_SSW_FBCK:
    case WIFI_MAC_CTL_DMG_SSW_ACK:
    case WIFI_MAC_CTL_DMG_GRANT_ACK:
    case WIFI_MAC_EXTENSION_DMG_BEACON:
    case WIFI_MAC_DATA_CFACK:
    case WIFI_MAC_DATA_CFPOLL:
    case WIFI_MAC_DATA_CFACK_CFPOLL:
    case WIFI_MAC_DATA_NULL:
    case WIFI_MAC_DATA_NULL_CFACK:
    case WIFI_MAC_DATA_NULL_CFPOLL:
    case WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA:
    case WIFI_MAC_QOSDATA_CFACK:
    case WIFI_MAC_QOSDATA_CFPOLL:
    case WIFI_MAC_QOSDATA_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL:
    case WIFI_MAC_QOSDATA_NULL_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL:
      break;
    }
}

uint32_t
WifiMacHeader::GetSerializedSize (void) const
{
  return GetSize ();
}

void
WifiMacHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteHtolsbU16 (GetFrameControl ());
  i.WriteHtolsbU16 (m_duration);
  WriteTo (i, m_addr1);
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      WriteTo (i, m_addr2);
      WriteTo (i, m_addr3);
      i.WriteHtolsbU16 (GetSequenceControl ());
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_RTS:
          WriteTo (i, m_addr2);
          break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
          break;
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
          WriteTo (i, m_addr2);
          break;
        case SUBTYPE_CTL_EXTENSION:
          switch (m_ctrlFrameExtension)
            {
            case SUBTYPE_CTL_EXTENSION_POLL:
            case SUBTYPE_CTL_EXTENSION_SPR:
            case SUBTYPE_CTL_EXTENSION_GRANT:
            case SUBTYPE_CTL_EXTENSION_DMG_CTS:
            case SUBTYPE_CTL_EXTENSION_GRANT_ACK:
            case SUBTYPE_CTL_EXTENSION_SSW:
            case SUBTYPE_CTL_EXTENSION_SSW_FBCK:
            case SUBTYPE_CTL_EXTENSION_SSW_ACK:
              WriteTo(i, m_addr2); // TA Address Field.
              break;
            case SUBTYPE_CTL_EXTENSION_DMG_DTS:
              break;
            }
          break;
        default:
          //NOTREACHED
          NS_ASSERT (false);
          break;
        }
      break;
    case TYPE_DATA:
      {
        WriteTo (i, m_addr2);
        WriteTo (i, m_addr3);
        i.WriteHtolsbU16 (GetSequenceControl ());
        if (m_ctrlToDs && m_ctrlFromDs)
          {
            WriteTo (i, m_addr4);
          }
        if (m_ctrlSubtype & 0x08)
          {
            i.WriteHtolsbU16 (GetQosControl ());
          }
      } break;
    case TYPE_Extension:
      break;
    default:
      //NOTREACHED
      NS_ASSERT (false);
      break;
    }
}

uint32_t
WifiMacHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint16_t frame_control = i.ReadLsbtohU16 ();
  SetFrameControl (frame_control);
  m_duration = i.ReadLsbtohU16 ();
  ReadFrom (i, m_addr1);
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      ReadFrom (i, m_addr2);
      ReadFrom (i, m_addr3);
      SetSequenceControl (i.ReadLsbtohU16 ());
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_RTS:
          ReadFrom (i, m_addr2);
          break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
          break;
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
          ReadFrom (i, m_addr2);
          break;
        case SUBTYPE_CTL_EXTENSION:
          switch (m_ctrlFrameExtension)
            {
            case SUBTYPE_CTL_EXTENSION_POLL:
            case SUBTYPE_CTL_EXTENSION_SPR:
            case SUBTYPE_CTL_EXTENSION_GRANT:
            case SUBTYPE_CTL_EXTENSION_DMG_CTS:
            case SUBTYPE_CTL_EXTENSION_GRANT_ACK:
            case SUBTYPE_CTL_EXTENSION_SSW:
            case SUBTYPE_CTL_EXTENSION_SSW_FBCK:
            case SUBTYPE_CTL_EXTENSION_SSW_ACK:
              ReadFrom(i, m_addr2); // TA Address Field.
              break;
            case SUBTYPE_CTL_EXTENSION_DMG_DTS:
              break;
            }
          break;
        }
      break;
    case TYPE_DATA:
      ReadFrom (i, m_addr2);
      ReadFrom (i, m_addr3);
      SetSequenceControl (i.ReadLsbtohU16 ());
      if (m_ctrlToDs && m_ctrlFromDs)
        {
          ReadFrom (i, m_addr4);
        }
      if (m_ctrlSubtype & 0x08)
        {
          SetQosControl (i.ReadLsbtohU16 ());
        }
      break;
    case TYPE_Extension:
      break;
    }
  return i.GetDistanceFrom (start);
}

/* 802.11ad Functions */

void
WifiMacHeader::SetPacketType (PacketType type)
{
  m_brpPacketType = type;
}

PacketType
WifiMacHeader::GetPacketType (void) const
{
  return m_brpPacketType;
}

void
WifiMacHeader::SetTrainngFieldLength (uint8_t length)
{
  m_trainingFieldLength = length;
}

uint8_t
WifiMacHeader::GetTrainngFieldLength (void) const
{
  return m_trainingFieldLength;
}

void
WifiMacHeader::RequestBeamRefinement (void)
{
  m_beamRefinementRequired = true;
}

void
WifiMacHeader::DisableBeamRefinement (void)
{
  m_beamRefinementRequired = false;
}

bool
WifiMacHeader::IsBeamRefinementRequested (void) const
{
  return (m_beamRefinementRequired == true);
}

void
WifiMacHeader::RequestBeamTracking (void)
{
  m_beamTrackingRequired = true;
}

void
WifiMacHeader::DisableBeamTracking (void)
{
  m_beamTrackingRequired = false;
}

bool
WifiMacHeader::IsBeamTrackingRequested (void) const
{
  return (m_beamTrackingRequired == true);
}

} //namespace ns3
