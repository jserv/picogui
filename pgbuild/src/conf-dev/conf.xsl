<?xml version="1.0" ?>
<!--
  This is an XML Stylesheet that pretty-prints the PGBuild
  configuration files in a web browser with XSL support.
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  
  <xsl:template match="/">
    <html>
      <body>
        <h1>PGBuild Configuration</h1>

        <xsl:for-each select="sources">
          <h2>Sources</h2>

          <xsl:for-each select="sites">
            <h3>Sites</h3>
            <ul>

              <xsl:for-each select="site">
                <li>
                  <xsl:value-of select="@name"/>
                  <ul><xsl:apply-templates match="a"/></ul>
                </li>
              </xsl:for-each>

            </ul>
          </xsl:for-each>

          <xsl:for-each select="packages">
            <h3>Packages</h3>
            <ul>

              <xsl:for-each select="package">
                <li>
                  <xsl:value-of select="@name"/>
                  <ul>
                    <xsl:for-each select="version">
                      <li>
                        Version: <xsl:value-of select="@name"/>
                        <ul><xsl:apply-templates match="a"/></ul>
                      </li>
                    </xsl:for-each>
                    <xsl:for-each select="require">
                      <li>Required</li>
                    </xsl:for-each>
                  </ul>
                </li>
              </xsl:for-each>

            </ul>
          </xsl:for-each>

        </xsl:for-each>

      </body>
    </html>
  </xsl:template>

  <xsl:template match="a">
    <li>
      <a>
        <xsl:attribute name="href">
          <xsl:value-of select="@href"/>
        </xsl:attribute>
        <xsl:value-of select="@href"/>
      </a>
      <xsl:for-each select="site">
        (at site <xsl:value-of select="@name"/>)
      </xsl:for-each>
    </li>
  </xsl:template>

</xsl:stylesheet>