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


import os, sys, logging
from matplotlib.figure import Figure
from matplotlib.colors import LogNorm
from obspy import UTCDateTime
from time import sleep
import json
import xml.etree.ElementTree as ET 
import geographiclib.geodesic as geo
from subprocess import Popen, PIPE
import pygmt
try:
    import smtplib
    from email.mime.text import MIMEText
    from email.mime.multipart import MIMEMultipart
    from email.mime.application import MIMEApplication
    from email.mime.image import MIMEImage
    from email.header import Header
    from email.utils import formatdate
except ImportError as e:
    print('%s (using Python%d.%d)' % (e,sys.version_info.major,sys.version_info.minor))
    raise e
# Configurations 
from makeFinDerPlot_config import conf


def prepareFinDerHTML(fdsols, fdevent):
    '''
    FinDer's final JSON output.
    Note that this is modelled on w-phase WPhaseResult class (psi/model.py)
    Args:
        fdsols: List of FinDer solutions dictionaries
        fdevent: Final FinDer event parameters
    Return:
        Email subject line, Email HTML body text 
    '''
    def addText(fstr):
        nonlocal body
        if len(fstr) > 0 and os.path.isfile(fstr):
            for line in open(fstr, 'r').readlines():
                body += line.replace('\n','<br/>')
        body += '<br/>'
        return

    # Email subject line
    subject = '%s FinDer M%.1f (%s; %s UTC)' % (conf['subj_prefix'], fdsols[-1]['mag'], \
            fdevent['locstr'], fdsols[-1]['t0'].strftime('%Y-%m-%d %H:%M'))

    body = '<html>\n<meta name="viewport" content="width=device-width, initial-scale=1.0"/>\n<head>\n<style>\n.content {width: 100%; max-width: 620px;}\nimg { max-width: 100%; height: auto; margin:5px 5px 5px 5px;display:block;vertical-align:middle;float:center; }\npre {max-width: 300px;display: block;font-family: monospace;white-space: pre;margin: 1em 0; font-size: 11px; }\n</style>\n</head>\n<body>\n<p>\n'
    # READ AN OPTIONAL HEADER FILE DEFINED BY USER
    addText(conf['email_header'])

    # Summary of final FinDer solution
    lastt = max([f['vtime'] for f in fdsols])
    fdsol = [f for f in fdsols if f['vtime'] == lastt][0]
    body += '<p>Final FinDer (%s) solution for this event (%s) has the following rupture parameters:</p>\n' % (fdsol['author'], fdevent['evid'])
    body += '<ul>'
    body += '<li>Fault length: %.1f km</li>'%fdsol['flen']
    body += '<li>Fault strike: %d</li>'%fdsol['fstrike']
    body += '<li>Fault end points: (%.3f, %.3f) (%.3f, %.3f)</li>' % \
            (fdsol['fcoords'][0][0], fdsol['fcoords'][0][1], \
                    fdsol['fcoords'][-1][0], fdsol['fcoords'][-1][1])
    body += '<li>Magnitude: %.1f</li>'%fdsol['mag']
    body += '</ul>\n'
    body += '<br/>'
    body += '<img src="cid:image2" alt="solutionmap" align=middle height="450">'
    body += '<br/>'
    body += '<img src="cid:image1" alt="timeevolution" align=middle width="700">'
    body += '<br/>\n'

    # Table of all FinDer solutions
    body += '<p>Full timeseries of FinDer solutions for this event:</p>\n'
    body += '<table border=1, style="border: 2px, solid; border-collapse: collapse; font-size: 14px;">\n'
    body += '<tr>'
    for head in ['Mag', 'Lat', 'Lon', 'Depth', 'Origin Time (UTC)', 'Likelihood', 'Length', 'Strike', 'Author', 'Creation time']:
        body += '<th style="padding: 2px;">' + head + '</th>'
    body += '</tr>'
    for fdsol in sorted(fdsols, key=lambda x:x['vtime']):
        body += '<tr>'
        for w in ['mag', 'elat', 'elon', 'depth', 't0', 'uncr', 'flen', 'fstrike', 'author', 'vtime']:
            if w in fdsol:
                if w == 'author':
                    val = fdsol[w]
                elif w in ['t0', 'vtime']:
                    val = fdsol[w].strftime('%Y-%m-%dT%H:%M:%SZ')
                else:
                    val = '%.1f'%fdsol[w]
            else:
                val = ''
            body += '<td style="padding: 1px; text-align: center;">' + val + '</td>'
        body += '</tr>\n'
    body += '</table><br/>\n'

    # ADD TIMESTAMP
    body += '<i>Message issued on %s<br/><br/></i>\n' % UTCDateTime().now().strftime("%Y-%m-%d at %H:%M:%S UTC")

    # READ AN OPTIONAL FOOTER FILE DEFINED BY USER
    addText(conf['email_footer'])
    if os.path.isfile(conf['email_footerfig']):
        body += '<br/><br/>'
        body += '<img src="cid:image0" alt="rcet" align=middle height="100">'
        body += '<br/><br/>'
    body += '</p>\n</body>\n</html>'
    return subject, body


