/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef DMG_AP_WIFI_MAC_H
#define DMG_AP_WIFI_MAC_H

#include "ns3/random-variable-stream.h"

#include "amsdu-subframe-header.h"
#include "dmg-beacon-dca.h"
#include "dmg-wifi-mac.h"
#include "dmg-wifi-scheduler.h"

namespace ns3 {

#define TU                      MicroSeconds (1024)     /* Time Unit defined in 802.11 std */
#define aMaxBIDuration          TU * 1024               /* Maximum BI Duration Defined in 802.11ad */
#define aMinChannelTime         aMaxBIDuration          /* Minimum Channel Time for Clustering */
#define aMinSSSlotsPerABFT      1                       /* Minimum Number of Sector Sweep Slots Per A-BFT */
#define aSSFramesPerSlot        8                       /* Number of SSW Frames per Sector Sweep Slot */
#define aDMGPPMinListeningTime  150                     /* The minimum time between two adjacent SPs with the same source or destination AIDs */

/**
 * \brief Wi-Fi DMG AP state machine
 * \ingroup wifi
 *
 * Handle association, dis-association and authentication,
 * of DMG STAs within an infrastructure DMG BSS.
 */
class DmgApWifiMac : public DmgWifiMac
{
public:
  static TypeId GetTypeId (void);

  DmgApWifiMac ();
  virtual ~DmgApWifiMac ();

  /**
   * Get Association Identifier (AID).
   * \return The AID of the station.
   */
  virtual uint16_t GetAssociationID (void);
  /**
   * \param stationManager the station manager attached to this MAC.
   */
  virtual void SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> stationManager);

  /**
   * \param linkUp the callback to invoke when the link becomes up.
   */
  virtual void SetLinkUpCallback (Callback<void> linkUp);
  /**
   * \param packet the packet to send.
   * \param to the address to which the packet should be sent.
   *
   * The packet should be enqueued in a tx queue, and should be
   * dequeued as soon as the channel access function determines that
   * access is granted to this MAC.
   */
  virtual void Enqueue (Ptr<const Packet> packet, Mac48Address to);
  /**
   * \param packet the packet to send.
   * \param to the address to which the packet should be sent.
   * \param from the address from which the packet should be sent.
   *
   * The packet should be enqueued in a tx queue, and should be
   * dequeued as soon as the channel access function determines that
   * access is granted to this MAC. The extra parameter "from" allows
   * this device to operate in a bridged mode, forwarding received
   * frames without altering the source address.
   */
  virtual void Enqueue (Ptr<const Packet> packet, Mac48Address to, Mac48Address from);
  virtual bool SupportsSendFrom (void) const;
  /**
   * \param address the current address of this MAC layer.
   */
  virtual void SetAddress (Mac48Address address);
  /**
   * \param dmgScheduler the dmg scheduling algorithm for this MAC.
   */
  void SetScheduler (Ptr<DmgWifiScheduler> dmgScheduler);
  /**
   * \return the dmg scheduling algorithm of this MAC.
   */
  Ptr<DmgWifiScheduler> GetScheduler (void) const;
  /**
   * \param interval the interval between two beacon transmissions.
   */
  void SetBeaconInterval (Time interval);
  /**
   * \return the interval between two beacon transmissions.
   */
  Time GetBeaconInterval (void) const;
  /**
   * Get Datas Transmission Interval Duration
   * \return The duration of DTI.
   */
  Time GetDTIDuration (void) const;
  /**
   * Get the amount of the remaining time in the DTI access period.
   * \return The remaining time in the DTI.
   */
  Time GetDTIRemainingTime (void) const;
  /**
   * \param interval the interval.
   */
  void SetBeaconTransmissionInterval (Time interval);
  /**
   * \return the interval between two beacon transmissions.
   */
  Time GetBeaconTransmissionInterval (void) const;
  /**
   * \param periodicity the A-BFT periodicity.
   */
  void SetAbftPeriodicity (uint8_t periodicity);
  /**
   * \return the A-BFT periodicity.
   */
  uint8_t GetAbftPeriodicity (void) const;
  /**
   * ContinueBeamformingInDTI
   */
  void ContinueBeamformingInDTI (void);
  /**
   * Initiate dynamic channel access procedure in the following BI.
   */
  void InitiateDynamicAllocation (void);
  /**
   * Initiate Polling Period with the specified length.
   * \param ppLength The length of the polling period.
   */
  void InitiatePollingPeriod (Time ppLength);
  /**
   * Get the duration of a polling period.
   * \param polledStationsCount The number of stations to be polled.
   * \return The corresponding polling period duration.
   */
  Time GetPollingPeriodDuration (uint8_t polledStationsCount);
  /**
   * Get associated station AID from its MAC address.
   * \param address The MAC address of the associated station.
   * \return The AID of the associated station.
   */
  uint8_t GetStationAid (Mac48Address address) const;
  /**
   * Get associated station MAC address from AID.
   * \param aid The AID of the associated station.
   * \return The MAC address of the assoicated station.
   */
  Mac48Address GetStationAddress (uint8_t aid) const;
  /**
   * Send DMG Add TS Response to DMG STA.
   * \param to The MAC address of the DMG STA which sent the DMG ADDTS Request.
   * \param code The status of the allocation.
   * \param delayElem The TS Delay element.
   * \param elem The TSPEC element.
   */
  void SendDmgAddTsResponse (Mac48Address to, StatusCode code, TsDelayElement &delayElem, DmgTspecElement &elem);
  /**
   * Get list of dynamic allocation info in the SPRs received during polling period.
   * \return
   */
  AllocationDataList GetSprList (void) const;
  /**
   * Add new dynamic allocation info field to the list of allocations to be announced in the Grant Period.
   * \param info The dynamic allocation information field.
   */
  void AddGrantData (AllocationData info);
  /**
   * Send Directional Channel Quality request.
   * \param to The MAC address of the DMG STA.
   * \param numOfRepts Number of measurements repetitions.
   * \param element The Directional Channel Quality Request Information Element.
   */
  void SendDirectionalChannelQualityRequest (Mac48Address to, uint16_t numOfRepts,
                                             Ptr<DirectionalChannelQualityRequestElement> element);

