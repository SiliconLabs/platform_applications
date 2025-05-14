#!/usr/bin/env python3
"""
Asset Tag GUI.
"""

# Copyright 2024 Silicon Laboratories Inc. www.silabs.com
#
# SPDX-License-Identifier: Zlib
#
# The licensor of this software is Silicon Laboratories Inc.
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

from dataclasses import dataclass
import os.path
import sys,os
import struct
from datetime import datetime
import pv_asset_gui as pv_asset_gui 
#import random

sys.path.append(os.path.join(os.path.dirname(__file__), "../.."))
from common.conversion import Ieee11073Float
from common.util import ArgumentParser, BluetoothApp, get_connector, find_service_in_advertisement
import common.status as status

#Switch to control dummy vs. real data
DUMMY = False

# Constants
HEALTH_THERMOMETER_SERVICE = b"\x09\x18"
TEMPERATURE_MEASUREMENT_CHAR = b"\x1c\x2a"

CONN_INTERVAL_MIN = 80   # 100 ms
CONN_INTERVAL_MAX = 80   # 100 ms
CONN_SLAVE_LATENCY = 0   # no latency
CONN_TIMEOUT = 100       # 1000 ms
CONN_MIN_CE_LENGTH = 0
CONN_MAX_CE_LENGTH = 65535

SCAN_INTERVAL = 16       # 10 ms
SCAN_WINDOW = 16         # 10 ms
SCAN_PASSIVE = 0
USE_CRC32 = 1            # if non-zero, CRC32 is appended to make sure that only asset-tag devices are detected.

## GUI visualization parameters
BATTERY_HIGH_VCAP = 2500    # mV
BATTERY_LOW_VCAP = 1200     # mV
TEMP_HIGH = 33              # °C
TEMP_LOW = 24               # °C

# The maximum number of connections has to match with the configuration on the target side.
SL_BT_CONFIG_MAX_CONNECTIONS = 4

power_states = [ "POWER_LEVEL_SKIP_2ND", "POWER_LEVEL_MIN", "POWER_LEVEL_MEDIUM", "POWER_LEVEL_HIGH", "POWER_LEVEL_MAX" ]

#functions for CRC32 calculation
def reflect_bits(data, num_bits):
    reflection = 0
    for i in range(num_bits):
        if data & (1 << i):
            reflection |= (1 << (num_bits - 1 - i))
    return reflection

def crc32_custom(data, poly=0x04C11DB7, init_val=0x00000000, final_xor_val=0x00000000):
    crc = init_val
    
    for byte in data:
        byte = reflect_bits(byte, 8)
        crc ^= byte << 24
        for _ in range(8):
            if crc & 0x80000000:
                crc = (crc << 1) ^ poly
            else:
                crc <<= 1
            crc &= 0xFFFFFFFF  # Ensure CRC remains within 32 bits

    crc = reflect_bits(crc, 32) ^ final_xor_val
    return crc

@dataclass
class Connection:
    """ Connection representation """
    address: str
    address_type: int
    service: int=None
    characteristic: int=None

