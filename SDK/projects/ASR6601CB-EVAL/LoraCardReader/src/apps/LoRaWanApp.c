#include "apps.h"
#include <stdio.h>
#include "utilities.h"
#include "LoRaMac.h"
#include "LoRaMacTest.h"
#include "Commissioning.h"
#include "rtc-board.h"

#define ACTIVE_REGION LORAMAC_REGION_EU868


#ifndef ACTIVE_REGION

#warning "No active region defined, LORAMAC_REGION_CN470 will be used as default."

#define ACTIVE_REGION LORAMAC_REGION_CN470

#endif

/*!
 * Default datarate
 */
#define LORAWAN_DEFAULT_DATARATE                    DR_0

#define LORAWAN_APP_DATA_BUFF_SIZE                  64

typedef enum eDeviceSendType_t
{
		DEVICE_SEND_NOT,
    DEVICE_SEND_PING,
    DEVICE_SEND_CARD,	
} eDeviceSendType;

typedef struct sTxInfo_t
{
	uint8_t Port;
	uint8_t Size;
	uint8_t Buff[LORAWAN_APP_DATA_BUFF_SIZE];
	bool IsConfirmed;
} sTxInfo;

typedef enum eLoRaWanState_t
{
    DEVICE_STATE_INIT,
    DEVICE_STATE_JOIN,
    DEVICE_STATE_SEND,
    DEVICE_STATE_CYCLE,
    DEVICE_STATE_IDLE
}eLoRaWanState;

typedef struct
{
	eLoRaWanState State;
	eDeviceSendType Type;
	bool NextTx;
	uint16_t TryPing;
	uint16_t TrySend;
	uint32_t PingTimeout;
} sLoraWanDrv;

static sTxInfo TxInfo;
static sDevSetting DevSetting;
static sLoraWanDrv LoraWanDrv;

/*!
 * Timer to handle the application data transmission duty cycle
 */
static TimerEvent_t PingPacketTimer;

static uint8_t PreparePingFrame(uint8_t *Buff)
{
	sCardReaderInfo Info;
	uint8_t pBuff = 0;
	uint32_t Time;
	
	CardReaderGetInfo(&Info);
	Time = (uint32_t)(RtcGetTimerValue() / 1000);
	Buff[pBuff++] = 0x01;
	Buff[pBuff++] = Info.isInit;
	Buff[pBuff++] = Info.ver;
	Buff[pBuff++] = (uint8_t)(Time >> 24);
	Buff[pBuff++] = (uint8_t)(Time >> 16);
	Buff[pBuff++] = (uint8_t)(Time >> 8);
	Buff[pBuff++] = (uint8_t)(Time >> 0);	
	
	return pBuff;
}

/*!
 * \brief   Prepares the payload of the frame
 */
static void PrepareTxFrame( eDeviceSendType Type )
{
		sCardInfo CardInfo;
	
		switch (Type)
		{
			case DEVICE_SEND_PING:
				SYSLOG_I("Send ping packet");
				TxInfo.Port = DevSetting.PingDefs.Port;
				if (DevSetting.PingDefs.NbTrials == 0)
				{
					TxInfo.IsConfirmed = false;			
				}
				else
				{
					TxInfo.IsConfirmed = true;		
				}
				TxInfo.Size = PreparePingFrame(TxInfo.Buff);
				break;
			case DEVICE_SEND_CARD:
				SYSLOG_I("Send card info packet");
				TxInfo.Port = DevSetting.SendDefs.Port;
				TxInfo.IsConfirmed = true;
				CardInfo = SendBufferPop(true);
				TxInfo.Size = PrepareCardInfoFrame(CardInfo, TxInfo.Buff);
				break;	
			default:
				TxInfo.Size = 0;
				break;
		}
		SYSDUMP_D("Data: ", TxInfo.Buff, TxInfo.Size);
		SYSLOG_D("TxDataPort=%d, IsTxConfirmed=%d",TxInfo.Port,TxInfo.IsConfirmed);
}

/*!
 * \brief   Prepares the payload of the frame
 *
 * \retval  [0: frame could be send, 1: error]
 */
