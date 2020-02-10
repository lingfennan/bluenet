/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Feb 7, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <events/cs_EventDispatcher.h>
#include <processing/cs_EncryptionHandler.h>
#include <tracking/cs_TrackedDevices.h>
#include <util/cs_BleError.h>
#include <util/cs_Utils.h>

TrackedDevices::TrackedDevices() {
	EventDispatcher::getInstance().addListener(this);
}

cs_ret_code_t TrackedDevices::handleRegister(internal_register_tracked_device_packet_t& packet) {
	TrackedDevice* device = findOrAdd(packet.data.deviceId);
	if (device == nullptr) {
		return ERR_NO_SPACE;
	}
	if (!hasAccess(*device, packet.accessLevel)) {
		return ERR_NO_ACCESS;
	}
	if (!isTokenOkToSet(*device, packet.data.deviceToken, sizeof(packet.data.deviceToken))) {
		return ERR_ALREADY_EXISTS;
	}
	setAccessLevel(*device, packet.accessLevel);
	setLocation(   *device, packet.data.locationId);
	setProfile(    *device, packet.data.profileId);
	setRssiOffset( *device, packet.data.rssiOffset);
	setFlags(      *device, packet.data.flags.asInt);
	setDevicetoken(*device, packet.data.deviceToken, sizeof(packet.data.deviceToken));
	setTTL(        *device, packet.data.timeToLiveMinutes);
	sendRegisterToMesh(*device);
	sendTokenToMesh(*device);
	return ERR_SUCCESS;
}

cs_ret_code_t TrackedDevices::handleUpdate(internal_update_tracked_device_packet_t& packet) {
	// For now, this packet is exactly the same as the register packet.
	return handleRegister(packet);
}

void TrackedDevices::handleMeshRegister(TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER)& packet) {
	TrackedDevice* device = findOrAdd(packet.deviceId);
	if (device == nullptr) {
		return;
	}
	// Access has been checked by sending crownstone.
	setLocation(*device, packet.locationId);
	setProfile(*device, packet.profileId);
	setRssiOffset(*device, packet.rssiOffset);
	setFlags(*device, packet.flags);
	setAccessLevel(*device, packet.accessLevel);
	sendLocationToMesh(*device);
}


void TrackedDevices::handleMeshToken(TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)& packet) {
	TrackedDevice* device = findOrAdd(packet.deviceId);
	if (device == nullptr) {
		return;
	}
	// Access has been checked by sending crownstone.
	if (!isTokenOkToSet(*device, packet.deviceToken, sizeof(packet.deviceToken))) {
		return;
	}
	setDevicetoken(*device, packet.deviceToken, sizeof(packet.deviceToken));
	setTTL(*device, packet.ttlMinutes);
}

void TrackedDevices::handleScannedDevice(adv_background_parsed_v1_t& packet) {
	TrackedDevice* device = findToken(packet.deviceToken, sizeof(packet.deviceToken));
	if (device == nullptr) {
		return;
	}
	// Maybe only check some fields to be set?
	if (!allFieldsSet(*device)) {
		return;
	}
	if (device->data.data.flags.flags.ignoreForBehaviour) {
		return;
	}
	if (!isValidTTL(*device)) {
		return;
	}
	sendLocation(*device);
}





TrackedDevices::TrackedDevice* TrackedDevices::findOrAdd(device_id_t deviceId) {
	TrackedDevice* device = find(deviceId);
	if (device == nullptr) {
		device = add();
		device->data.data.deviceId = deviceId;
	}
	return device;
}

TrackedDevices::TrackedDevice* TrackedDevices::find(device_id_t deviceId) {
	for (auto iter = devices.begin(); iter != devices.end(); ++iter) {
		if (iter->data.data.deviceId == deviceId) {
			return &(*iter);
		}
	}
	return nullptr;
}

TrackedDevices::TrackedDevice* TrackedDevices::findToken(uint8_t* deviceToken, uint8_t size) {
	assert(size == TRACKED_DEVICE_TOKEN_SIZE, "Wrong device token size");
	for (auto iter = devices.begin(); iter != devices.end(); ++iter) {
		if (memcmp(iter->data.data.deviceToken, deviceToken, size) == 0) {
			return &(*iter);
		}
	}
	return nullptr;
}

