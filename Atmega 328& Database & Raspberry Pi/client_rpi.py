'''
*       EE 459 Embeded System Laboratory
*               Spring 2024
*   Team 13: Justin Liu, Richard Chen, Sriya Kuruppath
*               05/06/2024
* 
'''
import spidev
import time
import socket
import re  # Import regular expressions for advanced pattern matching

# SPI setup
spi = spidev.SpiDev()
spi.open(0, 0)  # SPI bus 0, device 0 (CS0)
spi.max_speed_hz = 500000  # Set SPI speed
spi.mode = 0
def handle_temperature(data):
    """Handle temperature data by sending it to the server."""
    print(f"Sending temperature data to the server: {data}")
    s.sendall(data.encode())
    response = s.recv(1024)
    print("Received response from server:", response.decode())
def spi_exchange(data):
    """Exchange data with the MCU via SPI."""
    response = spi.xfer2(data)
    print(f"Raw SPI response: {response}")  # Debugging print
    return response
def clean_barcode(barcode):
    """Remove non-digit characters from the barcode, except for '!'.
    This allows the barcode to retain '!' which might be used for special processing flags."""
    # Modify the regular expression to exclude any characters except digits and '!'
    return re.sub('[^0-9!]+', '', barcode)
def clean_barcode_alpha(barcode):
    """Remove non-digit and non-letter characters from the barcode, except for '!', ':', ',', and '-'.
    This allows the barcode to retain '!', ':', ',', and '-' which might be used for special processing flags."""
    print(f"Barcode before cleaning: {barcode}")  # Debugging print
    cleaned = re.sub('[^a-zA-Z0-9!,:-]+', '', barcode)  # Include dash in the allowed characters
    print(f"Barcode after cleaning: {cleaned}")  # Debugging print
    return cleaned

def clean_barcode_again(barcode):
    """Remove non-digit characters from the barcode, except for '!'.
    This allows the barcode to retain '!' which might be used for special processing flags."""
    # Modify the regular expression to exclude any characters except digits and '!'
    return re.sub('[^0-9]+', '', barcode)

def is_alphanumeric(barcode):
    """Check if the barcode is alphanumeric."""
    return bool(re.match('^[a-zA-Z0-9]+$', barcode))

def receive_barcode():
    barcode = ""
    while True:
        response = spi_exchange([0xFF])  # Send 0xFF as the dummy byte to poll the MCU
        if response[0] != 0:  # If response is not null
            while response[0] != 0:
                barcode += chr(response[0])
                response = spi_exchange([0xFF])
            break
    print(f"Received barcode: {barcode}")  # Debugging print to check received data
    return barcode

def parse_data(data):
    """Parse the barcode and optional expiration date from the received data."""
    if ',' in data:
        barcode, expiration_date = data.split(',', 1)
        return barcode.strip(), expiration_date.strip()
    return data.strip(), None  # In case there's no comma separating the barcode and the date

# Socket setup for database query
HOST = '172.20.10.9'  # Server's IP address
PORT = 5555            # Port to connect to

try:
    # Connect to the server
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        print(f"Connected to {HOST} on port {PORT}")

        while True:
            #print("Polling the MCU for data...")

            data = receive_barcode()
 
            
            if not data:  # Check if data is empty
                
                #print("No data received, skipping send.")
                continue  # Skip this loop iteration if no data received

            data = clean_barcode_alpha(data)
            if data.startswith('Temperature:'):
                # Handle temperature data
                s.sendall(data.encode())
                response = s.recv(1024)
                print("Received response from server:", response.decode())
            else:
                barcode, expiration_date = parse_data(data)
                if barcode:
                    barcode = clean_barcode(barcode)
                    if(barcode.startswith('!')):
                        mode = 'removal'
                    else:
                        mode = 'addition'
                    is_digit = barcode.isdigit()
                    print(f"Mode: {mode}, Received barcode: {barcode} (is digit: {is_digit}), Expiration Date: {expiration_date if expiration_date else 'N/A'}")
                # barcode = clean_barcode_again(barcode)
                    is_digit = barcode.isdigit()
                    print(f"Mode: {mode}, Received barcode: {barcode} (is digit: {is_digit}), Expiration Date: {expiration_date if expiration_date else 'N/A'}")
                    # Only send data if barcode is alphanumeric in addition mode
                    if mode == 'addition' and is_digit:
                        s.sendall(data.encode())
                        response = s.recv(1024)
                        print("Received response from server:", response.decode())
                    elif mode == 'removal':
                        s.sendall(data.encode())
                        response = s.recv(1024)
                        print("Received response from server:", response.decode())
                    else:
                        print("Barcode is not valid for addition, skipping send.")
            time.sleep(1)  # Wait a bit before the next polling

except Exception as e:
    print(f"Failed to connect or send data to {HOST} on port {PORT}. Error: {e}")

finally:
    spi.close()  # Close the SPI connection
