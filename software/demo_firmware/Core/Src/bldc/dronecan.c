#include "main.h"
#include <string.h>
#include <stdio.h>

#include "uavcan.equipment.esc.RawCommand.h"
#include "uavcan.equipment.esc.Status.h"
#include "uavcan.protocol.NodeStatus.h"
#include "uavcan.protocol.GetNodeInfo.h"
#include "uavcan.protocol.dynamic_node_id.Allocation.h"
#include "uavcan.protocol.param.GetSet.h"
#include "uavcan.protocol.param.ExecuteOpcode.h"

static CanardInstance canard;
static uint8_t canard_memory_pool[1024];

static struct uavcan_protocol_NodeStatus node_status;

/*
  keep the state for firmware update
 */
// static struct {
//     char path[256];
//     uint8_t node_id;
//     uint8_t transfer_id;
//     uint32_t last_read_ms;
//     int fd;
//     uint32_t offset;
// } fwupdate;

static void handle_RawCommand(CanardInstance* ins, CanardRxTransfer* transfer) {
    struct uavcan_equipment_esc_RawCommand msg;
    if (uavcan_equipment_esc_RawCommand_decode(transfer, &msg)) {
        // Map to BLDC control
        if (msg.cmd.len > 0) {
            // Take the command for motor index 0 (assuming we are motor 0)
            int32_t motor_cmd = msg.cmd.data[0];
            (void)motor_cmd; 

            // TODO: Apply the received motor_cmd to the actual ESC control logic
            // bldc_set_duty_cycle(motor_cmd);
        }
    }
}



static void handle_GetNodeInfo(CanardInstance* ins, CanardRxTransfer* transfer) {
    struct uavcan_protocol_GetNodeInfoResponse msg;
    memset(&msg, 0, sizeof(msg));
    
    msg.name.len = (uint8_t)snprintf((char*)msg.name.data, sizeof(msg.name.data), "com.amarjay.bldc");
    msg.software_version.major = MAJOR_SW;
		msg.software_version.minor = MINOR_SW;

    msg.hardware_version.major = MAJOR_HW;
		msg.hardware_version.minor = MINOR_HW;

		uint64_t t_us = micros64();
    node_status.uptime_sec = t_us / 1000000ULL;
    msg.status = node_status;

		get_device_id(msg.hardware_version.unique_id);
    
    uint8_t buffer[UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_MAX_SIZE];
    uint32_t len = uavcan_protocol_GetNodeInfoResponse_encode(&msg, buffer);
    
    canardRequestOrRespond(ins,
                           transfer->source_node_id,
                           UAVCAN_PROTOCOL_GETNODEINFO_SIGNATURE,
                           UAVCAN_PROTOCOL_GETNODEINFO_ID,
                           &transfer->transfer_id,
                           transfer->priority,
                           CanardResponse,
                           &buffer[0],
                           (uint16_t)len);
}

static void handle_param_GetSet(CanardInstance* ins, CanardRxTransfer* transfer){
	// get or set parameters and returns value in either case, not implemented yet
}
static void handle_param_ExecuteOpcode(CanardInstance* ins, CanardRxTransfer* transfer){
	// purposely to reset all parameters to default opcode and to save to flash opcode for future use, not implemented yet
}



/*
  data for dynamic node allocation process
 */
static struct {
    uint32_t send_next_node_id_allocation_request_at_ms;
    uint32_t node_id_allocation_unique_id_offset;
} DNA;

/*
  handle a DNA allocation packet
 */
static void handle_DNA_Allocation(CanardInstance *ins, CanardRxTransfer *transfer)
{
    if (canardGetLocalNodeID(&canard) != CANARD_BROADCAST_NODE_ID)
        return;

    DNA.send_next_node_id_allocation_request_at_ms =
        millis32()
        + UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS
        + (rand32() % UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_FOLLOWUP_DELAY_MS);

    if (transfer->source_node_id == CANARD_BROADCAST_NODE_ID)
    {
        DNA.node_id_allocation_unique_id_offset = 0;
        return;
    }

    struct uavcan_protocol_dynamic_node_id_Allocation msg;
    uavcan_protocol_dynamic_node_id_Allocation_decode(transfer, &msg);

    uint8_t my_unique_id[sizeof(msg.unique_id.data)];
    get_device_id(my_unique_id);

    if (memcmp(msg.unique_id.data, my_unique_id, msg.unique_id.len) != 0)
    {
        DNA.node_id_allocation_unique_id_offset = 0;
        return;
    }

    if (msg.unique_id.len < sizeof(msg.unique_id.data))
    {
        DNA.node_id_allocation_unique_id_offset = msg.unique_id.len;

        DNA.send_next_node_id_allocation_request_at_ms -=
            UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS;
    }
    else
    {
        canardSetLocalNodeID(ins, msg.node_id);
    }
}





static void on_transfer_received(CanardInstance* ins, CanardRxTransfer* transfer) {
    // switch on data type ID to pass to the right handler function
    if (transfer->transfer_type == CanardTransferTypeRequest) {
        // check if we want to handle a specific service request
        switch (transfer->data_type_id) {
		        case UAVCAN_PROTOCOL_GETNODEINFO_ID: {
							handle_GetNodeInfo(ins, transfer);
							break;
						}
						case UAVCAN_PROTOCOL_PARAM_GETSET_ID: {
							handle_param_GetSet(ins, transfer);
							break;
						}
						case UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_ID: {
							handle_param_ExecuteOpcode(ins, transfer);
							break;
						}
					}
				// perhaps we want to handle some responses as well in the future for fw update ..., for now we don't expect any
			} else if (transfer->transfer_type == CanardTransferTypeBroadcast) {
				switch (transfer->data_type_id) {
					case UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID: {
						handle_DNA_Allocation(ins, transfer);
						break;
					}
					case UAVCAN_EQUIPMENT_ESC_RAWCOMMAND_ID: {
						handle_RawCommand(ins, transfer);
						break;
					}
				}
    }
}

