<?xml version="1.0" ?>
<!--
  This is an XML Stylesheet that can pretty-print any PGBuild
  configuration file by translating it into XHTML.
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!--================================== Document Root -->
  
  <xsl:template match="/pgbuild">
    <html>
      <head>
        <title><xsl:value-of select="@title"/></title>
        <style type="text/css" media="all"> @import url(pgbuild.css);</style>
      </head>
      <body>
        <div class="heading">
          <div class="fileType">PGBuild Configuration File:</div>
          <div class="title"><xsl:value-of select="@title"/></div>
        </div>

        <xsl:apply-templates select="packages"/>
        <xsl:apply-templates select="sites"/>

      </body>
    </html>
  </xsl:template>

  <!--================================== Packages -->

  <xsl:template match="packages">
    <span class="section">Packages</span>
    <div class="section">
      <div class="sectionTop"/>
      <xsl:apply-templates select="package">
        <xsl:sort select="@name"/>
      </xsl:apply-templates>
    </div>
  </xsl:template>

  <xsl:template match="package">
    <div class="row">
      <xsl:apply-templates select="require"/>
      <div class="item"><xsl:value-of select="@name"/></div>
      <div class="itemDetail">
        <xsl:apply-templates select="version">
          <xsl:sort select="@name"/>
        </xsl:apply-templates>
      </div>
    </div>
  </xsl:template>

  <xsl:template match="require">
    <span class="itemTag">Required</span>
  </xsl:template>

  <xsl:template match="version">
    <div class="row">
      <xsl:value-of select="../@name"/>-<xsl:value-of select="@name"/>
      <ul>
        <xsl:apply-templates select="a">
          <xsl:sort select="@href"/>
        </xsl:apply-templates>
      </ul>
    </div>
  </xsl:template>

  <!--================================== Download sites -->

  <xsl:template match="sites">
    <span class="section">Download Sites</span>
    <div class="section">
      <div class="sectionTop"/>
      <xsl:apply-templates select="site">
        <xsl:sort select="@name"/>
      </xsl:apply-templates>
    </div>
  </xsl:template>

  <xsl:template match="site">
    <div class="row">
      <div class="item"><xsl:value-of select="@name"/></div>
      <div class="itemDetail">
        <ul>
          <xsl:apply-templates select="a">
            <xsl:sort select="@href"/>
          </xsl:apply-templates>
        </ul>
      </div>
    </div>
  </xsl:template>

  <!--================================== Links -->

  <xsl:template match="a">
    <li>
      <xsl:value-of select="@href"/>
      <xsl:for-each select="site">
        <em> @ </em><xsl:value-of select="@name"/>
      </xsl:for-each>
    </li>
  </xsl:template>

</xsl:stylesheet>
