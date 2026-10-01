<?xml version="1.0" encoding="UTF-8" ?>

<!--
    Document   : vym2txt.xsl
    Created on : 20040317
	Modified   : 20040417
	Version    : 0.1
    Author     : Clemens Kraus (http://www.clemens-kraus.de)
    Description: transforms vym-files into text-format.
    
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output method="text" indent="no" encoding="UTF-8"/> 
  <xsl:param name="filenamep" />
  <xsl:variable name="filename" select="$filenamep"/>
  
  <xsl:variable name="fn" >
	<xsl:call-template name="getfn" >
		<xsl:with-param name="txt" select="$filename" />
	</xsl:call-template>
  </xsl:variable>

  <xsl:variable name="path" >
	  <xsl:value-of select="normalize-space( substring($filename, 1, number(string-length($filename)-string-length($fn))) )" />
  </xsl:variable>


  <xsl:template match="/">
  
    <xsl:text>#VYM-Export VYM-Version:</xsl:text><xsl:value-of select="vymmap/@version"/>
    <xsl:text>&#xA;&#xA;</xsl:text>
	  
	
    <xsl:value-of select="vymmap/mapcenter/heading"/>
	<xsl:text>&#xA;&#xA;</xsl:text>
		
	
	<xsl:apply-templates select="vymmap/mapcenter/branch" >
		<xsl:with-param name="indent" select="''" />
	</xsl:apply-templates>

  </xsl:template>

	
  
  
  <xsl:template match="branch">
    <xsl:param name="width" select="70" />
	
	
	<xsl:call-template name="gen-anchor-tag" >
		<xsl:with-param name="indent" select="$indent" />
	</xsl:call-template>
	  
	
	<xsl:apply-templates select="branch" >
		<xsl:with-param name="indent" select="concat($indent, ' ')" />
	</xsl:apply-templates>

  </xsl:template>


  
  
  <xsl:template name="gen-anchor-tag">
    <xsl:param name="indent"/>

	<xsl:value-of select="$indent"/><xsl:text>- </xsl:text><xsl:value-of select="heading"/>

	
    <xsl:if test="@url != ''"> (<xsl:value-of select="@url"/>)</xsl:if>
	
	
	<xsl:apply-templates select="note/@href" >
		<xsl:with-param name="indent" select="$indent" />
	</xsl:apply-templates>

    <xsl:text>&#xA;</xsl:text>
  </xsl:template>


  
  
  
  <xsl:template match="note/@href">
    <xsl:param name="indent"/>
    
    <xsl:variable name="actualnotename">
      <xsl:value-of select="substring-after(current(),':')"/>
    </xsl:variable>

    <xsl:variable name="note-name">
	    <xsl:if test="$path = ''">
			<xsl:value-of select="$actualnotename"/>
		</xsl:if>
	    <xsl:if test="not($path = '')">
			<xsl:value-of select="concat($path, '/', $actualnotename)"/>
		</xsl:if>
	</xsl:variable>
	
	
	<xsl:variable name="currentID" select="@id" />
	
    <xsl:text>&#xA;</xsl:text>
  
	<xsl:for-each select="document($note-name)/note/line">
		<xsl:value-of select="$indent"/><xsl:text>  </xsl:text><xsl:value-of select="."/>
	</xsl:for-each>
		
  </xsl:template>

  
  
  
  
  <xsl:template name="getfn">
	  <xsl:param name="txt" select="." />
	  
	  <xsl:choose>
		  <xsl:when test="contains($txt, '/')" >
		  	<xsl:variable name="right" select="substring-after($txt, '/')" />
			<xsl:if test="string-length($right)>1" >
			  <xsl:call-template name="getfn" >
				  <xsl:with-param name="txt" select="$right" />
			  </xsl:call-template>
			</xsl:if>
		  </xsl:when>
		  <xsl:otherwise>
			  <xsl:value-of select="$txt" />
		  </xsl:otherwise>
	  </xsl:choose>
	  
  </xsl:template>
  
</xsl:stylesheet>

