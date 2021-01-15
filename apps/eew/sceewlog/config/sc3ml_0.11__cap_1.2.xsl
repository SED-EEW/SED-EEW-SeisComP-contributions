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

    <!-- Starting point: Match the root node and select the one and only
         EventParameters node -->
    <xsl:template match="/">
    <alert xmlns="urn:oasis:names:tc:emergency:cap:1.2">
        <identifier><xsl:value-of select="scs:seiscomp/scs:EventParameters/scs:pick/@publicID"/></identifier>
        <sender>OVSICORI</sender>
        <sent><xsl:value-of select="scs:seiscomp/scs:EventParameters/scs:pick/scs:time/scs:value"/></sent>
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
            <headline>Earthquake Warning issued by OVSICORI</headline>
            <instruction>Stay away from windows and objects that could fall. Go to a safe place and take cover.</instruction> 
        </info>
    </alert>
    </xsl:template>
</xsl:stylesheet>
