#!/usr/bin/env python
# coding: utf-8

"""
### Summary

This script is designed to retrieve FinDer earthquake data from a PostgreSQL/PostGIS database, process rupture geometry, 
misfit functions (strike and length PDFs), and event information, create files needed for ShakeMap processing, and execute ShakeMaps.

The key steps performed by the script include:

1. **Database Connection**: Connects to a PostgreSQL database and fetches rupture geometry and event data (location, depth, magnitude, time).
2. **ShakeMap Preparation**: Prepares files required for ShakeMap, including event and station XML files.
3. **Geolocation**: Uses Geopy to determine the country code of the event location and configures ShakeMap settings based on it.
4. **ShakeMap Execution**: Launches ShakeMap calculations using the `shake` command.
5. **Restoring Configurations**: After ShakeMap processing, restores the original configuration files.

### Script Usage

Command-line arguments:
1. **event_id**: The ID of the earthquake event.
2. **origin_id**: The origin ID associated with the event.
3. **shakemap_flag**: False or True: Flag that determines if ShakeMap calculation shall be exectuted. (optional)
For example, to run the script:
```bash
python script_name.py --event_id <event_id> origin_id <origin_id> --shakemap_flag <shakemap_flag>
"""

import os
import sys
import logging
import argparse
import shutil
import psycopg2
from psycopg2.extras import RealDictCursor
import geojson
import base64
import json
import subprocess
from geopy.geocoders import Nominatim
import xml.etree.ElementTree as ET
from shapely.wkt import loads
from shapely.geometry import mapping

# Global variables
DBNAME = ""
USER = ""
HOST = ""
PW = ""
PORT = ""

OUTPUT_DIR = '/home/sysop/.seiscomp/log/shakemaps'
SHAKEMAP_CONFIG_DIR = '/home/sysop/shakemap_profiles/default/install/config'
GEOLOCATOR = Nominatim(user_agent="sm")

# Setup logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def create_directory(event_id):
    event_dir = os.path.join(OUTPUT_DIR, event_id)
    current_dir = os.path.join(event_dir, 'current')

    if not os.path.exists(current_dir):
        logging.info(f"Creating event directory for {event_id}")
        os.makedirs(current_dir, exist_ok=True)
    return current_dir


def connect_to_db():
    try:
        conn = psycopg2.connect(f"dbname={DBNAME} user={USER} host={HOST} password={PW} port={PORT}")
        return conn
    except Exception as e:
        logging.error(f"Unable to connect to the database: {e}")
        sys.exit(1)


def fetch_event_data_originbased(cursor, origin_id):
    query = """
    SELECT origin.m_latitude_value AS lat, origin.m_longitude_value AS lon,
           origin.m_depth_value AS dep, magnitude.m_magnitude_value AS mag,
           origin.m_time_value AS time
    FROM rupture
    INNER JOIN publicobject AS rupturepublicobject ON rupture._oid = rupturepublicobject._oid
    INNER JOIN strongorigindescription ON rupture._parent_oid = strongorigindescription._oid
    INNER JOIN publicobject AS originpublicobject ON strongorigindescription.m_originid = originpublicobject.m_publicid
    INNER JOIN origin ON originpublicobject._oid = origin._oid
    INNER JOIN originreference ON originpublicobject.m_publicid = originreference.m_originid
    INNER JOIN event ON originreference._parent_oid = event._oid
    LEFT JOIN magnitude ON magnitude._parent_oid = origin._oid
    WHERE magnitude.m_type = 'Mfd' AND originpublicobject.m_publicid = %s
    """
    cursor.execute(query, (origin_id,))
    return cursor.fetchone()


def fetch_event_data_eventbased(cursor, event_id):
    query = """
    SELECT origin.m_latitude_value AS lat, origin.m_longitude_value AS lon,
           origin.m_depth_value AS dep, magnitude.m_magnitude_value AS mag,
           origin.m_time_value AS time
    FROM rupture
    INNER JOIN publicobject AS rupturepublicobject ON rupture._oid = rupturepublicobject._oid
    INNER JOIN strongorigindescription ON rupture._parent_oid = strongorigindescription._oid
    INNER JOIN publicobject AS originpublicobject ON strongorigindescription.m_originid = originpublicobject.m_publicid
    INNER JOIN origin ON originpublicobject._oid = origin._oid
    INNER JOIN originreference ON originpublicobject.m_publicid = originreference.m_originid
    INNER JOIN event ON originreference._parent_oid = event._oid
    LEFT JOIN magnitude ON magnitude._parent_oid = origin._oid
    INNER JOIN publicobject AS eventpublicobject ON event._oid = eventpublicobject._oid
    WHERE magnitude.m_type = 'Mfd' AND eventpublicobject.m_publicid = %s
    """
    cursor.execute(query, (event_id,))
    return cursor.fetchone()


