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
from datetime import datetime

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
def update_temperature(temperature_data):
    temp_ref = db.reference('temperature')
    int_temp = int(temperature_data)
    temp_ref.set(int_temp)
    print(f"Updated temperature to: {temperature_data}")
    return f"Temperature updated to {temperature_data}"

def retrieve_product_by_barcode(barcode):
    groceries_ref = db.reference('groceries')
    all_groceries = groceries_ref.get()
    if not all_groceries:
        return None, "No groceries found in the database."
    
    if barcode.isdigit():
        int_barcode = int(barcode)
    else:
        return None, "Barcode is not a valid integer."

    for product_name, product_info in all_groceries.items():
        if product_info.get('barcode') == int_barcode:
            return product_info, product_name
    return None, "No existing product with this barcode."

def update_quantity(product_info, product_name, reset=False, expiration_date=None):
    groceries_ref = db.reference(f'groceries/{product_name}')
    updates = {}

    current_quantity = product_info.get('quantity', 0)
    updates['quantity'] = 0 if reset else current_quantity + 1

    # Parse and set the expiration date if provided
    if expiration_date:
        try:
            # Assuming the expiration date is given in 'YYYY-MM-DD' format
            parsed_date = datetime.strptime(expiration_date, '%Y-%m-%d')
            updates['expirationDate'] = expiration_date  # Add valid expiration date to the updates
        except ValueError:
            return f"Error: Invalid expiration date format. It should be YYYY-MM-DD."

    # Update the database entry
    groceries_ref.update(updates)
    return f"Updated {product_name}: Quantity to {updates['quantity']}, Expiration Date to {updates.get('expirationDate', 'N/A')}"


HOST = '0.0.0.0'
PORT = 5555

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()
    print(f"Server listening on {HOST}:{PORT}")
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
                    conn.sendall(response.encode())
            else:
                if ',' in decoded_data:
                    parts = decoded_data.split(',')
                    barcode = clean_barcode(parts[0].strip())
                    expiration_date = parts[1].strip() if len(parts) > 1 else None
                else:
                    barcode = clean_barcode(decoded_data.strip())
                    expiration_date = None

            # Inside the main loop where you handle socket connection
                if barcode.startswith('!'):
                    reset_quantity = True
                    barcode = barcode[1:]  # Remove '!' for processing
                else:
                    reset_quantity = False

                barcode = clean_barcode(barcode)  # Ensure barcode is clean
                print(f"Processing barcode: {barcode}, Expiration Date: {expiration_date if expiration_date else 'N/A'}")

                product_info, product_name = retrieve_product_by_barcode(barcode)
                if product_info:
                    update_message = update_quantity(product_info, product_name, reset=reset_quantity, expiration_date=expiration_date)
                    conn.sendall(update_message.encode())
                    print("Sent response back to client.")
                    
                else:
                    error_message = "Error: No product found for barcode " + barcode
                    print(error_message)
                    conn.sendall(error_message.encode())
                

