<?xml version="1.0" encoding="iso-8859-1" standalone="yes"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:exsl="http://exslt.org/common" extension-element-prefixes="exsl" version="1.0">

<xsl:template name="getTopicName">
	<xsl:param name="id"/>
	<xsl:variable name="searching" select="$topics[@id=$id]" />
	<xsl:variable name="get"><xsl:value-of select="$searching/baseName/baseNameString" /><xsl:value-of select="$searching/@name" /></xsl:variable>
	<xsl:if test="string( $get )"><xsl:value-of select="$get" /></xsl:if>
	<xsl:if test="not(string( $get ))"><span class="notFound"><xsl:value-of select="$id" /> not found.</span></xsl:if>
</xsl:template>
	
<xsl:template name="getParent">
	<xsl:param name="currentID"/>
	<xsl:call-template name="getAllAssocID">
		<xsl:with-param name="filter" select="'#group-subgroup'" />
		<xsl:with-param name="role" select="'#parent'" />
		<xsl:with-param name="currentID" select="$currentID" />
	</xsl:call-template>
</xsl:template>

<xsl:template name="getChild">
	<xsl:param name="currentID"/>
	<xsl:call-template name="getAllAssocID">
		<xsl:with-param name="filter" select="'#group-subgroup'" />
		<xsl:with-param name="role" select="'#child'" />
		<xsl:with-param name="currentID" select="$currentID" />
	</xsl:call-template>
</xsl:template>

<xsl:template name="getAllAssocID">
	<xsl:param name="filter"/>
	<xsl:param name="role"/>
	<xsl:param name="currentID"/>
	
	<xsl:variable name="theseOnes" select="$associations[instanceOf/topicRef/@xlink:href=$filter][member/topicRef/@xlink:href=concat('#',$currentID)]" />
	<xsl:variable name="fiddle" select="$theseOnes/member/topicRef[not(@xlink:href=concat('#',$currentID))][parent::*/roleSpec/topicRef/@xlink:href=$role]" />

	<xsl:value-of select="substring-after($fiddle[1]/@xlink:href, '#')" />
	
</xsl:template>

