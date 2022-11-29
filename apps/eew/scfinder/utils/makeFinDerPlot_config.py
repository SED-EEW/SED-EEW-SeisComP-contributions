""" 
 Copyright (C) by ETHZ/SED 
  
 This program is free software: you can redistribute it and/or modify 
 it under the terms of the GNU Affero General Public License as published 
 by the Free Software Foundation, either version 3 of the License, or 
 (at your option) any later version. 
  
 This program is distributed in the hope that it will be useful, 
 but WITHOUT ANY WARRANTY; without even the implied warranty of 
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 GNU Affero General Public License for more details. 
  
 Author of the Software: Jen Andrews (GNS)
 """ 

### Configurable arguments ###
# 'pga_dir_loc': The directory where FinDer will write its data_ files (finder.config DATA_FOLDER/temp_data)
# 'plot_dir': Directory where the plots will be stored, should exist
# 'scxmldump_cfg': Full path filename for configuration file scxmldump.cfg
# 'event_timeout': Used to determine when to send out email if running in SeisComP mode. Time in seconds after last update in the db.
# 'sleep_time': Loop to check if event_timeout has been reached uses this time in seconds to sleep.
# 'retries': Number of retries to get successful scxmldump.
# 'cities_file': Cities file for plotting, expects format same as ShakeMap cities.txt file
# 'faults_file': Faults shapefile
# 'mask_file': FinDer mask file (*.nc)
# 'plot_stns': True | False for plotting stations as triangles coloured by PGA
# 'plot_interp': True | False for plotting interpolated PGA using pyGMT commands
# 'email_recipients': [comma-separated list of email recipients]
# 'email_sender': email sender address
# 'subj_prefix': 'optional prefix to email subject line'
# 'email_header': 'optional file containing email header text'
# 'email_footer': 'optional file containing email footer text'
# 'email_footerfig': 'optional image file for footer'
# 'smtp_server': 'email server'
# 'email_port': 'email port'
# 'email_username': 
# 'email_password': 
# 'email_timelimit': Max age of event in seconds for email to be sent, older events are ignored
# 'email_maglimit': Min magnitude of event for email to be sent, smaller events are ignored
# 'email_sanitychk': Sanity check for event email, criteria for noise are few updates (<5) and large time delay from origin (>20s)
# 'tls': True | False Email TLS
# 'auth': True | False Email authorisation required (username, password above)
# 'ssl': True | False Email SSL
conf = {
        'pga_dir_loc': 'temp_data',
        'plot_dir': 'plots',
        'lastplotonly': True,
        'scxmldump_cfg': 'scxmldump.cfg',
        'event_timeout': 20.,
        'sleep_time': 2.,
        'retries': 20,
        'cities_file': 'cities1000.txt',
        'faults_file': 'faults.shp',
        'mask_file': 'mask.nc',
        'plot_stns': True,
        'plot_interp': True,
        'plot_PGA_units': 'g', # Use g, %g or cm/s/s, otherwise it will default to cm/s/s
        'email_recipients': ['nemo@nemo.com'],
        'email_sender': 'sender@host',
        'subj_prefix': '',
        'email_header': '',
        'email_footer': 'footer.txt',
        'email_footerfig': 'footerfig.png',
        'smtp_server': 'email_server',
        'email_port': '9999',
        'email_username': '',
        'email_password': '',
        'email_timelimit': 600.,
        'email_maglimit': 3.,
        'email_sanitychk': 'yes',
        'tls': False,
        'auth': False,
        'ssl': False,
}

