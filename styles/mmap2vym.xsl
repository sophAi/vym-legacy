<?xml version="1.0" encoding="UTF-8"?>

<!--
	Document   : mmap2vym.xsl
	Author     : Clemens Kraus (http://www.clemens-kraus.de)
			     Uwe Drechsel <vym@InSilmaril.de>
	
	Description: transforms exported MindManager-files (version x5/6) into vym-format.
	Bugs       : First Version, work still in progress!
	Todo       : - xlinks still to be implemented
	             - codes/symbols still to be implemented
				 - Multimap links (.mmap) still to be implemented
-->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:xlink="http://www.w3.org/1999/xlink"
	xmlns:ap="http://schemas.mindjet.com/MindManager/Application/2003"
	xmlns:exsl="http://exslt.org/common" extension-element-prefixes="exsl math func"
	xmlns:math="http://exslt.org/math"
	xmlns:func="http://exslt.org/functions"
	version="1.0">

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"/>
<xsl:param name="filenamep" />
<xsl:variable name="filename" select="$filenamep"/>
<!--<xsl:variable name="maxxCoord"><xsl:value-of select="math:max(map//xCoord)" /></xsl:variable>-->

<!-- root element -->	
<xsl:template match="/">
   	<xsl:element name="vymmap">
		<!-- default settings: -->
		<xsl:attribute name="comment">Converted from MindManager to vym
		using mmap2vym.xsl</xsl:attribute>

		<xsl:variable name="author" select="concat(/ap:Map/ap:DocumentGroup/ap:Author/@UserName, ' ', /ap:Map/ap:DocumentGroup/ap:Author/@UserEmail)" />
		<xsl:if test="$author">
			<xsl:attribute name="author"><xsl:value-of select="$author"/></xsl:attribute>
		</xsl:if>
			
		<xsl:element name="mapcenter">
			<!-- recurse to map title node ... -->
			<xsl:apply-templates select="ap:Map/ap:OneTopic/ap:Topic/ap:SubTopics" />
		</xsl:element>
	</xsl:element>
</xsl:template>

<xsl:template match="ap:Topic/ap:Text">
	<xsl:element name="heading">
		<xsl:value-of select="@PlainText" />
	</xsl:element>
</xsl:template>


<xsl:template match="ap:Topic">
	<xsl:element name="branch">
	<xsl:call-template name="position" />
	<!-- =============================== hyperlink ================================ 
	<xsl:variable name="url" select="ap:Hyperlink" />
	<xsl:if test="$url">
	<xsl:attribute name="url"><xsl:value-of select="ap:Hyperlink/@Url"/></xsl:attribute>
	</xsl:if>
	<xsl:variable name="rect" select="ap:SubTopicShape/@SubTopicShape" />
	<xsl:if test="contains($rect, 'Rectangle')">
	<xsl:attribute name="frameType">Rectangle</xsl:attribute>
	</xsl:if>
	-->
	<xsl:apply-templates/>
	</xsl:element>
</xsl:template>


<!-- =============================== position ================================ -->
<xsl:template name="position">
	<xsl:if test="ap:Offset/@CX!=''">
		<xsl:attribute name="relPosX">
			<xsl:value-of select="ap:Offset/@CX *3"/>
		</xsl:attribute>
	</xsl:if>		
	<xsl:if test="ap:Offset/@CY!=''">
		<xsl:attribute name="relPosY">
			<xsl:value-of select="floor( ap:Offset/@CY *3 )"/>
		</xsl:attribute>
	</xsl:if>		
</xsl:template>

	
<!-- =============================== Icons ================================ -->
<xsl:template match="ap:Icon">
	<xsl:element name="standardFlag">
		<xsl:choose>
			<xsl:when test="@IconType ='urn:mindjet:ExclamationMark'">exclamationmark</xsl:when>
			<xsl:when test="@IconType ='urn:mindjet:SmileyHappy'">smiley-good</xsl:when>
			<xsl:when test="@IconType ='urn:mindjet:SmileySad'">smiley-sad</xsl:when>
		<xsl:otherwise>
			<xsl:message>Unknown Flag found: <xsl:value-of select="@IconType"/></xsl:message>
		</xsl:otherwise>
		</xsl:choose>
	</xsl:element>
</xsl:template>
	
  
</xsl:stylesheet>


