

<%@ Page %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<html>
	<HEAD>
		<META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=Windows-1252">
		<META HTTP-EQUIV="MSThemeCompatible" CONTENT="Yes">
		<META NAME="MS.LOCALE" CONTENT="EN-US">
		<!-- #include file = "search.header.htm" -->
	</head>
	<body marginwidth="0" marginheight="0" LEFTMARGIN="0" TOPMARGIN="0" rightmargin="0" ONLOAD="BringToFront()">
		<!-- #include file = "search.heading.htm" -->
		<table class="content" width="100%" cellpadding="8">
			<tr>
				<td>
					<h1><img src="..\..\images\instance.gif" height="16" width="16" alt="instance info">Instance Info - Details</h1>
					Use the <b>Details</b> tab to view the name and descriptions of this 
					instance info.
					<UL>
						<li>
							<b>Interface tModel:</b> Displays the name of the tModel that this instance info refers to and its unique key for programmatic queries.
						</li>
						<li>
							<b>Description:</b> Lists descriptions of this instance info and the 
							language for which each description is written.
						</li>
					</UL>
					<H3>More Information</H3>
					
						<!-- #include file = "glossary.instanceinfo.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "search.footer.htm" -->
	</body>
</html>

 