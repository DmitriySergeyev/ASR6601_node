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
 * Defines the application data transmission duty cycle. 60s, value in [ms].
 */
#define PING_TX_DUTYCYCLE                           60000

/*!
 * Defines a random delay for application data transmission duty cycle. 5s,
 * value in [ms].
 */
#define PING_TX_DUTYCYCLE_RND                       5000

/*!
 * Default datarate
 */
#define LORAWAN_DEFAULT_DATARATE                    DR_0

/*!
 * LoRaWAN confirmed messages
 */
#define CONFIRMED_CARD_INFO_MSG_ON                  true

/*!
 * LoRaWAN Adaptive Data Rate
 *
 * \remark Please note that when ADR is enabled the end-device should be static
 */
#define LORAWAN_ADR_ON                               0

/*!
 * LoRaWAN application ports
 */
#define PING_TX_PORT                            		 2
#define CARD_INFO_TX_PORT                            3

static uint8_t enOTAA = OVER_THE_AIR_ACTIVATION;
   
static uint8_t DevEui[] = LORAWAN_DEVICE_EUI;
static uint8_t AppEui[] = LORAWAN_APPLICATION_EUI;
static uint8_t AppKey[] = LORAWAN_APPLICATION_KEY;


static uint8_t NwkSKey[] = LORAWAN_NWKSKEY;
static uint8_t AppSKey[] = LORAWAN_APPSKEY;
static uint32_t DevAddr = LORAWAN_DEVICE_ADDRESS;


/*!
 * Application port
 */
static uint8_t TxDataPort;

/*!
 * User application data size
 */
static uint8_t TxDataSize;
/*!
 * User application data buffer size
 */
#define LORAWAN_APP_DATA_BUFF_SIZE                           64

/*!
 * User application data
 */
static uint8_t TxData[LORAWAN_APP_DATA_BUFF_SIZE];

/*!
 * Indicates if the node is sending confirmed or unconfirmed messages
 */
static uint8_t IsTxConfirmed = false;

/*!
 * Defines the application data transmission duty cycle
 */
static uint32_t TxDutyCycleTime = PING_TX_DUTYCYCLE;

/*!
 * Timer to handle the application data transmission duty cycle
 */
static TimerEvent_t PingPacketTimer;

/*!
 * Indicates if a new packet can be sent
 */
static bool NextTx = true;

/*!
 * Device states
 */
static enum eDeviceState
{
    DEVICE_STATE_INIT,
    DEVICE_STATE_JOIN,
    DEVICE_STATE_SEND,
    DEVICE_STATE_CYCLE,
    DEVICE_STATE_IDLE
}DeviceState;

typedef enum
{
    DEVICE_SEND_PING,
    DEVICE_SEND_CARD,	
} eDeviceSendType;

