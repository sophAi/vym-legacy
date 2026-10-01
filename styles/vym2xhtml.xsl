<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE xsl:stylesheet
[
   <!-- Namespace for XHTML -->
   <!ENTITY xhtmlns "http://www.w3.org/1999/xhtml">
]>

<!--
    Document    : vym2xhtml.xsl
    Created     : 20040818
	Changed		: 20051123
    License     : GPL
    Version     : 0.4.0
    VYM version : 1.7.5
    Author      : Thomas Schraitle <tom_schr@web.de>
				  modified by Clemens Kraus (http://www.clemens-kraus.de)
    Description : transforms vym-files into XHTML.
    Bugs        : Many. ;) Produces at the moment not valid XHTML
                  Needs to checked.
									- li/ul structure not ok
		Changes			: - <br>s in headings removed
									- error fixed in "alt" and "title"
-->

<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:date="http://exslt.org/dates-and-times"
    extension-element-prefixes="date"
    xmlns="&xhtmlns;">


<xsl:output method="xml"
    doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
    doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"
    encoding="UTF-8"
    indent="yes"
    media-type="application/xhtml+xml"/>



<!-- ====================================================================== -->
<!-- 1 = true, 0 = false -->

<!-- URL to CSS stylesheet -->
<xsl:param name="css.stylesheet" select="'vym.css'"/>

<!-- Should a default CSS stylesheet be used? -->
<xsl:param name="use.default.css.stylesheet" select="1"/>

<!-- Should textcolors be used? -->
<xsl:param name="use.textcolor" select="0"/>

<!-- Should an imagemap be generated? -->
<xsl:param name="use.imagemap" select="1"/>

<!-- URL to image for imagemap -->
<xsl:param name="imagemap" select="''"/>

<!-- This stylesheet is able to process the following VYM version -->
<xsl:param name="vym.version" select="'1.7.10'"/>

<!-- Should the VYM XML format be checked -->
<xsl:param name="checkvym.version" select="1"/>

<!-- Which is the image extension? -->
<xsl:param name="image.extension" select="'.png'"/>

<!-- Where are the flags? -->
<xsl:param name="image.flags.path" select="'flags/'"/>

<!-- Filename of the XML document -->
<xsl:param name="mapname" />

<!-- Should a footer be generated? -->
<xsl:param name="use.footer" select="1"/>

<!-- How should Links generated:
    name : Use only the name of the branch (default)
    url  : Use only the URL of the branch
    both : Use both
-->
<xsl:param name="link.style" select="'name'"/>

<!-- Accept different html-notes? 
		Only for imported MindManager maps! -->
<xsl:param name="use.diffnotes" select="0"/>

<!-- Debuggin on/off? -->
<xsl:param name="debug" select="0"/>


<xsl:template name="generate.footer">
  <xsl:if test="$use.footer">
      <hr/>
      <table class="vym-footer">
         <tr >
         <td class="vym-footerL"><xsl:value-of select="$mapname"/></td>
         <td class="vym-footerC"><xsl:value-of select="vymmap/@date"/> </td>
         <!--<td class="vym-footerC"><xsl:value-of select="date:date()"/></td>-->
         <td class="vym-footerR">vym <xsl:value-of select="vymmap/@version"/></td>
         </tr>
      </table>
  </xsl:if>
</xsl:template>





<!-- ====================================================================== -->
<xsl:variable name="head.title">
   <xsl:choose>
      <xsl:when test="/vymmap/mapcenter/heading">
			
				<xsl:variable name="title">
					<xsl:call-template name="gettitle" >
						<xsl:with-param name="txt" select="/vymmap/mapcenter/heading" />
					</xsl:call-template>
				</xsl:variable>

        <xsl:value-of select="$title"/>
      </xsl:when>
      <xsl:otherwise></xsl:otherwise>
   </xsl:choose>
</xsl:variable>

<xsl:variable name="default.css.stylesheet">
   <xsl:text>
h1 {border-width: 1; border: solid; text-align: center}
div.imagemap { align: center; border: 0; }
   </xsl:text>
</xsl:variable>