TrackedDevices::TrackedDevice* TrackedDevices::add() {
	// Check number of devices.
	if (deviceListSize >= MAX_TRACKED_DEVICES) {
		cs_ret_code_t retCode = removeDevice();
		if (retCode != ERR_SUCCESS) {
			return nullptr;
		}
	}
	TrackedDevice device;
	devices.push_front(device);
	++deviceListSize;
	return &(devices.front());
}

cs_ret_code_t TrackedDevices::removeDevice() {
	uint16_t lowestTTL = 0xFFFF;
	auto iterToRemove = devices.end();
	auto prevIter = devices.before_begin();
	for (auto iter = devices.begin(); iter != devices.end(); ++iter) {
		if (iter->fieldsSet != ALL_FIELDS_SET) {
			iterToRemove = prevIter;
			break;
		}
		if (iter->data.data.timeToLiveMinutes <= lowestTTL) {
			lowestTTL = iter->data.data.timeToLiveMinutes;
			iterToRemove = prevIter;
		}
		prevIter = iter;
	}

//	if (lowestTTL == 0xFFFF) {
//		return ERR_NOT_AVAILABLE;
//	}
	devices.erase_after(iterToRemove);
	--deviceListSize;
	return ERR_SUCCESS;
}


bool TrackedDevices::hasAccess(TrackedDevice& device, uint8_t accessLevel) {
	if (!isValidTTL(device)) {
		// Device token timed out, so anyone is allowed to set a token for the device.
		return true;
	}
	if (BLEutil::isBitSet(device.fieldsSet, BIT_POS_ACCESS_LEVEL) &&
			(!EncryptionHandler::getInstance().allowAccess((EncryptionAccessLevel)device.data.accessLevel, (EncryptionAccessLevel)accessLevel))) {
		return false;
	}
	return true;
}

bool TrackedDevices::isTokenOkToSet(TrackedDevice& device, uint8_t* deviceToken, uint8_t size) {
	TrackedDevice* otherDevice = findToken(deviceToken, size);
	if (otherDevice == nullptr) {
		return true;
	}
	if (otherDevice->data.data.deviceId == device.data.data.deviceId) {
		return true;
	}
	return false;
}


bool TrackedDevices::allFieldsSet(TrackedDevice& device) {
	return device.fieldsSet == ALL_FIELDS_SET;
}

void TrackedDevices::setAccessLevel(TrackedDevice& device, uint8_t accessLevel) {
	device.data.accessLevel = accessLevel;
	BLEutil::setBit(device.fieldsSet, BIT_POS_ACCESS_LEVEL);
}

void TrackedDevices::setLocation(TrackedDevice& device, uint8_t locationId) {
	device.data.data.locationId = locationId;
	BLEutil::setBit(device.fieldsSet, BIT_POS_LOCATION);
}

void TrackedDevices::setProfile(TrackedDevice& device, uint8_t profileId) {
	device.data.data.profileId = profileId;
	BLEutil::setBit(device.fieldsSet, BIT_POS_PROFILE);
}

void TrackedDevices::setRssiOffset(TrackedDevice& device, int8_t rssiOffset) {
	device.data.data.rssiOffset = rssiOffset;
	BLEutil::setBit(device.fieldsSet, BIT_POS_RSSI_OFFSET);
}

void TrackedDevices::setFlags(TrackedDevice& device, uint8_t flags) {
	device.data.data.flags.asInt = flags;
	BLEutil::setBit(device.fieldsSet, BIT_POS_FLAGS);
}

void TrackedDevices::setDevicetoken(TrackedDevice& device, uint8_t* deviceToken, uint8_t size) {
	assert(size == TRACKED_DEVICE_TOKEN_SIZE, "Wrong device token size");
	memcpy(device.data.data.deviceToken, deviceToken, sizeof(device.data.data.deviceToken));
	BLEutil::setBit(device.fieldsSet, BIT_POS_DEVICE_TOKEN);
}

void TrackedDevices::setTTL(TrackedDevice& device, uint16_t ttlMinutes) {
	device.data.data.timeToLiveMinutes = ttlMinutes;
	BLEutil::setBit(device.fieldsSet, BIT_POS_TTL);
}

bool TrackedDevices::isValidTTL(TrackedDevice& device) {
	return (BLEutil::isBitSet(device.fieldsSet, BIT_POS_TTL)) && (device.data.data.timeToLiveMinutes != 0);
}

