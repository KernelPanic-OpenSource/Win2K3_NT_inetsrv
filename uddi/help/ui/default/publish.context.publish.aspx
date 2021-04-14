

<%@ Page %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<html>
	<HEAD>
		<META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=Windows-1252">
		<META HTTP-EQUIV="MSThemeCompatible" CONTENT="Yes">
		<META NAME="MS.LOCALE" CONTENT="EN-US">
		<!-- #include file = "publish.header.htm" -->
	</head>
	<body marginwidth="0" marginheight="0" LEFTMARGIN="0" TOPMARGIN="0" rightmargin="0" ONLOAD="BringToFront()">
		<!-- #include file = "publish.heading.htm" -->
		<table class="content" width="100%" cellpadding="8">
			<tr>
				<td>
					<H1><img src="..\..\images\my_uddi.gif" height="16" width="16" alt="My UDDI"> My UDDI</H1>
					The <b>My UDDI</b> tab is your starting point in
					<uddi:ContentController Runat='server' Mode='Private'> UDDI&nbsp;Services.</uddi:ContentController>
					<uddi:ContentController Runat='server' Mode='Public'> UDDI.</uddi:ContentController>
					Select either the Providers or tModels tab to manage and view your providers or tModels.
					<uddi:ContentController Runat='server' Mode='Private'>
					<P>
					If you are not yet familiar with publishing in UDDI&nbsp;Services, see the <a href="publish.publishinuddiservices.aspx">Introduction to Publishing</a> in UDDI&nbsp;Services.</uddi:ContentController>
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 

