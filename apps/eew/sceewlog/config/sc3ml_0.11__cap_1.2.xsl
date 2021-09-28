<?xml version="1.0" encoding="UTF-8"?>
<!-- **********************************************************************
 * SC3ML 0.7 to CAP 1.2 stylesheet converter
 * Author  : Felix David Suarez Bonilla
 * Email   : felixdavidsuarezbonilla@gmail.com
 * Version : 1
 * CAP and SC3ML are not similar schemas. They have different purposes. CAP
 * messages are meant for alerting about any emergency events and S3CM has
 * the purpose of representing seismological data.

 ********************************************************************** -->
<xsl:stylesheet version="1.0"
        xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
        xmlns:scs="http://geofon.gfz-potsdam.de/ns/seiscomp3-schema/0.11"
        exclude-result-prefixes="scs xsl">
    <xsl:output method="xml" encoding="UTF-8" indent="yes" omit-xml-declaration="yes"/>
    <xsl:strip-space elements="*"/>

    <!-- Define some global variables -->

    <xsl:variable name="mag" select="scs:seiscomp/scs:EventParameters/scs:origin/scs:magnitude/scs:magnitude/scs:value"/>
    <xsl:variable name="ag" select="scs:seiscomp/scs:EventParameters/scs:pick/scs:creationInfo/scs:agencyID"/>
    <xsl:variable name="mag_id" select="scs:seiscomp/scs:EventParameters/scs:origin/scs:magnitude/@publicID"/>
    <xsl:variable name="org_id" select="scs:seiscomp/scs:EventParameters/scs:origin/scs:time/scs:value"/>
    <xsl:variable name="event_id" select="scs:seiscomp/scs:EventParameters/scs:event/@publicID"/>
    <xsl:variable name="time2" select="scs:seiscomp/scs:EventParameters/scs:origin/scs:magnitude/scs:creationInfo/scs:creationTime"/>
    <xsl:variable name="loc" select="scs:seiscomp/scs:EventParameters/scs:event/scs:description/scs:text"/>
    <xsl:variable name="full_time" select="substring($org_id,1,25)"/>
    <xsl:variable name="hour" select="substring($org_id,12,2)"/>
    <xsl:variable name="minute" select="substring($org_id,21,2)"/>
    <xsl:variable name="second" select="substring($org_id,18,2)"/>
    <xsl:variable name="year" select="substring($org_id,1,4)"/>
    <xsl:variable name="month" select="substring($org_id,6,2)"/>
    <xsl:variable name="day" select="substring($org_id,9,2)"/>
    <xsl:variable name="new_time" select="substring($time2/text(),1,19)"/>

    <!-- Starting point: Match the root node and select the one and only
         EventParameters node -->
    <xsl:template match="/">
    <alert xmlns="urn:oasis:names:tc:emergency:cap:1.2">
        <identifier><xsl:value-of select="$event_id"/></identifier>
        <sender><xsl:value-of select="$ag"/></sender>
        <sent><xsl:value-of select="$new_time"/>-00:00</sent>
        <status>Actual</status>
        <msgType>Alert</msgType>
        <scope>Private</scope>
        <info>
            <language>en-US</language>
            <category>Met</category>
            <event>Earthquake Warning</event>
            <urgency>Immediate</urgency>
            <severity>Extreme</severity>
            <certainty>Observed</certainty>
	    <headline>M<xsl:value-of select="round($mag*10) div 10"/> Prueba de Alerta de Terremoto emitida por <xsl:value-of select="$ag"/> a las <xsl:value-of select="$hour"/>:<xsl:value-of select="$minute"/>:<xsl:value-of select="$second"/> el dia <xsl:value-of select="$day"/>-<xsl:value-of select="$month"/>-<xsl:value-of select="$year"/></headline>
            <instruction>Mantengase alejado de ventanas y objetos que puedan caer. Vaya a un lugar seguro y cubrase.</instruction> 
            <parameter>
	    <valueName>magnitudcreationTime</valueName>
	    <value><xsl:value-of select="$time2"/></value>
	    <valueName>originTime</valueName>
	    <value><xsl:value-of select="$org_id"/></value>
	    </parameter>
            <area>
                <areaDesc><xsl:value-of select="$loc"/></areaDesc>
	    </area>
        </info>
    </alert>
    </xsl:template>
</xsl:stylesheet>

