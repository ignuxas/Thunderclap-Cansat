import serial
import time
import pymongo

filename = "data.txt"

def parse_nmea_gga(lat, long):
    # Parse latitude and longitude
    latitude_deg = int(lat[:2])
    latitude_min = float(lat[2:])

    longitude_deg = int(long[:3])
    longitude_min = float(long[3:])
    
    # Convert minutes to decimal degrees and add to integer degrees
    latitude = f"{latitude_deg + latitude_min / 60:.6f}"
    longitude = f"{longitude_deg + longitude_min / 60:.6f}"

    return latitude, longitude

# Open the serial port
ser = serial.Serial('COM4', 115200, write_timeout=0) 

# Set up the connection details
mongo_uri = "mongodb://localhost:27017"  # Replace with your MongoDB URI
database_name = "cansat"  # Replace with your database name

# Create a MongoClient object
client = pymongo.MongoClient(mongo_uri)

# Access the database
db = client[database_name]


# Now you can perform operations on the database
# For example, you can access a collection and retrieve documents
collection = db["cansat"]  # Replace with your collection name

# Read and print the data
while True:
    data = ser.readline()

    try:
        if(data[0] == 0 and len(data) > 3):
            data = data[3:]
        data = data.decode('utf-8').strip().split("\n")

        for line in data:
            passed = True
            dataList = line.split(" ")
            dataToDb = {}

            try: dataToDb = {
                "temp": float(dataList[0]),
                "hum": int(dataList[1]),
                "ax": float(dataList[5]),
                "ay": float(dataList[6]),
                "az": float(dataList[7]),
                "gx": float(dataList[2]),
                "gy": float(dataList[3]),
                "gz": float(dataList[4]),
                "mT": int(dataList[9]),
                "time": time.time(),
                "timeGps": None,
                "lat": None,
                "lon": None,
                "alt": None,
                "sat": None,
                "hdop": None,
                "geoid": None,
            } 
            except: passed = False

            if(passed):
                #GPS
                try:
                    if(dataList[8] != "0"):
                        #GPGGA
                        gpsData = dataList[8].split(",")
                        dataToDb["timeGps"] = float(gpsData[1])
                        dataToDb["lat"], dataToDb["lon"] = parse_nmea_gga(gpsData[2], gpsData[4])
                        dataToDb["alt"] = float(gpsData[9])
                        dataToDb["sat"] = int(gpsData[7])
                        dataToDb["hdop"] = float(gpsData[8])
                        dataToDb["geoid"] = float(gpsData[11])
                except: dataToDb["timeGps"] = dataToDb["lat"] = dataToDb["lon"] = dataToDb["alt"] = dataToDb["sat"] = dataToDb["hdop"] = dataToDb["geoid"] = None
        
    except:
        print("Error")
        continue
    allNone = True
    for key in dataToDb:
        if(dataToDb[key] != None and key != "time"):
            allNone = False
            break

    if(not allNone):
        collection.insert_one(dataToDb)

    print(data)

# Close the serial port
ser.close()

# Remember to close the connection when you're done
client.close()