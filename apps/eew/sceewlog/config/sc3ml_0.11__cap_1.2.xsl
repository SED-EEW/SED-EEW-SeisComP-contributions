<?xml version="1.0" encoding="UTF-8"?>
<!-- **********************************************************************
 * SC3ML 0.7 to CAP 1.2 stylesheet converter
 * Author  : Felix David Suarez Bonilla
 * Email   : felixdavidsuarezbonilla@gmail.com
 * Version : 0
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

    <xsl:variable name="mag" select="scs:seiscomp/scs:EventParameters/scs:amplitude/scs:amplitude/scs:value"/>
    <xsl:variable name="ag" select="scs:seiscomp/scs:EventParameters/scs:pick/scs:creationInfo/scs:agencyID"/>
    <xsl:variable name="id" select="scs:seiscomp/scs:EventParameters/scs:pick/@publicID"/>
    <xsl:variable name="new_id" select="scs:seiscomp/scs:EventParameters/scs:amplitude/@publicID"/>
    <xsl:variable name="time" select="scs:seiscomp/scs:EventParameters/scs:pick/scs:time/scs:value"/>
    <xsl:variable name="loc" select="scs:seiscomp/scs:EventParameters/scs:event/scs:description/scs:text"/>
    <xsl:variable name="date" select="substring($time/text(),1,10)"/>
    <xsl:variable name="hour" select="substring($time/text(),12,8)"/>

    <!-- Starting point: Match the root node and select the one and only
         EventParameters node -->
    <xsl:template match="/">
    <alert xmlns="urn:oasis:names:tc:emergency:cap:1.2">
        <identifier><xsl:value-of select="$id"/></identifier>
        <sender>OVSICORI</sender>
        <sent><xsl:value-of select="$time"/></sent>
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
            <headline>M<xsl:value-of select="round($mag*10) div 10"/> Earthquake Warning issued by <xsl:value-of select="$ag"/> on <xsl:value-of select="$date"/> at <xsl:value-of select="$hour"/></headline>
            <instruction>Stay away from windows and objects that could fall. Go to a safe place and take cover.</instruction> 
            <area>
                <areaDesc><xsl:value-of select="$loc"/></areaDesc>
            </area>
        </info>
    </alert>
    </xsl:template>
</xsl:stylesheet>