static bool SendFrame( void )
{
    McpsReq_t mcpsReq;
    LoRaMacTxInfo_t txInfo;

    if( LoRaMacQueryTxPossible( TxInfo.Size, &txInfo ) != LORAMAC_STATUS_OK )
    {
        // Send empty frame in order to flush MAC commands
        mcpsReq.Type = MCPS_UNCONFIRMED;
        mcpsReq.Req.Unconfirmed.fBuffer = NULL;
        mcpsReq.Req.Unconfirmed.fBufferSize = 0;
        mcpsReq.Req.Unconfirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
    }
    else
    {
        if( TxInfo.IsConfirmed == false )
        {
            mcpsReq.Type = MCPS_UNCONFIRMED;
            mcpsReq.Req.Unconfirmed.fPort = TxInfo.Port;
            mcpsReq.Req.Unconfirmed.fBuffer = TxInfo.Buff;
            mcpsReq.Req.Unconfirmed.fBufferSize = TxInfo.Size;
            mcpsReq.Req.Unconfirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
        }
        else
        {
            mcpsReq.Type = MCPS_CONFIRMED;
            mcpsReq.Req.Confirmed.fPort = TxInfo.Port;
            mcpsReq.Req.Confirmed.fBuffer = TxInfo.Buff;
            mcpsReq.Req.Confirmed.fBufferSize = TxInfo.Size;
            mcpsReq.Req.Confirmed.NbTrials = 1;
            mcpsReq.Req.Confirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
        }
    }

    if( LoRaMacMcpsRequest( &mcpsReq ) == LORAMAC_STATUS_OK )
    {
        return false;
    }
    
    return true;
}

/*!
 * \brief Function executed on TxNextPacket Timeout event
 */
static void OnTxPingPacketTimerEvent( void )
{
    MibRequestConfirm_t mibReq;
    LoRaMacStatus_t status;

    TimerStop( &PingPacketTimer );

    mibReq.Type = MIB_NETWORK_JOINED;
    status = LoRaMacMibGetRequestConfirm( &mibReq );

    if( status == LORAMAC_STATUS_OK )
    {
        if( mibReq.Param.IsNetworkJoined == true )
        {
						LoraWanDrv.Type = DEVICE_SEND_PING;
            LoraWanDrv.State = DEVICE_STATE_SEND;
            LoraWanDrv.NextTx = true;
        }
        else
        {
            // Network not joined yet. Try to join again
            MlmeReq_t mlmeReq;
            mlmeReq.Type = MLME_JOIN;
            mlmeReq.Req.Join.DevEui = DevSetting.OTAKeys.deveui;
            mlmeReq.Req.Join.AppEui = DevSetting.OTAKeys.appeui;
            mlmeReq.Req.Join.AppKey = DevSetting.OTAKeys.appkey;

            if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
            {
                LoraWanDrv.State = DEVICE_STATE_IDLE;
            }
            else
            {
                LoraWanDrv.State = DEVICE_STATE_CYCLE;
            }
        }
    }
}

/*!
 * \brief Вызывается если есть данные в буфере
 */
static void OnTxCardInfoPacketEvent( void )
{
    MibRequestConfirm_t mibReq;
    LoRaMacStatus_t status;

    TimerStop( &PingPacketTimer );	
	
    mibReq.Type = MIB_NETWORK_JOINED;
    status = LoRaMacMibGetRequestConfirm( &mibReq );

    if( status == LORAMAC_STATUS_OK )
    {
        if( mibReq.Param.IsNetworkJoined == true )
        {
						LoraWanDrv.Type = DEVICE_SEND_CARD;
            LoraWanDrv.State = DEVICE_STATE_SEND;
            LoraWanDrv.NextTx = true;
        }
        else
        {
						LoraWanDrv.PingTimeout = DevSetting.PingDefs.Period + randr( 0, (DevSetting.PingDefs.Period / 10) );
						LoraWanDrv.State = DEVICE_STATE_CYCLE;
        }
    }	
}

/*!
 * \brief   MCPS-Confirm event function
 *
 * \param   [IN] mcpsConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void McpsConfirm( McpsConfirm_t *mcpsConfirm )
{
    if( mcpsConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
    {
        switch( mcpsConfirm->McpsRequest )
        {
            case MCPS_UNCONFIRMED:
            {
                // Check Datarate
                // Check TxPower
                break;
            }
            case MCPS_CONFIRMED:
            {
								SendBufferInc();
                // Check Datarate
                // Check TxPower
                // Check AckReceived
                // Check NbTrials
                break;
            }
            case MCPS_PROPRIETARY:
            {
                break;
            }
            default:
                break;
        }
    }
    LoraWanDrv.NextTx = true;
}

/*!
 * \brief   MCPS-Indication event function
 *
 * \param   [IN] mcpsIndication - Pointer to the indication structure,
 *               containing indication attributes.
 */
