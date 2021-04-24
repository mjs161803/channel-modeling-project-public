"""
This program...
    1) Gets position and time data from GPS device via serial port
    2) Forms the packet payload string
    3) Sends the packet payload to LoRa radio for transmission
    4) Pushes row to table in MySQL database

"""
#######################################
# MODULES AND LIBRARIES
#######################################

# Import Python System Libraries
import time
import math
import os
import sys

# Import Blinka Libraries
import busio
from digitalio import DigitalInOut, Direction, Pull
import board

# Import serial module and GPS Receiver module
import serial
import adafruit_gps

# Import LoRa radio module 
import adafruit_rfm9x

# Import module for connecting to MySQL database
import mysql.connector
from datetime import date, datetime, timedelta

#########################################
# SYSTEM CONFIGURATION
#########################################

# MySQL db Configuration
mysqlUser = ''
mysqlPw = ''
dbHostName = ''
dbName = ''
tableName = 'tx_packets'

# GPS Serial Device Configuration
serial_path = '/dev/ttyUSB0'
serial_baud = 9600
serial_timeout = 10
uart = serial.Serial(serial_path, baudrate = serial_baud, timeout = serial_timeout)
my_gps = adafruit_gps.GPS(uart, debug = False)

# Configure LoRa Radio
CS = DigitalInOut(board.CE1)
RESET = DigitalInOut(board.D25)
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)
rfm9x = adafruit_rfm9x.RFM9x(spi, CS, RESET, 915.0)
rfm9x.tx_power = 22
rfm9x.spreading_factor = 8
rfm9x.signal_bandwidth = 62500
rfm9x.coding_rate = 8
rfm9x.enable_crc = True

########################################
# MAIN CODE
########################################

my_gps.send_command(b"PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0")
my_gps.send_command(b"PMTK220,5000")

tx_packet_id = 1
tx_location_lat = 38.34
tx_location_long = -78.23
tx_time = '2021-03-06 13:12:00'
tx_power = rfm9x.tx_power
tx_packet_rawpayload = str(tx_packet_id) + ' ' + str(tx_location_lat) + ' ' + str(tx_location_long) + ' ' + tx_time + ' ' + str(tx_power)

while True:
    # get time and position data from serial device
    my_gps.update()
    if my_gps.update() and my_gps.has_fix:
        print("GPS Module has updated and has a fix...")
        tx_location_lat = "{:.6f}".format(my_gps.latitude)
        tx_location_long = "{:.6f}".format(my_gps.longitude)
        tx_time = "{:4}-{:02}-{:02} {:02}:{:02}:{:02}".format(my_gps.timestamp_utc.tm_year, my_gps.timestamp_utc.tm_mon, my_gps.timestamp_utc.tm_mday, my_gps.timestamp_utc.tm_hour, my_gps.timestamp_utc.tm_min, my_gps.timestamp_utc.tm_sec)
        print("GPS fix... tx_location_lat: {}".format(tx_location_lat))
        print("GPS fix... tx_location_long: {}".format(tx_location_long))
        print("GPS fix... timestamp: {}".format(tx_time))


        # construct packet payload string
        tx_packet_rawpayload = str(tx_packet_id) + ' ' + str(tx_location_lat) + ' ' + str(tx_location_long) + ' ' + tx_time + ' ' + str(tx_power)
    
        packet_payload = bytes(tx_packet_rawpayload,"utf-8")
        ts = time.strftime('%Y-%m-%d %H:%M:%S')
        print("["+ts+"]: Constructed packet payload: ", tx_packet_rawpayload)

        # transmit packet via LoRa radio
        rfm9x.send(packet_payload)
        print("Sent packet to LoRa radio: ", tx_packet_rawpayload)

        # push record to MySQL database
        dbHandle = mysql.connector.connect(user = mysqlUser, password = mysqlPw, host = dbHostName, database = dbName) 
        if dbHandle:
            ts = time.strftime('%Y-%m-%d %H:%M:%S')
            print("["+ts+"]: Successfully opened connection to database.")
            cursor = dbHandle.cursor()
            mysql_query_text = "INSERT INTO "+tableName+" (tx_packet_id, tx_location_lat, tx_location_long, tx_time, tx_power, tx_packet_rawpayload) VALUES (%s, %s, %s, %s, %s, %s)"
            values_for_row = (tx_packet_id, tx_location_lat, tx_location_long, tx_time, tx_power, tx_packet_rawpayload)
            cursor.execute(mysql_query_text, values_for_row)
            dbHandle.commit()
            ts = time.strftime('%Y-%m-%d %H:%M:%S')
            print("["+ts+"]: Tx packet info written to database: "+tx_packet_rawpayload)
            cursor.close()
            dbHandle.close()
        else:
            ts = time.strftime('%Y-%m-%d %H:%M:%S')
            print("["+ts+"]: Unable to open connection to database.")



    # Increment packet ID and sleep

        tx_packet_id += 1
        time.sleep(2)
    time.sleep(2)