<xsl:template name="getAllAssoc">
	<xsl:param name="filter"/>
	<xsl:param name="role"/>
	<xsl:param name="style" select="'default'" />
	<xsl:param name="title"/>
	<xsl:param name="box"/>
	<xsl:param name="without"/>
	<xsl:param name="currentID"/>
	<xsl:param name="againstID"/>
	
	<xsl:variable name="theseOnes" select="$associations[instanceOf/topicRef/@xlink:href=$filter][member/topicRef/@xlink:href=concat('#',$currentID)]" />

 	<xsl:variable name="found"><xsl:choose> 
			<xsl:when test="$style = 'sections'">
				<table border="0" cellspacing="0" cellpadding="0" class="sections"><tr>
				<xsl:for-each select="$theseOnes/member/topicRef[@xlink:href!=concat('#',$currentID)][parent::*/roleSpec/topicRef/@xlink:href=$role]"> 
					<xsl:variable name="thisOne" select="substring-after(@xlink:href, '#')" />
					<xsl:variable name="cat" select="concat('#',$currentID)" />
					<td valign="top" class="{$style}Menu">
						<a href="{$pre-link}{$thisOne}{$post-link}" class="{$style}MenuLink"><xsl:call-template name="getTopicName"><xsl:with-param name="id" select="$thisOne" /></xsl:call-template></a>
						<div class="{$style}MenuContent"><xsl:call-template name="getClue"><xsl:with-param name="currentID" select="$thisOne" /></xsl:call-template></div>
					</td>
		 		</xsl:for-each>
				</tr></table>
			</xsl:when>
			<xsl:when test="$style = 'news'">
				<xsl:for-each select="$theseOnes/member/topicRef[@xlink:href!=concat('#',$currentID)][parent::*/roleSpec/topicRef/@xlink:href=$role]"> 
					<xsl:variable name="thisOne" select="substring-after(@xlink:href, '#')" />
					<xsl:variable name="cat" select="concat('#',$currentID)" />
					<div class="{$style}Menu">
						<a href="{$pre-link}{$thisOne}{$post-link}" class="{$style}MenuLink">
							<xsl:call-template name="getTopicName"><xsl:with-param name="id" select="$thisOne" /></xsl:call-template> : 
							<xsl:call-template name="getClue"><xsl:with-param name="currentID" select="$thisOne" /><xsl:with-param name="style" select="'news'" /></xsl:call-template>
						</a>
					</div>
		 		</xsl:for-each>
			</xsl:when>
			<xsl:when test="$style = 'smallnews'">
				<xsl:for-each select="$theseOnes/member/topicRef[@xlink:href!=concat('#',$currentID)][parent::*/roleSpec/topicRef/@xlink:href=$role]"> 
					<xsl:variable name="thisOne" select="substring-after(@xlink:href, '#')" />
					<xsl:variable name="cat" select="concat('#',$currentID)" />
					<div class="{$style}Menu">
						<a href="{$pre-link}{$thisOne}{$post-link}" class="{$style}MenuLink">
							<xsl:call-template name="getTopicName"><xsl:with-param name="id" select="$thisOne" /></xsl:call-template> : 
							 <xsl:call-template name="getShort"><xsl:with-param name="currentID" select="$thisOne" /></xsl:call-template>
						</a>
					</div>
		 		</xsl:for-each>
			</xsl:when>
			<xsl:otherwise>
			
				<xsl:variable name="allThese" select="$theseOnes/member/topicRef[@xlink:href!=concat('#',$currentID)][parent::*/roleSpec/topicRef/@xlink:href=$role]" />
				<xsl:for-each select="$allThese"> 
				
				<xsl:variable name="thisOne" select="substring-after(@xlink:href, '#')" />
					<xsl:variable name="cat" select="concat('#',$currentID)" />
						<xsl:choose> 
							<xsl:when test="$style = 'right' or $style = 'left' or $style = 'full' or $style = 'tree'">
								<xsl:choose>
									<xsl:when test="$thisOne = $againstID">
										<div class="{$style}MenuSelected"><a href="{$pre-link}{$thisOne}{$post-link}"><xsl:call-template name="getTopicName"><xsl:with-param name="id" select="$thisOne" /></xsl:call-template></a> </div>
									</xsl:when>
									<xsl:otherwise>
										<div class="{$style}Menu"><a href="{$pre-link}{$thisOne}{$post-link}"><xsl:call-template name="getTopicName"><xsl:with-param name="id" select="$thisOne" /></xsl:call-template></a> </div>
									</xsl:otherwise>
								</xsl:choose>
							</xsl:when>
							<xsl:when test="$style = 'banner'">
								<xsl:choose>
									<xsl:when test="$thisOne = $againstID">
										<td class="{$style}MenuSelected"><a href="{$pre-link}{$thisOne}{$post-link}"><xsl:call-template name="getTopicName"><xsl:with-param name="id" select="$thisOne" /></xsl:call-template></a> </td>
									</xsl:when>
									<xsl:otherwise>
										<td class="{$style}Menu"><a href="{$pre-link}{$thisOne}{$post-link}"><xsl:call-template name="getTopicName"><xsl:with-param name="id" select="$thisOne" /></xsl:call-template></a> </td>
									</xsl:otherwise>
								</xsl:choose>
							</xsl:when>
							<xsl:otherwise>
								<xsl:if test="position() != 1"> / </xsl:if>
								<a href="{$pre-link}{$thisOne}{$post-link}"><xsl:call-template name="getTopicName"><xsl:with-param name="id" select="$thisOne" /></xsl:call-template> </a>
							</xsl:otherwise>
						</xsl:choose>
		 		</xsl:for-each>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:variable>
	<xsl:if test="string($found)">
		<xsl:choose>
			<xsl:when test="string($box) and $style != 'news' and $style != 'right' and $style != 'left' and $style != 'full' and $style = 'tree' and $style = 'banner'">
				<table class="{$box}">
				<tr>
					<xsl:if test="string($title)"><td class="{$box}Title"><xsl:value-of select="$title" /></td></xsl:if>
					<td class="{$box}Content"><xsl:copy-of select="$found" /></td>
				</tr>
				</table>
			</xsl:when>
			<xsl:otherwise>
				<xsl:copy-of select="$found" />
			</xsl:otherwise>
		</xsl:choose>
	</xsl:if>