static void McpsIndication( McpsIndication_t *mcpsIndication )
{
    if( mcpsIndication->Status != LORAMAC_EVENT_INFO_STATUS_OK )
    {
        return;
    }

    SYSLOG_D( "receive data: rssi = %d, snr = %d, datarate = %d", mcpsIndication->Rssi, (int)mcpsIndication->Snr,
                 (int)mcpsIndication->RxDatarate);
    switch( mcpsIndication->McpsIndication )
    {
        case MCPS_UNCONFIRMED:
        {
            break;
        }
        case MCPS_CONFIRMED:
        {
            break;
        }
        case MCPS_PROPRIETARY:
        {
            break;
        }
        case MCPS_MULTICAST:
        {
            break;
        }
        default:
            break;
    }

    // Check Multicast
    // Check Port
    // Check Datarate
    // Check FramePending
    if( mcpsIndication->FramePending == true )
    {
        // The server signals that it has pending data to be sent.
        // We schedule an uplink as soon as possible to flush the server.
        OnTxPingPacketTimerEvent( );
    }
    // Check Buffer
    // Check BufferSize
    // Check Rssi
    // Check Snr
    // Check RxSlot
    if( mcpsIndication->RxData == true )
    {
    }
}

/*!
 * \brief   MLME-Confirm event function
 *
 * \param   [IN] mlmeConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void MlmeConfirm( MlmeConfirm_t *mlmeConfirm )
{
    switch( mlmeConfirm->MlmeRequest )
    {
        case MLME_JOIN:
        {
            if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
            {
                SYSLOG_I("joined\r\n");
                // Status is OK, node has joined the network
                LoraWanDrv.State = DEVICE_STATE_SEND;
            }
            else
            {
                MlmeReq_t mlmeReq;
                
                SYSLOG_W("join failed\r\n");
                // Join was not successful. Try to join again
                mlmeReq.Type = MLME_JOIN;
                mlmeReq.Req.Join.DevEui = DevSetting.OTAKeys.deveui;;
                mlmeReq.Req.Join.AppEui = DevSetting.OTAKeys.appeui;;
                mlmeReq.Req.Join.AppKey = DevSetting.OTAKeys.appkey;;
                mlmeReq.Req.Join.NbTrials = 8;

                if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
                {
                    LoraWanDrv.State = DEVICE_STATE_IDLE;
                }
                else
                {
                    LoraWanDrv.State = DEVICE_STATE_CYCLE;
                }
            }
            break;
        }
        case MLME_LINK_CHECK:
        {
            if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
            {
                // Check DemodMargin
                // Check NbGateways
            }
            break;
        }
        default:
            break;
    }
    LoraWanDrv.NextTx = true;
}

/*!
 * \brief   MLME-Indication event function
 *
 * \param   [IN] mlmeIndication - Pointer to the indication structure.
 */
static void MlmeIndication( MlmeIndication_t *mlmeIndication )
{
    switch( mlmeIndication->MlmeIndication )
    {
        case MLME_SCHEDULE_UPLINK:
        {// The MAC signals that we shall provide an uplink as soon as possible
            OnTxPingPacketTimerEvent( );
            break;
        }
        default:
            break;
    }
}

static void lwan_dev_params_update( void )
{
    MibRequestConfirm_t mibReq;
    uint16_t channelsMaskTemp[6];
    channelsMaskTemp[0] = 0x00FF;
    channelsMaskTemp[1] = 0x0000;
    channelsMaskTemp[2] = 0x0000;
    channelsMaskTemp[3] = 0x0000;
    channelsMaskTemp[4] = 0x0000;
    channelsMaskTemp[5] = 0x0000;

    mibReq.Type = MIB_CHANNELS_DEFAULT_MASK;
    mibReq.Param.ChannelsDefaultMask = channelsMaskTemp;
    LoRaMacMibSetRequestConfirm(&mibReq);
    mibReq.Type = MIB_CHANNELS_MASK;
    mibReq.Param.ChannelsMask = channelsMaskTemp;
    LoRaMacMibSetRequestConfirm(&mibReq);
}

uint8_t BoardGetBatteryLevel( void )
{
    return 0;
}

static LoRaMacPrimitives_t LoRaMacPrimitives;
static LoRaMacCallback_t LoRaMacCallbacks;
static MibRequestConfirm_t mibReq;

void LoRaWanAppStart()
{
    LoraWanDrv.State = DEVICE_STATE_INIT;
		LoraWanDrv.Type = DEVICE_SEND_PING;
	
		if (ReadSettings(&DevSetting) != true)
		{
			SYSLOG_W("Used default setting");
		}

    SYSLOG_I("ClassA app start\r\n");	
}

