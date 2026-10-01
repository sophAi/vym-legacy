<?xml version="1.0" encoding="iso-8859-1" standalone="yes"?>
<xsl:stylesheet 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
	xmlns:exsl="http://exslt.org/common"
	extension-element-prefixes="exsl" version="1.0">
	
<xsl:variable name="controlCharacter" select="'¤'" />
<xsl:variable name="crlf" select="'&#10;'" />
<!-- <xsl:variable name="wikistyle" select="false()" /> -->

<xsl:variable name="supportMenu" select="'true'" />
<xsl:variable name="supportShowme" select="'true'" />
<xsl:variable name="supportSource" select="'true'" />
<xsl:variable name="supportImage" select="'true'" />

<xsl:template name="findNext">
	<xsl:param name="content" />
	<xsl:variable name="escaped" select="substring-after( $content, '\{' )" />
	<xsl:variable name="item-start" select="substring-after( $content, '{' )" />
	<xsl:variable name="item-end" select="substring-before( $item-start, '}' )" />
	<xsl:if test="normalize-space($escaped) != normalize-space($item-start)">
		<xsl:copy-of select="normalize-space($item-end)" />
	</xsl:if>
</xsl:template>

<xsl:template name="findNextTag">
	<xsl:param name="content" />
	<xsl:variable name="item-start" select="substring-after( $content, '|' )" />
	<xsl:variable name="item-end" select="substring-before( $item-start, '|' )" />
	<xsl:variable name="name-start" select="substring-after( $item-start, '(' )" />
	<xsl:variable name="name-end" select="substring-before( $name-start, ')' )" />
	<!-- <xsl:copy-of select="normalize-space($item-end)" /> -->
	<xsl:variable name="w" select="substring-after( $item-end, ') ')" />
	<xsl:if test="string($w)">
		<xsl:element name="{$w}">
			<xsl:value-of select="$name-end" />
		</xsl:element>
	</xsl:if>
</xsl:template>

<xsl:template name="lastCall">
	<xsl:param name="content" />

	<xsl:variable name="inTheBeginning"   select="substring-before( $content, '|' )" />
	<xsl:variable name="markup-start"     select="substring-after( $content, '|' )" />
	<xsl:variable name="markup-end"       select="substring-before( $markup-start, '|' )" />
	<xsl:variable name="markup-after"     select="substring-after( $markup-start, concat( $markup-end, '|') )" />
	
	<xsl:variable name="cmd"              select="substring-before( $markup-end, ';' )" />
	
	<xsl:copy-of select="$inTheBeginning" />
	
	<xsl:choose>
		<xsl:when test="string($markup-end)">
			<xsl:choose>
				<xsl:when test="starts-with( $markup-end, '!' )">
					<span class="note"><xsl:copy-of select="substring-after( $markup-end, '!')" /></span>
				</xsl:when>
				<xsl:when test="starts-with( $markup-end, '/' )">
					<em><xsl:copy-of select="substring-after( $markup-end, '/')" /></em>
				</xsl:when>
				<xsl:when test="starts-with( $markup-end, '*' )">
					<pre><xsl:copy-of select="substring-after( $markup-end, '*')" /></pre>
				</xsl:when>
				<xsl:otherwise>
					<strong><xsl:copy-of select="$markup-end" /></strong>
				</xsl:otherwise>
			</xsl:choose>
			<xsl:call-template name="lastCall">
				<xsl:with-param name="content" select="$markup-after" />
			</xsl:call-template>
			<!-- [<xsl:copy-of select="$markup-after" />] -->
		</xsl:when>
		<xsl:otherwise>
			<xsl:copy-of select="$content" />
		</xsl:otherwise>
	</xsl:choose>
	
</xsl:template>

