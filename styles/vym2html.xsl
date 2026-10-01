<?xml version="1.0" encoding="UTF-8" ?>

<!--
    Document   : vym2html.xsl
    Created on : 20040313
	Modified   : 20040512,20040622	Uwe Drechsel
	Version    : 0.7
    Author     : Clemens Kraus (http://www.clemens-kraus.de)
    Description: transforms vym-files into html format.
    
	Hint:
	tm.xsl and cm.xsl were taken from Alexander Johannesen's 
	beautiful xSiteable content management system: 
		http://xsiteable.org
	and are used with kind permission
-->

	

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

	<!-- Include external stylesheets -->
	<xsl:include href="tm.xsl" />
	<xsl:include href="cm.xsl" />
  
	<!-- Settings and parameters -->
	<xsl:output method="html" indent="no" encoding="UTF-8"/> 
	<xsl:param name="filenamep" />
	<xsl:param name="wikistylep" />
	<xsl:param name="genimagep" />
	<xsl:param name="imageonlyp" />
	<xsl:param name="stylesheetp" />
	<xsl:param name="urlHeadingp" />
	<xsl:param name="urlImagep" />
	<xsl:variable name="filename" select="$filenamep"/>
	<xsl:variable name="wikistyle" select="$wikistylep"/>
	<xsl:variable name="genimage" select="$genimagep"/>
	<xsl:variable name="imageonly" select="$imageonlyp"/>	
	<xsl:variable name="stylesheet" select="$stylesheetp"/>
	<xsl:variable name="topics" select="//item" /> 
	<xsl:variable name="urlHeading" select="$urlHeadingp"/> 
	<xsl:variable name="urlImage" select="$urlImagep"/> 

	<xsl:variable name="level" select="'0'"/>			
	<xsl:variable name="contentlist" select="false()"/> 

  
	<xsl:variable name="fn" >
		<xsl:call-template name="getfn" >
			<xsl:with-param name="txt" select="$filename" />
		</xsl:call-template>
	</xsl:variable>

	<xsl:variable name="path" >
	  <xsl:value-of select="normalize-space( substring($filename, 1, number(string-length($filename)-string-length($fn))) )" />
	</xsl:variable>

	<xsl:variable name="stylesheetn" >
	<xsl:call-template name="getfn" >
		<xsl:with-param name="txt" select="$stylesheet" />
	</xsl:call-template>
	</xsl:variable><!-- path -->



	<!-- Beginning of transformation here -->
	<xsl:template match="/">
	<xsl:variable name="mapversion" select="vymmap/@version"/>
	<xsl:variable name="header" >
		<xsl:call-template name="getheading" >
			<xsl:with-param name="txt" select="vymmap/mapcenter/heading" />
		</xsl:call-template>
	</xsl:variable><!-- header -->
		
	<!-- Write head of HTML document -->  
	<xsl:text disable-output-escaping="yes"><![CDATA[<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">]]>&#xA;</xsl:text>
	<html><xsl:text>&#xA;</xsl:text>
		<head><xsl:text>&#xA;</xsl:text>
			<title><xsl:value-of select="$header"/></title>
			<xsl:text disable-output-escaping="yes">&#xA;<![CDATA[<link href="]]></xsl:text>
			<xsl:value-of select="$stylesheetn"/>
			<xsl:text disable-output-escaping="yes"><![CDATA[" rel="stylesheet" id="]]></xsl:text>
			<xsl:value-of select="$stylesheetn"/>
			<xsl:text disable-output-escaping="yes"><![CDATA[">]]>&#xA;</xsl:text>
			<meta name="generator" content="vym (http://www.insilmaril.de/vym/) and vym2html.xsl (http://www.clemens-kraus.de)"/><xsl:text>&#xA;</xsl:text>
		</head>
		<xsl:text>&#xA;</xsl:text>

		<!-- Write body of HTML document -->  
		<body>
			<!-- vym title box at top of page -->
			<xsl:text>&#xA;</xsl:text>
			<div class="vymTitleBox">
				<xsl:value-of select="$header"/>
			</div>

			<!-- Include image of map, if wanted -->
			<xsl:if test="$genimage != ''">
				<xsl:variable name="image" select="concat($fn, '.png')" />

				<div class="vymImageMap">
					<img src="images/{$image}" 
						border="0" 
						usemap="#vymmap"
						alt="vymmap">
					</img>
				</div>
				<xsl:text>&#xA;</xsl:text>

				<!-- Generate image map by a first walk through all branches -->
				<map name="vymmap">
					<xsl:apply-templates select="vymmap/mapcenter/branch">
						<xsl:with-param name="genarea" select="true()"/>
						<xsl:with-param name="imageonly" select="$imageonly"/>
					</xsl:apply-templates>
				</map>

				<xsl:text>&#xA;</xsl:text>
			</xsl:if>

			<!-- Include note of mapcenter here -->
			<xsl:apply-templates select="vymmap/mapcenter/note/@href"/>


			<!-- Include all the branches, if not only image is wanted-->
			<xsl:if test="$imageonly = ''">
				<xsl:apply-templates select="vymmap/mapcenter/branch">
					<xsl:with-param name="genarea" select="false()"/>
					<xsl:with-param name="imageonly" select="$imageonly"/>
					<xsl:with-param name="depth" select="0"/>
				</xsl:apply-templates>
			</xsl:if>

			<!-- Box at bottom of page, containing comment and author -->
			<div class="vymBoxBottom">
				<xsl:value-of select="vymmap/@comment"/>
				<div class="vymBoxBottomR">
					<xsl:value-of select="vymmap/@author"/>
				</div>
			</div>

			<!-- Footer containing filename, date, selfpromotion -->
			<table class="vymFooter">
				<tr>
					<td class="vymFooterL">
						<xsl:value-of select="$fn"/>.vym
					</td>
					<td class="vymFooterC">
						<xsl:value-of select="vymmap/@date"/>
					</td>
					<td class="vymFooterR">
						vym <xsl:value-of select="$mapversion"/> 
					</td>  
				</tr>
			</table>

		</body>&#xA;
	</html>&#xA;
	</xsl:template><!-- Beginning of tranformation, "/" template -->

	
	<!-- Main Function generating a branch  -->
	<xsl:template match="branch">
		<!-- Work a branch twice: a) make id for anchors, b) make output -->
		<xsl:param name="genarea"/>
		<xsl:param name="imageonly"/>
		<xsl:param name="depth"/>
		<xsl:if test="$genarea">
			<!-- Generate id's and anchors, no output yet -->
			<xsl:if test="(@x1 != '' and @y1 != '' and @x2 != '' and @y2 != '') and not(../@scrolled = 'yes')">
				<!-- Heading is used in ALT field of anchor -->
				<xsl:variable name="header">
					<xsl:call-template name="getheading" >
						<xsl:with-param name="txt" select="heading" />
					</xsl:call-template>
				</xsl:variable>

				<xsl:if test="($imageonly and @url) or ($imageonly and @vymLink) or $imageonly=''">
					<area>
					<xsl:attribute name="shape">
						<xsl:text>rect</xsl:text>
					</xsl:attribute>
					<xsl:attribute name="coords">
						<xsl:value-of select="@x1"/>
						<xsl:text>,</xsl:text>
						<xsl:value-of select="@y1"/>
						<xsl:text>,</xsl:text>
						<xsl:value-of select="@x2"/>
						<xsl:text>,</xsl:text>
						<xsl:value-of select="@y2"/>
					</xsl:attribute>
					<xsl:attribute name="href">
						<xsl:if test="$imageonly = ''">
							<xsl:text>#</xsl:text>
							<xsl:value-of select="generate-id(.)"/>
						</xsl:if>
						<xsl:if test="$imageonly and @url">
							<xsl:value-of select="@url"/>
						</xsl:if>
						<xsl:if test="$imageonly and @vymLink">
							<xsl:value-of select="concat( substring-before(@vymLink, '.vym'), '.html')"/>
						</xsl:if>
					</xsl:attribute>
					<xsl:attribute name="alt">
						<xsl:value-of select="$header"/>
					</xsl:attribute>
					<xsl:attribute name="title">
						<xsl:value-of select="$header"/>
					</xsl:attribute>
					</area>
					<xsl:text>&#xA;</xsl:text>
				</xsl:if>
			</xsl:if> <!--generating id's and anchors -->

			<xsl:apply-templates select="branch">
				<xsl:with-param name="genarea" select="true()"/>
				<xsl:with-param name="imageonly" select="$imageonly"/>
				<xsl:with-param name="depth" select="$depth +1"/>
			</xsl:apply-templates>
		</xsl:if><!--genarea-->

		
		<xsl:if test="$imageonly = ''">
			<xsl:if test="not($genarea)">
				<xsl:call-template name="anchor">
					<xsl:with-param name="depth" select="$depth"/>
				</xsl:call-template>
				<xsl:text>&#xA;</xsl:text>
			</xsl:if>
		</xsl:if>
	</xsl:template><!--branch-->



	<xsl:template name="anchor">
		<xsl:param name="depth"/>
		<xsl:if test="$depth=0"> 
			<hr />
			<xsl:call-template name="gen-anchor-tag">
				<xsl:with-param name="depth" select="$depth"/>
			</xsl:call-template>
				<xsl:if test="count(descendant::branch)">
					<ul>
					<xsl:apply-templates select="branch">
						<xsl:with-param name="genarea" select="false()"/>
						<xsl:with-param name="depth" select="$depth+1"/>
					</xsl:apply-templates>
					</ul>
				</xsl:if>	
		</xsl:if>

	<xsl:if test="$depth > 0">
			<li>
				<xsl:call-template name="gen-anchor-tag">
					<xsl:with-param name="depth" select="$depth"/>
				</xsl:call-template>
				<xsl:if test="count(descendant::branch)">
					<ul>
					<xsl:apply-templates select="branch">
						<xsl:with-param name="genarea" select="false()"/>
						<xsl:with-param name="depth" select="$depth+1"/>
					</xsl:apply-templates>
					</ul>
				</xsl:if>	
			</li>
		</xsl:if>
	</xsl:template><!--anchor-->


	<xsl:template name="gen-anchor-tag">
		<xsl:param name="depth"/>

		<xsl:variable name="header">
			<xsl:call-template name="getheading" >
			<xsl:with-param name="txt" select="heading" />
			</xsl:call-template>
		</xsl:variable><!--header-->

		<div class="vymBranch{$depth}">
			<xsl:if test="not($genimage = '')">
				<a name="{generate-id(.)}"></a>
			</xsl:if>

			<!-- start header -->
			<xsl:value-of select="$header"/>

			<!-- Include flags -->
			<xsl:apply-templates select="./standardFlag" />

			<!-- URL to external document  -->
			<xsl:if test="@url != ''">
				<p class="vymURL">
				<xsl:element name="a">
					<xsl:attribute name="href">
						<xsl:value-of select="@url"/>
					</xsl:attribute>
					<xsl:if test="$urlImage= 'yes'">
						<img src="flags/url.png" 
							border="0" 
							valign="middle"
							alt="URL">
						</img>
					</xsl:if>
					<xsl:if test="$urlImage!= 'yes'">
						<xsl:text>&gt; </xsl:text>
					</xsl:if>
					<xsl:if test="$urlHeading = 'yes'">
						<xsl:value-of select="$header"/>
					</xsl:if>
					<xsl:if test="$urlHeading != 'yes'">
						<xsl:value-of select="@url"/>
					</xsl:if>
				</xsl:element>
				</p>
			</xsl:if><!-- URL to ext. doc -->
			<xsl:text>&#xA;</xsl:text>
		</div>

		<xsl:apply-templates select="note/@href"/>

		<xsl:text>&#xA;</xsl:text>
	</xsl:template><!--gen-anchor-tag-->


	<xsl:template match="standardFlag">
		<xsl:variable name="flag" select="concat(., '.png')" />
		<img src="flags/{$flag}" valign="middle" border="0" alt="{$flag}">
		</img>

	</xsl:template><!--standardFlag-->


	<xsl:template match="note/@href">
		<div class="vymNote">
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
		</xsl:variable><!--note-name-->


		<xsl:if test="../@fonthint = 'fixed'">
			<xsl:text disable-output-escaping="yes">&lt;pre></xsl:text>
		</xsl:if>

		<xsl:variable name="currentID" select="@id" />

		<xsl:apply-templates select="document($note-name)/note">
			<xsl:with-param name="currentID" select="$currentID" />
		</xsl:apply-templates>

		<xsl:if test="../@fonthint = 'fixed'">
			<xsl:text disable-output-escaping="yes">&lt;/pre></xsl:text>
		</xsl:if>

		</div>
	</xsl:template><!--note/@href-->


  
<!--	Wiki-style notation overview:
  		Lines:
		+ Big headlines start with the '+' character.
		- Small headlines start with the '-' character.
		Normal text doesn't have any starting notation.
		! Notes start with an exclamation.
		. Indented text starts with a dot.
		? Questions start with a question-mark, and
		= Answers starts with the equal-sign.
		* Points for a item-list
		# clues
		
		Links:
		use "{...}" or {(Clemens homepage) http://www.clemens-kraus.de/} for external links.
		
		Markup:
		|This is bold| text, while |/this text is italic|, |*this is pre-formatted|, and |!this is a note|
	-->
  
	<xsl:template match="note">
		<xsl:param name="currentID"/>


		<xsl:call-template name="doLine">
			<xsl:with-param name="content" select="concat(translate(.,$crlf,$controlCharacter), $controlCharacter)"/>
			<xsl:with-param name="currentID" select="$currentID"/>
		</xsl:call-template>
	</xsl:template><!--note-->

	  
  
  
  
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
	</xsl:template><!--getfn-->

  
	<!-- Take care of br's  (WHY?)  --> 
	<xsl:template name="getheading">
		<xsl:param name="txt" select="." />
		<xsl:variable name="br">
			<xsl:text disable-output-escaping="yes">&lt;br&gt;</xsl:text>
		</xsl:variable>
		<xsl:choose>
			<xsl:when test="contains($txt, $br)" >
				<xsl:variable name="right" select="substring-after($txt, $br)" />
				<xsl:variable name="left" select="substring-before($txt, $br)" />
				<xsl:value-of select="concat( $left, ' ', $right )" />
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="$txt" />
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template><!--getheading-->
  
</xsl:stylesheet>

