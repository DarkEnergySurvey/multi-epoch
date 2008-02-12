<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="/">
    <html>
      <head>
        <script src="qc_src/web/javascript/jquery.js" type="text/javascript"></script>
        <script src="qc_src/web/javascript/iutil.js" type="text/javascript"></script>
        <script src="qc_src/web/javascript/imagebox.js" type="text/javascript"></script>
        <script src="qc_src/web/javascript/jquery.history_remote.pack.js" type="text/javascript"></script>
        <script src="qc_src/web/javascript/jquery.tabs.js" type="text/javascript"></script>
        <script type="text/javascript">
          <![CDATA[
            $(document).ready(function(){ 
              $('#tabAnalysis').tabs();
              $('#tabAnalysis-1').tabs();
              $('#tabAnalysis-2').tabs();
              $('#tabAnalysis-3').tabs();
              $('#tabAnalysis-4').tabs();
              $('#tabAnalysis-5').tabs();
              $('#tabAnalysis-6').tabs();
              $('#tabAnalysis-7').tabs();
              $('#tabAnalysis-8').tabs();
              $('#tabAnalysis-9').tabs();
              $.ImageBox.init({
                loaderSRC: "",
                closeHTML: "<img src=\"qc_src/web/images/close.jpg\">",
		showCaption: 0
              });
            });
          ]]>
        </script>
        <style media="screen, projection" type="text/css">
          @import "qc_src/web/css/qc.css";
          @import "qc_src/web/css/jquery_tabs.css";
          
          #ImageBoxOverlay
          {
            background-color: #000;
          }
          #ImageBoxCaption
          {
            background-color: #F4F4EC;
          }
          #ImageBoxContainer
          {
            width: 250px;
            height: 250px;
            background-color: #F4F4EC;
          }
          #ImageBoxCaptionText
          {
            font-weight: bold;
            padding-bottom: 5px;
            font-size: 13px;
            color: #000;
          }
          #ImageBoxCaptionImages
          {
            margin: 0;
          }
          #ImageBoxNextImage
          {
            background-image: url('');
            background-color: transparent;
          }
          #ImageBoxPrevImage
          {
            background-image: url('');
            background-color: transparent;
          }
          #ImageBoxNextImage:hover
          {
            background-image: url('');
            background-repeat:  no-repeat;
            background-position: right top;
          }
          #ImageBoxPrevImage:hover
          {
            background-image: url('');
            background-repeat:  no-repeat;
            background-position: left bottom;
          }
          DIV.divs
          {
            border: solid;
            border-width: 1px;
            background: #FFFFCC;
            width: 260px;
          }
        </style>
      </head>
      <body>
        <div id="tabAnalysis">
          <ul>          
            <xsl:for-each select="/analysis/group">
              <li>
                <a href="#tabAnalysis-{position()}">
                  <span>
                    <xsl:value-of select="@name"/>
                  </span>
                </a>
              </li>
            </xsl:for-each>
          </ul>
          <xsl:for-each select="/analysis/group">
            <div id="tabAnalysis-{position()}">
              <ul>
                <xsl:for-each select="./subgroup">
                  <li>
                    <a href="#tabAnalysis-{../@name}-{position()}">
                      <span>
                        <xsl:value-of select="@name"/>
                      </span>
                    </a>
                  </li>
                </xsl:for-each>
              </ul>
              <xsl:for-each select="./subgroup">
                <div id="tabAnalysis-{../@name}-{position()}">
                  <center>
                    <xsl:for-each select="plot[position() mod 2 = 0]">
                      <div style="position: relative; width: 100%; height: 320px;">
                        <div class="divs" style="position: relative; float: left; left: 18%">
                          <div>
                            <font color="#0066CC"><xsl:value-of select="preceding-sibling::plot[1]/name"/></font>
                          </div>
                          <div>
                            <a href="{preceding-sibling::plot[1]/image}" target="_blank" rel ="imagebox-{preceding-sibling::plot[1]/image}" title=" {preceding-sibling::plot[1]/sub}">
                              <img width="250" src="{preceding-sibling::plot[1]/image} " />
                            </a>
                          </div>
                        </div>
                        <div class="divs" style="position: relative; float: right; right: 18%">
                          <div>
                            <font color="#0066CC"><xsl:value-of select="name"/></font>
                          </div>
                          <div>
                            <a href="{image}" target="_blank" rel="imagebox-{image}" title=" {sub}">
                              <img width="250" src="{image}" />
                            </a>
                          </div>
                        </div>
                      </div>
                    </xsl:for-each>
                    <xsl:for-each select="plot">
                      <xsl:if test="position() = last() and position() mod 2 = 1">
                        <div style="position: relative; width: 100%; height: 320px;">
                          <div class="divs" style="position: relative; float: left; left: 18%">
                            <div>
                              <font color="#0066CC"><xsl:value-of select="name"/></font>
                            </div>
                            <div>
                              <a href="{image}" target="_blank" rel="imagebox-{/image}" title="{sub}">
                                <img width="250" src="{image}" />
                              </a>
                            </div>
                          </div>
                        </div>
                      </xsl:if>
                    </xsl:for-each>
                    <table class="data_table" style="border-collapse: collapse; width: 82%; border-width: 3px; border-style: ridge; border-spacing: 0px;">
                      <xsl:for-each select="statistics/stats">
                        <tr>
                          <td style="width: 50%; border-width: 3px; border-style: ridge; border-spacing: 0px;" align="left"><xsl:value-of select="@name" /></td>
                          <td style="width: 50%; border-width: 3px; border-style: ridge; border-spacing: 0px;" align="right"><xsl:value-of select="." /></td>
                        </tr>
                      </xsl:for-each>
                    </table>
                  </center>
                </div>
              </xsl:for-each>

              <center>
                <xsl:if test="./@name = 'Completeness'">
                  <xsl:for-each select="table">
                    <table class="data_table_qc" style="width: 75%;">
                      <tr>
                        <xsl:for-each select="stats">
                          <th style="background:#84B2C6; color:#000000; border-collapse:collapse;">
                            <xsl:value-of select="@name" />
                          </th>
                        </xsl:for-each>
                      </tr>
                      <tr>
                        <xsl:for-each select="stats">
                          <td>
                            <xsl:value-of select="." />
                          </td>
                        </xsl:for-each>
                      </tr>
                    </table>
                  </xsl:for-each>
                  <br />
                  <br />
                  <xsl:for-each select="table/plot">
                    <div style="position: relative; width: 100%; height: 320px;">
                      <div class="divs" style="position: relative;">
                        <div>
                          <font color="#0066CC"><xsl:value-of select="name"/></font>
                        </div>
                        <div>
                          <a href="{image}" target="_blank" rel="imagebox-{/image}" title="{sub}">
                            <img width="250" src="{image}" />
                          </a>
                        </div>
                      </div>
                    </div>
                  </xsl:for-each>
                  <xsl:for-each select="./plot[position() mod 3 = 0]">
                    <div style="position: relative; width: 90%; height: 320px;">
                      <div class="divs" style="position: relative; float: left;">
                        <div>
                          <font color="#0066CC"><xsl:value-of select="preceding-sibling::plot[2]/name"/></font>
                        </div>
                        <div>
                          <a href="{preceding-sibling::plot[2]/image}" target="_blank" rel="imagebox-{preceding-sibling::plot[2]/image}" title="{preceding-sibling::plot[2]/sub}">
                            <img width="250" src="{preceding-sibling::plot[2]/image}" />
                          </a>
                        </div>
                      </div>
                      <div class="divs" style="position: relative; float: right;">
                        <div>
                          <font color="#0066CC"><xsl:value-of select="name"/></font>
                        </div>
                        <div>
                          <a href="{image}" target="_blank" rel="imagebox-{image}" title="{sub}">
                            <img width="250" src="{image}" />
                          </a>
                        </div>
                      </div>
                      <div class="divs" style="position: relative;">
                        <div>
                          <font color="#0066CC"><xsl:value-of select="preceding-sibling::plot[1]/name"/></font>
                        </div>
                        <div>
                          <a href="{preceding-sibling::plot[1]/image}" target="_blank" rel="imagebox-{preceding-sibling::plot[1]/image}" title="{preceding-sibling::plot[1]/sub}">
                            <img width="250" src="{preceding-sibling::plot[1]/image}" />
                          </a>
                        </div>
                      </div>
                    </div>
                  </xsl:for-each>
                  <xsl:for-each select="./plot">
                    <xsl:if test="position() = last() and position() mod 3 = 1">
                      <div style="position: relative; width: 100%; height: 320px;">
                        <div class="divs" style="position: relative; float: left; left: 18%">
                          <div>
                            <font color="#0066CC"><xsl:value-of select="name"/></font>
                          </div>
                          <div>
                            <a href="{image}" target="_blank" rel="imagebox-{/image}" title="{sub}">
                              <img width="250" src="{image}" />
                            </a>
                          </div>
                        </div>
                      </div>
                    </xsl:if>
                  </xsl:for-each>
                  <table class="data_table_qc" style="border-collapse: collapse; width: 75%; border-width: 3px; border-style: ridge; border-spacing: 0px;">
                    <xsl:for-each select="statistics/stats">
                      <tr>
                        <td style="width: 50%; border-width: 3px; border-style: ridge; border-spacing: 0px;" align="left"><xsl:value-of select="@name" /></td>
                        <td style="width: 50%; border-width: 3px; border-style: ridge; border-spacing: 0px;" align="right"><xsl:value-of select="." /></td>
                      </tr>
                    </xsl:for-each>
                  </table>
                </xsl:if>

                <xsl:if test="./@name = 'Spurious'">
                  <xsl:for-each select="table">
                    <table class="data_table_qc" style="width: 75%;">
                      <tr>
                        <xsl:for-each select="stats">
                          <th style="background:#84B2C6; color:#000000; border-collapse:collapse;">
                            <xsl:value-of select="@name" />
                          </th>
                        </xsl:for-each>
                      </tr>
                      <tr>
                        <xsl:for-each select="stats">
                          <td>
                            <xsl:value-of select="." />
                          </td>
                        </xsl:for-each>
                      </tr>
                    </table>
                  </xsl:for-each>
                  <br />
                  <br />
                  <xsl:for-each select="table/plot">
                    <div style="position: relative; width: 100%; height: 320px;">
                      <div class="divs" style="position: relative;">
                        <div>
                          <font color="#0066CC"><xsl:value-of select="name"/></font>
                        </div>
                        <div>
                          <a href="{image}" target="_blank" rel="imagebox-{/image}" title="{sub}">
                            <img width="250" src="{image}" />
                          </a>
                        </div>
                      </div>
                    </div>
                  </xsl:for-each>
                  <xsl:for-each select="./plot[position() mod 3 = 0]">
                    <div style="position: relative; width: 90%; height: 320px;">
                      <div class="divs" style="position: relative; float: left;">
                        <div>
                          <font color="#0066CC"><xsl:value-of select="preceding-sibling::plot[2]/name"/></font>
                        </div>
                        <div>
                          <a href="{preceding-sibling::plot[2]/image}" target="_blank" rel="imagebox-{preceding-sibling::plot[2]/image}" title="{preceding-sibling::plot[2]/sub}">
                            <img width="250" src="{preceding-sibling::plot[2]/image}" />
                          </a>
                        </div>
                      </div>
                      <div class="divs" style="position: relative; float: right;">
                        <div>
                          <font color="#0066CC"><xsl:value-of select="preceding-sibling::plot[1]/name"/></font>
                        </div>
                        <div>
                          <a href="{preceding-sibling::plot[1]/image}" target="_blank" rel="imagebox-{preceding-sibling::plot[1]/image}" title="{preceding-sibling::plot[1]/sub}">
                            <img width="250" src="{preceding-sibling::plot[1]/image}" />
                          </a>
                        </div>
                      </div>
                      <div class="divs" style="position: relative;">
                        <div>
                          <font color="#0066CC"><xsl:value-of select="name"/></font>
                        </div>
                        <div>
                          <a href="{image}" target="_blank" rel="imagebox-{image}" title="{sub}">
                            <img width="250" src="{image}" />
                          </a>
                        </div>
                      </div>
                    </div>
                  </xsl:for-each>
                  <xsl:for-each select="./plot">
                    <xsl:if test="position() = last() and position() mod 3 = 1">
                      <div style="position: relative; width: 100%; height: 320px;">
                        <div class="divs" style="position: relative; float: left; left: 18%">
                          <div>
                            <font color="#0066CC"><xsl:value-of select="name"/></font>
                          </div>
                          <div>
                            <a href="{image}" target="_blank" rel="imagebox-{/image}" title="{sub}">
                              <img width="250" src="{image}" />
                            </a>
                          </div>
                        </div>
                      </div>
                    </xsl:if>
                  </xsl:for-each>
                  <table class="data_table_qc" style="border-collapse: collapse; width: 75%; border-width: 3px; border-style: ridge; border-spacing: 0px;">
                    <xsl:for-each select="statistics/stats">
                      <tr>
                        <td style="width: 50%; border-width: 3px; border-style: ridge; border-spacing: 0px;" align="left"><xsl:value-of select="@name" /></td>
                        <td style="width: 50%; border-width: 3px; border-style: ridge; border-spacing: 0px;" align="right"><xsl:value-of select="." /></td>
                      </tr>
                    </xsl:for-each>
                  </table>
                </xsl:if>
		
                <xsl:if test="./@name != 'Spurious' and ./@name != 'Completeness'">
                  <xsl:for-each select="table">
                    <table class="data_table_qc" style="width: 75%;">
                      <tr>
                        <xsl:for-each select="stats">
                          <th style="background:#84B2C6; color:#000000; border-collapse:collapse;">
                            <xsl:value-of select="@name" />
                            <br />
                            <xsl:if test="@unit = ''">
                              &#160;
                            </xsl:if>
                            <xsl:if test="@unit != ''">
                              (<xsl:value-of select="@unit" />)
                            </xsl:if>
                          </th>
                        </xsl:for-each>
                      </tr>
                      <tr>
                        <xsl:for-each select="stats">
                          <td>
                            <xsl:value-of select="." />
                          </td>
                        </xsl:for-each>
                      </tr>
                    </table>
                  </xsl:for-each>
                  <br />
                  <br />
  		
                  <xsl:for-each select="table/plot">
                    <div style="position: relative; width: 100%; height: 320px;">
                      <div class="divs" style="position: relative;">
                        <div>
                          <font color="#0066CC"><xsl:value-of select="name"/></font>
                        </div>
                        <div>
                          <a href="{image}" target="_blank" rel="imagebox-{/image}" title="{sub}">
                            <img width="250" src="{image}" />
                          </a>
                        </div>
                      </div>
                    </div>
                  </xsl:for-each>

                  <xsl:for-each select="./plot[position() mod 2 = 0]">
                  <div style="position: relative; width: 100%; height: 320px;">
                    <div class="divs" style="position: relative; float: left; left: 18%">
                      <div>
                        <font color="#0066CC"><xsl:value-of select="preceding-sibling::plot[1]/name"/></font>
                      </div>
                      <div>
                        <a href="{preceding-sibling::plot[1]/image}" target="_blank" rel="imagebox-{preceding-sibling::plot[1]/image}" title="{preceding-sibling::plot[1]/sub}">
                          <img width="250" src="{preceding-sibling::plot[1]/image}" />
                        </a>
                      </div>
                    </div>
                    <div class="divs" style="position: relative; float: right; right: 18%">
                      <div>
                        <font color="#0066CC"><xsl:value-of select="name"/></font>
                      </div>
                      <div>
                        <a href="{image}" target="_blank" rel="imagebox-{image}" title="{sub}">
                          <img width="250" src="{image}" />
                        </a>
                      </div>
                    </div>
                  </div>
                  </xsl:for-each>
                  <xsl:for-each select="./plot">
                    <xsl:if test="position() = last() and position() mod 2 = 1">
                    <div style="position: relative; width: 100%; height: 320px;">
                      <div class="divs" style="position: relative; float: left; left: 18%">
                        <div>
                          <font color="#0066CC"><xsl:value-of select="name"/></font>
                        </div>
                        <div>
                          <a href="{image}" target="_blank" rel="imagebox-{/image}" title="{sub}">
                            <img width="250" src="{image}" />
                          </a>
                        </div>
                      </div>
                    </div>
                    </xsl:if>
                  </xsl:for-each>

                  <xsl:for-each select="frame">
                    <div style="position: relative; width: 100%;">
                      <iframe style="border: 0px none" src="{src}" width="95%" height="660"></iframe>
                    </div>
                  </xsl:for-each>
  
                  <table class="data_table_qc" style="border-collapse: collapse; width: 75%; border-width: 3px; border-style: ridge; border-spacing: 0px;">
                    <xsl:for-each select="statistics/stats">
                      <tr>
                        <td style="width: 50%; border-width: 3px; border-style: ridge; border-spacing: 0px;" align="left"><xsl:value-of select="@name" /></td>
                        <td style="width: 50%; border-width: 3px; border-style: ridge; border-spacing: 0px;" align="right"><xsl:value-of select="." /></td>
                      </tr>
                    </xsl:for-each>
                  </table>
                </xsl:if>
              </center>
            </div>
          </xsl:for-each>
        </div>
      </body>
    </html>
  </xsl:template>

</xsl:stylesheet>