  /**
   * Start DMG AP Operation by transmitting Beaconing.
   */
  void StartAccessPoint (void);

protected:
  friend class DmgBeaconDca;
  friend class DmgWifiScheduler;

  Time GetBTIRemainingTime (void) const;
  /**
   * Start monitoring Beacon SP for DMG Beacons.
   */
  void StartMonitoringBeaconSP (uint8_t beaconSPIndex);
  /**
   * End monitoring Beacon SP for DMG Beacons.
   * \param beaconSPIndex The index of the Beacon SP.
   */
  void EndMonitoringBeaconSP (uint8_t beaconSPIndex);
  /**
   * End channel monitoring for DMG Beacons during Beacon SPs.
   * \param clusterID The MAC address of the cluster.
   */
  void EndChannelMonitoring (Mac48Address clusterID);
  /**
   * Start Syn Beacon Interval.
   */
  void StartSynBeaconInterval (void);
  /**
   * Return the DMG capability of the current PCP/AP.
   * \return the DMG capabilities the PCP/AP supports.
   */
  Ptr<DmgCapabilities> GetDmgCapabilities (void) const;
  /**
   * Dmg Scheduler to be used by the PCP/AP.
   */
  Ptr<DmgWifiScheduler> m_dmgScheduler;

private:
  virtual void DoDispose (void);
  virtual void DoInitialize (void);

  void StartBeaconInterval (void);
  void EndBeaconInterval (void);
  void StartBeaconTransmissionInterval (void);
  void StartAssociationBeamformTraining (void);
  void StartAnnouncementTransmissionInterval (void);
  void StartDataTransmissionInterval (void);
  void FrameTxOk (const WifiMacHeader &hdr);
  virtual void BrpSetupCompleted (Mac48Address address);
  virtual void NotifyBrpPhaseCompleted (void);
  virtual void Receive (Ptr<Packet> packet, const WifiMacHeader *hdr);

