# python stomp_report_client.py
import socket

host = '127.0.0.1'
port = 7777

username = 'alice'
passcode = 'pass'

def send_recv(s, frame):
    s.sendall(frame.encode('utf-8'))
    data = b''
    while True:
        chunk = s.recv(4096)
        if not chunk:
            break
        data += chunk
        if b'\0' in chunk:
            break
    return data.decode('utf-8', errors='replace').rstrip('\0')

s = socket.create_connection((host, port))

# connect
connect = f"CONNECT\nlogin:{username}\npasscode:{passcode}\n\n\0"
print("-> CONNECT")
print(send_recv(s, connect))

# subscribe 
subscribe = "SUBSCRIBE\ndestination:/app/report\nid:sub-0\n\n\0"
print("-> SUBSCRIBE /app/report")
print(send_recv(s, subscribe))

# send
send_report = "SEND\ndestination:/app/report\n\nrequest report\0"
print("-> SEND /app/report")
response = send_recv(s, send_report)
print("<< server response:\n")
print(response)

s.close()