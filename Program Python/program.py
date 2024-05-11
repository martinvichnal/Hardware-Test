import struct

import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# Getting data packets from Arduino.
# data packet format: 
# {
#     start_byte (1 bytes - 0x55), 
#     sonic_data (4 bytes), 
#     photo_data (4 bytes), 
#     checksum (1 bytes), 
#     end_byte (1 bytes - 0xAA)
# }
def reciveData(ser):
    startByte = ser.read(1)
    # Wait for start byte (0x55)
    while startByte != b'\x55':
        startByte = ser.read(1)
    # Read sonic data
    sonicRawData = ser.read(4)
    sonicData = struct.unpack('f', sonicRawData)
    # print("[reciveData] - raw Sonic Data", sonicData)
    # Read photo data
    photoRawData = ser.read(4)
    photoData = struct.unpack('I', photoRawData)
    # print("[reciveData] - raw Photo Data", photoData)
    # Read checksum
    checksum = ser.read(1)
    # print("[reciveData] - Checksum", checksum)
    # Checksum validation
    
    # Read end byte (0xAA)
    endByte = ser.read(1)
    if endByte == b'\xAA':
        return sonicData, photoData


def update_sonic_plot(frame):
    sonic_data, _ = reciveData(ser)
    print("Sonic Data:", sonic_data)
    sonic_data_list.extend(sonic_data)
    ax1.clear()
    ax1.plot(sonic_data_list, label='Sonic Data')
    ax1.set_xlabel('Time')
    ax1.set_ylabel('Sonic value [cm]')
    ax1.set_ylim(0, 25)
    ax1.legend()

def update_photo_plot(frame):
    _, photo_data = reciveData(ser)
    print("Photo Data:", photo_data)
    photo_data_list.extend(photo_data)
    ax2.clear()
    ax2.plot(photo_data_list, label='Photo Data', color='orange')
    ax2.set_xlabel('Time')
    ax2.set_ylabel('Photo ADC Value')
    ax2.set_ylim(0, 1024)
    ax2.legend()


ser = serial.Serial("COM15", 9600)
print("Serial port opened")

sonic_data_list = []
photo_data_list = []

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 6))
ani1 = FuncAnimation(fig, update_sonic_plot, interval=20)
# ani2 = FuncAnimation(fig, update_photo_plot, interval=20)
plt.show()
ser.close()