def fetch_station_data_originbased(cursor, origin_id, rupid):
    query = """
    SELECT event._oid,
           rupture._oid,
           magnitude.m_type,
           magnitude.m_magnitude_value,
           record.m_waveformid_stationcode AS code,
           station.m_description AS name,
           record.m_waveformid_channelcode AS insttype,
           station.m_latitude AS lat,
           station.m_longitude AS lon,
           station.m_affiliation AS source,
           record.m_waveformid_networkcode AS netid,
           station.m_description AS loc,
           record.m_waveformid_channelcode AS comp_name,
           peakmotion.m_motion_value AS acc_value
    FROM rupture
    INNER JOIN publicobject AS rupturepublicobject ON rupture._oid = rupturepublicobject._oid
    INNER JOIN strongorigindescription ON rupture._parent_oid = strongorigindescription._oid
    INNER JOIN publicobject AS originpublicobject ON strongorigindescription.m_originid = originpublicobject.m_publicid
    INNER JOIN origin ON originpublicobject._oid = origin._oid
    INNER JOIN originreference ON originpublicobject.m_publicid = originreference.m_originid
    INNER JOIN event ON originreference._parent_oid = event._oid
    LEFT JOIN magnitude ON magnitude._parent_oid = origin._oid
    INNER JOIN publicobject AS eventpublicobject ON event._oid = eventpublicobject._oid
    INNER JOIN eventrecordreference ON eventrecordreference._parent_oid = strongorigindescription._oid
    INNER JOIN publicobject AS recordpublicobject ON eventrecordreference.m_recordid = recordpublicobject.m_publicid
    INNER JOIN record ON recordpublicobject._oid = record._oid
    INNER JOIN peakmotion ON peakmotion._parent_oid = record._oid
    INNER JOIN station ON record.m_waveformid_stationcode = station.m_code
    WHERE magnitude.m_type='Mfd'
    AND (record.m_waveformid_channelcode ='HG' OR record.m_waveformid_channelcode ='HN' OR record.m_waveformid_channelcode ='HH')
    AND originpublicobject.m_publicid = %s
    AND rupture._oid = %s
    ORDER BY m_waveformid_networkcode, m_waveformid_stationcode, m_waveformid_locationcode, m_waveformid_channelcode
    """
    cursor.execute(query, (origin_id, rupid))
    return cursor.fetchall()


def fetch_station_data_eventbased(cursor, event_id, rupid):
    query = """
    SELECT event._oid,
           rupture._oid,
           magnitude.m_type,
           magnitude.m_magnitude_value,
           record.m_waveformid_stationcode AS code,
           station.m_description AS name,
           record.m_waveformid_channelcode AS insttype,
           station.m_latitude AS lat,
           station.m_longitude AS lon,
           station.m_affiliation AS source,
           record.m_waveformid_networkcode AS netid,
           station.m_description AS loc,
           record.m_waveformid_channelcode AS comp_name,
           peakmotion.m_motion_value AS acc_value
    FROM rupture
    INNER JOIN publicobject AS rupturepublicobject ON rupture._oid = rupturepublicobject._oid
    INNER JOIN strongorigindescription ON rupture._parent_oid = strongorigindescription._oid
    INNER JOIN publicobject AS originpublicobject ON strongorigindescription.m_originid = originpublicobject.m_publicid
    INNER JOIN origin ON originpublicobject._oid = origin._oid
    INNER JOIN originreference ON originpublicobject.m_publicid = originreference.m_originid
    INNER JOIN event ON originreference._parent_oid = event._oid
    LEFT JOIN magnitude ON magnitude._parent_oid = origin._oid
    INNER JOIN publicobject AS eventpublicobject ON event._oid = eventpublicobject._oid
    INNER JOIN eventrecordreference ON eventrecordreference._parent_oid = strongorigindescription._oid
    INNER JOIN publicobject AS recordpublicobject ON eventrecordreference.m_recordid = recordpublicobject.m_publicid
    INNER JOIN record ON recordpublicobject._oid = record._oid
    INNER JOIN peakmotion ON peakmotion._parent_oid = record._oid
    INNER JOIN station ON record.m_waveformid_stationcode = station.m_code
    WHERE magnitude.m_type='Mfd' AND (record.m_waveformid_channelcode ='HG' OR record.m_waveformid_channelcode ='HN' OR record.m_waveformid_channelcode ='HH')
    AND eventpublicobject.m_publicid = %s 
    AND rupture._oid = %s
    ORDER BY m_waveformid_networkcode, m_waveformid_stationcode, m_waveformid_locationcode, m_waveformid_channelcode
    """
    cursor.execute(query, (event_id, rupid))
    return cursor.fetchall()


