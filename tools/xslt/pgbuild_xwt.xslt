<?xml version="1.0" ?>
<!--
  An experimental proof-of-concept XWT for translating a PGBuild
  configuration file into an XML Widget Template. Something similar
  to this could be done to automatically form a package listing GUI.
-->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!--================================== Document Root -->
  
  <xsl:template match="/pgbuild">
    <application>
      <xsl:attribute name="text">      
        <xsl:value-of select="@title"/>
      </xsl:attribute>

      <xsl:apply-templates select="packages"/>
      <xsl:apply-templates select="sites"/>

    </application>
  </xsl:template>

  <!--================================== Packages -->

  <xsl:template match="packages">
    <tabpage text="Packages">
      <scrollbox side="all">
        <xsl:apply-templates select="package">
          <xsl:sort select="@name"/>
        </xsl:apply-templates>
      </scrollbox>
    </tabpage>
  </xsl:template>

  <xsl:template match="package">
    <checkbox>
      <xsl:value-of select="@name"/>
      <box side="right" size="75" sizemode="percent">
        <xsl:apply-templates select="description"/>
      </box>
    </checkbox>
  </xsl:template>

  <!--================================== Download sites -->

  <xsl:template match="sites">
    <tabpage text="Download Sites">
      <scrollbox side="all">
        <xsl:apply-templates select="site">
          <xsl:sort select="@name"/>
        </xsl:apply-templates>
      </scrollbox>
    </tabpage>
  </xsl:template>

  <xsl:template match="site">
    <checkbox>
      <xsl:value-of select="@name"/>
    </checkbox>
  </xsl:template>

  <!--================================== Descriptions -->

  <xsl:template match="description">
    <textbox readonly="1">
      <xsl:value-of select="."/>
    </textbox>
  </xsl:template>

  <!--================================== Links -->

  <xsl:template match="a">
    <xsl:choose>
      <xsl:when test="site">
        <!-- Site link -->
        <xsl:value-of select="@href"/> @ <xsl:value-of select="site/@name"/>
      </xsl:when>
      <xsl:otherwise>
        <!-- Normal link -->
        <xsl:value-of select="@href"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:transform>