</xsl:template>

<xsl:template name="getAllAssocTwo">
	<xsl:param name="filter"/>
	<xsl:param name="role"/>
	<xsl:param name="type"/>
	<xsl:param name="style" select="'default'" />
	<xsl:param name="title"/>
	<xsl:param name="box"/>
	<xsl:param name="without"/>
	<xsl:param name="currentID"/>
	<xsl:param name="againstID"/>
	<xsl:variable name="theseOnes" select="$associations[instanceOf/topicRef/@xlink:href=$filter][member/topicRef/@xlink:href=concat('#',$currentID)]" />
	<xsl:variable name="allThese" select="$theseOnes/member/topicRef[@xlink:href!=concat('#',$currentID)][parent::*/roleSpec/topicRef/@xlink:href=$role]" />
	<xsl:variable name="w" select="substring-after($type, '#')" />
	<xsl:variable name="all" select="$topics[starts-with(@id, concat($w, '-'))]" />
	<xsl:variable name="found">
		<xsl:for-each select="$all"> 
			<xsl:variable name="this" select="@id" />
			<xsl:variable name="c" select="count( $allThese[substring-after(@xlink:href, '#')=$this])" />
			<xsl:if test="$c &gt; 0">
				<xsl:variable name="single" select="$this[1]" />
				<a href="{$pre-link}{$single}{$post-link}" class="foundLink"><xsl:call-template name="getTopicName"><xsl:with-param name="id" select="$single" /></xsl:call-template></a>
			</xsl:if>
		</xsl:for-each>
	</xsl:variable>
	<xsl:if test="string($found)"><xsl:copy-of select="$found" /></xsl:if>
</xsl:template>

<xsl:template name="drawNextLevel">	
	<xsl:param name="currentID"/>
	<xsl:param name="againstID"/>
	<xsl:param name="currentType"/>
	<xsl:param name="currentRole"/>
	<xsl:param name="style"/>
	<xsl:call-template name="getAllAssoc">
		<xsl:with-param name="currentID" select="$currentID" />
		<xsl:with-param name="againstID" select="$againstID" />
		<xsl:with-param name="filter" select="'#group-subgroup'" />
		<xsl:with-param name="role" select="$currentRole" />
		<xsl:with-param name="box" select="'box'" />
		<xsl:with-param name="style" select="$style" />
	</xsl:call-template>
</xsl:template>

<xsl:template name="drawRelated">
	<xsl:param name="currentID"/>
	<xsl:param name="currentType"/>
	<xsl:param name="currentRole"/>
	<xsl:param name="style" select="'box'" />
	<xsl:call-template name="getAll">
		<xsl:with-param name="ThisThat" select="'#this'" />
		<xsl:with-param name="role" select="$currentRole" />
		<xsl:with-param name="box" select="$style" />
		<xsl:with-param name="currentID" select="$currentID" />
	</xsl:call-template>
	<xsl:call-template name="getAll">
		<xsl:with-param name="ThisThat" select="'#that'" />
		<xsl:with-param name="role" select="$currentRole" />
		<xsl:with-param name="box" select="$style" />
		<xsl:with-param name="currentID" select="$currentID" />
	</xsl:call-template>
</xsl:template> 	