def fetch_rupture_geometry_originbased(cursor, origin_id):
    query = """
    SELECT rupture.m_rupturegeometrywkt AS fault, rupture._oid AS rupid
    FROM rupture
    INNER JOIN publicobject AS rupturepublicobject ON rupture._oid = rupturepublicobject._oid
    INNER JOIN strongorigindescription ON rupture._parent_oid = strongorigindescription._oid
    INNER JOIN publicobject AS originpublicobject ON strongorigindescription.m_originid = originpublicobject.m_publicid
    INNER JOIN origin ON originpublicobject._oid = origin._oid
    INNER JOIN originreference ON originpublicobject.m_publicid = originreference.m_originid
    INNER JOIN event ON originreference._parent_oid = event._oid
    WHERE originpublicobject.m_publicid = %s
    ORDER BY rupture._last_modified DESC LIMIT 1
    """
    cursor.execute(query, (origin_id,))
    result = cursor.fetchone()
    return result[0], result[1]  # fault WKT and rupture ID


def fetch_rupture_geometry_eventbased(cursor, event_id):
    query = """
    SELECT rupture.m_rupturegeometrywkt AS fault, rupture._oid AS rupid
    FROM rupture
    INNER JOIN publicobject AS rupturepublicobject ON rupture._oid = rupturepublicobject._oid
    INNER JOIN originreference ON rupture.m_centroidreference = originreference.m_originid
    INNER JOIN publicobject AS originpublicobject ON rupture.m_centroidreference = originpublicobject.m_publicid
    INNER JOIN origin ON originpublicobject._oid = origin._oid
    INNER JOIN event ON originreference._parent_oid = event._oid \
    INNER JOIN publicobject AS eventpublicobject ON event._oid = eventpublicobject._oid \
    WHERE eventpublicobject.m_publicid = %s
    ORDER BY rupture._last_modified DESC LIMIT 1
    """
    cursor.execute(query, (event_id,))
    result = cursor.fetchone()
    return result[0], result[1]  # fault WKT and rupture ID


def fetch_misfit_functions_originbased(cursor, origin_id):
    query = """
    SELECT eventpublicobject.m_publicid AS eventpublicid,
        originpublicobject.m_publicid AS originpublicid,
        rupturepublicobject.m_publicid AS ruptureid,
        origin.m_latitude_value AS centroid_lat, 
        origin.m_longitude_value AS centroid_lon, 
        origin.m_depth_value AS centroiddepth, 
        origin.m_time_value AS centroid_time,
        rupture.m_width_value AS rupturewidth, 
        rupture.m_displacement_value AS displacement,
        rupture.m_risetime_value AS risetime, 
        rupture.m_vt_to_vs_value AS vt_to_vs,
        rupture.m_shallowasperitydepth_value AS shallowasperitydepth,
        rupture.m_shallowasperity AS shallowasperity,
        rupture.m_strike_value AS strike,
        encode(rupture.m_strike_pdf_variable_content, 'escape'::character varying) AS strike_pdf_variable_content,
        encode(rupture.m_strike_pdf_probability_content, 'escape'::character varying) AS strike_pdf_probability_content,
        rupture.m_length_value AS rupturelength,
        encode(rupture.m_length_pdf_variable_content, 'escape'::character varying) AS length_pdf_variable_content,
        encode(rupture.m_length_pdf_probability_content, 'escape'::character varying) AS length_pdf_probability_content,
        rupture.m_area_value AS rupturearea, 
        rupture.m_rupturevelocity_value AS rupturevelocity,
        rupture.m_stressdrop_value AS stressdrop, 
        rupture.m_momentreleasetop5km_value AS momentrelease_top5km,
        rupture.m_fwhwindicator, 
        rupture.m_rupturegeometrywkt, 
        rupture.m_faultid
    FROM rupture
    INNER JOIN strongorigindescription ON strongorigindescription._oid = rupture._parent_oid
    INNER JOIN publicobject AS rupturepublicobject ON rupture._oid = rupturepublicobject._oid
    INNER JOIN originreference ON strongorigindescription.m_originid = originreference.m_originid
    INNER JOIN publicobject AS originpublicobject ON strongorigindescription.m_originid = originpublicobject.m_publicid
    INNER JOIN origin ON originpublicobject._oid = origin._oid
    INNER JOIN event ON originreference._parent_oid = event._oid
    INNER JOIN publicobject AS eventpublicobject ON event._oid = eventpublicobject._oid
    WHERE originpublicobject.m_publicid = %s
    ORDER BY rupture._oid DESC
    """
    cursor.execute(query, (origin_id,))
    row = cursor.fetchone()

    if row:
        # Extract the relevant fields (the PDFs)
        strike_pdf_variable_content = row['strike_pdf_variable_content']
        strike_pdf_probability_content = row['strike_pdf_probability_content']
        length_pdf_variable_content = row['length_pdf_variable_content']
        length_pdf_probability_content = row['length_pdf_probability_content']

        # Convert these blank-separated strings into lists
        strike_values = strike_pdf_variable_content.split()  # List of strike values
        strike_probabilities = strike_pdf_probability_content.split()  # List of strike probabilities
        length_values = length_pdf_variable_content.split()  # List of length values
        length_probabilities = length_pdf_probability_content.split()  # List of length probabilities

        # Structure this into a JSON-friendly dictionary
        pdf_data = {
            "strike_pdf": {
                "values": strike_values,
                "probabilities": strike_probabilities
            },
            "length_pdf": {
                "values": length_values,
                "probabilities": length_probabilities
            }
        }

        # Convert the dictionary to JSON format
        json_data = json.dumps(pdf_data)

        return json_data
    else:
        return None