static bool should_accept_transfer(const CanardInstance* ins,
                                   uint64_t* out_data_type_signature,
                                   uint16_t data_type_id,
                                   CanardTransferType transfer_type,
                                   uint8_t source_node_id) {
    if (transfer_type == CanardTransferTypeBroadcast) {
        if (data_type_id == UAVCAN_EQUIPMENT_ESC_RAWCOMMAND_ID) {
            *out_data_type_signature = UAVCAN_EQUIPMENT_ESC_RAWCOMMAND_SIGNATURE;
            return true;
        }
    } else if (transfer_type == CanardTransferTypeRequest) {
        if (data_type_id == UAVCAN_PROTOCOL_GETNODEINFO_ID) {
            *out_data_type_signature = UAVCAN_PROTOCOL_GETNODEINFO_SIGNATURE;
            return true;
        }
    }
    return false;
}

void bldc_dronecan_init(void) {
    canardInit(&canard,
               canard_memory_pool,
               sizeof(canard_memory_pool),
               on_transfer_received,
               should_accept_transfer,
               NULL);
    canardSetLocalNodeID(&canard, 42); // Configurable via parameters or dynamic node ID allocation in the future
}

static inline void bldc_dronecan_pub_esc_status(void) {
    struct uavcan_equipment_esc_Status msg;
    memset(&msg, 0, sizeof(msg));
    
		// publish
		// TODO: fill in with actual values from BLDC telemetry and status
    // Set ESC status fields
    msg.esc_index = 0; // Usually should match this ESC's index (0-based)
    msg.voltage = 12.0f; // Replace with actual measurement: bldc_get_bus_voltage()
    msg.current = 0.0f;  // Replace with actual measurement: bldc_get_phase_current()
    msg.temperature = 298.15f; // 25C in Kelvin. Replace with logic
    msg.rpm = 0; // Replace with actual measurement
    msg.power_rating_pct = 0; // 0-100%
    msg.error_count = 0;
    
    uint8_t buffer[UAVCAN_EQUIPMENT_ESC_STATUS_MAX_SIZE];
    uint32_t len = uavcan_equipment_esc_Status_encode(&msg, buffer);
    
    static uint8_t transfer_id = 0;
    canardBroadcast(&canard,
                    UAVCAN_EQUIPMENT_ESC_STATUS_SIGNATURE,
                    UAVCAN_EQUIPMENT_ESC_STATUS_ID,
                    &transfer_id,
                    CANARD_TRANSFER_PRIORITY_LOW,
                    buffer,
                    (uint16_t)len);
}


static inline void bldc_dronecan_pub_node_status(void) {
	    uint8_t buffer[UAVCAN_PROTOCOL_NODESTATUS_MAX_SIZE];

    node_status.uptime_sec = micros64() / 1000000ULL;
    node_status.health = UAVCAN_PROTOCOL_NODESTATUS_HEALTH_OK;
    node_status.mode = UAVCAN_PROTOCOL_NODESTATUS_MODE_OPERATIONAL;
    node_status.sub_mode = 0;
    // put whatever you like in here for display in GUI
    node_status.vendor_specific_status_code = 1234;

    /*
      when doing a firmware update put the size in kbytes in VSSC so
      the user can see how far it has reached
     */
  //   if (fwupdate.node_id != 0) {
	// node_status.vendor_specific_status_code = fwupdate.offset / 1024;
	// node_status.mode = UAVCAN_PROTOCOL_NODESTATUS_MODE_SOFTWARE_UPDATE;
    // }

    uint32_t len = uavcan_protocol_NodeStatus_encode(&node_status, buffer);

    // we need a static variable for the transfer ID. This is
    // incremeneted on each transfer, allowing for detection of packet
    // loss
    static uint8_t transfer_id;

    canardBroadcast(&canard,
                    UAVCAN_PROTOCOL_NODESTATUS_SIGNATURE,
                    UAVCAN_PROTOCOL_NODESTATUS_ID,
                    &transfer_id,
                    CANARD_TRANSFER_PRIORITY_LOW,
                    buffer,
                    (uint16_t)len);
}

void bldc_dronecan_update(void) {
    // 1. Hardware CAN RX
    // Check if new CAN frames are available in your hardware driver / HAL buffer.
    // If a frame is available, convert it to a CanardCANFrame and call:
    // uint64_t timestamp = get_microseconds_timestamp();
    // canardHandleRxFrame(&canard, &rx_frame, timestamp);
    
    // 2. Hardware CAN TX
    // Check if the hardware driver / HAL can accept a new CAN frame.
    // If so, retrieve it from libcanard:
    // const CanardCANFrame* tx_frame = canardPeekTxQueue(&canard);
    // if (tx_frame != NULL) {
    //     bool success = transmit_can_frame_via_hal(tx_frame);
    //     if (success) {
    //         canardPopTxQueue(&canard);
    //     }
    // }
}


static uint32_t last_1hz = 0;
static uint32_t last_10hz = 0;
void bldc_dronecan_pub(void){
    bldc_dronecan_update();
    uint32_t now = HAL_GetTick();
    if (now - last_1hz >= 1000) {
      last_1hz = now;
      bldc_dronecan_pub_node_status();
    }
    
    if (now - last_10hz >= 100) {
      last_10hz = now;
      bldc_dronecan_pub_esc_status();
    }
}