def sendHTML(subject, body, figbody, attachment):
    '''
    Send the HTML email as MIMEMultipart
    Args:
        subject: email subject line
        body: email body
        figbody: List of FinDer figures to include in the body
        attachment: kml attachemnt
    Return:
        True if email could be created and sent
    '''
    mail = MIMEMultipart('related')

    # Email body
    mailhtml = MIMEText(body,'html')
    mail.attach(mailhtml)

    # Attach images
    figbody.append(conf['email_footerfig'])
    for iid, figname in zip(range(len(figbody)-1, -1, -1), figbody):
        try:
            if os.path.isfile(figname):
                fp = open(figname, 'rb')
                msgImage = MIMEImage(fp.read())
                fp.close()
                # Define the image's ID as referenced above in htmlcode
                msgImage.add_header('Content-ID', '<image%d>'%iid)
                msgImage.add_header('content-disposition', 'attachment',\
                                    name=os.path.basename(figname),\
                                    filename=os.path.basename(figname))
                mail.attach(msgImage)
        except Exception as e:
            logging.error("Image '%s', %s" % (os.path.basename(figname),e))
            return False

    # Attach the kml
    try:
        with open(attachment, "rb") as f:
            part = MIMEApplication(f.read(), Name = os.path.basename(attachment))
#        # After the file is closed
        part['Content-Disposition'] = 'attachment; filename="%s"' % os.path.basename(attachment)
        mail.attach(part)
    except Exception as e:
        logging.error("Attachment '%s', %s" % (os.path.basename(attachment),e))

    # Sender info
    mail['bcc']     = ','.join(conf['email_recipients'])
    mail['to']      = conf['email_sender']
    mail['from']    = conf['email_sender']
    mail['date']    = formatdate()
    mail['subject'] = Header(subject)

    # Launch the SMTP connection
    try:
        if conf['ssl']:
            s = smtplib.SMTP_SSL(host=conf['smtp_server'], port=conf['email_port'], timeout=5)
        else:
            s = smtplib.SMTP(host=conf['smtp_server'], port=conf['email_port'], timeout=5)
    except Exception as e:
        logging.warning('Cannot connect to smtp server: %s' % e)
        return False

    # Set any connection info and send
    try:
        if conf['tls']:
            s.starttls()
        if conf['auth']:
            s.login(conf['email_username'], conf['email_password'])
        s.sendmail(conf['email_sender'], conf['email_recipients'], mail.as_string())
        logging.info('A mail as been sent (format html).')
    except Exception as e:
        logging.warning('Email could not be sent: %s' % e)
    s.quit()
    return True


def setBasemap(bounds = None):
    '''
    Create the basic map, including projection, extent. Add coastlines, gridlines and background
    image based on Stamen terrain map tiles.
    Args:
        bounds: xmin, xmax, ymin, ymax as list or tuple
    Returns:
        fig, axes, proj: figure, axis and projection 
    '''
    import cartopy.crs as ccrs
    from cartopy.io.img_tiles import Stamen
    tiler = Stamen('terrain-background')
    proj = tiler.crs
    fig = Figure()
    ax = fig.add_subplot(111, projection = proj)
    if bounds == None:
        bounds = [-47.5, -34.0, 166.0, 179.0]
    ax.set_extent(bounds, crs=ccrs.PlateCarree())
    ax.add_image(tiler, 8, alpha=0.7, zorder=0.)
    # ax.coastlines(resolution='50m') # requires download
    gl = ax.gridlines(draw_labels=True, linestyle='--')
    gl.top_labels = False
    gl.right_labels = False
    return fig, ax, ccrs.PlateCarree()


def importCityFile(fname):
    '''
    Import the city file provided (originally this comes from ShakeMap cities1000.
    Args:
        fname: filename with path
    Returns:
        df: pandas dataframe containing city data including name, location, population
    '''
    from numpy import array
    from pandas import DataFrame
    CAPFLAG1 = 'PPLC'
    CAPFLAG2 = 'PPLA'
    mydict = {'name': [],
              'ccode': [],
              'lat': [],
              'lon': [],
              'iscap': [],
              'pop': []}
    f = open(fname, 'r')
    for line in f.readlines():
        parts = line.split('\t')
        tname = parts[2].strip()
        if not tname:
            continue
        myvals = array([ord(c) for c in tname])
        if len((myvals > 127).nonzero()[0]):
            continue
        mydict['name'].append(tname)
        mydict['ccode'].append(parts[8].strip())
        mydict['lat'].append(float(parts[4].strip()))
        mydict['lon'].append(float(parts[5].strip()))
        capfield = parts[7].strip()
        iscap = (capfield == CAPFLAG1) or (capfield == CAPFLAG2)
        mydict['iscap'].append(iscap)
        mydict['pop'].append(int(parts[14].strip()))
    f.close()
    df = DataFrame.from_dict(mydict)
    return df


def getFinDerDataFileTimes(ddir):
    '''
    Get creation time of the data_ files to enable solution matching
    Args:
        ddir: data directory containing FinDer data_ files
    Returns:
        filetimes: list of filename and file creation time pairs
    '''
    filetimes = []
    for f in os.listdir(ddir):
        if f.find('data_') == -1 or f.find('.') != -1:
            continue
        filetimes.append([f, UTCDateTime(os.path.getctime(os.path.join(ddir, f)))])
    return filetimes


def importFinDerDataFile(fname, pgathresh = None):
    '''
    Import one data_ file created by FinDer (used as input to finder_file)
    Args:
        fname: filename with path of the data_ file
        pgathresh: PGA threshold in cm/s/s to apply (minimum) when reading data in
    Returns:
        pgadata: list of pgadata with entries [lon, lag, pga] with pga in log10(PGA) [cm/s/s]
    '''
    from math import log10
    fin = open(fname, 'r')
    pgadata = []
    for l in fin:
        if l.startswith('#'):
            continue
        fs = l.split()
        pga = float(fs[-1])
        if pgathresh != None:
            if len(fs) > 3:
                if pga < pgathresh:
                    continue
                pgaout = log10(float(pga))
            else:
                if pga < log10(pgathresh):
                    continue
                pgaout = float(pga)
        lon = float(fs[1])
        lat = float(fs[0])
        pgadata.append([lon, lat, pgaout])
    fin.close()
    return pgadata