  /**
   * Start Beacon Header Interval (BHI).
   */
  void StartBeaconHeaderInterval (void);
  /**
   * The packet we sent was successfully received by the receiver
   * (i.e. we received an ACK from the receiver). If the packet
   * was an association response to the receiver, we record that
   * the receiver is now associated with us.
   *
   * \param hdr the header of the packet that we successfully sent
   */
  virtual void TxOk (Ptr<const Packet> packet, const WifiMacHeader &hdr);
  /**
   * The packet we sent was not successfully received by the receiver
   * (i.e. we did not receive an ACK from the receiver). If the packet
   * was an association response to the receiver, we record that
   * the receiver is not associated with us yet.
   *
   * \param hdr the header of the packet that we failed to sent
   */
  virtual void TxFailed (const WifiMacHeader &hdr);
  /**
   * This method is called to de-aggregate an A-MSDU and forward the
   * constituent packets up the stack. We override the WifiMac version
   * here because, as an AP, we also need to think about redistributing
   * to other associated STAs.
   *
   * \param aggregatedPacket the Packet containing the A-MSDU.
   * \param hdr a pointer to the MAC header for \c aggregatedPacket.
   */
  virtual void DeaggregateAmsduAndForward (Ptr<Packet> aggregatedPacket, const WifiMacHeader *hdr);
  /**
   * Get MultiBand Element corresponding to this DMG STA.
   * \return Pointer to the MultiBand element.
   */
  Ptr<MultiBandElement> GetMultiBandElement (void) const;
  /**
   * Start A-BFT Sector Sweep Slot.
   */
  void StartSectorSweepSlot (void);
  /**
   * Establish BRP Setup Subphase
   */
  void DoBrpSetupSubphase (void);
  /**
   * Forward the packet down to DCF/EDCAF (enqueue the packet). This method
   * is a wrapper for ForwardDown with traffic id.
   *
   * \param packet the packet we are forwarding to DCF/EDCAF
   * \param from the address to be used for Address 3 field in the header
   * \param to the address to be used for Address 1 field in the header
   */
  void ForwardDown (Ptr<const Packet> packet, Mac48Address from, Mac48Address to);
  /**
   * Forward the packet down to DCF/EDCAF (enqueue the packet).
   *
   * \param packet the packet we are forwarding to DCF/EDCAF
   * \param from the address to be used for Address 3 field in the header
   * \param to the address to be used for Address 1 field in the header
   * \param tid the traffic id for the packet
   */
  void ForwardDown (Ptr<const Packet> packet, Mac48Address from, Mac48Address to, uint8_t tid);
  /**
   * Forward a probe response packet to the DCF. The standard is not clear on the correct
   * queue for management frames if QoS is supported. We always use the DCF.
   *
   * \param to the MAC address of the STA we are sending a probe response to.
   */
  void SendProbeResp (Mac48Address to);
  /**
   * Forward an association response packet to the DCF. The standard is not clear on the correct
   * queue for management frames if QoS is supported. We always use the DCF.
   *
   * \param to the MAC address of the STA we are sending an association response to.
   * \param success indicates whether the association was successful or not.
   * \return If success return the AID of the station, otherwise 0.
   */
  uint16_t SendAssocResp (Mac48Address to, bool success);
  /**
   * Get the duration of a polling period.
   * \param pollFrameTxTime The TX time of a poll frame.
   * \param sprFrameTxTime The TX time of an SPR frame.
   * \param polledStationsCount The number of stations to be polled.
   * \return The corresponding polling period duration.
   */
  Time GetPollingPeriodDuration (Time pollFrameTxTime, Time sprFrameTxTime, uint8_t polledStationsCount);
  /**
   * Start Polling Period for dynamic allocation of service period.
   */
  void StartPollingPeriod (void);
  /**
   * Polling Period for dynamic allocation of service period has completed.
   */
  void PollingPeriodCompleted (void);
  /**
   * Start Grant Period for dynamic allocation of service period.
   */
  void StartGrantPeriod (void);
  /**
   * Send Grant frame(s) for DMG STAs during GP period.
   */
  void SendGrantFrames (void);
  /**
   * Grant Period for dynamic allocation of service period has completed.
   */
  void GrantPeriodCompleted (void);
  /**
   * GetOffsetOfSprTransmission
   * \param index
   * \return
   */
  Time GetOffsetOfSprTransmission (uint32_t index);
  /**
   * Get Duration Of ongoing Poll Transmission.
   * \return
   */
  Time GetDurationOfPollTransmission (void);
  /**
   * Get Poll Response Offset in MicroSeconds.
   * \return The response offset value to be used in the Poll Frame.
   */
  Time GetResponseOffset (void);
  /**
   * Get Poll frame header duration.
   * \return The duration value of the Poll frame in MicroSeconds
   */
  Time GetPollFrameDuration (void);
  /**
   * Send Poll frame to the specified DMG STA.
   * \param to The MAC address of the DMG STA.
   */
  void SendPollFrame (Mac48Address to);
  /**
   * Send Grant frame to a specified DMG STA.
   * \param to The MAC address of the DMG STA.
   * \param duration The duration of the grant frame as described in 802.11ad-2012 8.3.1.13.
   * \param info The dynamic allocation info field.
   */
  void SendGrantFrame (Mac48Address to, Time duration, DynamicAllocationInfoField &info, BF_Control_Field &bf);
  /**
   * Send directional Announce frame to DMG STA.
   * \param to The MAC address of the DMG STA.
   */
  void SendAnnounceFrame (Mac48Address to);
  /**
   * Get DMG Operation element.
   * \return Pointer to the DMG Operation element.
   */
  Ptr<DmgOperationElement> GetDmgOperationElement (void) const;
  /**
   * Get Next DMG ATI Information element.
   * \return The DMG ATI information element.
   */
  Ptr<NextDmgAti> GetNextDmgAtiElement (void) const;
  /**
   * Get Extended Schedule element.
   * \return The extended schedule element.
   */
  Ptr<ExtendedScheduleElement> GetExtendedScheduleElement (void) const;
  /**
   * Calculate BTI access period variables.
   */
  void CalculateBTIVariables (void);
  /**
   * Send One DMG Beacon frame with the provided arguments.
   */
  void SendOneDMGBeacon (void);
  /**
   * Get Beacon Header Interval Duration
   * \return The duration of BHI.
   */
  Time GetBHIDuration (void) const;
  /**
   * \return the next Association ID to be allocated by the DMG PCP/AP.
   */
  uint16_t GetNextAssociationId (void);

private:
  /** DMG PCP/AP Power Status **/
  bool m_startedAP;                     //!< Flag to indicate whether we started DMG AP.

