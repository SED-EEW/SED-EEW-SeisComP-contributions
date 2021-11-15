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

    <!-- global variables -->
    <!-- The main variables are collected based on preferred Origin and its magnitude -->
    <xsl:variable name="preferredOrigin" select="scs:seiscomp/scs:EventParameters/scs:event/scs:preferredOriginID"/>
    <xsl:variable name="preferredMagnitude" select="scs:seiscomp/scs:EventParameters/scs:event/scs:preferredMagnitudeID"/>
    <xsl:variable name="latitude" select="scs:seiscomp/scs:EventParameters/scs:origin[@publicID=$preferredOrigin]/scs:latitude/scs:value"/> 
    <xsl:variable name="longitude" select="scs:seiscomp/scs:EventParameters/scs:origin[@publicID=$preferredOrigin]/scs:longitude/scs:value"/> 
    <xsl:variable name="magnitude" select="scs:seiscomp/scs:EventParameters/scs:origin[@publicID=$preferredOrigin]/scs:magnitude[@publicID=$preferredMagnitude]/scs:magnitude/scs:value"/>
    <xsl:variable name="depth" select="scs:seiscomp/scs:EventParameters/scs:origin[@publicID=$preferredOrigin]/scs:depth/scs:value"/>
    <xsl:variable name="agency" select="scs:seiscomp/scs:EventParameters/scs:event/scs:creationInfo/scs:agencyID"/>
    <xsl:variable name="orId" select="scs:seiscomp/scs:EventParameters/scs:origin/@publicID"/>
    <xsl:variable name="magId" select="scs:seiscomp/scs:EventParameters/scs:origin/scs:magnitude/@publicID"/>
    <xsl:variable name="eventId" select="scs:seiscomp/scs:EventParameters/scs:event/@publicID"/>
    <xsl:variable name="creationTime" select="scs:seiscomp/scs:EventParameters/scs:origin[@publicID=$preferredOrigin]/scs:magnitude[@publicID=$preferredMagnitude]/scs:creationInfo/scs:creationTime"/>
    <xsl:variable name="region" select="scs:seiscomp/scs:EventParameters/scs:event/scs:description/scs:text"/>
    <xsl:variable name="originTime" select="scs:seiscomp/scs:EventParameters/scs:origin[@publicID=$preferredOrigin]/scs:time/scs:value"/>
    <xsl:variable name="evaluationMode" select="scs:seiscomp/scs:EventParameters/scs:origin[@publicID=$preferredOrigin]/scs:time/scs:evaluationMode"/>
    
    <!-- Not used variables -->
    <xsl:variable name="hour" select="substring($originTime,12,2)"/>
    <xsl:variable name="minute" select="substring($originTime,15,2)"/>
    <xsl:variable name="second" select="substring($originTime,18,2)"/>
    <xsl:variable name="year" select="substring($originTime,1,4)"/>
    <xsl:variable name="month" select="substring($originTime,6,2)"/>
    <xsl:variable name="day" select="substring($originTime,9,2)"/>
  
    <!-- Starting point: Match the root node and select the one and only
         EventParameters node -->
    <xsl:template match="/">
    <alert xmlns="urn:oasis:names:tc:emergency:cap:1.2">
        <identifier><xsl:value-of select="$eventId"/></identifier>
        <sender><xsl:value-of select="$agency"/></sender>
        <sent><xsl:value-of select="$creationTime"/></sent>
        <status>Actual</status>
        <msgType>Alert</msgType>
        <scope>Private</scope>
        <info>
            <language>en-US</language>
            <category>Geo</category>
            <event>Earthquake</event>
            <urgency>Immediate</urgency>
            <!-- The severity should be function of magnitude ? -->
            <severity>Extreme</severity>
            <!-- Possible to use likelihood value for certainty ? -->
            <certainty>Observed</certainty>
            <headline>
                <xsl:value-of select="$agency"/> - Magnitude: <xsl:value-of select="round($magnitude*10) div 10"/>, date and Time (UTC): <xsl:value-of select="$originTime"/>
            </headline>
            <!-- <headline><xsl:value-of select="$agency"/> Info - Sismo Mag: <xsl:value-of select="round($mag*10) div 10"/>, <xsl:value-of select="$region"></headline> -->
            <instruction>Drop, Cover and Hold on</instruction> 
            <parameter>
	            <valueName>magnitudCreationTime</valueName>
	            <value><xsl:value-of select="$creationTime"/></value>
	        </parameter>
            <parameter>
	            <valueName>originTime</valueName>
	            <value><xsl:value-of select="$originTime"/> </value>
	        </parameter>
            <parameter>
				<valueName>magnitude</valueName>
				<value><xsl:value-of select="$magnitude"/></value>
			</parameter>
			<parameter>
				<valueName>latitude</valueName>
				<value><xsl:value-of select="$latitude"/></value>
			</parameter>
			<parameter>
				<valueName>longitude</valueName>
				<value><xsl:value-of select="$longitude"/></value>
			</parameter>
			<parameter>
				<valueName>depth</valueName>
				<value><xsl:value-of select="$depth"/></value>
			</parameter>
			<parameter>
				<valueName>status</valueName>
				<value><xsl:value-of select="$evaluationMode"/> solution</value>
			</parameter>
            <area>
                <areaDesc><xsl:value-of select="$region"/></areaDesc>
	        </area>
        </info>
        <info>
			<language>es-US</language>
            <category>Geo</category>
            <event>Sismo</event>
            <urgency>Immediata</urgency>
            <!-- The severity should be function of magnitude ? -->
            <severity>Extrema</severity>
            <!-- Possible to use likelihood value for certainty ? -->
            <certainty>Observada</certainty>
            <headline>
                <xsl:value-of select="$agency"/> - Magnitud: <xsl:value-of select="round($magnitude*10) div 10"/>, Fecha y Hora (UTC): <xsl:value-of select="$originTime"/>
            </headline>
            <!-- <headline><xsl:value-of select="$agency"/> Info - Sismo Mag: <xsl:value-of select="round($mag*10) div 10"/>, <xsl:value-of select="$region"></headline> -->
            <instruction>Mantengase alejado de ventanas y objetos que puedan caer. Vaya a un lugar seguro y cubrase.</instruction> 
            <parameter>
	            <valueName>magnitudCreationTime</valueName>
	            <value><xsl:value-of select="$creationTime"/></value>
	        </parameter>
            <parameter>
	            <valueName>originTime</valueName>
	            <value><xsl:value-of select="$originTime"/> </value>
	        </parameter>
            <parameter>
				<valueName>magnitude</valueName>
				<value><xsl:value-of select="$magnitude"/></value>
			</parameter>
			<parameter>
				<valueName>latitude</valueName>
				<value><xsl:value-of select="$latitude"/></value>
			</parameter>
			<parameter>
				<valueName>longitude</valueName>
				<value><xsl:value-of select="$longitude"/></value>
			</parameter>
			<parameter>
				<valueName>depth</valueName>
				<value><xsl:value-of select="$depth"/></value>
			</parameter>
			<parameter>
				<valueName>Estado</valueName>
				<value><xsl:value-of select="$evaluationMode"/> solution</value>
			</parameter>
            <area>
                <areaDesc><xsl:value-of select="$region"/></areaDesc>
	        </area>
		</info>
    </alert>
    </xsl:template>
</xsl:stylesheet>