extern void LoRaWanAppLoop()
{
	  if (Radio.IrqProcess != NULL) 
		{
			Radio.IrqProcess();
    }
					
		switch( LoraWanDrv.State )
		{
				case DEVICE_STATE_INIT:
				{
						LoRaMacPrimitives.MacMcpsConfirm = McpsConfirm;
						LoRaMacPrimitives.MacMcpsIndication = McpsIndication;
						LoRaMacPrimitives.MacMlmeConfirm = MlmeConfirm;
						LoRaMacPrimitives.MacMlmeIndication = MlmeIndication;
						LoRaMacCallbacks.GetBatteryLevel = BoardGetBatteryLevel;
						LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, ACTIVE_REGION );
						LoRaMacTestSetDutyCycleOn(false);

						TimerInit( &PingPacketTimer, OnTxPingPacketTimerEvent );

						mibReq.Type = MIB_ADR;
						if (DevSetting.AdrMode == ADR_MODE_ON)
						{
							mibReq.Param.AdrEnable = true;
						}
						else
						{
							mibReq.Param.AdrEnable = false;
						}
						LoRaMacMibSetRequestConfirm( &mibReq );

						mibReq.Type = MIB_PUBLIC_NETWORK;
						mibReq.Param.EnablePublicNetwork = LORAWAN_PUBLIC_NETWORK;
						LoRaMacMibSetRequestConfirm( &mibReq );
						
						lwan_dev_params_update();
						
						LoraWanDrv.State = DEVICE_STATE_JOIN;
						break;
				}
				case DEVICE_STATE_JOIN:
				{
						if( DevSetting.JoinMode == JOIN_MODE_OTAA )
						{
							MlmeReq_t mlmeReq;

							// Initialize LoRaMac device unique ID
							//BoardGetUniqueId( DevEui );

							mlmeReq.Type = MLME_JOIN;

							mlmeReq.Req.Join.DevEui = DevSetting.OTAKeys.deveui;
							mlmeReq.Req.Join.AppEui = DevSetting.OTAKeys.appeui;
							mlmeReq.Req.Join.AppKey = DevSetting.OTAKeys.appkey;;
							mlmeReq.Req.Join.NbTrials = 8;

							if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
							{
									LoraWanDrv.State = DEVICE_STATE_IDLE;
							}
							else
							{
									LoraWanDrv.State = DEVICE_STATE_CYCLE;
							}
						}
						else
						{
							mibReq.Type = MIB_NET_ID;
							mibReq.Param.NetID = LORAWAN_NETWORK_ID;
							LoRaMacMibSetRequestConfirm( &mibReq );

							mibReq.Type = MIB_DEV_ADDR;
							mibReq.Param.DevAddr = DevSetting.ABPKeys.devaddr;
							LoRaMacMibSetRequestConfirm( &mibReq );

							mibReq.Type = MIB_NWK_SKEY;
							mibReq.Param.NwkSKey = DevSetting.ABPKeys.nwkskey;
							LoRaMacMibSetRequestConfirm( &mibReq );

							mibReq.Type = MIB_APP_SKEY;
							mibReq.Param.AppSKey = DevSetting.ABPKeys.appskey;
							LoRaMacMibSetRequestConfirm( &mibReq );

							mibReq.Type = MIB_NETWORK_JOINED;
							mibReq.Param.IsNetworkJoined = true;
							LoRaMacMibSetRequestConfirm( &mibReq );

							LoraWanDrv.State = DEVICE_STATE_SEND;
						}
						break;
				}
				case DEVICE_STATE_SEND:
				{
						if( LoraWanDrv.NextTx == true )
						{
								PrepareTxFrame( LoraWanDrv.Type );

								LoraWanDrv.NextTx = SendFrame( );
						}
						
						// Schedule next packet transmission
						LoraWanDrv.PingTimeout = DevSetting.PingDefs.Period + randr( 0, (DevSetting.PingDefs.Period / 10) );
						LoraWanDrv.State = DEVICE_STATE_CYCLE;
						break;
				}
				case DEVICE_STATE_CYCLE:
				{
						LoraWanDrv.State = DEVICE_STATE_IDLE;
						SYSLOG_I("Set ping timeout = %d msec", LoraWanDrv.PingTimeout);
						// Schedule next packet transmission
						TimerSetValue( &PingPacketTimer, LoraWanDrv.PingTimeout );
						TimerStart( &PingPacketTimer );
						break;
				}
				case DEVICE_STATE_IDLE:
				{
						if ((LoraWanDrv.NextTx == true) && (SendBufferGetCount() != 0))
						{
							OnTxCardInfoPacketEvent();
						}
						break;
				}
				default:
				{
						LoraWanDrv.State = DEVICE_STATE_INIT;
						break;
				}
		}	
}