<xsl:template name="getAll">
	<xsl:param name="ThisThat"/>
	<xsl:param name="filter"/>
	<xsl:param name="title"/>
	<xsl:param name="role"/>
	<xsl:param name="box"/>
	<xsl:param name="without"/>
	<xsl:param name="currentID"/>
	<xsl:variable name="allRelations" select="$topics[starts-with(@id, 'relation-')]" />
	<xsl:variable name="allAssociations" select="$associations[member/topicRef/@xlink:href=concat('#',$currentID)]" />
	<xsl:for-each select="$allRelations">
		<xsl:variable name="relID" select="concat('#', @id)" />
		<xsl:variable name="ThisThatRole"><xsl:choose><xsl:when test="$ThisThat = '#that'">#child</xsl:when><xsl:otherwise>#parent</xsl:otherwise></xsl:choose></xsl:variable>
		<xsl:variable name="thisName"><xsl:value-of select="baseName[scope/topicRef/@xlink:href=$ThisThat]/baseNameString" /></xsl:variable>
		<xsl:variable name="found">
			<xsl:for-each select="$allAssociations">
				<xsl:variable name="assocInstance" select="instanceOf/topicRef/@xlink:href" />
				<xsl:variable name="assocHref" select="member[roleSpec/topicRef/@xlink:href=$ThisThatRole]/topicRef/@xlink:href" />
				<xsl:variable name="thisID" select="substring-after($assocHref, '#')" />
				<xsl:if test="$relID = $assocInstance and $thisID != $currentID">			
					<span class="{$box}Element"><a href="{$pre-link}{$thisID}{$post-link}"><xsl:call-template name="getTopicName"><xsl:with-param name="id" select="$thisID" /></xsl:call-template></a></span>
				</xsl:if>
			</xsl:for-each>
		</xsl:variable>
		<xsl:if test="string($found) and string($thisName)">
			<xsl:call-template name="drawRelationsBox">
				<xsl:with-param name="style" select="$box" />
				<xsl:with-param name="title" select="$thisName" />
				<xsl:with-param name="content" select="$found" />
			</xsl:call-template>
		</xsl:if>
	</xsl:for-each>	
</xsl:template>

<xsl:template name="getAllBlogs">
	<xsl:param name="currentID"/>
	<xsl:param name="notraverse"/>
	
	<xsl:variable name="parentID">
		<xsl:call-template name="getParent">
			<xsl:with-param name="currentID" select="$currentID" />
		</xsl:call-template>
	</xsl:variable>
	<xsl:choose>
		<xsl:when test="string($notraverse)">
			<xsl:variable name="theseOnes" select="$topics[instanceOf/topicRef/@xlink:href='#blog']" />
			<xsl:variable name="assoc" select="$associations[instanceOf/topicRef/@xlink:href='#group-subgroup'][member/topicRef/@xlink:href=concat('#',$currentID)]/member[roleSpec/topicRef/@xlink:href='#child']/topicRef" />
			<div class="doc">
				<xsl:for-each select="$theseOnes">
					<xsl:variable name="ct" select="." />
					<xsl:for-each select="$assoc">
						<xsl:variable name="ca" select="." />
						<xsl:variable name="t" select="substring-after($ca/@xlink:href, '#')" />
						<xsl:if test="$t = $ct/@id">
							<xsl:variable name="currentDate" select="$ct/occurrence[instanceOf/topicRef/@xlink:href='#blog-date']/resourceData" />
							<div class="blogDate"><xsl:value-of select="$currentDate" /></div>
							<xsl:call-template name="displayBlogItem">
								<xsl:with-param name="item" select="$ct" />
							</xsl:call-template>
						</xsl:if>
					</xsl:for-each>
				</xsl:for-each>
			</div>
		</xsl:when>
		<xsl:otherwise>
			<xsl:variable name="items" select="$topics[instanceOf/topicRef/@xlink:href='#blog']" />
			<xsl:call-template name="traverseBlog">
				<xsl:with-param name="items" select="$items[position() &lt; 6]" />
				<xsl:with-param name="currentNo" select="'1'" />
			</xsl:call-template>
		</xsl:otherwise>
	</xsl:choose>