def importFinDerLog(fname):
    '''
    Read FinDer log to obtain FinDer solutions
    Args:
        fname: filename with path of FinDer log file
    Returns:
        fdsols: list of FinDer solution dictionaries. Dictionaries contain entries for: 
        version number and time, magnitudes, fault length, strike, epicentral location, 
        fault coordinates, uncertainty, tstep
    '''
    allfdevents = {}
    fin = open(fname, 'r')
    tstepstr = ''
    for l in fin:
        if l.find('calculate_origin_time: epi lat') > -1:
            fs = l.split()
            elat = float(fs[-6])
            elon = float(fs[-1])
            continue
        if l.find('calculate_origin_time: min_time') > -1:
            fs = l.split()
            origin_time = UTCDateTime(float(fs[-5]) - float(fs[-1]))
            continue
        if l.find('Likelihood_estimate from lat/lon uncr calculation block') > -1:
            uncr = float(l.split()[-1])
            continue
        if l.find('the input data file for this time step is') > -1:
            tstepstr = l.split()[-1].split('_')[-1].rstrip().lstrip()
            continue
        if l.find(' REGRESSION') > -1:
            if l.find('FAILED') > -1 or l.find('NO MAG') > -1:
                fdsol['mag_regr'] = -1.
                fdsol['mag'] = fdsol['mag_rup']
            elif l.find('NOT USED') > -1:
                fdsol['mag_regr'] = float(l.split()[-6])
                fdsol['mag'] = fdsol['mag_rup']
            else:
                fdsol['mag_regr'] = float(l.split()[-1])
                fdsol['mag'] = fdsol['mag_regr']
            fdsol['author'] = 'finder-offline'
            continue
        if l.find('Event_ID') > -1:
            evid = l.split('=')[1].rstrip().lstrip()
            if evid not in allfdevents:
                allfdevents[evid] = [fdsol]
            else:
                allfdevents[evid].append(fdsol)
            continue
        if l.find('SOLUTION RUPTURE') > -1:
            fdsol = {}
            if len(tstepstr) > 0:
                fdsol['tstep'] = int(tstepstr)
            fs = l.replace(',','').split()
            fdsol['mag_rup'] = float(fs[-1])
            fdsol['flen'] = float(fs[-7])
            fdsol['fstrike'] = float(fs[-4])
            fdsol['version'] = int(fs[-22])
            fdsol['vtime'] = UTCDateTime().strptime('%s %s'%(fs[-27], fs[-26]), '%Y-%m-%d %H:%M:%S:%f')
            fdsol['uncr'] = uncr
            fdsol['elat'] = elat
            fdsol['elon'] = elon
            fdsol['t0'] = origin_time
            continue
        if l.find('SOLUTION COORDINATES') > -1:
            fs = l.replace(',','').split()
            fdsol['fcoords'] = [[float(fs[-21]), float(fs[-17])], [float(fs[-13]), float(fs[-9])], [float(fs[-5]), float(fs[-1])]]
            continue
    fin.close()
    return allfdevents


def scxml2fdsol(xml):
    '''
    Reads the XML produced by scxmldump (SeisComp utility to dump db contents for an event ID) and
    populates a FinDer solution dictionary (fdsol) that is passed for plotting.
    Args:
        xml: XML output from scxmldump
    Returns:
        fdsols: list of FinDer solution dictionaries. Dictionaries contain entries for: 
        version number and time, magnitudes, fault length, strike, epicentral location, 
        fault coordinates, uncertainty
    '''

    def getElem(obj, name1, name2):
        if obj is None:
            return "-9"
        if obj.find('solution:%s'%name1, ns) is None:
            return "-9"
        return obj.find('solution:%s'%name1, ns).find('solution:%s'%name2, ns).text.rstrip().lstrip()

    def getLatLon(lat, lon, azi, dist):
        dist *= 1000.0
        ret = geo.Geodesic.WGS84.Direct(lat, lon, azi, dist)
        return [ret['lat2'], ret['lon2']]

    logging.info('scxml2fdsol')
    # Read the xml
    try:
        root = ET.fromstring(xml)
    except:
        return False, [], {}, 0.

    # Parse following https://docs.python.org/3/library/xml.etree.elementtree.html#parsing-xml
    ns = {'solution': root.tag.split('{')[1].split('}')[0]}

    # Get the overall event info
    fdevent = {}
    event = [x for x in root.iter('{%s}event' % ns['solution'])]
    if len(event) > 0:
        fdevent['evid'] = event[0].attrib['publicID'].rstrip()
        fdevent['locstr'] = getElem(event[0], 'description', 'text')
    else:
        return False, [], {}, 0.

    # Get the full origin set
    searchtag = '{%s}origin' % ns['solution']
    fdsols = []
    for origin in root.iter(searchtag):
        creationtime = UTCDateTime().strptime(getElem(origin, 'creationInfo', 'creationTime'), \
                '%Y-%m-%dT%H:%M:%S.%fZ')
        fdsol = [f for f in fdsols if f['vtime'] == creationtime]
        if len(fdsol) == 0: 
            fdsol = {}
            fdsol['vtime'] = creationtime
            fdsol['t0'] = UTCDateTime().strptime(getElem(origin, 'time', 'value'), '%Y-%m-%dT%H:%M:%S.%fZ')
            fdsols.append(fdsol)
        else:
            fdsol = fdsol[0]
        otype = origin.find('solution:type', ns)
        if otype == None:
            fdsol['elat'] = float(getElem(origin, 'latitude', 'value'))
            fdsol['elon'] = float(getElem(origin, 'longitude', 'value'))
            fdsol['depth'] = float(getElem(origin, 'depth', 'value'))
        elif otype.text == 'centroid':
            fdsol['clat'] = float(getElem(origin, 'latitude', 'value'))
            fdsol['clon'] = float(getElem(origin, 'longitude', 'value'))
            fdsol['author'] = getElem(origin, 'creationInfo', 'author').split('@')[0]
        for mag in origin.findall('solution:magnitude', ns):
            mtype = mag.find('solution:type', ns).text.rstrip().lstrip()
            if mtype == 'Mfd':
                fdsol['mag'] = float(getElem(mag, 'magnitude', 'value'))
                for comm in mag.findall('solution:comment', ns):
                    ctype = comm.find('solution:id', ns).text
                    if ctype == 'rupture-strike':
                        fdsol['fstrike'] = float(comm.find('solution:text', ns).text.rstrip().lstrip())
                    elif ctype == 'rupture-length':
                        fdsol['flen'] = float(comm.find('solution:text', ns).text.rstrip().lstrip())
                    elif ctype == 'likelihood':
                        fdsol['uncr'] = float(comm.find('solution:text', ns).text.rstrip().lstrip())
            elif mtype == 'Mfdl':
                fdsol['mag_rup'] = float(getElem(mag, 'magnitude', 'value'))
            elif mtype == 'Mfdr':
                fdsol['mag_regr'] = float(getElem(mag, 'magnitude', 'value'))

    if len(fdsols) == 0:
        return False, [], {}, 0.

    # Add versions and compute fcoords from centroid, strike and length
    ind = 0
    for fdsol in sorted(fdsols, key=lambda x:x['vtime']):
        fdsol['version'] = ind
        ind += 1
        if 'clat' not in fdsol or 'clon' not in fdsol or 'fstrike' not in fdsol or 'flen' not in fdsol:
            continue
        end1 = getLatLon(fdsol['clat'], fdsol['clon'], fdsol['fstrike'], fdsol['flen']/2.)
        end2 = getLatLon(fdsol['clat'], fdsol['clon'], fdsol['fstrike']+180., fdsol['flen']/2.)
        fdsol['fcoords'] = [end1, [fdsol['clat'], fdsol['clon']], end2]

    # Time of last update
    lastt = max([f['vtime'] for f in fdsols])

    return True, fdsols, fdevent, lastt


