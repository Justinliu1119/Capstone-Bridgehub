'''
*       EE 459 Embeded System Laboratory
*               Spring 2024
*   Team 13: Justin Liu, Richard Chen, Sriya Kuruppath
*               05/06/2024
* 
'''
import socket
import spidev
import time

# SPI setup
spi = spidev.SpiDev()
spi.open(0, 0)  # SPI bus 0, device 0 (CS0)
spi.max_speed_hz = 500000  # Set SPI speed
spi.mode = 0

def spi_exchange(data):
    response = spi.xfer2(data)
    return response

def send_message_to_mcu(message):
    """Send a string message to the MCU, followed by a null byte to indicate end of message."""
    message_bytes = [ord(char) for char in message] + [0]  # Append a null byte at the end
    for byte in message_bytes:
        spi_exchange([byte])
        time.sleep(0.05)  # Short delay to ensure the MCU processes each byte

def start_server(host, port):
    """Start a server to listen for incoming connections."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)  # Allow reusing address
        s.bind((host, port))
        s.listen()
        print(f"Server listening on {host}:{port}")

        while True:
            conn, addr = s.accept()
            with conn:
                print(f"Connected by {addr}")
                while True:
                    data = conn.recv(1024)
                    if not data:
                        break  # Break the loop if no data is sent
                    process_data(data, conn)

def process_data(data, conn):
    """Process received data and perform actions based on message format."""
    message = data.decode().strip()
    print(f"Received: {message}")

    if message.startswith("!"):
        # This is a zero quantity update, message should be "!barcode"
        barcode = message[1:]  # Remove the '!' to get the barcode
        print(f"Zero quantity for Barcode: {barcode}")
        send_message_to_mcu(f"!{barcode}")  # Send command to MCU
    elif "," in message:
        # This is a regular item update with barcode and expiration date
        barcode, expirationDate = message.split(',', 1)
        print(f"Barcode: {barcode}, Expiration Date: {expirationDate}")
        send_message_to_mcu(f"{barcode},{expirationDate}")  # Send both barcode and date to MCU
    else:
        print(f"Error processing data: Unexpected format. Received data: {message}")
        conn.sendall("Error: Incorrect data format. Please use ',' as a delimiter.".encode())

if __name__ == "__main__":
    HOST = '0.0.0.0'  # Listen on all network interfaces
    PORT = 5678        # Port number to listen on
    try:
        start_server(HOST, PORT)
    finally:
        spi.close()  # Ensure SPI is closed properly
