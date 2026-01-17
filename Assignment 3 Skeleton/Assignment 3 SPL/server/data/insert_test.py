import sqlite3
conn=sqlite3.connect('data/db.sqlite')
c=conn.cursor()
c.execute("INSERT INTO users(username,password,registration_date) VALUES (?,?,datetime('now'))", ("alice","pwd123"))
c.execute("INSERT INTO login_history(username,login_time) VALUES (?,datetime('now'))", ("alice",))
c.execute("INSERT INTO file_tracking(username,filename,upload_time,game_channel) VALUES (?,?,datetime('now'),?)", ("alice","events1.json","usa_mexico"))
conn.commit()
conn.close()
print("Inserted test rows")