  /** Assoication Information **/
  std::map<uint16_t, Mac48Address> m_staList;//!< Map of all stations currently associated to the DMG PCP/AP with their association ID.

  /** BTI Period Variables **/
  Ptr<DmgBeaconDca> m_beaconDca;        //!< Dedicated DcaTxop for DMG Beacons.
  EventId m_beaconEvent;		//!< Event to generate one DMG Beacon.
  Time m_btiStarted;                    //!< The time at which we started BTI access period.
  Time m_dmgBeaconDuration;             //!< Exact DMG beacon duration.
  Time m_dmgBeaconDurationUs;           //!< DMG BEacon Duration in Microseconds.
  Time m_nextDmgBeaconDelay;            //!< DMG Beacon transmission delay due to difference in clock.
  Time m_btiDuration;                   //!< The length of the Beacon Transmission Interval (BTI).
  bool m_beaconRandomization;           //!< Flag to indicate whether we want to randomize selection of DMG Beacon at each BI.
  Ptr<RandomVariableStream> m_beaconJitter; //!< RandomVariableStream used to randomize the time of the first DMG beacon.
  bool m_enableBeaconJitter;            //!< Flag whether the first beacon should be generated at random time.
  bool m_allowBeaconing;                //!< Flag to indicate whether we want to start Beaconing upon initialization.
  bool m_announceDmgCapabilities;       //!< Flag to indicate whether we announce DMG Capabilities in DMG Beacons.
  bool m_announceOperationElement;      //!< Flag to indicate whether we transmit DMG operation element in DMG Beacons.
  bool m_scheduleElement;               //!< Flag to indicate whether we transmit Extended Schedule element in DMG Beacons.
  bool m_isABFTResponderTXSS;           //!< Flag to indicate whether the responder in A-BFT is TxSS or RxSS.
  std::vector<Mac48Address> m_beamformingInDTI; //!< List of the stations to train in DTI because beamforming is not completed in BTI.