def getDataBounds(data, pad=0.):
    '''
    Get min/max values from data to use as (map) bounds.
    Optionally add padding to bounds using non-zero pad.
    Args:
        data: List of data with, at minimum, x,y as first two elements of entry
    Returns:
        tuple of xmin, xmax, ymin, ymax
    '''
    xmin = min([x[0] for x in data]) - pad
    xmax = max([x[0] for x in data]) + pad
    ymin = min([x[1] for x in data]) - pad
    ymax = max([x[1] for x in data]) + pad
    return [xmin, xmax, ymin, ymax]


def limitByBounds(cities, bounds):
    """Search for cities within a bounding box (xmin,xmax,ymin,ymax).
    Args:
        cities: Cities dataframe
        bounds: Sequence containing xmin,xmax,ymin,ymax (decimal degrees).
    Returns:
        New Cities instance containing smaller cities data set.
    """
    # TODO: figure out what to do with a meridian crossing?
    newdf = cities.copy()
    xmin, xmax, ymin, ymax = bounds
    newdf = newdf.loc[(newdf['lat'] >= ymin) & (newdf['lat'] <= ymax) &
                      (newdf['lon'] >= xmin) & (newdf['lon'] <= xmax)]
    return newdf


def limitByGrid(cities, nx=1, ny=1, cities_per_grid=5):
    """
    Create a smaller Cities dataset by gridding cities, then limiting
    cities in each grid by population.
    Args:
        nx: Desired number of columns for grid.
        ny: Desired number of rows for grid.
        cities_per_cell: Maximum number of cities allowed per grid cell.
    Raises:
        KeyError: When Cities instance does not contain population data.
    Returns:
        New Cities instance containing cities limited by number in each
        grid cell.
    """
    from pandas import __version__
    if 'pop' not in cities.columns:
        raise KeyError('Cities instance does not contain population '
                       'information')
    xmin = cities['lon'].min()
    xmax = cities['lon'].max()
    ymin = cities['lat'].min()
    ymax = cities['lat'].max()
    dx = (xmax - xmin) / nx
    dy = (ymax - ymin) / ny
    newdf = None
    # start from the bottom left of our grid, and trim our cities.
    for i in range(0, ny):
        cellymin = ymin + (i * dy)
        cellymax = cellymin + dy
        for j in range(0, nx):
            cellxmin = xmin + (j * dx)
            cellxmax = cellxmin + dx
            tcities = limitByBounds(cities,
                (cellxmin, cellxmax, cellymin, cellymax))
            # older versions of pandas use a different sort function
            if __version__ < '0.17.0':
                tdf = tcities.sort(columns='pop', ascending=False)
            else:
                tdf = tcities.sort_values(by='pop', ascending=False)
            tdf = tdf[0:cities_per_grid]
            if newdf is None:
                newdf = tdf.copy()
            else:
                newdf = newdf.append(tdf)
    return newdf


