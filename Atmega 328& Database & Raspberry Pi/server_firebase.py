'''
*       EE 459 Embeded System Laboratory
*               Spring 2024
*   Team 13: Justin Liu, Richard Chen, Sriya Kuruppath
*               05/06/2024
* 
'''
import socket
import firebase_admin
from firebase_admin import credentials, db
import re  # Import regular expressions for advanced pattern matching

# Firebase setup
cred_path = 'fridgehub-cd720-firebase-adminsdk-8j9v0-1bdde894ad.json'
cred = credentials.Certificate(cred_path)
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://fridgehub-cd720-default-rtdb.firebaseio.com/'
})

def clean_barcode(barcode):
    """Remove non-digit characters from the barcode, except for '!'."""
    cleaned = re.sub('[^0-9!]+', '', barcode)
    print(f"Cleaned barcode from {barcode} to {cleaned}")
    return cleaned

def retrieve_product_by_barcode(barcode):
    groceries_ref = db.reference('groceries')
    all_groceries = groceries_ref.get()
    if not all_groceries:
        return None, "No groceries found in the database."
    
    try:
        int_barcode = int(barcode)
    except ValueError:
        return None, "Barcode is not a valid integer."

    for product_name, product_info in all_groceries.items():
        if product_info.get('barcode') == int_barcode:
            return product_info, product_name
    return None, "No existing product with this barcode."

def update_quantity(product_info, product_name, reset=False):
    current_quantity = product_info.get('quantity', 0)
    updated_quantity = 0 if reset else current_quantity + 1
    groceries_ref = db.reference(f'groceries/{product_name}')
    groceries_ref.update({'quantity': updated_quantity})
    print(f"Updated quantity for {product_name}: {updated_quantity}")
    return f"Updated quantity for {product_name}: {updated_quantity}"

def update_temperature(temperature_data):
    temp_ref = db.reference('temperature')
    int_temp = int(temperature_data)
    temp_ref.set(int_temp)
    print(f"Updated temperature to: {temperature_data}")
    return f"Temperature updated to {temperature_data}"

HOST = '0.0.0.0'
PORT = 5555

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()
    print(f"Server listening on {HOST}:{PORT}")

    while True:
        conn, addr = s.accept()
        with conn:
            print(f"Connected by {addr}")
            while True:
                data = conn.recv(1024)
                if not data:
                    print("No data received, ending connection.")
                    break
                decoded_data = data.decode().strip()
                print(f"Received raw data: {decoded_data}")

                if decoded_data.startswith("Temperature:"):
                    # Handle temperature data
                    temperature = decoded_data.split("Temperature:")[1].strip()
                    response = update_temperature(temperature)
                else:
                    # Handle barcode data
                    if ',' in decoded_data:
                        parts = decoded_data.split(',')
                        barcode = clean_barcode(parts[0].strip())
                        expiration_date = parts[1].strip() if len(parts) > 1 else None
                    else:
                        barcode = clean_barcode(decoded_data.strip())
                        expiration_date = None

                    if barcode.startswith('!'):
                        reset_quantity = True
                        barcode = barcode[1:]  # Remove '!' for processing
                    else:
                        reset_quantity = False

                    print(f"Processing barcode: {barcode}, Expiration Date: {expiration_date if expiration_date else 'N/A'}")
                    product_info, product_name = retrieve_product_by_barcode(barcode)
                    if product_info:
                        response = update_quantity(product_info, product_name, reset=reset_quantity)
                    else:
                        response = "Error: No product found for barcode " + barcode

                conn.sendall(response.encode())
                print("Sent response back to client.")