  /** DMG PCP/AP Clustering **/
  bool m_enableDecentralizedClustering; //!< Flag to inidicate if decentralized clustering is enabled.
  bool m_enableCentralizedClustering;   //!< Flag to indicate if centralized clustering is enabled.
  Mac48Address m_ClusterID;             //!< The ID of the cluster.
  uint8_t m_clusterMaxMem;              //!< The maximum number of cluster members.
  uint8_t m_beaconSPDuration;           //!< The size of a Beacon SP in MicroSeconds.
  ClusterMemberRole m_clusterRole;      //!< The role of the node in the current cluster.
  typedef std::map<uint8_t, bool> BEACON_SP_STATUS_MAP;                 //!< Typedef for mapping the status of each BeaconSP.
  typedef BEACON_SP_STATUS_MAP::const_iterator BEACON_SP_STATUS_MAP_CI; //!< Typedef for const iterator through BeaconSP.
  BEACON_SP_STATUS_MAP m_spStatus;      //!< The status of each Beacon SP in the monitor period.
  bool m_monitoringChannel;             //!< Flag to indicate if we have started monitoring the channel for cluster formation.
  bool m_beaconReceived;                //!< Flag to indicate if we have received beacon during BeaconSP.
  uint8_t m_selectedBeaconSP;           //!< Selected Beacon SP for DMG Transmission.
  Time m_clusterTimeInterval;           //!< The interval between two consectuve Beacon SPs.
  Time m_channelMonitorTime;            //!< The channel monitor time.
  Time m_startedMonitoringChannel;      //!< The time we started monitoring channel for DMG Beacons.
  Time m_clusterBeaconSPDuration;       //!< The duration of the Beacon SP.

  TracedCallback<Mac48Address, uint8_t> m_joinedCluster;  //!< The PCP/AP has joined a cluster.
  /**
   * TracedCallback signature for DTI access period start event.
   *
   * \param clusterID The MAC address of the cluster.
   * \param beaconSP The index of the BeaconSP.
   */
  typedef void (* JoinedClusterCallback)(Mac48Address clusterID, uint8_t index);

  /** A-BFT Access Period Variables **/
  uint8_t m_abftPeriodicity;            //!< The periodicity of the A-BFT in DMG Beacon.
  EventId m_sswFbckEvent;               //!< Event related to sending SSW FBCK.
  /* Ensure only one DMG STA is communicating with us during single A-BFT slot */
  bool m_receivedOneSSW;                //!< Flag to indicate if we received SSW Frame during SSW-Slot in A-BFT period.
  bool m_abftCollision;                 //!< Flad to indicate if we experienced any collision in the current A-BFT slot.
  Mac48Address m_peerAbftStation;       //!< The MAC address of the station we received SSW from.
  uint8_t m_remainingSlots;
  Time m_atiStartTime;                  //!< The start time of ATI Period.

  /** BRP Phase Variables **/
  typedef std::map<Mac48Address, bool> STATION_BRP_MAP;
  STATION_BRP_MAP m_stationBrpMap;      //!< Map to indicate if a station has conducted BRP Phase or not.

