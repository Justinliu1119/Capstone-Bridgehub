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
import functools

# Firebase setup
cred_path = 'fridgehub-cd720-firebase-adminsdk-8j9v0-1bdde894ad.json'
cred = credentials.Certificate(cred_path)
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://fridgehub-cd720-default-rtdb.firebaseio.com/'
})

item_details_cache = {}  # Cache to store item details

def listener(event, conn, initial_load_handled):
    print(f"Event type: {event.event_type}, Path: {event.path}, Data: {event.data}")

    if event.event_type == 'put' and event.path == "/" and not initial_load_handled["handled"]:
        initial_load_handled["handled"] = True
        print("Initial data loaded, all items captured.")
        update_all_items(event.data)
    elif event.event_type in ['put', 'patch']:
        handle_data_update(event, conn)

def update_all_items(data):
    for item_name, item_details in data.items():
        update_cache(item_name, item_details)
def handle_data_update(event, conn):
    parts = event.path.strip('/').split('/')
    item_name = parts[0]
    data = event.data

    print(f"Handling update for {item_name}, Path parts: {parts}, Event type: {event.event_type}, Data: {data}")

    if len(parts) == 1:
        # Single part path, need to check if it's a full item update or a specific 'quantity' update
        if isinstance(data, dict) and 'quantity' in data and len(data) == 1:
            # Specific 'quantity' update
            quantity = int(data['quantity'])
            print(f"Detected quantity-only update via single path for {item_name}: {quantity}")
            handle_quantity_update(conn, item_name, quantity)
        else:
            # Full item update or other type of update
            update_cache(item_name, data)
            if 'quantity' in data:  # Check if quantity was part of a full update
                handle_quantity_update(conn, item_name, int(data['quantity']))
            if 'barcode' in data or 'expirationDate' in data:
                send_item_details_notification(conn, item_name)
                print(f"Sent full item update for {item_name}")
    elif len(parts) == 2:
        property_name, property_value = parts[1], data
        update_cache(item_name, {property_name: property_value})
        if property_name == 'quantity':
            quantity = int(property_value)
            print(f"Quantity update for {item_name} to {quantity}")
            handle_quantity_update(conn, item_name, quantity)
        else:
            send_item_details_notification(conn, item_name)
            print(f"Sent update notification for non-quantity change for {item_name}")



def handle_quantity_update(conn, item_name, quantity):
    """Handle changes to item quantity and send notifications if quantity is zero."""
    if quantity == 0:
        send_zero_quantity_notification(conn, item_name)
        print(f"Sent zero quantity notification for {item_name}")
    else:
        print(f"Quantity of {item_name} updated to {quantity}, no zero-quantity action needed.")


    

def send_item_details_notification(conn, item_name):
    """Sends notification with barcode and expiration date for added/updated items."""
    if item_name in item_details_cache:
        item = item_details_cache[item_name]
        barcode = item.get('barcode', 'Unknown Barcode')
        expirationDate = item.get('expirationDate', 'Unknown Expiration')
        message = f"{barcode},{expirationDate}\n"
        send_update_to_pi(conn, message)
        print(f"Sent update for {item_name}: Barcode - {barcode}, Expiration Date - {expirationDate}")
    else:
        print("No details available in cache for {item_name}, unable to send notification.")

def send_zero_quantity_notification(conn, item_name):
    """Send notification with just the barcode when quantity is zero."""
    if item_name in item_details_cache:
        item = item_details_cache[item_name]
        barcode = item.get('barcode', 'Unknown Barcode')
        message = f"!{barcode}"  # Format for zero quantity: !<barcode>
        send_update_to_pi(conn, message)
        print(f"Zero quantity alert sent to Raspberry Pi for {item_name}: {message}")
    else:
        print(f"No cache entry for {item_name}, unable to send zero quantity alert.")



def update_cache(item_name, item_details):
    """Update or initialize cache with new item details."""
    global item_details_cache
    if item_name in item_details_cache:
        item_details_cache[item_name].update(item_details)
    else:
        item_details_cache[item_name] = item_details
    print(f"Cache updated for {item_name}: {item_details}")

def send_update_to_pi(conn, message):
    try:
        conn.sendall(message.encode())
        print("Sent to Raspberry Pi:", message)
    except Exception as e:
        print("Error sending to Raspberry Pi:", e)

def reconnect_socket(HOST, PORT):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, PORT))
    return s

def main():
    HOST = '172.20.10.8'
    PORT = 5678
    initial_load_handled = {"handled": False}
    s = None
    try:
        s = reconnect_socket(HOST, PORT)
        print("Connected to Raspberry Pi")
        groceries_ref = db.reference('groceries')
        bound_listener = functools.partial(listener, conn=s, initial_load_handled=initial_load_handled)
        groceries_ref.listen(bound_listener)
    except socket.error as e:
        print(f"Failed to connect to Raspberry Pi: {e}")
        if s:
            s.close()
    except Exception as general_error:
        print(f"An error occurred: {general_error}")
        if s:
            s.close()

if __name__ == '__main__':
    main()