void TrackedDevices::decreaseTTL() {
	for (auto iter = devices.begin(); iter != devices.end(); ++iter) {
		if ((iter->fieldsSet & BIT_POS_TTL) && (iter->data.data.timeToLiveMinutes != 0)) {
			iter->data.data.timeToLiveMinutes--;
		}
	}
}

void TrackedDevices::sendLocation(TrackedDevice& device) {
	if (!allFieldsSet(device)) {
		return;
	}
	TYPIFY(EVT_PROFILE_LOCATION) eventData;
	eventData.fromMesh = false;
	eventData.profileId = device.data.data.profileId;
	eventData.locationId = device.data.data.locationId;
	event_t event(CS_TYPE::EVT_PROFILE_LOCATION, &eventData, sizeof(eventData));
	event.dispatch();
}

void TrackedDevices::sendRegisterToMesh(TrackedDevice& device) {
	TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_REGISTER) eventData;
	eventData.accessLevel = device.data.accessLevel;
	eventData.deviceId    = device.data.data.deviceId;
	eventData.flags       = device.data.data.flags.asInt;
	eventData.locationId  = device.data.data.locationId;
	eventData.profileId   = device.data.data.profileId;
	eventData.rssiOffset  = device.data.data.rssiOffset;
	event_t event(CS_TYPE::CMD_SEND_MESH_MSG_TRACKED_DEVICE_REGISTER, &eventData, sizeof(eventData));
	event.dispatch();
}

void TrackedDevices::sendTokenToMesh(TrackedDevice& device) {
	TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_TOKEN) eventData;
	eventData.deviceId   = device.data.data.deviceId;
	memcpy(eventData.deviceToken, device.data.data.deviceToken, sizeof(device.data.data.deviceToken));
	eventData.ttlMinutes = device.data.data.timeToLiveMinutes;
	event_t event(CS_TYPE::CMD_SEND_MESH_MSG_TRACKED_DEVICE_TOKEN, &eventData, sizeof(eventData));
	event.dispatch();
}

void TrackedDevices::sendLocationToMesh(TrackedDevice& device) {
	TYPIFY(CMD_SEND_MESH_MSG_PROFILE_LOCATION) eventData;
	eventData.profile = device.data.data.profileId;
	eventData.location = device.data.data.locationId;
	event_t event(CS_TYPE::CMD_SEND_MESH_MSG_PROFILE_LOCATION, &eventData, sizeof(eventData));
	event.dispatch();
}

void TrackedDevices::handleEvent(event_t& evt) {
	switch(evt.type) {
		case CS_TYPE::CMD_REGISTER_TRACKED_DEVICE: {
			internal_register_tracked_device_packet_t* data = reinterpret_cast<TYPIFY(CMD_REGISTER_TRACKED_DEVICE)*>(evt.data);
			evt.result.returnCode = handleRegister(*data);
			break;
		}
		case CS_TYPE::CMD_UPDATE_TRACKED_DEVICE: {
			internal_update_tracked_device_packet_t* data = reinterpret_cast<TYPIFY(CMD_UPDATE_TRACKED_DEVICE)*>(evt.data);
			evt.result.returnCode = handleUpdate(*data);
			break;
		}
		case CS_TYPE::EVT_MESH_TRACKED_DEVICE_REGISTER: {
			TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER)* data = reinterpret_cast<TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER)*>(evt.data);
			handleMeshRegister(*data);
			break;
		}
		case CS_TYPE::EVT_MESH_TRACKED_DEVICE_TOKEN: {
			TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)* data = reinterpret_cast<TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)*>(evt.data);
			handleMeshToken(*data);
			break;
		}
		case CS_TYPE::EVT_ADV_BACKGROUND_PARSED_V1: {
			TYPIFY(EVT_ADV_BACKGROUND_PARSED_V1)* data = reinterpret_cast<TYPIFY(EVT_ADV_BACKGROUND_PARSED_V1)*>(evt.data);
			handleScannedDevice(*data);
			break;
		}
		case CS_TYPE::EVT_TICK: {
			if (--ticksLeft == 0) {
				ticksLeft = TICKS_PER_MINUTES;
				decreaseTTL();
			}
			break;
		}
		default:
			break;
	}
}
