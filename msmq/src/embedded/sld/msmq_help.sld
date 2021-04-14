<?xml version="1.0" encoding="UTF-16"?>
<!DOCTYPE DCARRIER SYSTEM "Mantis.DTD">

  <DCARRIER
    CarrierRevision="1"
    DTDRevision="16"
  >
    <TASKS
      Context="1"
      PlatformGUID="{00000000-0000-0000-0000-000000000000}"
    >    </TASKS>

    <PLATFORMS
      Context="1"
    >    </PLATFORMS>

    <REPOSITORIES
      Context="1"
      PlatformGUID="{00000000-0000-0000-0000-000000000000}"
    >    </REPOSITORIES>

    <GROUPS
      Context="1"
      PlatformGUID="{00000000-0000-0000-0000-000000000000}"
    >    </GROUPS>

    <COMPONENTS
      Context="1"
      PlatformGUID="{00000000-0000-0000-0000-000000000000}"
    >
      <COMPONENT
        ComponentVSGUID="{3582A35D-15B2-4625-AA24-D54778043431}"
        ComponentVIGUID="{88EC042C-169A-4FF2-A1A9-67D4A107C7A2}"
        Revision="5"
        RepositoryVSGUID="{8E0BE9ED-7649-47F3-810B-232D36C430B4}"
        Visibility="1000"
        MultiInstance="False"
        Released="False"
        Editable="True"
        HTMLFinal="False"
        IsMacro="False"
        Opaque="False"
        Context="1"
        PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
      >
        <HELPCONTEXT
          src=".\_msmq_help_component_description.htm"
        ><![CDATA[<html xmlns:v="urn:schemas-microsoft-com:vml"
xmlns:o="urn:schemas-microsoft-com:office:office"
xmlns:w="urn:schemas-microsoft-com:office:word"
xmlns="http://www.w3.org/TR/REC-html40">

<head>
<meta http-equiv=Content-Type content="text/html; charset=windows-1252">
<meta name=ProgId content=Word.Document>
<meta name=Generator content="Microsoft Word 10">
<meta name=Originator content="Microsoft Word 10">
<link rel=File-List href="_msmq_help_component_description_files/filelist.xml">
<link rel=Edit-Time-Data
href="_msmq_help_component_description_files/editdata.mso">
<title>MSDN Authoring Template</title>
<!--[if gte mso 9]><xml>
 <o:DocumentProperties>
  <o:Author>Rayne Wiselman</o:Author>
  <o:Description>Use for authoring only. Publication should be accomplished with CSS for HTML, or a related Word template for print or WinHelp.</o:Description>
  <o:Template>comp_help_template.dot</o:Template>
  <o:LastAuthor>Rayne Wiselman</o:LastAuthor>
  <o:Revision>4</o:Revision>
  <o:TotalTime>10</o:TotalTime>
  <o:LastPrinted>2000-10-23T08:48:00Z</o:LastPrinted>
  <o:Created>2001-09-13T11:47:00Z</o:Created>
  <o:LastSaved>2001-09-13T12:23:00Z</o:LastSaved>
  <o:Pages>1</o:Pages>
  <o:Words>160</o:Words>
  <o:Characters>916</o:Characters>
  <o:Company>MSFT</o:Company>
  <o:Lines>7</o:Lines>
  <o:Paragraphs>2</o:Paragraphs>
  <o:CharactersWithSpaces>1074</o:CharactersWithSpaces>
  <o:Version>10.2625</o:Version>
 </o:DocumentProperties>
</xml><![endif]--><!--[if gte mso 9]><xml>
 <w:WordDocument>
  <w:ActiveWritingStyle Lang="EN-US" VendorID="8" DLLVersion="513" NLCheck="0">1</w:ActiveWritingStyle>
  <w:GrammarState>Clean</w:GrammarState>
  <w:AttachedTemplate HRef="comp_help_template.dot"></w:AttachedTemplate>
  <w:UpdateStylesOnOpen/>
  <w:DisplayHorizontalDrawingGridEvery>0</w:DisplayHorizontalDrawingGridEvery>
  <w:DisplayVerticalDrawingGridEvery>0</w:DisplayVerticalDrawingGridEvery>
  <w:UseMarginsForDrawingGridOrigin/>
  <w:Compatibility>
   <w:FootnoteLayoutLikeWW8/>
   <w:ShapeLayoutLikeWW8/>
   <w:AlignTablesRowByRow/>
   <w:ForgetLastTabAlignment/>
   <w:LayoutRawTableWidth/>
   <w:LayoutTableRowsApart/>
   <w:UseWord97LineBreakingRules/>
  </w:Compatibility>
  <w:BrowserLevel>MicrosoftInternetExplorer4</w:BrowserLevel>
 </w:WordDocument>
</xml><![endif]-->
<style>
<!--
 /* Font Definitions */
 @font-face
	{font-family:Verdana;
	panose-1:2 11 6 4 3 5 4 4 2 4;
	mso-font-charset:0;
	mso-generic-font-family:swiss;
	mso-font-pitch:variable;
	mso-font-signature:536871559 0 0 0 415 0;}
@font-face
	{font-family:"Microsoft Logo 95";
	panose-1:0 0 0 0 0 0 0 0 0 0;
	mso-font-alt:Symbol;
	mso-font-charset:2;
	mso-generic-font-family:auto;
	mso-font-format:other;
	mso-font-pitch:variable;
	mso-font-signature:0 0 0 0 0 0;}
 /* Style Definitions */
 p.MsoNormal, li.MsoNormal, div.MsoNormal
	{mso-style-name:"Normal\,APPLY ANOTHER STYLE";
	mso-style-parent:"";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:yellow;}
h1
	{mso-style-name:"Heading 1\,h1\,Level 1 Topic Heading";
	mso-style-parent:"";
	mso-style-next:"Text\,t";
	margin-top:9.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:18.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	page-break-after:avoid;
	mso-outline-level:1;
	font-size:16.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	color:black;
	mso-font-kerning:12.0pt;
	font-weight:bold;
	mso-bidi-font-weight:normal;}
h2
	{mso-style-name:"Heading 2\,h2\,Level 2 Topic Heading";
	mso-style-parent:"Heading 1\,h1\,Level 1 Topic Heading";
	mso-style-next:"Text\,t";
	margin-top:9.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:18.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	page-break-after:avoid;
	mso-outline-level:2;
	font-size:16.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	color:gray;
	mso-font-kerning:12.0pt;
	font-weight:bold;
	mso-bidi-font-weight:normal;}
h3
	{mso-style-name:"Heading 3\,h3\,Level 3 Topic Heading";
	mso-style-parent:"Heading 1\,h1\,Level 1 Topic Heading";
	mso-style-next:"Text\,t";
	margin-top:9.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:18.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	page-break-after:avoid;
	mso-outline-level:3;
	font-size:16.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	color:silver;
	mso-font-kerning:12.0pt;
	font-weight:bold;
	mso-bidi-font-weight:normal;}
h4
	{mso-style-name:"Heading 4\,h4\,First Subheading";
	mso-style-parent:"Heading 1\,h1\,Level 1 Topic Heading";
	mso-style-next:"Text\,t";
	margin-top:9.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:16.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	page-break-after:avoid;
	mso-outline-level:4;
	font-size:14.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	color:black;
	mso-font-kerning:12.0pt;
	font-weight:bold;
	mso-bidi-font-weight:normal;}
h5
	{mso-style-name:"Heading 5\,h5\,Second Subheading";
	mso-style-parent:"Heading 1\,h1\,Level 1 Topic Heading";
	mso-style-next:"Text\,t";
	margin-top:9.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:15.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	page-break-after:avoid;
	mso-outline-level:5;
	font-size:13.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	color:black;
	mso-font-kerning:12.0pt;
	font-weight:bold;
	mso-bidi-font-weight:normal;}
h6
	{mso-style-name:"Heading 6\,h6\,Third Subheading";
	mso-style-parent:"Heading 1\,h1\,Level 1 Topic Heading";
	mso-style-next:"Text\,t";
	margin-top:9.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:12.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	page-break-after:avoid;
	mso-outline-level:6;
	font-size:10.0pt;
	font-family:Verdana;
	color:black;
	mso-font-kerning:12.0pt;
	font-weight:bold;
	mso-bidi-font-weight:normal;}
p.MsoIndex1, li.MsoIndex1, div.MsoIndex1
	{mso-style-name:"Index 1\,idx1";
	mso-style-noshow:yes;
	mso-style-parent:"Text\,t";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:9.0pt;
	text-indent:-9.0pt;
	line-height:11.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	mso-list:l45 level1 lfo1;
	font-size:8.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:maroon;}
p.MsoIndex2, li.MsoIndex2, div.MsoIndex2
	{mso-style-name:"Index 2\,idx2";
	mso-style-noshow:yes;
	mso-style-parent:"Index 1\,idx1";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:27.0pt;
	text-indent:-9.0pt;
	line-height:11.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	mso-list:l45 level1 lfo1;
	font-size:8.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:maroon;}
p.MsoIndex3, li.MsoIndex3, div.MsoIndex3
	{mso-style-name:"Index 3\,idx3";
	mso-style-noshow:yes;
	mso-style-parent:"Index 1\,idx1";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:45.0pt;
	text-indent:-9.0pt;
	line-height:11.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	mso-list:l45 level1 lfo1;
	font-size:8.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:maroon;}
p.MsoToc1, li.MsoToc1, div.MsoToc1
	{mso-style-name:"TOC 1\,toc1";
	mso-style-noshow:yes;
	mso-style-parent:"Heading 1\,h1\,Level 1 Topic Heading";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:12.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	page-break-after:avoid;
	tab-stops:.25in;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:maroon;
	mso-font-kerning:12.0pt;
	font-weight:bold;
	mso-bidi-font-weight:normal;}
p.MsoToc2, li.MsoToc2, div.MsoToc2
	{mso-style-name:"TOC 2\,toc2";
	mso-style-noshow:yes;
	mso-style-parent:"Text\,t";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.25in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:maroon;}
p.MsoToc3, li.MsoToc3, div.MsoToc3
	{mso-style-name:"TOC 3\,toc3";
	mso-style-noshow:yes;
	mso-style-parent:"TOC 2\,toc2";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.5in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:maroon;}
p.MsoToc4, li.MsoToc4, div.MsoToc4
	{mso-style-name:"TOC 4\,toc4";
	mso-style-noshow:yes;
	mso-style-parent:"TOC 2\,toc2";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.75in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:maroon;}
p.MsoFootnoteText, li.MsoFootnoteText, div.MsoFootnoteText
	{mso-style-name:"Footnote Text\,ft\,Used by Word for text of Help footnotes";
	mso-style-noshow:yes;
	mso-style-parent:"Text\,t";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:red;}
p.MsoCommentText, li.MsoCommentText, div.MsoCommentText
	{mso-style-name:"Comment Text\,ct\,Used by Word for text of author queries";
	mso-style-noshow:yes;
	mso-style-parent:"Text\,t";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.MsoHeader, li.MsoHeader, div.MsoHeader
	{mso-style-name:"Header\,h";
	mso-style-parent:"Footer\,f";
	margin:0in;
	margin-bottom:.0001pt;
	text-align:right;
	line-height:11.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:8.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:maroon;}
p.MsoFooter, li.MsoFooter, div.MsoFooter
	{mso-style-name:"Footer\,f";
	mso-style-parent:"";
	margin:0in;
	margin-bottom:.0001pt;
	text-align:right;
	line-height:11.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:8.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:maroon;}
p.MsoIndexHeading, li.MsoIndexHeading, div.MsoIndexHeading
	{mso-style-name:"Index Heading\,ih";
	mso-style-noshow:yes;
	mso-style-parent:"Heading 1\,h1\,Level 1 Topic Heading";
	mso-style-next:"Index 1\,idx1";
	margin-top:9.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:15.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	page-break-after:avoid;
	mso-outline-level:5;
	font-size:13.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:maroon;
	mso-font-kerning:12.0pt;
	font-weight:bold;
	mso-bidi-font-weight:normal;}
span.MsoFootnoteReference
	{mso-style-name:"Footnote Reference\,fr\,Used by Word for Help footnote symbols";
	mso-style-noshow:yes;
	color:red;
	vertical-align:super;}
span.MsoPageNumber
	{mso-style-name:"Page Number\,pn";
	mso-ansi-font-size:8.0pt;
	font-family:Verdana;
	mso-ascii-font-family:Verdana;
	mso-hansi-font-family:Verdana;
	color:maroon;
	font-weight:bold;
	mso-bidi-font-weight:normal;}
a:link, span.MsoHyperlink
	{color:blue;
	text-decoration:underline;
	text-underline:single;}
a:visited, span.MsoHyperlinkFollowed
	{color:purple;
	text-decoration:underline;
	text-underline:single;}
p
	{mso-margin-top-alt:auto;
	margin-right:0in;
	mso-margin-bottom-alt:auto;
	margin-left:0in;
	mso-pagination:widow-orphan;
	font-size:12.0pt;
	font-family:"Times New Roman";
	mso-fareast-font-family:"Times New Roman";}
p.Text, li.Text, div.Text
	{mso-style-name:"Text\,t";
	mso-style-parent:"";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.Figure, li.Figure, div.Figure
	{mso-style-name:"Figure\,fig";
	mso-style-parent:"Text\,t";
	mso-style-next:"Text\,t";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:9.0pt;
	margin-left:0in;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.Code, li.Code, div.Code
	{mso-style-name:"Code\,c";
	mso-style-parent:"";
	margin-top:0in;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:15.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:"Courier New";
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;
	mso-no-proof:yes;}
p.TextinList2, li.TextinList2, div.TextinList2
	{mso-style-name:"Text in List 2\,t2";
	mso-style-parent:"Text\,t";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.5in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.Label, li.Label, div.Label
	{mso-style-name:"Label\,l";
	mso-style-parent:"Text\,t";
	mso-style-next:"Text\,t";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;
	font-weight:bold;
	mso-bidi-font-weight:normal;}
p.NumberedList2, li.NumberedList2, div.NumberedList2
	{mso-style-name:"Numbered List 2\,nl2";
	mso-style-parent:"Text in List 2\,t2";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.5in;
	text-indent:-.25in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	mso-list:l42 level1 lfo3;
	tab-stops:list .5in;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.Syntax, li.Syntax, div.Syntax
	{mso-style-name:"Syntax\,s";
	mso-style-parent:"Code\,c";
	margin-top:0in;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:15.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:"Courier New";
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;
	mso-no-proof:yes;}
p.TableFootnote, li.TableFootnote, div.TableFootnote
	{mso-style-name:"Table Footnote\,tf";
	mso-style-parent:"Text\,t";
	mso-style-next:"Text\,t";
	margin-top:2.0pt;
	margin-right:0in;
	margin-bottom:4.0pt;
	margin-left:0in;
	line-height:11.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	border:none;
	mso-border-top-alt:solid windowtext .5pt;
	padding:0in;
	mso-padding-alt:1.0pt 0in 0in 0in;
	font-size:8.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.TableSpacingAfter, li.TableSpacingAfter, div.TableSpacingAfter
	{mso-style-name:"Table Spacing After\,tsa";
	mso-style-parent:"Text\,t";
	mso-style-next:"Text\,t";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:0in;
	margin-left:0in;
	margin-bottom:.0001pt;
	line-height:6.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:6.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:yellow;}
p.CodeinList2, li.CodeinList2, div.CodeinList2
	{mso-style-name:"Code in List 2\,c2";
	mso-style-parent:"Code\,c";
	margin-top:0in;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.5in;
	line-height:15.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:"Courier New";
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;
	mso-no-proof:yes;}
p.FigureinList2, li.FigureinList2, div.FigureinList2
	{mso-style-name:"Figure in List 2\,fig2";
	mso-style-parent:"Figure\,fig";
	mso-style-next:"Text in List 2\,t2";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:9.0pt;
	margin-left:.5in;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.FigureEmbedded, li.FigureEmbedded, div.FigureEmbedded
	{mso-style-name:"Figure Embedded\,fige";
	mso-style-parent:"Text\,t";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:9.0pt;
	margin-left:0in;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.TableFootnoteinList2, li.TableFootnoteinList2, div.TableFootnoteinList2
	{mso-style-name:"Table Footnote in List 2\,tf2";
	mso-style-parent:"Text in List 2\,t2";
	mso-style-next:"Text in List 2\,t2";
	margin-top:2.0pt;
	margin-right:0in;
	margin-bottom:4.0pt;
	margin-left:.5in;
	line-height:11.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	border:none;
	mso-border-top-alt:solid windowtext .5pt;
	padding:0in;
	mso-padding-alt:1.0pt 0in 0in 0in;
	font-size:8.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.TextinList1, li.TextinList1, div.TextinList1
	{mso-style-name:"Text in List 1\,t1";
	mso-style-parent:"Text\,t";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.25in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.CodeinList1, li.CodeinList1, div.CodeinList1
	{mso-style-name:"Code in List 1\,c1";
	mso-style-parent:"Code\,c";
	margin-top:0in;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.25in;
	line-height:15.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:"Courier New";
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;
	mso-no-proof:yes;}
p.FigureinList1, li.FigureinList1, div.FigureinList1
	{mso-style-name:"Figure in List 1\,fig1";
	mso-style-parent:"Figure\,fig";
	mso-style-next:"Text in List 1\,t1";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:9.0pt;
	margin-left:.25in;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.TableFootnoteinList1, li.TableFootnoteinList1, div.TableFootnoteinList1
	{mso-style-name:"Table Footnote in List 1\,tf1";
	mso-style-parent:"Text in List 1\,t1";
	mso-style-next:"Text in List 1\,t1";
	margin-top:2.0pt;
	margin-right:0in;
	margin-bottom:4.0pt;
	margin-left:.25in;
	line-height:11.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	border:none;
	mso-border-top-alt:solid windowtext .5pt;
	padding:0in;
	mso-padding-alt:1.0pt 0in 0in 0in;
	font-size:8.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.AlertText, li.AlertText, div.AlertText
	{mso-style-name:"Alert Text\,at";
	mso-style-parent:"Text\,t";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.25in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.AlertTextinList1, li.AlertTextinList1, div.AlertTextinList1
	{mso-style-name:"Alert Text in List 1\,at1";
	mso-style-parent:"Text in List 1\,t1";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.5in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.AlertTextinList2, li.AlertTextinList2, div.AlertTextinList2
	{mso-style-name:"Alert Text in List 2\,at2";
	mso-style-parent:"Text in List 2\,t2";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.75in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.RevisionHistory, li.RevisionHistory, div.RevisionHistory
	{mso-style-name:"Revision History\,rh";
	mso-style-parent:"Text\,t";
	margin-top:3.0pt;
	margin-right:1.0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:purple;
	display:none;
	mso-hide:all;
	font-style:italic;
	mso-bidi-font-style:normal;}
p.BulletedList1, li.BulletedList1, div.BulletedList1
	{mso-style-name:"Bulleted List 1\,bl1";
	mso-style-parent:"Text in List 1\,t1";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:1.0in;
	text-indent:-.25in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	mso-list:l1 level1 lfo56;
	tab-stops:.25in list 1.0in;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.TextIndented, li.TextIndented, div.TextIndented
	{mso-style-name:"Text Indented\,ti";
	mso-style-parent:"Text\,t";
	margin-top:3.0pt;
	margin-right:.25in;
	margin-bottom:3.0pt;
	margin-left:.25in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.BulletedList2, li.BulletedList2, div.BulletedList2
	{mso-style-name:"Bulleted List 2\,bl2";
	mso-style-parent:"Text in List 2\,t2";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.25in;
	text-indent:-.25in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	mso-list:l47 level1 lfo61;
	tab-stops:list .25in left .5in;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.DefinedTerm, li.DefinedTerm, div.DefinedTerm
	{mso-style-name:"Defined Term\,dt";
	mso-style-parent:"Text\,t";
	mso-style-next:"Definition\,d";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:0in;
	margin-left:0in;
	margin-bottom:.0001pt;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.Definition, li.Definition, div.Definition
	{mso-style-name:"Definition\,d";
	mso-style-parent:"Text\,t";
	mso-style-next:"Defined Term\,dt";
	margin-top:0in;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.25in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.NumberedList1, li.NumberedList1, div.NumberedList1
	{mso-style-name:"Numbered List 1\,nl1";
	mso-style-parent:"Text in List 1\,t1";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.25in;
	text-indent:-.25in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	mso-list:l46 level1 lfo9;
	tab-stops:.25in;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.GlueLinkText, li.GlueLinkText, div.GlueLinkText
	{mso-style-name:"Glue Link Text\,glt";
	mso-style-parent:"Text\,t";
	mso-style-next:"Text\,t";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.IndexTag, li.IndexTag, div.IndexTag
	{mso-style-name:"Index Tag\,it";
	mso-style-parent:"Text\,t";
	margin-top:3.0pt;
	margin-right:1.0in;
	margin-bottom:0in;
	margin-left:0in;
	margin-bottom:.0001pt;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:olive;
	display:none;
	mso-hide:all;}
p.Copyright, li.Copyright, div.Copyright
	{mso-style-name:"Copyright\,copy";
	mso-style-parent:"Text\,t";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	text-indent:0in;
	line-height:11.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	mso-list:l27 level1 lfo11;
	font-size:8.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;}
p.PrintDivisionTitle, li.PrintDivisionTitle, div.PrintDivisionTitle
	{mso-style-name:"Print Division Title\,pdt";
	mso-style-parent:"";
	margin-top:9.0pt;
	margin-right:0in;
	margin-bottom:9.0pt;
	margin-left:0in;
	text-align:right;
	line-height:20.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:18.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:maroon;
	font-weight:bold;
	mso-bidi-font-weight:normal;}
p.PrintMSCorp, li.PrintMSCorp, div.PrintMSCorp
	{mso-style-name:"Print MS Corp\,pms";
	mso-style-parent:"";
	mso-style-next:"Text\,t";
	margin-top:9.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	text-align:right;
	line-height:15.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:13.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:"Microsoft Logo 95";
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:maroon;
	mso-no-proof:yes;}
p.Slugline, li.Slugline, div.Slugline
	{mso-style-name:"Slugline\,slug";
	mso-style-parent:"Footer\,f";
	margin:0in;
	margin-bottom:.0001pt;
	line-height:9.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	mso-element:frame;
	mso-element-frame-height:40.0pt;
	mso-element-frame-hspace:9.0pt;
	mso-element-frame-vspace:9.0pt;
	mso-element-wrap:around;
	mso-element-anchor-vertical:page;
	mso-element-anchor-horizontal:margin;
	mso-element-top:730.05pt;
	mso-height-rule:exactly;
	font-size:6.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:maroon;}
p.MultilanguageMarkerExplicitBegin, li.MultilanguageMarkerExplicitBegin, div.MultilanguageMarkerExplicitBegin
	{mso-style-name:"Multilanguage Marker Explicit Begin\,mmeb";
	mso-style-parent:"Text\,t";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:11.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:8.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:fuchsia;}
p.MultilanguageMarkerExplicitEnd, li.MultilanguageMarkerExplicitEnd, div.MultilanguageMarkerExplicitEnd
	{mso-style-name:"Multilanguage Marker Explicit End\,mmee";
	mso-style-parent:"Multilanguage Marker Explicit Begin\,mmeb";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:11.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:8.0pt;
	mso-bidi-font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:purple;}
p.LabelSpecial, li.LabelSpecial, div.LabelSpecial
	{mso-style-name:"Label Special\,ls";
	mso-style-parent:"Label\,l";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:0in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;
	font-weight:bold;
	mso-bidi-font-weight:normal;}
p.PrintDivisionNumber, li.PrintDivisionNumber, div.PrintDivisionNumber
	{mso-style-name:"Print Division Number\,pdn";
	mso-style-parent:"Print Division Title\,pdt";
	margin-top:9.0pt;
	margin-right:-6.0pt;
	margin-bottom:0in;
	margin-left:0in;
	margin-bottom:.0001pt;
	text-align:right;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:maroon;
	text-transform:uppercase;
	letter-spacing:6.0pt;}
p.LabelinList1, li.LabelinList1, div.LabelinList1
	{mso-style-name:"Label in List 1\,l1";
	mso-style-parent:"Text in List 1\,t1";
	mso-style-next:"Text in List 1\,t1";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.25in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;
	font-weight:bold;
	mso-bidi-font-weight:normal;}
p.LabelinList2, li.LabelinList2, div.LabelinList2
	{mso-style-name:"Label in List 2\,l2";
	mso-style-parent:"Text in List 2\,t2";
	mso-style-next:"Text in List 2\,t2";
	margin-top:3.0pt;
	margin-right:0in;
	margin-bottom:3.0pt;
	margin-left:.5in;
	line-height:13.0pt;
	mso-line-height-rule:exactly;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:Verdana;
	mso-fareast-font-family:"Times New Roman";
	mso-bidi-font-family:"Times New Roman";
	color:black;
	font-weight:bold;
	mso-bidi-font-weight:normal;}
span.CodeEmbedded
	{mso-style-name:"Code Embedded\,ce";
	mso-ansi-font-size:10.0pt;
	font-family:"Courier New";
	mso-ascii-font-family:"Courier New";
	mso-hansi-font-family:"Courier New";}
span.LabelEmbedded
	{mso-style-name:"Label Embedded\,le";
	mso-ansi-font-size:10.0pt;
	font-family:Verdana;
	mso-ascii-font-family:Verdana;
	mso-hansi-font-family:Verdana;
	font-weight:bold;
	mso-bidi-font-weight:normal;
	text-decoration:none;
	text-underline:none;}
span.LinkText
	{mso-style-name:"Link Text\,lt";
	color:green;
	text-decoration:underline;
	text-underline:double;}
span.LinkTextPopup
	{mso-style-name:"Link Text Popup\,ltp";
	color:green;
	text-decoration:underline;
	text-underline:single;}
span.LinkID
	{mso-style-name:"Link ID\,lid";
	color:red;
	display:none;
	mso-hide:all;}
span.ConditionalMarker
	{mso-style-name:"Conditional Marker\,cm";
	mso-ansi-font-size:10.0pt;
	font-family:"Courier New";
	mso-ascii-font-family:"Courier New";
	mso-hansi-font-family:"Courier New";
	color:black;
	display:none;
	mso-hide:all;
	border:none;
	background:#FFFFBF;
	mso-shading:windowtext;
	mso-pattern:gray-375 yellow;}
span.HTML
	{mso-style-name:HTML;
	mso-ansi-font-size:10.0pt;
	font-family:"Courier New";
	mso-ascii-font-family:"Courier New";
	mso-hansi-font-family:"Courier New";
	color:black;
	display:none;
	mso-hide:all;
	border:none;
	background:#DDFFDD;
	mso-shading:windowtext;
	mso-pattern:gray-25 lime;}
span.CodeFeaturedElement
	{mso-style-name:"Code Featured Element\,cfe";
	mso-ansi-font-size:10.0pt;
	font-family:"Courier New";
	mso-ascii-font-family:"Courier New";
	mso-hansi-font-family:"Courier New";
	font-weight:bold;
	mso-bidi-font-weight:normal;}
span.MultilanguageMarkerAuto
	{mso-style-name:"Multilanguage Marker Auto\,mma";
	mso-ansi-font-size:8.0pt;
	font-family:Verdana;
	mso-ascii-font-family:Verdana;
	mso-hansi-font-family:Verdana;
	color:fuchsia;}
span.Bold
	{mso-style-name:"Bold\,b";
	font-weight:bold;
	mso-bidi-font-weight:normal;}
span.BoldItalic
	{mso-style-name:"Bold Italic\,bi";
	font-weight:bold;
	mso-bidi-font-weight:normal;
	font-style:italic;
	mso-bidi-font-style:normal;}
span.Italic
	{mso-style-name:"Italic\,i";
	font-style:italic;
	mso-bidi-font-style:normal;}
span.Strikethrough
	{mso-style-name:"Strikethrough\,strike";
	text-decoration:line-through;}
span.Subscript
	{mso-style-name:"Subscript\,sub";
	vertical-align:sub;}
span.Superscript
	{mso-style-name:"Superscript\,sup";
	vertical-align:super;}
@page Section1
	{size:8.5in 11.0in;
	margin:1.0in 70.5pt 1.0in 91.5pt;
	mso-header-margin:.75in;
	mso-footer-margin:.75in;
	mso-even-footer:url("_msmq_help_component_description_files/header.htm") ef1;
	mso-footer:url("_msmq_help_component_description_files/header.htm") f1;
	mso-paper-source:0;}
div.Section1
	{page:Section1;}
 /* List Definitions */
 @list l0
	{mso-list-id:-132;
	mso-list-type:simple;
	mso-list-template-ids:-1025456648;}
@list l0:level1
	{mso-level-tab-stop:1.25in;
	mso-level-number-position:left;
	margin-left:1.25in;
	text-indent:-.25in;}
@list l1
	{mso-list-id:-131;
	mso-list-type:simple;
	mso-list-template-ids:1704917546;}
@list l1:level1
	{mso-level-tab-stop:1.0in;
	mso-level-number-position:left;
	margin-left:1.0in;
	text-indent:-.25in;}
@list l2
	{mso-list-id:-130;
	mso-list-type:simple;
	mso-list-template-ids:-1166372666;}
@list l2:level1
	{mso-level-tab-stop:.75in;
	mso-level-number-position:left;
	margin-left:.75in;
	text-indent:-.25in;}
@list l3
	{mso-list-id:-129;
	mso-list-type:simple;
	mso-list-template-ids:-1283709830;}
@list l3:level1
	{mso-level-tab-stop:.5in;
	mso-level-number-position:left;
	text-indent:-.25in;}
@list l4
	{mso-list-id:-128;
	mso-list-type:simple;
	mso-list-template-ids:-1261818240;}
@list l4:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:1.25in;
	mso-level-number-position:left;
	margin-left:1.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l5
	{mso-list-id:-127;
	mso-list-type:simple;
	mso-list-template-ids:-2137080934;}
@list l5:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:1.0in;
	mso-level-number-position:left;
	margin-left:1.0in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l6
	{mso-list-id:-126;
	mso-list-type:simple;
	mso-list-template-ids:480825406;}
@list l6:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.75in;
	mso-level-number-position:left;
	margin-left:.75in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l7
	{mso-list-id:-125;
	mso-list-type:simple;
	mso-list-template-ids:-629221776;}
@list l7:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.5in;
	mso-level-number-position:left;
	text-indent:-.25in;
	font-family:Symbol;}
@list l8
	{mso-list-id:-120;
	mso-list-type:simple;
	mso-list-template-ids:1309295726;}
@list l8:level1
	{mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;}
@list l9
	{mso-list-id:-119;
	mso-list-type:simple;
	mso-list-template-ids:1565455404;}
@list l9:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l10
	{mso-list-id:68771112;
	mso-list-type:simple;
	mso-list-template-ids:67698703;}
@list l10:level1
	{mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;}
@list l11
	{mso-list-id:98985430;
	mso-list-type:simple;
	mso-list-template-ids:67698703;}
@list l11:level1
	{mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;}
@list l12
	{mso-list-id:152069974;
	mso-list-type:simple;
	mso-list-template-ids:67698703;}
@list l12:level1
	{mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;}
@list l13
	{mso-list-id:213931746;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l13:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l14
	{mso-list-id:309751557;
	mso-list-type:simple;
	mso-list-template-ids:1104607418;}
@list l14:level1
	{mso-level-tab-stop:.75in;
	mso-level-number-position:left;
	text-indent:-.25in;}
@list l15
	{mso-list-id:318533751;
	mso-list-type:simple;
	mso-list-template-ids:67698703;}
@list l15:level1
	{mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;}
@list l16
	{mso-list-id:414674143;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l16:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l17
	{mso-list-id:475953915;
	mso-list-type:simple;
	mso-list-template-ids:1980517918;}
@list l17:level1
	{mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;}
@list l18
	{mso-list-id:521362406;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l18:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l19
	{mso-list-id:555043825;
	mso-list-type:simple;
	mso-list-template-ids:309995936;}
@list l19:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l20
	{mso-list-id:624120574;
	mso-list-type:simple;
	mso-list-template-ids:67698703;}
@list l20:level1
	{mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;}
@list l21
	{mso-list-id:653800487;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l21:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l22
	{mso-list-id:681705840;
	mso-list-type:simple;
	mso-list-template-ids:67698703;}
@list l22:level1
	{mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;}
@list l23
	{mso-list-id:769737302;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l23:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l24
	{mso-list-id:1054350067;
	mso-list-type:simple;
	mso-list-template-ids:-1624840694;}
@list l24:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l25
	{mso-list-id:1079012351;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l25:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l26
	{mso-list-id:1157041145;
	mso-list-type:simple;
	mso-list-template-ids:-13829058;}
@list l26:level1
	{mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;}
@list l27
	{mso-list-id:1161654485;
	mso-list-type:simple;
	mso-list-template-ids:520143332;}
@list l27:level1
	{mso-level-number-format:bullet;
	mso-level-style-link:Copyright;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l28
	{mso-list-id:1242174498;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l28:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l29
	{mso-list-id:1247305601;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l29:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l30
	{mso-list-id:1352609911;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l30:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l31
	{mso-list-id:1387410818;
	mso-list-type:simple;
	mso-list-template-ids:67698703;}
@list l31:level1
	{mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;}
@list l32
	{mso-list-id:1434664814;
	mso-list-type:simple;
	mso-list-template-ids:-1000035972;}
@list l32:level1
	{mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;}
@list l33
	{mso-list-id:1435589206;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l33:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l34
	{mso-list-id:1440906259;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l34:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l35
	{mso-list-id:1484086278;
	mso-list-type:simple;
	mso-list-template-ids:67698703;}
@list l35:level1
	{mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;}
@list l36
	{mso-list-id:1602645195;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l36:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l37
	{mso-list-id:1610774877;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l37:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l38
	{mso-list-id:1615479013;
	mso-list-type:simple;
	mso-list-template-ids:70557352;}
@list l38:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l39
	{mso-list-id:1656881520;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l39:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l40
	{mso-list-id:1686596083;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l40:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l41
	{mso-list-id:1780173060;
	mso-list-type:simple;
	mso-list-template-ids:67698703;}
@list l41:level1
	{mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;}
@list l42
	{mso-list-id:1809861688;
	mso-list-type:simple;
	mso-list-template-ids:1656119020;}
@list l42:level1
	{mso-level-style-link:"Numbered List 2";
	mso-level-tab-stop:.5in;
	mso-level-number-position:left;
	text-indent:-.25in;}
@list l43
	{mso-list-id:1835412302;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l43:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l44
	{mso-list-id:1839688267;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l44:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
@list l45
	{mso-list-id:1892156636;
	mso-list-type:simple;
	mso-list-template-ids:2143072124;}
@list l45:level1
	{mso-level-number-format:bullet;
	mso-level-style-link:"Index 1";
	mso-level-text:\F0B7;
	mso-level-tab-stop:.5in;
	mso-level-number-position:left;
	text-indent:-.25in;
	font-family:Symbol;}
@list l46
	{mso-list-id:1908110580;
	mso-list-type:simple;
	mso-list-template-ids:-1994478282;}
@list l46:level1
	{mso-level-style-link:"Numbered List 1";
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;}
@list l47
	{mso-list-id:1925800502;
	mso-list-type:simple;
	mso-list-template-ids:-387552862;}
@list l47:level1
	{mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;}
@list l48
	{mso-list-id:2128041332;
	mso-list-type:simple;
	mso-list-template-ids:67698689;}
@list l48:level1
	{mso-level-number-format:bullet;
	mso-level-text:\F0B7;
	mso-level-tab-stop:.25in;
	mso-level-number-position:left;
	margin-left:.25in;
	text-indent:-.25in;
	font-family:Symbol;}
ol
	{margin-bottom:0in;}
ul
	{margin-bottom:0in;}
-->
</style>
<!--[if gte mso 10]>
<style>
 /* Style Definitions */
 table.MsoNormalTable
	{mso-style-name:"Table Normal";
	mso-tstyle-rowband-size:0;
	mso-tstyle-colband-size:0;
	mso-style-noshow:yes;
	mso-style-parent:"";
	mso-padding-alt:0in 5.4pt 0in 5.4pt;
	mso-para-margin:0in;
	mso-para-margin-bottom:.0001pt;
	mso-pagination:widow-orphan;
	font-size:10.0pt;
	font-family:"Times New Roman";}
</style>
<![endif]--><!--[if gte mso 9]><xml>
 <o:shapedefaults v:ext="edit" spidmax="4098"/>
</xml><![endif]--><!--[if gte mso 9]><xml>
 <o:shapelayout v:ext="edit">
  <o:idmap v:ext="edit" data="1"/>
 </o:shapelayout></xml><![endif]-->
</head>

<body lang=EN-US link=blue vlink=purple style='tab-interval:5.0in'>

<div class=Section1>

<h1>Message Queuing Help Component Description</h1>

<p class=Text>Message Queuing is a messaging infrastructure and a development
tool for creating distributed messaging applications for Microsoft Windows
operating systems. This component includes the online help files for Message
Queuing.These online help files provide conceptual information about Message
Queuing features, and procedural directions for administrative tasks.</p>

<p class=Text><o:p>&nbsp;</o:p></p>

<h2><span style='color:black'>Component Configuration<o:p></o:p></span></h2>

<p class=Text>There are no configuration requirements for this component.<span
style='color:windowtext'><o:p></o:p></span></p>

<h3><span style='color:black'>Special Notes<o:p></o:p></span></h3>

<p class=Text>You can install the Message Queuing Help component without
installing any other Message Queuing components, but to carry out
administrative tasks described in the online help, you need to install the Message
Queuing Administration component.</p>

<h3><span style='color:black'>For More Information<o:p></o:p></span></h3>

<p class=Text>For more information on this component, visit this <a
href="http://www.microsoft.com/isapi/redir.dll?prd=Windows&amp;sbp=MSMQ&amp;Pver=3.0">Microsoft
Web site</a>.<span style='color:windowtext'><o:p></o:p></span></p>

<p class=Text><o:p>&nbsp;</o:p></p>

<h3><span style='color:black'>Additional Information<o:p></o:p></span></h3>

<p class=Text>This information will not be published in the database and is for
internal use only.</p>

<p class=Text>Contact name: <!--[if supportFields]><span style='background:
yellow;mso-highlight:yellow'><span style='mso-element:field-begin'></span><span
style='mso-bookmark:Text5'><span style='mso-spacerun:yes'> </span>FORMTEXT <span
style='mso-element:field-separator'></span></span></span><![endif]--><span
style='mso-bookmark:Text5'><span style='background:yellow;mso-highlight:yellow'><![if !supportNestedAnchors]><a
name=Text5></a><![endif]><span style='mso-no-proof:yes'>email alias</span></span><!--[if gte mso 9]><xml>
 <w:data>FFFFFFFF00000000000005005400650078007400350000000B0065006D00610069006C00200061006C0069006100730000000000000000000000000000000000000000000000</w:data>
</xml><![endif]--></span><!--[if supportFields]><span style='mso-bookmark:Text5'></span><span
style='mso-element:field-end'></span><![endif]--><span style='mso-bookmark:
Text5'></span></p>

<p class=Text><o:p>&nbsp;</o:p></p>

</div>

</body>

</html>
]]></HELPCONTEXT>

        <PROPERTIES
          Context="1"
          PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
        >        </PROPERTIES>

        <RESOURCES
          Context="1"
          PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
        >
          <RESOURCE
            Name="File(819):&quot;%18%&quot;,&quot;msmq.chm&quot;"
            ResTypeVSGUID="{E66B49F6-4A35-4246-87E8-5C1A468315B5}"
            BuildTypeMask="819"
            BuildOrder="1000"
            Localize="False"
            Disabled="False"
            Context="1"
            PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
          >
            <PROPERTIES
              Context="1"
              PlatformGUID="{00000000-0000-0000-0000-000000000000}"
            >
              <PROPERTY
                Name="NoExpand"
                Format="Boolean"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              >False</PROPERTY>

              <PROPERTY
                Name="DstName"
                Format="String"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              >msmq.chm</PROPERTY>

              <PROPERTY
                Name="DstPath"
                Format="String"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              >%18%</PROPERTY>

              <PROPERTY
                Name="SrcName"
                Format="String"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              >msmqw.chm</PROPERTY>

              <PROPERTY
                Name="ComponentVSGUID"
                Format="GUID"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              >{00000000-0000-0000-0000-000000000000}</PROPERTY>

              <PROPERTY
                Name="SrcFileCRC"
                Format="Integer"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              >0</PROPERTY>

              <PROPERTY
                Name="SrcFileSize"
                Format="Integer"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              >0</PROPERTY>

              <PROPERTY
                Name="SrcPath"
                Format="String"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              ></PROPERTY>
            </PROPERTIES>

            <DISPLAYNAME>msmq.chm</DISPLAYNAME>

            <DESCRIPTION></DESCRIPTION>
          </RESOURCE>

          <RESOURCE
            Name="File(819):&quot;%18%&quot;,&quot;msmqconcepts.chm&quot;"
            ResTypeVSGUID="{E66B49F6-4A35-4246-87E8-5C1A468315B5}"
            BuildTypeMask="819"
            BuildOrder="1000"
            Localize="False"
            Disabled="False"
            Context="1"
            PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
          >
            <PROPERTIES
              Context="1"
              PlatformGUID="{00000000-0000-0000-0000-000000000000}"
            >
              <PROPERTY
                Name="NoExpand"
                Format="Boolean"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              >False</PROPERTY>

              <PROPERTY
                Name="DstName"
                Format="String"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              >msmqconcepts.chm</PROPERTY>

              <PROPERTY
                Name="DstPath"
                Format="String"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              >%18%</PROPERTY>

              <PROPERTY
                Name="SrcName"
                Format="String"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              >msmqconw.chm</PROPERTY>

              <PROPERTY
                Name="ComponentVSGUID"
                Format="GUID"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              >{00000000-0000-0000-0000-000000000000}</PROPERTY>

              <PROPERTY
                Name="SrcFileCRC"
                Format="Integer"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              >0</PROPERTY>

              <PROPERTY
                Name="SrcFileSize"
                Format="Integer"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              >0</PROPERTY>

              <PROPERTY
                Name="SrcPath"
                Format="String"
                Context="1"
                PlatformGUID="{00000000-0000-0000-0000-000000000000}"
              ></PROPERTY>
            </PROPERTIES>

            <DISPLAYNAME>msmqconcepts.chm</DISPLAYNAME>

            <DESCRIPTION></DESCRIPTION>
          </RESOURCE>
        </RESOURCES>

        <GROUPMEMBERS
        >
          <GROUPMEMBER
            GroupVSGUID="{388249D2-1897-44FF-86F2-E159A27AA037}"
          ></GROUPMEMBER>

          <GROUPMEMBER
            GroupVSGUID="{E01B4103-3883-4FE8-992F-10566E7B796C}"
          ></GROUPMEMBER>
        </GROUPMEMBERS>

        <DEPENDENCIES
          Context="1"
          PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
        >        </DEPENDENCIES>

        <DISPLAYNAME>MSMQ Help</DISPLAYNAME>

        <VERSION>1.0</VERSION>

        <DESCRIPTION>Message Queuing online help</DESCRIPTION>

        <OWNERS>t-itaig</OWNERS>

        <AUTHORS>t-itaig</AUTHORS>

        <DATECREATED>8/29/2001 10:56:49 AM</DATECREATED>

        <DATEREVISED>9/16/2001 6:49:40 AM</DATEREVISED>
      </COMPONENT>
    </COMPONENTS>

    <RESTYPES
      Context="1"
      PlatformGUID="{00000000-0000-0000-0000-000000000000}"
    >    </RESTYPES>
  </DCARRIER>