def makeFinDerSolutionKML(fdsols, pgadata, odir='.', prefix=''):
    '''
    Create KML with FinDer solution
    Args:
        fdsols: List of FinDer solution dictionaries
        pgadata: pgadata but only used for locations, not PGA
        odir: Output directory
        prefix: prefix for the plot files
    Returns:
        No return, kml is created and saved to file
    '''
    import simplekml as skml
    kml = skml.Kml()
    bFirst = True
    for i, fdsol in enumerate(fdsols):
        if 'fcoords' not in fdsol:
            continue
        if i < len(fdsols)-1:
            etime = fdsols[i+1]['vtime']
        else:
            etime = fdsol['vtime'] + 1.
        for coords in fdsol['fcoords']:
            pnt = kml.newpoint(name='')
            pnt.coords = [(coords[1], coords[0])]
            pnt.timespan = skml.TimeSpan(begin=fdsol['vtime'].strftime('%Y-%m-%dT%H:%M:%S.%f'), \
                end=etime.strftime('%Y-%m-%dT%H:%M:%S.%f'))
            pnt.style.linestyle.color = skml.Color.black
            pnt.style.iconstyle.icon.href = 'http://maps.google.com/mapfiles/kml/shapes/star.png'
        ls = kml.newlinestring(name='FinDer rupture')
        ls.coords = [(coords[1],coords[0],10.0) for coords in fdsol['fcoords']]
        ls.timespan = skml.TimeSpan(begin=fdsol['vtime'].strftime('%Y-%m-%dT%H:%M:%S.%f'), \
                end=etime.strftime('%Y-%m-%dT%H:%M:%S.%f'))
        ls.altitudemode = skml.AltitudeMode.relativetoground
        ls.style.linestyle.width = 4
        ls.style.linestyle.color = skml.Color.black
        # Stations
        if bFirst:
            bFirst = False
            for point in pgadata:
                pnt = kml.newpoint(name='')
                pnt.coords = [(point[0],point[1])]
                pnt.style.iconstyle.icon.href = 'http://maps.google.com/mapfiles/dir_60.png'
    kmlname = os.path.join(odir, '%sfinder_ruptures.kml'%prefix)
    logging.info('Saving %s'%kmlname)
    try:
        kml.save(kmlname)
    except:
        logging.warning('Cannot save %s' % kmlname)
    return kmlname

def makeFinDerSolutionTimePlots(fdsols, odir='.', prefix=''):
    '''
    Create plots showing solution evoluion
    Args:
        fdsols: List of FinDer solution dictionaries
        odir: Output directory
        prefix: prefix for the plot files
    Returns:
        No return, figure is created and saved to file
    '''
    import matplotlib.pyplot as plt
    mint = min([x['vtime'] for x in fdsols])
    time = [x['vtime']-mint for x in fdsols]
    fig, ax = plt.subplots(1,3,figsize=(9,2))
    # Plot fault length
    ax[0].scatter(time, [x['flen'] for x in fdsols], marker='x', c='r')
    ax[0].set_ylabel('fault length (km)')
    # Plot fault strike
    ax[1].scatter(time, [x['fstrike'] for x in fdsols], marker='x', c='r')
    ax[1].set_ylabel('fault strike (deg)')
    # Plot inferred magnitude
    ax[2].scatter(time, [x['mag'] for x in fdsols], marker='x', c='r')
    ax[2].set_ylabel('mag')
    for a in ax:
        a.set_xlabel('time from 1st alert (s)')
        a.grid(which='both')
    plt.tight_layout()
    figname = os.path.join(odir, '%sfinder_timesols.png'%prefix)
    logging.info('Saving %s'%figname)
    try:
        plt.savefig(figname)
    except:
        logging.warning('Cannot save %s' % figname)
    return figname