  /**
   * Type definition for storing IEs of the associated stations.
   */
  typedef std::map<Mac48Address, WifiInformationElementMap> AssociatedStationsInformation;
  typedef AssociatedStationsInformation::iterator AssociatedStationsInformationI;
  typedef AssociatedStationsInformation::const_iterator AssociatedStationsInformationCI;
  AssociatedStationsInformation m_associatedStationsInfoByAddress;
  std::map<uint16_t, WifiInformationElementMap> m_associatedStationsInfoByAid;

  /** Beacon Interval **/
  TracedCallback<Mac48Address, Time, Time, Time> m_biStarted;  //!< Trace Callback for starting new Beacon Interval.
  /**
   * TracedCallback signature for the BI start.
   *
   * \param address The MAC address of the PCP/AP.
   * \param biDuration The duration of the BI.
   * \param bhiDuration The duration of the BHI.
   * \param atiDuration The duration of the ATI.
   */
  typedef void (* BiStartedCallback)(Mac48Address address, Time biDuration, Time bhiDuration, Time atiDuration);

  /** Traffic Stream Allocation **/
  TracedCallback<Mac48Address, DmgTspecElement> m_addTsRequestReceived;   //!< DMG ADDTS Request received.
  /**
   * TracedCallback signature for receiving ADDTS Request.
   *
   * \param address The MAC address of the requesting station.
   * \param element The TSPEC information element.
   */
  typedef void (* AddTsRequestReceivedCallback)(Mac48Address address, DmgTspecElement element);

  /** Traffic Stream Deletion **/
  TracedCallback<Mac48Address, DmgAllocationInfo> m_delTsRequestReceived;  //!< DELTS Request received.
  /**
   * TracedCallback signature for receiving DELTS Request.
   *
   * \param address The MAC address of the requesting station.
   * \param element The TSPEC information element.
   */
  typedef void (* DelTsRequestReceivedCallback)(Mac48Address address, DmgAllocationInfo info);

  /** Dynamic Allocation of Service Period **/
  bool m_initiateDynamicAllocation;                 //!< Flag to indicate whether to commence PP phase at the beginning of the DTI.
  uint32_t m_polledStationsCount;                   //!< Number of DMG STAs to poll.
  uint32_t m_polledStationIndex;                    //!< The index of the current station being polled.
  uint32_t m_grantIndex;                            //!< The index of the current Grant frame.
  Time m_responseOffset;                            //!< Response offset in the current Poll frame.
  Time m_pollFrameTxTime;                           //!< The Tx duration of Poll frame.
  Time m_sprFrameTxTime;                            //!< The Tx duration of SPR frame.
  Time m_grantFrameTxTime;                          //!< The Tx duration of Grant frame.
  std::vector<Mac48Address> m_pollStations;         //!< List of stations to poll during the PP phase.
  AllocationDataList m_sprList;                     //!< List of info received in the SPRs.
  AllocationDataList m_grantList;                   //!< List of allocation info to be assigned during the Grant Period.
  TracedCallback<Mac48Address> m_ppCompleted;       //!< Polling period has ended up.
  TracedCallback<Mac48Address> m_gpCompleted;       //!< Grant period has ended up.
  DynamicAllocationInfoField n_grantDynamicInfo;    //!< The dynamic information of the current Grant frame.

  /* Channel Quality Measurement */
  TracedCallback<Mac48Address, Ptr<DirectionalChannelQualityReportElement> > m_qualityReportReceived;   //!< Received Quality Report
  /**
   * TracedCallback signature for receiving Channel Quality Report.
   *
   * \param address The MAC address of the station.
   * \param element The Directional Quality Report information element.
   */
  typedef void (* QualityReportReceivedCallback)(Mac48Address address, Ptr<DirectionalChannelQualityReportElement> element);

};

} // namespace ns3

#endif /* DMG_AP_WIFI_MAC_H */