</xsl:template>

<xsl:template name="traverseBlog">
	<xsl:param name="items" />
	<xsl:param name="currentNo" />
	<xsl:param name="previousDate" />
	
	<xsl:variable name="item" select="$items[position() = $currentNo]" />

	<xsl:variable name="currentDate" select="$item/occurrence[instanceOf/topicRef/@xlink:href='#blog-date']/resourceData" />

	<xsl:if test="string($currentDate)">
	
		<xsl:if test="string($previousDate) = string($currentDate)">
			<div class="blogEnd"> </div>
		</xsl:if>
		
		<xsl:if test="$previousDate != $currentDate">
			<div class="blogDate"><xsl:value-of select="$currentDate" /></div>
		</xsl:if>
		
		<xsl:call-template name="displayBlogItem">
			<xsl:with-param name="item" select="$item" />
		</xsl:call-template>
		
		<xsl:if test="string($items[position() = $currentNo+1])">
			<xsl:call-template name="traverseBlog">
				<xsl:with-param name="items" select="$items" />
				<xsl:with-param name="currentNo" select="$currentNo + 1" />
				<xsl:with-param name="previousDate" select="$currentDate" />
			</xsl:call-template>
		</xsl:if>
	
	</xsl:if>
	
</xsl:template>

<xsl:template name="displayBlogItem">
	<xsl:param name="item" />
	
	<xsl:variable name="currentHeader" select="$item/baseName/baseNameString" />
	<xsl:variable name="currentDate" select="$item/occurrence[instanceOf/topicRef/@xlink:href='#blog-date']/resourceData" />
	<xsl:variable name="currentLink" select="$item/occurrence[instanceOf/topicRef/@xlink:href='#blog-link']/resourceData" />
	<xsl:variable name="currentShort" select="$item/occurrence[instanceOf/topicRef/@xlink:href='#blog-short']/resourceData" />
	
	<div class="blogItem">
		<xsl:if test="string($currentHeader)"><div class="blogHeader"><xsl:value-of select="$currentHeader" /></div></xsl:if>
		
		<xsl:variable name="fileName">../content/<xsl:value-of select="$item/@id" />.xml</xsl:variable>
		<xsl:variable name="this" select="document($fileName)/div/*" />
		<xsl:apply-templates select="$this"><xsl:with-param name="currentID" select="$item/@id" /></xsl:apply-templates>
		
		<xsl:if test="string($currentLink)"><div class="blogLinkBox">Read the full story at &lt; <a href="{$currentLink}"><xsl:value-of select="$currentHeader" /></a> &gt;</div></xsl:if>

		<xsl:variable name="theseOnes" select="$associations[instanceOf/topicRef/@xlink:href='#group-subgroup'][member/topicRef/@xlink:href=concat('#',$item/@id)]/member[roleSpec/topicRef/@xlink:href='#parent']" />

		<span class="blogLinks"><a href="{$item/@id}.html" class="blogLink">Permalink</a>added <strong><xsl:value-of select="$currentDate" /></strong>.</span> 
		
		<xsl:if test="count($theseOnes) &gt; 0">
			<span class="blogLinks">
				Catalogued under:
				<xsl:for-each select="$theseOnes">
					<xsl:variable name="chug" select="substring-after(topicRef/@xlink:href, '#')" />
					<a href="{$pre-link}{$chug}{$post-link}" class="blogLink"><xsl:call-template name="getTopicName"><xsl:with-param name="id" select="$chug" /></xsl:call-template> </a>
				</xsl:for-each>
	 		</span>
		</xsl:if>
		
	</div>
	
</xsl:template>


</xsl:stylesheet>