<!-- ====================================================================== -->
<xsl:template name="generate.head">
   <head>
      <title><xsl:value-of select="$head.title"/></title>
      <xsl:if test="$use.default.css.stylesheet">
         <style type="text/css">
            <xsl:value-of select="$default.css.stylesheet"/>
         </style>
      </xsl:if>
      <xsl:if test="vymmap/@author!=''">
         <meta name="author" content="{vymmap/@author}"/>
      </xsl:if>
      <xsl:if test="vymmap/@comment!=''">
         <meta name="comment" content="{vymmap/@comment}"/>
      </xsl:if>
      <meta name="generator" content="vym"/>
      <xsl:if test="$css.stylesheet!=''">
         <link rel="stylesheet" id="css.stylesheet" href="{$css.stylesheet}"/>
      </xsl:if>
   </head>
</xsl:template>


<xsl:template name="check.vym.version">
   <xsl:if test="$checkvym.version">
      <xsl:if test="not(/vymmap/@version=$vym.version)">
         <xsl:message>
            <xsl:text>&#10;</xsl:text>
            <xsl:text>  WARNING:</xsl:text>
            <xsl:text> This stylesheet applies to VYM XML format v.</xsl:text>
            <xsl:value-of select="$vym.version"/>
            <xsl:text>.&#10;  Your XML format has v</xsl:text>
            <xsl:value-of select="/vymmap/@version"/>
            <xsl:text>.&#10;  Check your HTML output!</xsl:text>
            <xsl:text>&#10;&#10;</xsl:text>
         </xsl:message>
      </xsl:if>
   </xsl:if>
</xsl:template>


<!-- ====================================================================== -->
<xsl:template match="*">
   <xsl:message>
      <xsl:text>WARNING: Unknown tag "</xsl:text>
      <xsl:value-of select="local-name(.)"/>
      <xsl:text>": </xsl:text>
      <xsl:value-of select="normalize-space(.)"/>
      <xsl:text>&#10;</xsl:text>
   </xsl:message>
</xsl:template>


<xsl:template match="/">
   <xsl:call-template name="check.vym.version"/>

   <html xmlns="&xhtmlns;">
      <xsl:call-template name="generate.head"/>
      <body>
         <xsl:apply-templates/>
         <xsl:call-template name="generate.footer"/>
      </body>
   </html>
</xsl:template>


<xsl:template match="vymmap">
   <div class="vymmap">
      <xsl:apply-templates/>
   </div>
</xsl:template>


<xsl:template match="mapcenter">
   <div class="mapcenter">
      <xsl:apply-templates/>
   </div>
</xsl:template>


<xsl:template match="mapcenter/heading">
   <div class="vym-header">
	   <xsl:apply-templates/>
   </div>
 
      <xsl:if test="$use.imagemap=1">
         <div class="vym-imagemap">
            <img src="{$imagemap}"
               alt="Imagemap"
               class="imagemap"
               usemap="#vym_imagemap"/>
         </div>
         <map name="vym_imagemap">
            <xsl:apply-templates select="../branch" mode="imagemap"/>
         </map>
      </xsl:if>
</xsl:template>


<xsl:template match="mapcenter/branch">
   <hr/>
   <ul class="branch">
      <xsl:apply-templates/>
   </ul>
</xsl:template>


<xsl:template match="branch">
   <ul class="branch">
      <xsl:apply-templates/>
   </ul>
</xsl:template>