class PVAssetTagApp(BluetoothApp):
    """ Application derived from generic BluetoothApp. """
    def __init__(self, connector):
        super().__init__(connector)
        self.conn_state = ""
        self.connections = dict()
        self.AssetTagData = dict() #containst the data from the PV asset tags

    def bt_evt_system_boot(self, evt):
        """ Bluetooth event callback
        This event indicates that the device has started and the radio is ready.
        Do not call any stack command before receiving this boot event!
        """
        # Set the default connection parameters for subsequent connections
        self.lib.bt.connection.set_default_parameters(
            CONN_INTERVAL_MIN,
            CONN_INTERVAL_MAX,
            CONN_SLAVE_LATENCY,
            CONN_TIMEOUT,
            CONN_MIN_CE_LENGTH,
            CONN_MAX_CE_LENGTH)
        # Start scanning - looking for thermometer devices
        self.lib.bt.scanner.start(
            self.lib.bt.scanner.SCAN_PHY_SCAN_PHY_1M,
            self.lib.bt.scanner.DISCOVER_MODE_DISCOVER_GENERIC)
        self.log.info("Scanning started...")
        self.conn_state = "scanning"
        self.connections = dict[int, Connection]()
    
    def bt_evt_scanner_legacy_advertisement_report(self, evt):
        """ Bluetooth event callback """
        self.timestampDict = dict()
        if USE_CRC32 and (len(evt.data) == 19 or len(evt.data) == 31) or USE_CRC32 == 0 and (len(evt.data) == 15 or len(evt.data) == 28) :
            if evt.data[0:3] == b'\x02\x01\x06' and evt.data[4:7] == b'\xff\xff\x02':
                # calculate immediately CRC. If CRC32 is not ok then discard data
                # Remove the first 5 bytes
                modified_array = evt.data[5:]
                # Prepend the binary value corresponding to "SLBS", i.e. SILABS without wovels
                prefix = b"SLBS"                    
                result_array = prefix + modified_array              
                # calculate CRC32
                # Calculate the CRC32 with the custom polynomial
                crc = crc32_custom(result_array)
                if (crc):
                    return  # this is not an asset tag. Return.
                devid = evt.data[7:15].hex()
                if evt.address not in self.AssetTagData.keys():
                    self.AssetTagData[evt.address] = []
                self.timestampDict["devId"] = devid
                #get and print timestamp
                now = datetime.now()
                # dd/mm/YY H:M:S
                timestamp = now.strftime("%d/%m/%Y %H:%M:%S")
                self.timestampDict["timestamp"] = timestamp
                payloadLen = evt.data[3] - 3
                self.timestampDict["payloadLen"] = payloadLen
                self.timestampDict["MAC"] = evt.address
                self.timestampDict["Raw data"] = evt.data.hex()
                data_str = "MAC = {}, Raw data = {}, payload length = {}".format(evt.address, evt.data.hex(), payloadLen) + "\n"
                if len(evt.data) == 28 and USE_CRC32 == 0 or len(evt.data) == 31 and USE_CRC32:
                    (temp, Vcap, deltaVcap, intensity, power) = struct.unpack('<hHhBb', evt.data[15:23])

                    nextInterval = int.from_bytes(evt.data[23:26], byteorder='little', signed=False)
                    if (USE_CRC32):
                        state = evt.data[26]
                        mode = (~state >> 4) & 0x03
                        state = state & 0x0F
                    else:
                        (mode, state) = struct.unpack('<BB', evt.data[26:28])
                        state ^= 0x03
                    temp = temp / 100.0
                    power = power / 10.0
                    data_str = ("ID = {0} Temperature = {1:+5.2f} °C, Vcap = {2: 4d} mV, deltaVcap = {3:+4d} mV, Power = {5:+4.1f} dBm, Next = {6:5d} ms, Mode = {7:d}, State = {8:s}".format(devid, temp, Vcap, deltaVcap, intensity, power, nextInterval, mode, power_states[state])) + "\n"
                    self.timestampDict["Temperature"] = temp
                    self.timestampDict["Vcap"] = Vcap
                    self.timestampDict["deltaVcap"] = deltaVcap
                    self.timestampDict["Intensity"] = intensity
                    self.timestampDict["Power"] = power
                    self.timestampDict["Next"] = nextInterval
                    self.timestampDict["Mode"] = mode
                    self.timestampDict["State"] = power_states[state]
                else:
                    data_str = ("Low power mode, only devId available: {0}".format(devid)) + "\n"
                    self.timestampDict["Temperature"] = None
                    self.timestampDict["Vcap"] = None
                    self.timestampDict["deltaVcap"] = None
                    self.timestampDict["Intensity"] = None
                    self.timestampDict["Power"] = None
                    self.timestampDict["Next"] = None
                    self.timestampDict["Mode"] = None
                    self.timestampDict["State"] = None
                self.log.info("Data received and stored in the following dictionary:")
                self.log.info(self.timestampDict)
                self.AssetTagData[evt.address].append(self.timestampDict)
                #print("==================")
                #print (self.AssetTagData)
            

if __name__ == "__main__":
    if DUMMY == False:
        parser = ArgumentParser(description=__doc__)
        
        parser.add_argument("-cli", "--cli", action="store_true", help="Use CLI interface")
        args = parser.parse_args()
        
        connector = get_connector(args)
        # Instantiate the application.
        pvassttagapp = PVAssetTagApp(connector)
        #pvassttagapp = None

        # Running the application blocks execution until it terminates.
        if args.cli:
            while True:
                # Prompt the user for input.
                user_input = input("Enter 'start' to run the application or 'stop' to quit: ").lower()
                
                if user_input == "start":
                    # Running the application blocks execution until it terminates.
                    pvassttagapp.start()
                elif user_input == "stop":
                    # Assuming your App class has a stop method to stop the application.
                    pvassttagapp.stop()
                    print("Application stopped.")
                    break
                elif user_input == "reset":
                    pvassttagapp.reset()
                elif user_input == "exit":
                    break
                else:
                    print("Invalid input. Please enter 'start' or 'stop'.")
        else:
        # GUI mode
            thresholds = [BATTERY_HIGH_VCAP, BATTERY_LOW_VCAP, TEMP_HIGH, TEMP_LOW] 
            gui_app = pv_asset_gui.GUIApp(pvassttagapp,thresholds)
            gui_app.run()
    else:
        thresholds = [BATTERY_HIGH_VCAP, BATTERY_LOW_VCAP, TEMP_HIGH, TEMP_LOW] 
        gui_app = pv_asset_gui.GUIApp(None,thresholds)
        gui_app.run()