def makeFinDerSolutionPlot(fdsol, pgadata, odir='.', prefix='', epihist=[]):
    '''
    Create plot with FinDer solution
    Args:
        fdsol: FinDer solution dictionary
        pgadata: PGA data in log10(PGA) cm/s/s
        odir: Output directory
    Returns:
        No return, figure is created and saved to file
    '''
    from numpy import arange, ones
    def addBorderData(pgadata):
        border_degrees = 2.
        minpga = -4.
        minlat = min([x[1] for x in pgadata]) - border_degrees
        maxlat = max([x[1] for x in pgadata]) + border_degrees
        minlon = min([x[0] for x in pgadata]) - border_degrees
        maxlon = max([x[0] for x in pgadata]) + border_degrees
        latstep = (maxlat-minlat)/10.
        lonstep = (maxlon-minlon)/10.
        for nlat in arange(minlat, maxlat+latstep/2., latstep):
            for lon in [minlon, maxlon]:
                pgadata.append([lon, nlat, minpga])
        for nlon in arange(minlon, maxlon+lonstep/2., lonstep):
            for lat in [minlat, maxlat]:
                pgadata.append([nlon, lat, minpga])
        return pgadata, [minlon, maxlon, minlat, maxlat]

    if 'fcoords' not in fdsol:
        return ''
    if len(pgadata) > 0:
        mbData = getDataBounds(pgadata, 0.5)
        mbSolSmall = getDataBounds([(fdsol['fcoords'][0][1], fdsol['fcoords'][0][0]), \
                (fdsol['fcoords'][-1][1], fdsol['fcoords'][-1][0])], 0.5)
        mbSolLarge = getDataBounds([(fdsol['fcoords'][0][1], fdsol['fcoords'][0][0]), \
                (fdsol['fcoords'][-1][1], fdsol['fcoords'][-1][0])], 1.5)
        mapBounds = []
        for i, (data, small, large) in enumerate(zip(mbData, mbSolSmall, mbSolLarge)):
            if i in [0,2]: # minima bounds
                if data < small and data > large:
                    mapBounds.append(data)
                else:
                    if data > small:
                        mapBounds.append(small)
                    else:
                        mapBounds.append(large)
            else: # maxima bounds
                if data > small and data < large:
                    mapBounds.append(data)
                else:
                    if data < small:
                        mapBounds.append(small)
                    else:
                        mapBounds.append(large)
    else:
        mapBounds = getDataBounds([(fdsol['fcoords'][0][1], fdsol['fcoords'][0][0]), \
                (fdsol['fcoords'][-1][1], fdsol['fcoords'][-1][0])], 1.)
    fig, ax, proj = setBasemap(bounds=mapBounds)
    # Plot PGA Data
    if conf['plot_stns']:
        if conf['plot_PGA_units'] == 'g':
            plotpga = [pow(10., x[2]-2.991521) for x in pgadata]
            pgamin = 0.002039432
        elif conf['plot_PGA_units'] == '%g':
            plotpga = [pow(10., 2+x[2]-2.991521) for x in pgadata]
            pgamin = 0.2039432
        elif conf['plot_PGA_units'] == 'cm/s/s':
            plotpga = [pow(10., x[2]) for x in pgadata]
            pgamin = 2.
        else:
            conf['plot_PGA_units'] = 'cm/s/s' # invalid unit passed, so set to default
            plotpga = [pow(10., x[2]) for x in pgadata]
            pgamin = 2.
        cb = ax.scatter([x[0] for x in pgadata], [x[1] for x in pgadata], c=plotpga, norm=LogNorm(vmin=pgamin), \
                marker='^', cmap='jet', zorder=20, lw=0.5, edgecolors='k', s=100., transform=proj)
        fig.colorbar(cb, ax=ax, label='PGA [%s]'%conf['plot_PGA_units'])
    # PyGMT
    if conf['plot_interp']:
        projection = 'M15c'
        borders = '1/0.5p'
        spacing = '5k'
        # Station mask
        if not os.path.isfile(conf['mask_file']):
            logging.warning('Mask file not available')
        mask = pygmt.grd2xyz(grid=conf['mask_file'])
        pgadata, bounds = addBorderData(pgadata)
        region = '/'.join(['%.2f'%x for x in bounds])
        # Blockmean to average the stations for spacing of regular grid
        data_bmean = pygmt.blockmean(x=[x[0] for x in pgadata], y=[x[1] for x in pgadata], \
                z=[x[2] for x in pgadata], region=region, spacing=spacing)
        # Triangulate to interpolate the stations to the same regular grid
        tri = pygmt.triangulate()
        # regular_grid is equivalent of using -G
        data_tri = tri.regular_grid(data=data_bmean, region=region, spacing=spacing) 
        # Grdfilter to apply a smoothing to the interpolated data
        data_filt = pygmt.grdfilter(grid=data_tri, filter='c1', distance='4', nans='i')
        mask_grid = pygmt.xyz2grd(data=mask, region=region, spacing=spacing)
        data_masked = data_filt * mask_grid
        X = data_masked.coords['lon'].data
        Y = data_masked.coords['lat'].data
        Z = data_masked.data
        cb = ax.pcolormesh(X, Y, Z, shading='gouraud', cmap='jet', alpha=0.1, transform=proj, vmin=0.3)
        if not conf['plot_stns']:
            fig.colorbar(cb, ax=ax, label='log10(PGA) [cm/s/s]')
    # Plot FinDer rupture
    fdlat = [x[0] for x in fdsol['fcoords']]
    fdlon = [x[1] for x in fdsol['fcoords']]
    ax.plot(fdlon, fdlat, lw=4, c='r', transform=proj)
    ax.scatter(fdlon, fdlat, marker=(5,1), c='red', s=100., edgecolors='k', zorder=20, transform=proj)
    ax.set_title('Version %d at %s (%s),\nM%.1f (%.2f,%.2f), %.1f km/%d deg, corr. %.2f\nOrigin time %s, Mfd %.1f, Mregr %.1f' % \
                (fdsol['version'], fdsol['vtime'].strftime('%Y/%m/%d %H:%M:%S'), fdsol['author'],\
                fdsol['mag'], \
                fdsol['elat'], fdsol['elon'], fdsol['flen'], fdsol['fstrike'], fdsol['uncr'], \
                fdsol['t0'].strftime('%Y/%m/%d %H:%M:%S'), fdsol['mag_rup'], fdsol['mag_regr']))
    # Plot all epicenters
    if len(epihist) > 0:
        ax.scatter([x[0] for x in epihist], [x[1] for x in epihist], marker=(5,1), c='grey', s=50, transform=proj)
    # Plot faults
    faultFile = conf['faults_file']
    if os.path.isfile(faultFile):
        # JA - N.B. this plotting throws an error in python2.7
        #from cartopy.io.shapereader import Reader
        #from cartopy.feature import ShapelyFeature
        #faults = ShapelyFeature(Reader(faultFile).geometries(), ccrs.PlateCarree(), edgecolor='black')
        #ax.add_feature(faults, c='grey', lw=0.5, transform=proj, zorder=15) # causing savefig problems
        pass
    # Plot cities
    cityFile = conf['cities_file']
    if os.path.isfile(cityFile): 
        cities = importCityFile(cityFile)
        sel_cities = limitByBounds(cities, mapBounds)
        sel_cities = limitByGrid(sel_cities, nx=1, ny=1, cities_per_grid=5)
        sel_cities = sel_cities.values.tolist()
        if len(sel_cities) > 0:
            clon = [x[3] for x in sel_cities]
            clat = [x[2] for x in sel_cities]
            ax.scatter(clon, clat, marker='o', c='k', zorder=15, transform=proj)
            for x,y,s in zip(clon, clat, [x[0] for x in sel_cities]):
                ax.text(x+0.1, y, s, zorder=14, va='center', ha='left', transform=proj)
    # Save
    figname = os.path.join(odir, '%sdata_%s.png'%(prefix,fdsol['version']))
    logging.info('Saving %s'%figname)
    try:
        fig.savefig(figname)
    except Exception as e:
        logging.warning('Unable to save %s (%s)' % (figname, str(e)))
    return figname