<xsl:template match="heading">
   <li class="heading">
	  <span id="{generate-id(..)}">
      <xsl:if test="@textColor!='' and $use.textcolor=1">
         <xsl:attribute name="style" >color: <xsl:value-of select="@textColor" />
		 </xsl:attribute>
      </xsl:if>
      <xsl:choose>
         <xsl:when test="../@url">
            <xsl:variable name="url" select="../@url"/>

            <!-- Check, how links should be generated -->
            <xsl:choose>
               <xsl:when test="$link.style = 'name'">
                   <a href="{$url}">
						<img src="{concat($image.flags.path,'flag-url-16x16.png')}" border="0" alt="URL"/>
						<xsl:text> </xsl:text>
						<xsl:apply-templates/>
					</a>
               </xsl:when>
               <xsl:when test="$link.style = 'url'">
					<a href="{$url}">
						<img src="{concat($image.flags.path,'flag-url-16x16.png')}" border="0" alt="URL"/>
						<xsl:text> </xsl:text>
						<xsl:value-of select="$url"/>
					</a>
               </xsl:when>
               <xsl:when test="$link.style = 'both'">
                  <a href="{$url}">
						<img src="{concat($image.flags.path,'flag-url-16x16.png')}" border="0" alt="URL"/>
						<xsl:text> </xsl:text>
						<xsl:apply-templates/> (<xsl:value-of select="$url"/>)
					</a>
               </xsl:when>
               <xsl:otherwise>
                  <xsl:message>
                     <xsl:text>WARNING: Parameter link.style doesn't contain the correct</xsl:text>
                     <xsl:text> value (name|url|both)</xsl:text>
                     <xsl:text>&#10; was "</xsl:text>
                     <xsl:value-of select="$link.style"/>
                     <xsl:text>"</xsl:text>
                  </xsl:message>
                  <a href="{$url}"><xsl:apply-templates/></a>
               </xsl:otherwise>
            </xsl:choose>

         </xsl:when>
         <xsl:otherwise>
						<xsl:call-template name="gettitle" >
							<xsl:with-param name="txt" select="." />
						</xsl:call-template>
						
         </xsl:otherwise>
      </xsl:choose>

      <xsl:for-each select="following-sibling::standardflag">
         <xsl:apply-templates select="current()" mode="standardflag"/><xsl:text> </xsl:text>
      </xsl:for-each>
      </span>
   </li>
</xsl:template>


<xsl:template match="floatimage">
   <xsl:variable name="filename">
      <xsl:choose>
         <xsl:when test="contains(@href,':')">
            <xsl:value-of select="substring-after(@href,':')"/>
         </xsl:when>
         <xsl:otherwise>
            <xsl:value-of select="@href"/>
         </xsl:otherwise>
      </xsl:choose>
   </xsl:variable>
   
  <xsl:if test="@floatExport='true'">
      <span><img src="{$filename}" alt="{$filename}"/></span>
  </xsl:if>
</xsl:template>


<xsl:template match="standardflag"/><!-- Do nothing in normal mode -->

<xsl:template match="standardflag" mode="standardflag">
   <span class="standardflag">
      <xsl:element name="img">
         <xsl:variable name="_srcimg">
         <xsl:choose>
            <xsl:when test="$image.flags.path">
               <xsl:value-of select="concat($image.flags.path,
                  .,
                  $image.extension)"/>
            </xsl:when>
            <xsl:otherwise>
               <xsl:value-of select="concat(., $image.extension)"/>
            </xsl:otherwise>
         </xsl:choose>
         </xsl:variable>
         <xsl:attribute name="src">
            <xsl:value-of select="$_srcimg"/>
         </xsl:attribute>
         <xsl:attribute name="alt">
            <xsl:value-of select="$_srcimg"/>
         </xsl:attribute>
      </xsl:element>
   </span>
</xsl:template>

<!-- Do nothing! -->
<xsl:template match="select"/>
<xsl:template match="setting"/>


<xsl:template match="htmlnote">
   <div class="vym-htmlnote">
			<xsl:choose>
				<xsl:when test="$use.diffnotes=1">
	      	<xsl:copy-of select="."/>
				</xsl:when>
				<xsl:otherwise>
					<xsl:apply-templates select=".//body/*"/><!-- Select only body elements -->
				</xsl:otherwise>
			</xsl:choose>
   </div>
</xsl:template>

<!-- Do nothing! We don't need some informational elements -->
<xsl:template match="htmlnote/html/*"/>

<xsl:template match="htmlnote/html/body">
   <xsl:copy-of select="."/>
</xsl:template>

<xsl:template match="htmlnote/html/body/*">
   <xsl:copy-of select="."/>
</xsl:template>