static eDeviceSendType DeviceSendType;

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
				printf("Send ping packet\r\n");
				TxDataPort = PING_TX_PORT;
				IsTxConfirmed = false;			
				TxDataSize = PreparePingFrame(TxData);
				break;
			case DEVICE_SEND_CARD:
				printf("Send card info packet\r\n");
				TxDataPort = CARD_INFO_TX_PORT;
				IsTxConfirmed = CONFIRMED_CARD_INFO_MSG_ON;
				CardInfo = SendBufferPop(CONFIRMED_CARD_INFO_MSG_ON);
				TxDataSize = PrepareCardInfoFrame(CardInfo, TxData);
				break;	
			default:
				TxDataSize = 0;
				break;
		}
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

    if( LoRaMacQueryTxPossible( TxDataSize, &txInfo ) != LORAMAC_STATUS_OK )
    {
        // Send empty frame in order to flush MAC commands
        mcpsReq.Type = MCPS_UNCONFIRMED;
        mcpsReq.Req.Unconfirmed.fBuffer = NULL;
        mcpsReq.Req.Unconfirmed.fBufferSize = 0;
        mcpsReq.Req.Unconfirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
    }
    else
    {
        if( IsTxConfirmed == false )
        {
            mcpsReq.Type = MCPS_UNCONFIRMED;
            mcpsReq.Req.Unconfirmed.fPort = TxDataPort;
            mcpsReq.Req.Unconfirmed.fBuffer = TxData;
            mcpsReq.Req.Unconfirmed.fBufferSize = TxDataSize;
            mcpsReq.Req.Unconfirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
        }
        else
        {
            mcpsReq.Type = MCPS_CONFIRMED;
            mcpsReq.Req.Confirmed.fPort = TxDataPort;
            mcpsReq.Req.Confirmed.fBuffer = TxData;
            mcpsReq.Req.Confirmed.fBufferSize = TxDataSize;
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
						DeviceSendType = DEVICE_SEND_PING;
            DeviceState = DEVICE_STATE_SEND;
            NextTx = true;
        }
        else
        {
            // Network not joined yet. Try to join again
            MlmeReq_t mlmeReq;
            mlmeReq.Type = MLME_JOIN;
            mlmeReq.Req.Join.DevEui = DevEui;
            mlmeReq.Req.Join.AppEui = AppEui;
            mlmeReq.Req.Join.AppKey = AppKey;

            if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
            {
                DeviceState = DEVICE_STATE_IDLE;
            }
            else
            {
                DeviceState = DEVICE_STATE_CYCLE;
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
						DeviceSendType = DEVICE_SEND_CARD;
            DeviceState = DEVICE_STATE_SEND;
            NextTx = true;
        }
        else
        {
						TxDutyCycleTime = PING_TX_DUTYCYCLE + randr( 0, PING_TX_DUTYCYCLE_RND );
						DeviceState = DEVICE_STATE_CYCLE;
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
    NextTx = true;
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

    printf( "receive data: rssi = %d, snr = %d, datarate = %d\r\n", mcpsIndication->Rssi, (int)mcpsIndication->Snr,
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
                printf("joined\r\n");
                // Status is OK, node has joined the network
                DeviceState = DEVICE_STATE_SEND;
            }
            else
            {
                MlmeReq_t mlmeReq;
                
                printf("join failed\r\n");
                // Join was not successful. Try to join again
                mlmeReq.Type = MLME_JOIN;
                mlmeReq.Req.Join.DevEui = DevEui;
                mlmeReq.Req.Join.AppEui = AppEui;
                mlmeReq.Req.Join.AppKey = AppKey;
                mlmeReq.Req.Join.NbTrials = 8;

                if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
                {
                    DeviceState = DEVICE_STATE_IDLE;
                }
                else
                {
                    DeviceState = DEVICE_STATE_CYCLE;
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
    NextTx = true;
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
    DeviceState = DEVICE_STATE_INIT;
		DeviceSendType = DEVICE_SEND_PING;

    printf("ClassA app start\r\n");	
}

extern void LoRaWanAppLoop()
{
		switch( DeviceState )
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
						mibReq.Param.AdrEnable = LORAWAN_ADR_ON;
						LoRaMacMibSetRequestConfirm( &mibReq );

						mibReq.Type = MIB_PUBLIC_NETWORK;
						mibReq.Param.EnablePublicNetwork = LORAWAN_PUBLIC_NETWORK;
						LoRaMacMibSetRequestConfirm( &mibReq );
						
						lwan_dev_params_update();
						
						DeviceState = DEVICE_STATE_JOIN;
						break;
				}
				case DEVICE_STATE_JOIN:
				{
						if( enOTAA != 0 )
						{
							MlmeReq_t mlmeReq;

							// Initialize LoRaMac device unique ID
							//BoardGetUniqueId( DevEui );

							mlmeReq.Type = MLME_JOIN;

							mlmeReq.Req.Join.DevEui = DevEui;
							mlmeReq.Req.Join.AppEui = AppEui;
							mlmeReq.Req.Join.AppKey = AppKey;
							mlmeReq.Req.Join.NbTrials = 8;

							if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
							{
									DeviceState = DEVICE_STATE_IDLE;
							}
							else
							{
									DeviceState = DEVICE_STATE_CYCLE;
							}
						}
						else
						{
							mibReq.Type = MIB_NET_ID;
							mibReq.Param.NetID = LORAWAN_NETWORK_ID;
							LoRaMacMibSetRequestConfirm( &mibReq );

							mibReq.Type = MIB_DEV_ADDR;
							mibReq.Param.DevAddr = DevAddr;
							LoRaMacMibSetRequestConfirm( &mibReq );

							mibReq.Type = MIB_NWK_SKEY;
							mibReq.Param.NwkSKey = NwkSKey;
							LoRaMacMibSetRequestConfirm( &mibReq );

							mibReq.Type = MIB_APP_SKEY;
							mibReq.Param.AppSKey = AppSKey;
							LoRaMacMibSetRequestConfirm( &mibReq );

							mibReq.Type = MIB_NETWORK_JOINED;
							mibReq.Param.IsNetworkJoined = true;
							LoRaMacMibSetRequestConfirm( &mibReq );

							DeviceState = DEVICE_STATE_SEND;
						}
						break;
				}
				case DEVICE_STATE_SEND:
				{
						if( NextTx == true )
						{
								PrepareTxFrame( DeviceSendType );

								NextTx = SendFrame( );
						}
						
						// Schedule next packet transmission
						TxDutyCycleTime = PING_TX_DUTYCYCLE + randr( 0, PING_TX_DUTYCYCLE_RND );
						DeviceState = DEVICE_STATE_CYCLE;
						break;
				}
				case DEVICE_STATE_CYCLE:
				{
						DeviceState = DEVICE_STATE_IDLE;

						// Schedule next packet transmission
						TimerSetValue( &PingPacketTimer, TxDutyCycleTime );
						TimerStart( &PingPacketTimer );
						break;
				}
				case DEVICE_STATE_IDLE:
				{
						if ((NextTx == true) && (SendBufferGetCount() != 0))
						{
							OnTxCardInfoPacketEvent();
						}
						// Process Radio IRQ
						Radio.IrqProcess( );
						break;
				}
				default:
				{
						DeviceState = DEVICE_STATE_INIT;
						break;
				}
		}	
}