def runStandalone():
    '''
    Run the script from offline files which have been passed as command line arguments:
    -log is a FinDer log file
    -datadir is the location of the data_ FinDer files
    '''
    import argparse
    from fnmatch import filter

    parser = argparse.ArgumentParser(description='Create simple plots of FinDer solutions')
    parser.add_argument('-log', type=str, help='FinDer log file with path', required=True)
    parser.add_argument('-datadir', type=str, help='Path to folder containing FinDer data_ files', required=True)
    args = parser.parse_args()

    finderlog = args.log
    finderdata = args.datadir

    logging.basicConfig(filename='makeFinDerPlot_%s.log'% \
                UTCDateTime.now().strftime('%y%m%dT%H%M%S'), level=logging.INFO)

    # Check input parameters are viable
    if not os.path.isfile(finderlog):
        logging.error('Specified FinDer log file not available %s' % finderlog)
        sys.exit()
    else:
        logging.info('Using FinDer log file %s' % finderlog)

    if not os.path.isdir(finderdata):
        logging.error('Specified data directory not available %s' % finderdata)
        sys.exit()
    else:
        logging.info('Using FinDer data files at %s' % finderdata)

    timestep = None
    if timestep == None:
        datafilelist = filter(os.listdir(finderdata), 'data_*')

    # FinDer solutions from log file
    allfdevents = importFinDerLog(finderlog)
    for fdev in allfdevents:
        fdevent = {}
        fdevent['evid'] = fdev
        fdevent['locstr'] = ''
        fdsols = allfdevents[fdev]
        # Looping overdata file
        for datafile in sorted(datafilelist, key=lambda x:int(x.split('_')[-1])):
            tstep = datafile.replace('data_','')
            pgadata = importFinDerDataFile(os.path.join(finderdata, datafile), pgathresh=2.)
            res = [(i,f) for i,f in enumerate(fdsols) if str(f['tstep']) == tstep]
            if len(res) == 0:
                continue
            if len(res) > 1:
                logging.warning('No unique FinDer data for timestep %s' % tstep)
                sys.exit()
            fdsol = res[0][1]
            ofig = makeFinDerSolutionPlot(fdsol, pgadata, prefix=fdev+'_')
        tsfile = makeFinDerSolutionTimePlots(fdsols, prefix='%s_'%auth)
        okml = makeFinDerSolutionKML(fdsols, pgadata, prefix=fdev+'_')
        subject, body = prepareFinDerHTML(fdsols, fdevent)
        if conf['smtp_server'] == None:
            logging.warning('Writing to file instead of emailing')
            f = open('%s_finder_email.html'%fdev, 'w+')
            f.write(body)
            f.close()
    return