<!-- ====================================================================== -->
<xsl:template match="branch" mode="imagemap"> 
   <xsl:param name="node"/> 
   <xsl:variable name="title"> 
      <xsl:apply-templates mode="imagemap"/> 
   </xsl:variable> 
 
   <xsl:if test="$debug=1"> 
      <xsl:message> 
      branch/heading = "<xsl:value-of select="normalize-space($title)"/>" 
      </xsl:message> 
   </xsl:if> 
 
   <area shape="rect"> 
      <xsl:attribute name="href"> 
         <xsl:choose><!-- Fix begin (!) --> 
            <xsl:when test="$imagemap != ''"> 
               <xsl:value-of select="concat('#', generate-id(.))"/> 
            </xsl:when> 
            <xsl:when test="$imagemap and @url"> 
               <xsl:value-of select="@url"/> 
            </xsl:when> 
            <xsl:when test="$imagemap and @vymLink"> 
               <xsl:value-of select="concat( substring-before(@vymLink, 
'.vym'), '.html')"/> 
            </xsl:when> 
         </xsl:choose><!-- Fix end --> 
      </xsl:attribute> 
      <xsl:attribute name="alt"> 
				 <xsl:call-template name="gettitle" >
				 	<xsl:with-param name="txt" select="heading" />
				 </xsl:call-template>
      </xsl:attribute> 
      <xsl:attribute name="title"> 
				<xsl:call-template name="gettitle" >
				 	<xsl:with-param name="txt" select="heading" />
				</xsl:call-template>
      </xsl:attribute> 
      <xsl:attribute name="coords"> 
         <xsl:choose> 
            <xsl:when test="@x1!='' and @x2!='' and @y1!='' and @y2!=''"> 
               <xsl:value-of select="@x1"/> 
               <xsl:text>,</xsl:text> 
               <xsl:value-of select="@y1"/> 
               <xsl:text>,</xsl:text> 
               <xsl:value-of select="@x2"/> 
               <xsl:text>,</xsl:text> 
               <xsl:value-of select="@y2"/> 
            </xsl:when> 
            <xsl:otherwise> 
               <!-- 
               <xsl:message> 
                  <xsl:text>ERROR: Some coordinates in branch are 
missing!&#10;</xsl:text> 
                  <xsl:text>       See branch with </xsl:text> 
                  <xsl:value-of select="normalize-space($title)"/> 
               </xsl:message>
               //--> 
            </xsl:otherwise> 
         </xsl:choose> 
      </xsl:attribute> 
   </area> 
   <xsl:apply-templates select="./branch" mode="imagemap"/> 
</xsl:template> 


<xsl:template match="heading" mode="imagemap">
		<xsl:call-template name="gettitle" >
			<xsl:with-param name="txt" select="." />
		</xsl:call-template>
		<!--<xsl:message>title2: <xsl:value-of select="$title" /></xsl:message>-->

    <xsl:apply-templates mode="imagemap"/>
</xsl:template>


<xsl:template match="xlink">
		<xsl:element name="a">
			<xsl:attribute name="name">
				<xsl:value-of select="translate(@beginBranch, ':,', '')"/>
			</xsl:attribute>
		</xsl:element>

		<div class="xlink">
			<xsl:text>See: </xsl:text>
			<xsl:element name="a">
				<xsl:attribute name="href">
					<xsl:text>#</xsl:text><!--<xsl:value-of select="translate(@endBranch, ':,', '')"/>-->
				</xsl:attribute>
				<!--<xsl:value-of select="translate(@endBranch, ':,', '')"/>-->reference
			</xsl:element>
				<!--<xsl:apply-templates/>-->
				<!--<xsl:message>->xlink: <xsl:value-of select="concat(@endBranch, ' ', position())" /></xsl:message>-->
   </div>
</xsl:template>


<xsl:template name="gettitle">
		<xsl:param name="txt" select="." />
		
		<xsl:variable name="br">
			<xsl:text disable-output-escaping="yes">&lt;br&gt;</xsl:text>
		</xsl:variable>
		
		<xsl:choose>
			<xsl:when test="contains($txt, $br)" >
				<xsl:variable name="right" select="substring-after($txt, $br)" />
				<xsl:variable name="left" select="substring-before($txt, $br)" />
				<xsl:variable name="txt" select="concat( $left, ' ', $right )" />
					<xsl:call-template name="gettitle" >
						<xsl:with-param name="txt" select="$txt" />
					</xsl:call-template>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="$txt" />
			</xsl:otherwise>
		</xsl:choose>
</xsl:template>


</xsl:stylesheet>