def create_geojson(fault_wkt):
    return geojson.dumps(mapping(loads(fault_wkt)))


def create_event_xml(event_data, event_id):
    root = ET.Element("earthquake", id=event_id, netid="SED-INGV",
                      lat=str(event_data[0]), lon=str(event_data[1]),
                      depth=str(event_data[2]), mag=str(event_data[3]),
                      locstring="DT-GEO Finder Test", network="SED-INGV",
                      time=str(event_data[4]).replace(" ", "T") + "Z")
    tree = ET.ElementTree(root)
    xml_path = os.path.join(OUTPUT_DIR, event_id, 'current', 'event.xml')
    tree.write(xml_path, encoding='us-ascii', xml_declaration=True)
    logging.info(f"Created {xml_path}")


def create_station_xml(station_data, event_id):
    # Initialize the XML structure with the header
    xml_header = '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<!DOCTYPE earthquake SYSTEM "stationlist.dtd">
<stationlist created="" xmlns="ch.ethz.sed.shakemap.usgs.xml">
'''
    # List to store all station data strings
    stations_data = []

    # Iterate through all rows returned by the cursor (database query results)
    for station in station_data:
        # Format each station entry using f-string for cleaner code
    	station_str = f'''
    	<station code="{station[4]}" name="{station[5]}" insttype="{station[6]}" lat="{station[7]}" lon="{station[8]}"
               	source="{station[9]}" commtype="DIG" netid="{station[10]}" loc="{station[5]}">
        	<comp name="{station[6]}N">
            	<acc value="{station[13] / 9.81}" flag="0" />
        	</comp>
    	</station>
    	'''
    	# Append the station entry to the list
    	stations_data.append(station_str)

    # Join all station data into a single string
    stations_str = ''.join(stations_data)

    # Closing XML tag
    xml_footer = '</stationlist>'

    # Combine the header, body (stations), and footer into the full XML content
    xml_content = xml_header + stations_str + xml_footer

    # Define the full path for saving the XML
    xml_path = os.path.join(OUTPUT_DIR, event_id, 'current', 'event_dat.xml')

    # Write the full XML content to the file
    with open(xml_path, 'w') as file:
        file.write(xml_content)
    logging.info(f"Created {xml_path}")


def write_json_to_file(data, filename):
    # Writes the data to a JSON file
    with open(filename, 'w') as json_file:
        json.dump(data, json_file, indent=4)


def copy_config_files(country_code):
    config_files = {
        'it': 'italy',
        'ch': 'switzerland'
    }
    if country_code not in config_files:
        logging.warning(f"No configuration for country code: {country_code}")
        return

    country_dir = config_files[country_code]
    for conf_file in ['gmpe_sets.conf', 'model.conf', 'modules.conf', 'select.conf']:
        src = os.path.join(SHAKEMAP_CONFIG_DIR, country_dir, conf_file)
        dest = os.path.join(SHAKEMAP_CONFIG_DIR, conf_file)
        shutil.copy2(src, dest)
        logging.info(f"Copied {conf_file} for {country_code}")


def get_country_code(lat, lon):
    location = GEOLOCATOR.reverse((lat, lon))
    country_code = location.raw['address'].get('country_code', '')
    logging.info(f"Event located in country: {country_code}")
    return country_code


def parse_arguments():
    """Parse command-line arguments using argparse"""
    parser = argparse.ArgumentParser(description="Process event data.")
    parser.add_argument('--event_id', required=True, help="Event ID")
    parser.add_argument('--origin_id', default='None', help="Origin ID (default is 'None')")
    parser.add_argument('--shakemap_flag', type=bool, default=False, help="Shakemap Flag (default is False)")    
    return parser.parse_args()


def main():
    # Parse arguments
    args = parse_arguments()

    event_id = args.event_id
    origin_id = args.origin_id
    shakemap_flag = args.shakemap_flag

    current_dir = create_directory(event_id)

    # Connect to the database and handle errors
    try:
        conn = connect_to_db()
        cursor = conn.cursor()
    except Exception as e:
        logging.error(f"Failed to connect to the database: {e}")
        sys.exit(1)

    # Fetch event data, rupture geometry, station data, and misfit functions for rupture length and strike:
    if origin_id != 'None':
        event_data = fetch_event_data_originbased(cursor, origin_id)
        fault_wkt, rupture_id = fetch_rupture_geometry_originbased(cursor, origin_id)
        station_data = fetch_station_data_originbased(cursor, origin_id, rupture_id)
    else:
        event_data = fetch_event_data_eventbased(cursor, event_id)
        fault_wkt, rupture_id = fetch_rupture_geometry_eventbased(cursor, event_id)
        station_data = fetch_station_data_eventbased(cursor, event_id, rupture_id)
    
    # COULD WE HAVE AN EVENT-BASED SQL COMMAND?
    misfit_data_json = fetch_misfit_functions_originbased(conn.cursor(cursor_factory=RealDictCursor), origin_id)

    # Create event XML file
    create_event_xml(event_data, event_id)

    # Create event_dat XML file (from station data)
    create_station_xml(station_data, event_id)  # Replace with actual station data

    # Create GeoJSON for rupture
    geojson_str = create_geojson(fault_wkt)
    geojson_path = os.path.join(current_dir, 'rupture.json')
    with open(geojson_path, 'w') as f:
        f.write(f'{{ "type": "FeatureCollection", "metadata": {{ "reference": "FinDer DTGEO" }} , "features": [{{ "type": "Feature", "properties": {{ "rupture type": "rupture extent" }} , "geometry": {geojson_str} }} ] }}')
    logging.info(f"Created {geojson_path}")

    # Create JSON for misfit
    if misfit_data_json:
        # Parse the JSON string into a Python dictionary
        pdf_data = json.loads(misfit_data_json)
        path = os.path.join(OUTPUT_DIR, event_id, 'current', 'event_dat.xml')
        # Write the PDFs to two separate JSON files
        write_json_to_file(pdf_data["strike_pdf"], os.path.join(OUTPUT_DIR, event_id, 'current', 'strike_pdf.json'))
        write_json_to_file(pdf_data["length_pdf"], os.path.join(OUTPUT_DIR, event_id, 'current', 'length_pdf.json'))
        logging.info(f"PDF data has been written to strike_pdf.json and length_pdf.json")
    else:
        logging.info(f"No misfit data found for the given origin ID.")

    # Cleanup and close the database connection
    cursor.close()
    conn.close()
    
    # Start ShakeMap calculation
    if shakemap_flag:
        # Get country code and copy config files
        country_code = get_country_code(event_data[0], event_data[1])
        copy_config_files(country_code)

       # Run ShakeMap calculations
        logging.info(f"Launching ShakeMap calculations for {event_id}")
        subprocess.call([
            "/home/sysop/miniconda/envs/shakemap/bin/shake", "-d", event_id, "select", "assemble",
            "-c", "dtgeosf-test", "model", "contour", "mapping", "transfer_email"
        ])
 
        # Restore original config files
        shutil.copy2(os.path.join(SHAKEMAP_CONFIG_DIR, 'gmpe_sets.conf.orig'), os.path.join(SHAKEMAP_CONFIG_DIR, 'gmpe_sets.conf'))
        shutil.copy2(os.path.join(SHAKEMAP_CONFIG_DIR, 'model.conf.orig'), os.path.join(SHAKEMAP_CONFIG_DIR, 'model.conf'))
        shutil.copy2(os.path.join(SHAKEMAP_CONFIG_DIR, 'modules.conf.orig'), os.path.join(SHAKEMAP_CONFIG_DIR, 'modules.conf'))
        shutil.copy2(os.path.join(SHAKEMAP_CONFIG_DIR, 'select.conf.orig'), os.path.join(SHAKEMAP_CONFIG_DIR, 'select.conf'))

        logging.info("ShakeMap calculation is done.")


if __name__ == '__main__':
    main()
