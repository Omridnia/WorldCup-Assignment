#!/usr/bin/env python3
import os
import socket
import sqlite3
import threading


DB_FILENAME = os.path.join(os.path.dirname(__file__), 'db.sqlite')
HOST = '127.0.0.1'
PORT = 7778


def init_db(conn):
    cur = conn.cursor()
    # Users table
    cur.execute('''
    CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT NOT NULL UNIQUE,
        password TEXT NOT NULL,
        registration_date TEXT
    )
    ''')

    # Login history (username used for simplicity to match Java)
    cur.execute('''
    CREATE TABLE IF NOT EXISTS login_history (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT NOT NULL,
        login_time TEXT,
        logout_time TEXT
    )
    ''')

    # File tracking
    cur.execute('''
    CREATE TABLE IF NOT EXISTS file_tracking (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT NOT NULL,
        filename TEXT NOT NULL,
        upload_time TEXT,
        game_channel TEXT
    )
    ''')

    conn.commit()


def handle_client(conn_sock, db_conn):
    try:
        data = bytearray()
        while True:
            chunk = conn_sock.recv(4096)
            if not chunk:
                break
            data.extend(chunk)
            if b'\0' in chunk:
                break

        if not data:
            return

        # strip trailing null and whitespace
        raw = data.split(b'\0', 1)[0].decode('utf-8', errors='replace').strip()
        if raw == '':
            conn_sock.sendall(b"ERROR:Empty query\0")
            return

        cur = db_conn.cursor()
        try:
            sql = raw.strip()
            # Simple detection for SELECT
            if sql.lower().startswith('select'):
                cur.execute(sql)
                rows = cur.fetchall()
                if not rows:
                    resp = 'SUCCESS'
                else:
                    parts = ['SUCCESS']
                    for r in rows:
                        parts.append(str(r))
                    resp = '|'.join(parts)
            else:
                # non-select statements
                cur.execute(sql)
                db_conn.commit()
                # try to return lastrowid when applicable
                try:
                    lr = cur.lastrowid
                    resp = 'SUCCESS' if lr is None else f"SUCCESS|{lr}"
                except Exception:
                    resp = 'SUCCESS'

        except Exception as e:
            resp = 'ERROR:' + str(e)

        # send response terminated by null
        conn_sock.sendall(resp.encode('utf-8') + b'\0')

    finally:
        try:
            conn_sock.close()
        except Exception:
            pass


def serve():
    # ensure db dir exists
    db_dir = os.path.dirname(DB_FILENAME)
    os.makedirs(db_dir, exist_ok=True)

    db_conn = sqlite3.connect(DB_FILENAME, check_same_thread=False)
    init_db(db_conn)

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((HOST, PORT))
    srv.listen(5)
    print(f"SQL server listening on {HOST}:{PORT}, DB: {DB_FILENAME}")

    try:
        while True:
            client, addr = srv.accept()
            t = threading.Thread(target=handle_client, args=(client, db_conn), daemon=True)
            t.start()
    except KeyboardInterrupt:
        print('Shutting down SQL server')
    finally:
        db_conn.commit()
        db_conn.close()
        srv.close()


if __name__ == '__main__':
    serve()
