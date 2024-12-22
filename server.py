import socket
import threading
import os
from datetime import datetime


key = b'\xAA'  

def xor_encrypt(message, key):
    encrypted_message = bytearray()
    key_length = len(key)
    for i, byte in enumerate(message.encode()):  # Ensure the message is encoded to bytes
        encrypted_message.append(byte ^ key[i % key_length])
    return bytes(encrypted_message)  # Return as bytes for sending

def xor_decrypt(encrypted_message, key):
    decrypted_message = bytearray()
    key_length = len(key)
    for i, byte in enumerate(encrypted_message):  # Encrypted message is already bytes
        decrypted_message.append(byte ^ key[i % key_length])
    return decrypted_message.decode() 

clients = {}  

client_messages_dir = "clients_messages"

if not os.path.exists(client_messages_dir):
    os.makedirs(client_messages_dir)

def broadcast_clients_list():
    clients_list = "\n".join(clients.keys())
    message = f"Connected clients: {clients_list}"
    encrypted_message = xor_encrypt(message, key)
    for client_socket in clients.values():
        client_socket.send(encrypted_message)

def handle_client(client_socket):
    try:
        name = client_socket.recv(1024).decode()
        clients[name] = client_socket
        print(name)

        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        client_file_path = os.path.join(client_messages_dir, f"{name}_{timestamp}.txt")
        
        with open(client_file_path, 'w') as client_file:
            client_file.write(f"Messages for {name} (Started at {timestamp})\n\n")

        broadcast_clients_list()

        while True:
            encrypted_message = client_socket.recv(1024)
            if not encrypted_message:
                break

            message = xor_decrypt(encrypted_message, key)

            with open(client_file_path, 'a') as client_file:
                client_file.write(f"{datetime.now().strftime('%Y-%m-%d %H:%M:%S')} - {message}\n")
                
            if ">" in message:
                recipient_name, msg_text = message.split(">", 1)
                recipient_name = recipient_name.strip()

                if recipient_name in clients:
                    recipient_socket = clients[recipient_name]
                    recipient_socket.send(xor_encrypt(f"{name}: {msg_text}", key))
                else:
                    client_socket.send(xor_encrypt("Recipient not found.", key))
            else:
                client_socket.send(xor_encrypt("Invalid message format. Use 'recipient_name>message_text'.", key))
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if name in clients:
            del clients[name]
            broadcast_clients_list()
        client_socket.close()

def start_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('127.0.0.1', 8000))
    server_socket.listen(5)
    print("Server started. Waiting for connections...")

    while True:
        client_socket, client_address = server_socket.accept()
        print(f"Connection from {client_address}")
        threading.Thread(target=handle_client, args=(client_socket,), daemon=True).start()

if __name__ == "__main__":
    start_server()
