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
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:output method="xml" encoding="UTF-8" indent="yes" omit-xml-declaration="yes"/>
    <xsl:template match="/">
    <alert xmlns="urn:oasis:names:tc:emergency:cap:1.2">
        <identifier>2318174</identifier>
        <sender>SENDER</sender>
        <sent>2006-09-10T04:26:33.610000Z</sent>
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
            <effective>EFFECTIVE</effective>
            <expires>EXPIRES</expires>
            <headline>Earthquake Warning issued by OVSICORI</headline>
            <instruction>Stay away from windows and objects that could fall. Go to a safe place and take cover.</instruction> 
            <area>
                <areaDesc>AREADESC</areaDesc>
                <polygon> +9.614, +121.961</polygon> 
            </area>
        </info>
    </alert>
    </xsl:template>

</xsl:stylesheet>