<xsl:template name="replace-tags">
	<xsl:param name="content" />
	<xsl:param name="currentID" />
	<xsl:variable name="topic"><xsl:call-template name="findNext"><xsl:with-param name="content" select="$content" /></xsl:call-template></xsl:variable>
	<!-- <xsl:variable name="this" select="$thisDoc/item[@id=$topic]" /> -->
	<!-- <xsl:variable name="name" select="$this/@name" /> -->
	<xsl:variable name="name"><xsl:call-template name="getTopicName"><xsl:with-param name="id" select="$topic" /></xsl:call-template></xsl:variable>
	
	<xsl:variable name="escaped-start"    select="substring-after( $content, '\{' )" />
	<xsl:variable name="item-start"       select="substring-before( $content, '{' )" />

	<xsl:variable name="item-end"         select="substring-after( $content, '}' )" />
	<xsl:variable name="escaped-end"      select="substring-after( $content, '\}' )" />

	<xsl:if test="string(normalize-space($item-start))">
		<xsl:if test="normalize-space($escaped-start) != normalize-space($item-start)">
			<xsl:call-template name="lastCall">
				<xsl:with-param name="content" select="$item-start" />
			</xsl:call-template>
		</xsl:if>
		<xsl:if test="normalize-space($escaped-start) = normalize-space($item-start)">
			<xsl:call-template name="lastCall">
				<xsl:with-param name="content" select="$escaped-start" />
			</xsl:call-template>
		</xsl:if>
	</xsl:if>

	<xsl:variable name="middle">
		<xsl:if test="string(normalize-space($topic))">
		
			<!-- <xsl:value-of select="$topic" />(<xsl:value-of select="substring($topic, 1)" />) -->
			<xsl:choose>
				<xsl:when test="substring($topic, 1, 1) = '('">
				
					<xsl:variable name="tmp"><xsl:value-of select="substring-before( $topic, ')' )" /></xsl:variable>
					<xsl:variable name="thisName" select="substring-after( $topic, ') ' )" />
					<xsl:variable name="name2">
						<xsl:choose>
							<xsl:when test="substring($topic, 2, 1) = '%'"><img src="{substring( $tmp, 3 )}" class="image" border="0" alt="" /></xsl:when>
							<xsl:otherwise><xsl:value-of select="substring( $tmp, 2 )" /></xsl:otherwise>
						</xsl:choose>	
					</xsl:variable>
					<xsl:choose>
						<xsl:when test="starts-with($thisName, 'http')">
							<xsl:element name="a">
								<xsl:attribute name="href"><xsl:value-of select="$thisName" /></xsl:attribute>
								<xsl:if test="substring($topic, 2, 1) != '%'">
									<xsl:attribute name="class">extLink</xsl:attribute>
								</xsl:if><xsl:copy-of select="$name2" />
							</xsl:element>
						</xsl:when>
						<xsl:when test="starts-with($thisName, 'mailto')">
							<xsl:element name="a">
								<xsl:attribute name="href"><xsl:value-of select="$thisName" /></xsl:attribute>
								<xsl:if test="substring($topic, 2, 1) != '%'">
									<xsl:attribute name="class">extLink</xsl:attribute>
								</xsl:if><xsl:copy-of select="$name2" />
							</xsl:element>
						</xsl:when>
						<xsl:otherwise>
							<xsl:element name="a">
								<xsl:attribute name="href"><xsl:value-of select="$thisName" />.html</xsl:attribute>
								<xsl:if test="not(substring($topic, 2, 1) = '%')">
									<xsl:attribute name="class">topicLink</xsl:attribute>
								</xsl:if>
								<xsl:copy-of select="$name2" />
							</xsl:element>
						</xsl:otherwise>
					</xsl:choose>
	
				</xsl:when>
				<xsl:when test="substring($topic, 1, 1) = '!' and string($supportMenu)">
					<menu style="{substring-after($topic, '!')}"/>
				</xsl:when>
				<xsl:when test="substring($topic, 1, 1) = '#' and string($supportShowme)">
					<show-me what="{substring-after($topic, '#')}" id="{$currentID}" />
				</xsl:when>
				<xsl:when test="substring($topic, 1, 1) = '+' and string($supportSource)">
					<div class="viewSource"><a href="content/{$currentID}.orig" class="viewSource">view content source file</a></div>
				</xsl:when>
				<xsl:when test="substring($topic, 1, 1) = '%' and string($supportImage)">
					<image href="{substring-after($topic, '%')}" id="{$currentID}" />
				</xsl:when>
				<xsl:when test="starts-with($topic, 'http')">
					<xsl:element name="a">
						<xsl:attribute name="href"><xsl:value-of select="$topic" /></xsl:attribute>
						<xsl:attribute name="class">extLink</xsl:attribute><xsl:value-of select="$topic" />
					</xsl:element>
				</xsl:when>
				<xsl:otherwise>
					<xsl:element name="a">
						<xsl:attribute name="href"><xsl:value-of select="$topic" />.html</xsl:attribute>
						<xsl:attribute name="class">topicLink</xsl:attribute><xsl:value-of select="$name" />
					</xsl:element>
				</xsl:otherwise>
			</xsl:choose>
			
		</xsl:if>
	</xsl:variable>

			<xsl:call-template name="lastCall">
				<xsl:with-param name="content" select="$middle" />
			</xsl:call-template>
<!-- 	<xsl:copy-of select="$middle" />
 -->
	<xsl:variable name="more">
		<xsl:call-template name="findNext">
			<xsl:with-param name="content" select="$item-end" />
		</xsl:call-template>
	</xsl:variable>
	
	<xsl:choose>
		<xsl:when test="string(normalize-space($more))">
			<xsl:call-template name="replace-tags"><xsl:with-param name="content" select="$item-end" /><xsl:with-param name="currentID" select="$currentID" /></xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
			<xsl:if test="string(normalize-space($item-end))">
				<xsl:call-template name="lastCall">
					<xsl:with-param name="content" select="$item-end" />
				</xsl:call-template>