def runSeisComp(scevid = None):
    '''
    Run the script for SeisComP solutions.
    FinDer solution information is obtained using a dump from the seiscomp database
    N.B. This REQUIRES scxmldump to be setup for the seiscomp database, if that step fails, check
    for scxmldump.cfg in the .seiscomp directory
    # database = <service>://<uname>:<pwd>@<host>/<db> (e.g. service=postgresql)
    PGA data are obtained from FinDer's data_ files, note that the pga_dir_loc must be set
    correctly. If a data directory, or associated data directory for the event (based on timestamp)
    or the data_ file that matches version number, the plotting will proceed without PGA. Data
    directory searchs +/-5seconds around the timestamp of first solution creation.
    '''

    # Create as nested function because of looping
    def call_scxmldump(retry_count = 0):
        nonlocal len_xml
        nonlocal bPreferredOnly
        if retry_count == conf['retries']:
            logging.error('scxmldump failed to get valid XML after %d retries, exiting!'%conf['retries'])
            sys.exit()
        # Dump complete event parameters (may be slow). 
        command = ["scxmldump", "--config-file", conf['scxmldump_cfg'], "-m",  "-E", evid]
        if bPreferredOnly:
            command.append('-p')
        p = Popen(command, universal_newlines=True, stdout=PIPE)
        data = p.communicate()[0]
        if p.returncode != 0:
            retry_count += 1
            logging.warning('Failure to call scxmldump, retry %d' % retry_count)
            logging.warning('scxmldump failed command: %s' % ' '.join(command))
            xml = call_scxmldump(retry_count)
        else:
            xml = ''.join([line for line in str(data).split('\n') if line.startswith('<')])
        if not bPreferredOnly:
            if len(xml) == 0:
                retry_count += 1
                logging.warning('0 length XML, retry %d' % retry_count)
                xml = call_scxmldump(retry_count)
            if len(xml) < len_xml:
                retry_count += 1
                logging.warning('XML length decreased to %d from %d, retry %d' % (len(xml), len_xml, retry_count))
                xml = call_scxmldump(retry_count)
        len_xml = len(xml)
        return xml

    def wrap_scxml2fdsol(xml):
        nonlocal num_fdsols
        try:
            xmlvalid, fdsols, fdevent, lastt = scxml2fdsol(xml) 
        except Exception as e:
            logging.warning(e)
            xmlvalid = False
            fdsols = []
        retry_count = 0
        while not xmlvalid or len(fdsols) < num_fdsols:
            if retry_count == conf['retries']:
                break
            retry_count += 1
            logging.warning("XML wasn't valid when parsing, so retry %d" % retry_count)
            logging.info('Num_fdsols %d and len(fdsols) %d' % (num_fdsols, len(fdsols)))
            xml = call_scxmldump()
            try:
                xmlvalid, fdsols, fdevent, lastt = scxml2fdsol(xml) 
            except:
                xmlvalid = False
                fdsols = []
        if not xmlvalid:
            logging.error('No valid XML parsing, exiting!')
            sys.exit()
        num_fdsols = len(fdsols)
        return fdsols, fdevent, lastt

    if scevid == None:
        logging.error('Cannot proceed without event ID')
        sys.exit()
    else:
        evid = scevid

    logging.basicConfig(filename='makeFinDerPlot_%s.log'% \
                UTCDateTime.now().strftime('%y%m%dT%H%M%S'), level=logging.INFO)
    logging.info('Running script for seiscomp eventID %s'%evid)

    # FinDer solutions from seiscomp db
    len_xml = 0
    num_fdsols = 0
    bPreferredOnly = True
    xml = call_scxmldump()
    fdsols, fdevent, lastt = wrap_scxml2fdsol(xml) 
    logging.info('Solutions %s %d' % (fdevent['evid'], len(fdsols)))
    # Allow event timeout after last update before plotting
    logging.info('Last update %s, (%.2f)' % (lastt.strftime('%Y%m%dT%H:%M:%S'), UTCDateTime.now()-lastt))
    while UTCDateTime.now() - lastt < conf['event_timeout']:
        logging.info('Sleeping to allow event timeout')
        sleep(conf['sleep_time'])
        xml = call_scxmldump()
        fdsols, fdevent, lastt = wrap_scxml2fdsol(xml) 
        logging.info('Solutions %s %d' % (fdevent['evid'], len(fdsols)))
        logging.info('Last update %s, (%.2f, %s)' % (lastt.strftime('%Y%m%dT%H:%M:%S'), \
                UTCDateTime.now()-lastt, UTCDateTime.now().strftime('%Y%m%dT%H:%M:%S')))

    logging.info('Continuing for eventID %s'%evid)
    bPreferredOnly = False
    xml = call_scxmldump()
    nfdsols, fdevent, lastt = wrap_scxml2fdsol(xml) 
    
    for auth in set([x['author'] for x in nfdsols if 'author' in x]):
        # filter fdsols based on author
        fdsols = [x for x in nfdsols if 'author' in x and x['author'] == auth]
        lastt = max([f['vtime'] for f in fdsols])

        # Get PGA data from data_ files
        pga_dir = None
        posix = int(min([f['vtime'] for f in fdsols])._get_ns()//1e9)
        for t in range(posix-5, posix+6):
            if os.path.isdir(os.path.join(conf['pga_dir_loc'], str(t))):
                pga_dir = os.path.join(conf['pga_dir_loc'], str(t))
                logging.info('Selected PGA data directory %d'%t)
                break
        if pga_dir == None:
            logging.warning('Cannot find a PGA data directory for event [%d-%d]'%(posix-5, posix+5))

        # Call plotting routine
        pgadata = []
        odir = conf['plot_dir']
        if pga_dir != None:
            filetimes = getFinDerDataFileTimes(pga_dir)
        for fdsol in fdsols:
            if conf['lastplotonly'] and fdsol['vtime'] != lastt:
                continue
            pgadata = []
            if 'fcoords' not in fdsol:
                continue
            if pga_dir != None:
                allt = [(fdsol['vtime']-ft[1],ft[0]) for ft in filetimes if fdsol['vtime']-ft[1]>0.]
                if len(allt) == 0:
                    allt = [(fdsol['vtime']-ft[1],ft[0]) for ft in filetimes]
                mint = min([x[0] for x in allt])
                fdsol['tstep'] = [x[1] for x in allt if x[0] == mint][0]
                dfile = os.path.join(pga_dir, fdsol['tstep'])
                if os.path.isfile(dfile):
                    logging.info('Making plot using FinDer solution version %d and %s file' \
                            %(fdsol['version'], dfile))
                    pgadata = importFinDerDataFile(dfile, pgathresh=2.)
            epihist = [(f['elon'], f['elat']) for f in fdsols]
            pngfile = makeFinDerSolutionPlot(fdsol, pgadata, odir=odir, prefix='%s_'%auth, epihist=epihist)
            # Keep the filename of the latest png
            if fdsol['vtime'] == lastt:
                pngname = pngfile
                lfdsol = fdsol
        tsfile = makeFinDerSolutionTimePlots(fdsols, odir=odir, prefix='%s_'%auth)
        kmlname = makeFinDerSolutionKML(fdsols, pgadata, odir=odir, prefix='%s_'%auth)
        # Emailing out the solution
        # Perform solution checks for whether to proceed
        if lfdsol['mag'] >= conf['email_maglimit'] and \
                (UTCDateTime().now()-lfdsol['vtime']) <= conf['email_timelimit']:
            if conf['email_sanitychk'] and len(fdsols) < 5 and \
                fdsols[0]['vtime']-fdsols[0]['t0'] > 30.:
                    logging.warning('Event failed sanity criteria for emailing: only one report with creation time longer than 30s after origin')
            else:
                subject, body = prepareFinDerHTML(fdsols, fdevent)
                if False:
                    fout = open('debug.html', 'w')
                    fout.write(body)
                    fout.close()
                sendHTML(subject, body, [pngname, tsfile], kmlname)
    return


if __name__ == '__main__':
    '''
    Script for plotting, and possibly emailing, FinDer solution
    Designed to be called from command line or from SeisComp scalert
    '''
    if sys.argv[1] == '-h':
        print('\n\n\nScript for plotting FinDer solutions, it has two modes:\n \
* run from the command line with arguments: -log <FinDer library log file> -ddir <FinDer directory containing data_ files>\n \
* run from command line with a SeisComP event ID.\n\n \
Note that for the latter it requires scxmldump to be configured.\n\n\n')
        sys.exit()
    if len(sys.argv) == 2 and '-log' not in sys.argv:
        runSeisComp(sys.argv[1])
    else:
        runStandalone()

