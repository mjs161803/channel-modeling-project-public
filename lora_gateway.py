"""
Code for a Simple LoRa Gateway device.  This Gateway listens for 
LoRa packets, parses received packets, and pushes results to
a database along with information on the received SNR and RSSI.

ECE6784 - Wireless Systems
Author: Michael Sanda

Gateway code based on:
Example for using the RFM9x Radio with Raspberry Pi.
Learn Guide: https://learn.adafruit.com/lora-and-lorawan-for-raspberry-pi
Author: Brent Rubell for Adafruit Industries
"""
# Import Python System Libraries
import time
# Import Blinka Libraries
import busio
from digitalio import DigitalInOut, Direction, Pull
import board
# Import the SSD1306 module.
import adafruit_ssd1306
# Import RFM9x
import adafruit_rfm9x

# Import module for connecting to MySQL database
import mysql.connector
from datetime import date, datetime, timedelta

# Gateway Geographic Location
rx_location_lat = '38.3474'
rx_location_long = '-78.2262'

# MySQL db Configuration
mysqlUser = 'mjsanda_loragw'
mysqlPw = 'a!12zEFJ7@#jJ47'
dbHostName = 'agronicaiot.com'
dbName = 'mjsanda_lora_project'
tableName = 'rx_packets'

# Configure LoRa Radio
CS = DigitalInOut(board.CE1)
RESET = DigitalInOut(board.D25)
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)
rfm9x = adafruit_rfm9x.RFM9x(spi, CS, RESET, 915.0)
rfm9x.tx_power = 5
rfm9x.spreading_factor = 8
rfm9x.signal_bandwidth = 62500
rfm9x.coding_rate = 8
rfm9x.enable_crc = True

prev_packet = None

while True:
    packet = None

    # check for packet rx
    packet = rfm9x.receive()
    if packet is not None:
        # Display the packet text and rssi
        prev_packet = packet
        packet_text = str(prev_packet, "utf-8")
        packet_rssi = rfm9x.last_rssi
        packet_snr = rfm9x.last_snr
        
        rx_time = time.strftime('%Y-%m-%d %H:%H:%S')

        # Parse packet payload to data fields
        tx_packet_id, tx_location_lat, tx_location_long, packet_remainder = packet_text.split(" ", 3)

        ts = time.strftime('%Y-%m-%d %H:%H:%S')
        print("["+ts+"]: Received LoRa Packet...")
        print("["+ts+"]:    Packet ID#: "+tx_packet_id)
        print("["+ts+"]:    Transmitter Location#: "+tx_location_lat+", "+tx_location_long)
        print("["+ts+"]:    RSSI: "+str(packet_rssi))
        print("["+ts+"]:    SNR: "+str(packet_snr))

        # push record to MySQL database
        db_handle = mysql.connector.connect(user = mysqlUser, password = mysqlPw, host = dbHostName, database = dbName)

        if db_handle:
            ts = time.strftime('%Y-%m-%d %H:%H:%S')
            print("["+ts+"]: Successfully opened connection to database.")
            cursor = db_handle.cursor()
            mysql_query_text = "INSERT INTO "+tableName+" (tx_packet_id, rx_location_lat, rx_location_long, rx_time, packet_rssi, packet_snr) VALUES (%s, %s, %s, %s, %s, %s)"

            values_for_row = (str(tx_packet_id), str(rx_location_lat), str(rx_location_long), str(rx_time), str(packet_rssi), str(packet_snr))
            cursor.execute(mysql_query_text, values_for_row)
            db_handle.commit()
            ts = time.strftime('%Y-%m-%d %H:%H:%S')
            print("["+ts+"]: Row written to table: "+str(tx_packet_id)+" / "+str(rx_location_lat)+" / "+str(rx_location_long)+" / "+str(rx_time)+" / "+str(packet_rssi)+" / "+str(packet_snr))
            cursor.close()
            db_handle.close()
        else:
            ts = time.strftime('%Y-%m-%d %H:%H:%S')
            print("["+ts+"]: Unable to open connection to database")
            


    time.sleep(2)