<!-- 				<xsl:copy-of select="$item-end" />
 -->			</xsl:if>
		</xsl:otherwise>
	</xsl:choose>

	<xsl:if test="not(string(normalize-space($item-start))) and not(string(normalize-space($item-end))) and not(string(normalize-space($topic)))">
			<xsl:call-template name="lastCall">
				<xsl:with-param name="content" select="$content" />
			</xsl:call-template>
<!-- 		<xsl:copy-of select="$content" /> -->
	</xsl:if>

</xsl:template>

<xsl:template name="markupLine">
	<xsl:param name="content" />
	<xsl:param name="currentID" />
	<xsl:variable name="first" select="substring($content, 1, 1)" />
	<xsl:variable name="c">
		<xsl:if test="$wikistyle">
		  <xsl:if test="string($first) = '?'">question</xsl:if>
		  <xsl:if test="string($first) = '='">answer</xsl:if>
		  <xsl:if test="string($first) = '!'">note</xsl:if>
		  <xsl:if test="string($first) = '*'">point</xsl:if>
		  <xsl:if test="string($first) = '.'">indent</xsl:if>
		  <xsl:if test="string($first) = '|'">code</xsl:if>
		  <xsl:if test="string($first) = ':'">blank</xsl:if>
		  <xsl:if test="string($first) = '-'">headerSmall</xsl:if>
		  <xsl:if test="string($first) = '+'">headerLarge</xsl:if>
		  <xsl:if test="string($first) = '#'">meta</xsl:if>
		</xsl:if>
		<xsl:if test="not($wikistyle)"></xsl:if>
	</xsl:variable>

	<!-- <xsl:message>
		<xsl:text>c: </xsl:text><xsl:value-of select="$c"/>
		<xsl:text> wikistyle: </xsl:text><xsl:value-of select="$wikistyle"/>
	</xsl:message> -->
	
	<xsl:variable name="cont">
		<xsl:if test="string($c) and $wikistyle">
			<xsl:copy-of select="substring($content, 2)" /></xsl:if>
		<xsl:if test="not(string($c))">
			<xsl:copy-of select="$content" /></xsl:if>
	</xsl:variable>
	
	<xsl:variable name="class">
		<xsl:if test="string($c)">
			<xsl:copy-of select="$c" /></xsl:if>
		<xsl:if test="not(string($c))">normal</xsl:if>
	</xsl:variable>
	
	<xsl:choose>
		<xsl:when test="$c = 'meta'">
			<xsl:variable name="what" select="$cont" />
			<xsl:variable name="before" select="substring-before($what, '=')" />
			<xsl:variable name="after" select="substring-after($what, '=')" />
			<xsl:if test="string($before)">
				<xsl:text disable-output-escaping="yes"><![CDATA[<]]></xsl:text><xsl:copy-of select="normalize-space($before)" /><xsl:text disable-output-escaping="yes"><![CDATA[>]]></xsl:text>
					<xsl:copy-of select="$after" />
				<xsl:text disable-output-escaping="yes"><![CDATA[</]]></xsl:text><xsl:copy-of select="normalize-space($before)" /><xsl:text disable-output-escaping="yes"><![CDATA[>]]></xsl:text>
			</xsl:if>
		</xsl:when>
		<xsl:when test="$c = 'code'">
			<p class="{$class}"><xsl:copy-of select="$cont" /></p>
		</xsl:when>
		<xsl:when test="not($wikistyle)">
			<p class="{$class}"><xsl:copy-of select="$cont" /></p>
			<!-- <xsl:message terminate="yes" /> -->
		</xsl:when>
		<xsl:otherwise>
			<p class="{$class}"><xsl:call-template name="replace-tags"><xsl:with-param name="content" select="$cont" /><xsl:with-param name="currentID" select="$currentID" /></xsl:call-template></p>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template name="doLine">
	<xsl:param name="content" />
	<xsl:param name="currentID" />
	<xsl:variable name="line" select="normalize-space(concat( substring-before( $content, $controlCharacter ), $controlCharacter))" />
	<xsl:variable name="lineClear" select="normalize-space(translate( $line,  $controlCharacter, ' ' ))" />
	<xsl:variable name="next" select="normalize-space(substring-after( $content, $line ))" />
<!-- 	[<xsl:value-of select="$content" />]<br/>
	[<xsl:value-of select="$line" />]<br/>
	[<xsl:value-of select="$lineClear" />]<br/>
	[<xsl:value-of select="$next" />]<br/> -->
	<xsl:if test="string(normalize-space($lineClear))">
		<xsl:call-template name="markupLine"><xsl:with-param name="content"><xsl:copy-of select="$lineClear" /></xsl:with-param><xsl:with-param name="currentID" select="$currentID" /></xsl:call-template>
	</xsl:if>
	<xsl:if test="string(normalize-space($next))">
		<xsl:call-template name="doLine"><xsl:with-param name="content"><xsl:copy-of select="$next" /></xsl:with-param><xsl:with-param name="currentID" select="$currentID" /></xsl:call-template>
	</xsl:if>
</xsl:template>

</xsl:stylesheet>
